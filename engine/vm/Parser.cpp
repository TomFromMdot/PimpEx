#include "vm/Parser.hpp"
#include "vm/Lexer.hpp"
#include "core/Strings.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>

namespace PimpEx::vm {
namespace {

// An error in one line; the parser turns it into a Diagnostic.
struct LineError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

std::string to_upper(std::string_view s) { return to_upper_ascii(s); }

const std::vector<std::string_view> Instructions = {
    "MOVE", "SET",  "ADD", "MUL",  "SPAWN",    "DESTROY",     "FOLLOW",
    "TEXT", "LOG",  "CALL", "IF",  "PROC_ADD", "PROC_REMOVE", "SCENE",
    "GUI_SHOW", "GUI_HIDE", "GUI_TOGGLE"};

std::string suggestion(std::string_view word) {
  const std::vector<std::string> names(Instructions.begin(), Instructions.end());
  const auto best = closest_match(word, names); // close typos only
  return best.empty() ? "" : std::format(" Did you mean {}?", best);
}

class Cursor {
  const std::vector<Token> &_tokens;
  std::size_t _pos = 0;

public:
  explicit Cursor(const std::vector<Token> &tokens) : _tokens(tokens) {}

  bool done() const { return _pos >= _tokens.size(); }
  const Token *peek() const { return done() ? nullptr : &_tokens[_pos]; }

  const Token &next(std::string_view what) {
    if (done())
      throw LineError(std::format("missing: {}", what));
    return _tokens[_pos++];
  }

  // Case-insensitive.
  bool accept(std::string_view keyword) {
    const auto *t = peek();
    if (t && t->kind == Token::Kind::Word && to_upper(t->text) == keyword) {
      ++_pos;
      return true;
    }
    return false;
  }

  std::string word(std::string_view what) {
    const auto &t = next(what);
    if (t.kind != Token::Kind::Word && t.kind != Token::Kind::Number)
      throw LineError(std::format("expected {}, got '{}'", what, t.text));
    return t.text;
  }

  // Actor / PROC / scene name – no dots.
  std::string name(std::string_view what) {
    auto text = word(what);
    if (text.find_first_of("./@") != std::string::npos)
      throw LineError(std::format("{} cannot contain '.', '/' or '@': '{}'", what, text));
    return text;
  }

  // `Player`, `SELF`, `@coin` (tag) or, in FOR, `Health` (component).
  std::string selector(std::string_view what) {
    auto text = word(what);
    const auto body = text.starts_with('@') ? std::string_view(text).substr(1) : std::string_view(text);
    if (body.empty() || body.find_first_of("./@") != std::string_view::npos)
      throw LineError(std::format("{}: expected a name or @tag, got '{}'", what, text));
    return text;
  }

  double number(std::string_view what) {
    const auto &t = next(what);
    if (t.kind != Token::Kind::Number)
      throw LineError(std::format("expected a number ({}), got '{}'", what, t.text));
    return t.number;
  }

  Ref ref(std::string_view what) {
    const auto text = word(what);
    const auto dot = text.find('.');
    if (dot == std::string::npos || dot == 0 || dot + 1 == text.size())
      throw LineError(std::format(
          "expected a field as Actor.field ({}), got '{}'", what, text));
    return Ref{text.substr(0, dot), text.substr(dot + 1)};
  }

  Value value(std::string_view what) {
    const auto *t = peek();
    if (!t)
      throw LineError(std::format("missing: {}", what));

    if (t->kind == Token::Kind::Number) {
      ++_pos;
      Value v;
      v.number = t->number;
      return v;
    }
    if (t->kind == Token::Kind::Minus) {
      ++_pos;
      Value v;
      v.kind = Value::Kind::Ref;
      v.ref = ref(what);
      v.negate = true;
      return v;
    }
    if (accept("RANDOM")) {
      Value v;
      v.kind = Value::Kind::Random;
      v.min = number("RANDOM od");
      v.max = number("RANDOM do");
      if (v.min > v.max)
        std::swap(v.min, v.max);
      return v;
    }
    if (t->kind == Token::Kind::Word && t->text.find('.') != std::string::npos) {
      Value v;
      v.kind = Value::Kind::Ref;
      v.ref = ref(what);
      return v;
    }

    throw LineError(std::format(
        "expected {}: a number, Actor.field or RANDOM a b, got '{}'", what,
        t->text));
  }

  std::string rest_as_text() {
    if (!done() && peek()->kind == Token::Kind::String) {
      auto text = _tokens[_pos++].text;
      return text;
    }
    std::string text;
    while (!done()) {
      if (!text.empty())
        text += ' ';
      text += _tokens[_pos++].text;
    }
    return text;
  }

  void expect_end() {
    if (!done())
      throw LineError(std::format("unexpected word '{}' at the end of the line", peek()->text));
  }
};

Key parse_key(Cursor &c) {
  const auto name = c.word("key name");
  if (auto key = key_from_name(name))
    return *key;
  throw LineError(std::format(
      "unknown key '{}' (letters A-Z, digits 0-9, SPACE, ENTER, ESCAPE, "
      "SHIFT, LEFT, RIGHT, UP, DOWN)",
      name));
}

Condition parse_condition(Cursor &c) {
  Condition cond;
  if (c.accept("NOT"))
    cond.negate = true;

  if (c.accept("KEY")) {
    cond.kind = Condition::Kind::KeyDown;
    cond.key = parse_key(c);
  } else if (c.accept("PRESSED")) {
    cond.kind = Condition::Kind::KeyPressed;
    cond.key = parse_key(c);
  } else if (c.accept("TOUCH")) {
    cond.kind = Condition::Kind::Touch;
    cond.a = c.selector("first actor in TOUCH");
    cond.b = c.selector("second actor in TOUCH");
  } else if (c.accept("EXISTS")) {
    cond.kind = Condition::Kind::Exists;
    cond.a = c.selector("actor in EXISTS");
  } else if (c.accept("CHANCE")) {
    cond.kind = Condition::Kind::Chance;
    cond.left = c.value("percentage in CHANCE");
  } else {
    cond.kind = Condition::Kind::Compare;
    cond.left = c.value("left side of the comparison");
    const auto &op = c.next("comparison operator (<, <=, >, >=, ==, !=)");
    if (op.kind != Token::Kind::Compare)
      throw LineError(std::format(
          "expected a comparison operator (<, <=, >, >=, ==, !=), got '{}'", op.text));
    if (op.text == "<") cond.op = CompareOp::Less;
    else if (op.text == "<=") cond.op = CompareOp::LessEqual;
    else if (op.text == ">") cond.op = CompareOp::Greater;
    else if (op.text == ">=") cond.op = CompareOp::GreaterEqual;
    else if (op.text == "==") cond.op = CompareOp::Equal;
    else cond.op = CompareOp::NotEqual;
    cond.right = c.value("right side of the comparison");
  }
  return cond;
}

Instr parse_statement(Cursor &c, int line) {
  Instr instr;
  instr.line = line;

  // IF condition [AND condition...] : instruction (IFs can nest)
  while (c.accept("IF")) {
    instr.guard.push_back(parse_condition(c));
    while (c.accept("AND"))
      instr.guard.push_back(parse_condition(c));
    const auto &colon = c.next("colon ':' after the IF condition");
    if (colon.kind != Token::Kind::Colon)
      throw LineError(std::format(
          "the IF condition needs a colon ':', got '{}'", colon.text));
  }

  const auto *first = c.peek();
  if (!first)
    throw LineError("missing instruction after ':'");
  const auto keyword = to_upper(c.word("instruction"));

  if (keyword == "MOVE") {
    instr.op = Op::Move;
    instr.target = c.name("actor in MOVE");
    instr.values = {c.value("X offset"), c.value("Y offset")};
    if (!c.done())
      instr.values.push_back(c.value("Z offset"));
  } else if (keyword == "SET" || keyword == "ADD" || keyword == "MUL") {
    instr.op = keyword == "SET" ? Op::Set : keyword == "ADD" ? Op::Add : Op::Mul;
    instr.ref = c.ref("field to change");
    if (keyword == "SET" && !c.done() && c.peek()->kind == Token::Kind::String) {
      Value text;
      text.kind = Value::Kind::Text;
      text.text = c.next("text").text;
      instr.values = {text};
    } else {
      instr.values = {c.value("value")};
    }
  } else if (keyword == "SPAWN") {
    instr.op = Op::Spawn;
    instr.target = c.name("archetype in SPAWN");
    if (c.accept("AS"))
      instr.other = c.name("new actor name after AS");
    if (c.accept("AT")) {
      instr.values = {c.value("X position"), c.value("Y position")};
      if (!c.done())
        instr.values.push_back(c.value("Z position"));
    }
  } else if (keyword == "DESTROY") {
    instr.op = Op::Destroy;
    instr.target = c.name("actor in DESTROY");
  } else if (keyword == "FOLLOW") {
    instr.op = Op::Follow;
    instr.target = c.name("following actor");
    instr.other = c.name("FOLLOW target");
    instr.values = {c.value("FOLLOW speed")};
  } else if (keyword == "TEXT") {
    instr.op = Op::Text;
    instr.target = c.name("actor in TEXT");
    instr.text = c.rest_as_text();
  } else if (keyword == "LOG") {
    instr.op = Op::Log;
    instr.text = c.rest_as_text();
  } else if (keyword == "CALL") {
    instr.op = Op::Call;
    instr.target = c.name("Lua function name");
    while (!c.done())
      instr.args.push_back(c.next("argument").text);
  } else if (keyword == "PROC_ADD" || keyword == "PROC_REMOVE") {
    instr.op = keyword == "PROC_ADD" ? Op::ProcAdd : Op::ProcRemove;
    instr.target = c.name("PROC name");
  } else if (keyword == "SCENE") {
    instr.op = Op::Scene;
    instr.target = c.name("scene name");
  } else if (keyword == "GUI_SHOW" || keyword == "GUI_HIDE" || keyword == "GUI_TOGGLE") {
    instr.op = keyword == "GUI_SHOW" ? Op::GuiShow : keyword == "GUI_HIDE" ? Op::GuiHide : Op::GuiToggle;
    instr.target = c.name("GUI name (file gui/<name>.xml)");
  } else if (keyword == "PROC" || keyword == "END") {
    throw LineError(std::format("{} cannot follow IF", keyword));
  } else {
    throw LineError(std::format("unknown instruction '{}'.{}", first->text,
                                suggestion(first->text)));
  }

  c.expect_end();
  return instr;
}

} // namespace

const std::vector<std::string_view> &instruction_names() { return Instructions; }

InstructionsResult parse_instructions(std::string_view source, const std::string &file, int line) {
  InstructionsResult result;
  auto parse_one = [&](std::string_view text) {
    auto lexed = tokenize_line(text);
    if (!lexed.error.empty()) {
      result.diagnostics.push_back({Diagnostic::Severity::Error, file, line, lexed.error});
      return;
    }
    if (lexed.tokens.empty())
      return;
    Cursor c(lexed.tokens);
    try {
      if (c.accept("PROC") || c.accept("END"))
        throw LineError("write plain instructions here, without PROC and END");
      result.instructions.push_back(parse_statement(c, line));
    } catch (const LineError &e) {
      result.diagnostics.push_back({Diagnostic::Severity::Error, file, line, e.what()});
    }
  };
  // A quoted ";" (TEXT Hud "a;b") does not split.
  bool quoted = false;
  std::size_t start = 0;
  for (std::size_t i = 0; i <= source.size(); ++i) {
    if (i < source.size() && source[i] == '"')
      quoted = !quoted;
    if (i == source.size() || (source[i] == ';' && !quoted)) {
      parse_one(source.substr(start, i - start));
      start = i + 1;
    }
  }
  return result;
}

ParseResult parse_program(std::string_view source, const std::string &file) {
  ParseResult result;
  auto error = [&](int line, std::string message) {
    result.diagnostics.push_back(
        {Diagnostic::Severity::Error, file, line, std::move(message)});
  };

  Proc *current = nullptr;
  int lineNo = 0;
  std::size_t start = 0;

  while (start <= source.size()) {
    auto end = source.find('\n', start);
    if (end == std::string_view::npos)
      end = source.size();
    const auto line = source.substr(start, end - start);
    start = end + 1;
    ++lineNo;

    auto lexed = tokenize_line(line);
    if (!lexed.error.empty()) {
      error(lineNo, lexed.error);
      continue;
    }
    if (lexed.tokens.empty())
      continue;

    Cursor c(lexed.tokens);
    try {
      if (c.accept("PROC")) {
        if (current) {
          error(current->firstLine,
                std::format("PROC {} has no END (the next PROC starts on line {})",
                            current->name, lineNo));
          current->lastLine = lineNo - 1;
          current = nullptr;
        }
        Proc proc;
        proc.name = c.name("PROC name");
        proc.firstLine = lineNo;
        if (result.program.find(proc.name))
          error(lineNo, std::format("PROC {} is already defined", proc.name));
        // The PROC is open from here even if the rest of the header is broken,
        // so its END is not reported as "END without PROC".
        result.program.procs.push_back(proc);
        current = &result.program.procs.back();
        while (!c.done()) {
          if (current->mode == ProcMode::Loop && c.accept("ONCE")) {
            current->mode = ProcMode::Once;
          } else if (current->mode == ProcMode::Loop && c.accept("EVERY")) {
            current->mode = ProcMode::Every;
            const auto n = c.number("frame count after EVERY");
            if (n < 1 || n != std::floor(n))
              throw LineError("EVERY needs a positive whole number of frames");
            current->every = static_cast<int>(n);
          } else if (current->forEach.empty() && c.accept("FOR")) {
            current->forEach = c.selector("component or @tag after FOR");
          } else {
            throw LineError(std::format("unknown word '{}' after the PROC name (allowed: ONCE, "
                                        "EVERY n, FOR Component, FOR @tag)",
                                        c.peek()->text));
          }
        }
        continue;
      }

      if (c.accept("END")) {
        c.expect_end();
        if (!current) {
          error(lineNo, "END without PROC");
          continue;
        }
        current->lastLine = lineNo;
        current = nullptr;
        continue;
      }

      if (!current) {
        error(lineNo, "instruction outside a PROC – every instruction must be "
                      "between `PROC name` and `END`");
        continue;
      }
      current->body.push_back(parse_statement(c, lineNo));
    } catch (const LineError &e) {
      error(lineNo, e.what());
    }
  }

  if (current) {
    error(current->firstLine, std::format("PROC {} has no END", current->name));
    current->lastLine = lineNo;
  }
  return result;
}

} // namespace PimpEx::vm
