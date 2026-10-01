#include "vm/Parser.hpp"
#include "vm/Lexer.hpp"
#include "core/Strings.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>

namespace PimpEx::vm {
namespace {

// Błąd w jednej linii – łapany przez parser i zamieniany na Diagnostic.
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
  const auto best = closest_match(word, names); // tylko bliskie literówki
  return best.empty() ? "" : std::format(" Czy chodziło o {}?", best);
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
      throw LineError(std::format("brakuje: {}", what));
    return _tokens[_pos++];
  }

  // Sprawdza (bez względu na wielkość liter), czy następne słowo to `keyword`.
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
      throw LineError(std::format("oczekiwano: {}, a jest '{}'", what, t.text));
    return t.text;
  }

  // Nazwa aktora / PROC / sceny – bez kropki.
  std::string name(std::string_view what) {
    auto text = word(what);
    if (text.find_first_of("./@") != std::string::npos)
      throw LineError(std::format("{} nie może zawierać '.', '/' ani '@': '{}'", what, text));
    return text;
  }

  // Aktor albo grupa: `Player`, `SELF`, `@coin` (tag), a w FOR także `Health` (komponent).
  std::string selector(std::string_view what) {
    auto text = word(what);
    const auto body = text.starts_with('@') ? std::string_view(text).substr(1) : std::string_view(text);
    if (body.empty() || body.find_first_of("./@") != std::string_view::npos)
      throw LineError(std::format("{}: oczekiwano nazwy albo @tagu, a jest '{}'", what, text));
    return text;
  }

  double number(std::string_view what) {
    const auto &t = next(what);
    if (t.kind != Token::Kind::Number)
      throw LineError(std::format("oczekiwano liczby ({}), a jest '{}'", what, t.text));
    return t.number;
  }

  Ref ref(std::string_view what) {
    const auto text = word(what);
    const auto dot = text.find('.');
    if (dot == std::string::npos || dot == 0 || dot + 1 == text.size())
      throw LineError(std::format(
          "oczekiwano pola w formie Aktor.pole ({}), a jest '{}'", what, text));
    return Ref{text.substr(0, dot), text.substr(dot + 1)};
  }

  Value value(std::string_view what) {
    const auto *t = peek();
    if (!t)
      throw LineError(std::format("brakuje: {}", what));

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
        "oczekiwano {}: liczby, Aktor.pole albo RANDOM a b, a jest '{}'", what,
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
      throw LineError(std::format("nadmiarowe słowo '{}' na końcu linii", peek()->text));
  }
};

Key parse_key(Cursor &c) {
  const auto name = c.word("nazwa klawisza");
  if (auto key = key_from_name(name))
    return *key;
  throw LineError(std::format(
      "nieznany klawisz '{}' (litery A-Z, cyfry 0-9, SPACE, ENTER, ESCAPE, "
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
    cond.a = c.selector("pierwszy aktor w TOUCH");
    cond.b = c.selector("drugi aktor w TOUCH");
  } else if (c.accept("EXISTS")) {
    cond.kind = Condition::Kind::Exists;
    cond.a = c.selector("aktor w EXISTS");
  } else if (c.accept("CHANCE")) {
    cond.kind = Condition::Kind::Chance;
    cond.left = c.value("procent w CHANCE");
  } else {
    cond.kind = Condition::Kind::Compare;
    cond.left = c.value("lewa strona porównania");
    const auto &op = c.next("operator porównania (<, <=, >, >=, ==, !=)");
    if (op.kind != Token::Kind::Compare)
      throw LineError(std::format(
          "oczekiwano operatora porównania (<, <=, >, >=, ==, !=), a jest '{}'", op.text));
    if (op.text == "<") cond.op = CompareOp::Less;
    else if (op.text == "<=") cond.op = CompareOp::LessEqual;
    else if (op.text == ">") cond.op = CompareOp::Greater;
    else if (op.text == ">=") cond.op = CompareOp::GreaterEqual;
    else if (op.text == "==") cond.op = CompareOp::Equal;
    else cond.op = CompareOp::NotEqual;
    cond.right = c.value("prawa strona porównania");
  }
  return cond;
}

Instr parse_statement(Cursor &c, int line) {
  Instr instr;
  instr.line = line;

  // IF warunek [AND warunek...] : instrukcja  (można zagnieżdżać IF)
  while (c.accept("IF")) {
    instr.guard.push_back(parse_condition(c));
    while (c.accept("AND"))
      instr.guard.push_back(parse_condition(c));
    const auto &colon = c.next("dwukropek ':' po warunku IF");
    if (colon.kind != Token::Kind::Colon)
      throw LineError(std::format(
          "po warunku IF potrzebny jest dwukropek ':', a jest '{}'", colon.text));
  }

  const auto *first = c.peek();
  if (!first)
    throw LineError("brakuje instrukcji po ':'");
  const auto keyword = to_upper(c.word("instrukcja"));

  if (keyword == "MOVE") {
    instr.op = Op::Move;
    instr.target = c.name("aktor w MOVE");
    instr.values = {c.value("przesunięcie X"), c.value("przesunięcie Y")};
    if (!c.done())
      instr.values.push_back(c.value("przesunięcie Z"));
  } else if (keyword == "SET" || keyword == "ADD" || keyword == "MUL") {
    instr.op = keyword == "SET" ? Op::Set : keyword == "ADD" ? Op::Add : Op::Mul;
    instr.ref = c.ref("pole do zmiany");
    if (keyword == "SET" && !c.done() && c.peek()->kind == Token::Kind::String) {
      Value text;
      text.kind = Value::Kind::Text;
      text.text = c.next("tekst").text;
      instr.values = {text};
    } else {
      instr.values = {c.value("wartość")};
    }
  } else if (keyword == "SPAWN") {
    instr.op = Op::Spawn;
    instr.target = c.name("archetyp w SPAWN");
    if (c.accept("AS"))
      instr.other = c.name("nazwa nowego aktora po AS");
    if (c.accept("AT")) {
      instr.values = {c.value("pozycja X"), c.value("pozycja Y")};
      if (!c.done())
        instr.values.push_back(c.value("pozycja Z"));
    }
  } else if (keyword == "DESTROY") {
    instr.op = Op::Destroy;
    instr.target = c.name("aktor w DESTROY");
  } else if (keyword == "FOLLOW") {
    instr.op = Op::Follow;
    instr.target = c.name("aktor, który podąża");
    instr.other = c.name("cel FOLLOW");
    instr.values = {c.value("prędkość FOLLOW")};
  } else if (keyword == "TEXT") {
    instr.op = Op::Text;
    instr.target = c.name("aktor w TEXT");
    instr.text = c.rest_as_text();
  } else if (keyword == "LOG") {
    instr.op = Op::Log;
    instr.text = c.rest_as_text();
  } else if (keyword == "CALL") {
    instr.op = Op::Call;
    instr.target = c.name("nazwa funkcji Lua");
    while (!c.done())
      instr.args.push_back(c.next("argument").text);
  } else if (keyword == "PROC_ADD" || keyword == "PROC_REMOVE") {
    instr.op = keyword == "PROC_ADD" ? Op::ProcAdd : Op::ProcRemove;
    instr.target = c.name("nazwa PROC");
  } else if (keyword == "SCENE") {
    instr.op = Op::Scene;
    instr.target = c.name("nazwa sceny");
  } else if (keyword == "GUI_SHOW" || keyword == "GUI_HIDE" || keyword == "GUI_TOGGLE") {
    instr.op = keyword == "GUI_SHOW" ? Op::GuiShow : keyword == "GUI_HIDE" ? Op::GuiHide : Op::GuiToggle;
    instr.target = c.name("nazwa GUI (plik gui/nazwa.xml)");
  } else if (keyword == "PROC" || keyword == "END") {
    throw LineError(std::format("{} nie może stać za IF", keyword));
  } else {
    throw LineError(std::format("nieznana instrukcja '{}'.{}", first->text,
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
        throw LineError("tu wpisz same instrukcje, bez PROC i END");
      result.instructions.push_back(parse_statement(c, line));
    } catch (const LineError &e) {
      result.diagnostics.push_back({Diagnostic::Severity::Error, file, line, e.what()});
    }
  };
  // Średnik w cudzysłowie (np. TEXT Hud "a;b") nie dzieli instrukcji.
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
                std::format("PROC {} nie ma END (następna PROC zaczyna się w linii {})",
                            current->name, lineNo));
          current->lastLine = lineNo - 1;
          current = nullptr;
        }
        Proc proc;
        proc.name = c.name("nazwa PROC");
        proc.firstLine = lineNo;
        if (result.program.find(proc.name))
          error(lineNo, std::format("PROC {} jest już zdefiniowana", proc.name));
        // Od tej chwili PROC jest otwarta – nawet jeśli dalsza część nagłówka
        // ma błąd, jej END nie będzie „END bez PROC”.
        result.program.procs.push_back(proc);
        current = &result.program.procs.back();
        while (!c.done()) {
          if (current->mode == ProcMode::Loop && c.accept("ONCE")) {
            current->mode = ProcMode::Once;
          } else if (current->mode == ProcMode::Loop && c.accept("EVERY")) {
            current->mode = ProcMode::Every;
            const auto n = c.number("liczba klatek po EVERY");
            if (n < 1 || n != std::floor(n))
              throw LineError("EVERY wymaga dodatniej liczby całkowitej klatek");
            current->every = static_cast<int>(n);
          } else if (current->forEach.empty() && c.accept("FOR")) {
            current->forEach = c.selector("komponent albo @tag po FOR");
          } else {
            throw LineError(std::format("nieznane słowo '{}' po nazwie PROC (dozwolone: ONCE, "
                                        "EVERY n, FOR Komponent, FOR @tag)",
                                        c.peek()->text));
          }
        }
        continue;
      }

      if (c.accept("END")) {
        c.expect_end();
        if (!current) {
          error(lineNo, "END bez PROC");
          continue;
        }
        current->lastLine = lineNo;
        current = nullptr;
        continue;
      }

      if (!current) {
        error(lineNo, "instrukcja poza PROC – każda instrukcja musi być "
                      "między `PROC nazwa` a `END`");
        continue;
      }
      current->body.push_back(parse_statement(c, lineNo));
    } catch (const LineError &e) {
      error(lineNo, e.what());
    }
  }

  if (current) {
    error(current->firstLine, std::format("PROC {} nie ma END", current->name));
    current->lastLine = lineNo;
  }
  return result;
}

} // namespace PimpEx::vm
