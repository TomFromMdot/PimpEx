#include "vm/Parser.hpp"
#include "vm/Lexer.hpp"

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

std::string to_upper(std::string_view s) {
  std::string out(s);
  for (auto &c : out)
    if (c >= 'a' && c <= 'z')
      c = static_cast<char>(c - 'a' + 'A');
  return out;
}

std::size_t edit_distance(std::string_view a, std::string_view b) {
  std::vector<std::size_t> row(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j)
    row[j] = j;
  for (std::size_t i = 1; i <= a.size(); ++i) {
    std::size_t prev = row[0];
    row[0] = i;
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const std::size_t cur = row[j];
      row[j] = std::min({row[j] + 1, row[j - 1] + 1,
                         prev + (a[i - 1] == b[j - 1] ? 0 : 1)});
      prev = cur;
    }
  }
  return row[b.size()];
}

const std::vector<std::string_view> Instructions = {
    "MOVE", "SET",  "ADD", "MUL",  "SPAWN",    "DESTROY",     "FOLLOW",
    "TEXT", "LOG",  "CALL", "IF",  "PROC_ADD", "PROC_REMOVE", "SCENE"};

std::string suggestion(std::string_view word) {
  const auto upper = to_upper(word);
  std::string_view best;
  std::size_t bestDistance = 3; // podpowiadamy tylko bliskie literówki
  for (auto name : Instructions) {
    const auto d = edit_distance(upper, name);
    if (d < bestDistance) {
      bestDistance = d;
      best = name;
    }
  }
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
    if (text.find('.') != std::string::npos || text.find('/') != std::string::npos)
      throw LineError(std::format("{} nie może zawierać '.' ani '/': '{}'", what, text));
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
    const auto dot = text.rfind('.');
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
    cond.a = c.name("pierwszy aktor w TOUCH");
    cond.b = c.name("drugi aktor w TOUCH");
  } else if (c.accept("EXISTS")) {
    cond.kind = Condition::Kind::Exists;
    cond.a = c.name("aktor w EXISTS");
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
  } else if (keyword == "SET" || keyword == "ADD" || keyword == "MUL") {
    instr.op = keyword == "SET" ? Op::Set : keyword == "ADD" ? Op::Add : Op::Mul;
    instr.ref = c.ref("pole do zmiany");
    instr.values = {c.value("wartość")};
  } else if (keyword == "SPAWN") {
    instr.op = Op::Spawn;
    instr.target = c.name("archetyp w SPAWN");
    if (c.accept("AS"))
      instr.other = c.name("nazwa nowego aktora po AS");
    if (c.accept("AT"))
      instr.values = {c.value("pozycja X"), c.value("pozycja Y")};
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
        }
        Proc proc;
        proc.name = c.name("nazwa PROC");
        proc.firstLine = lineNo;
        if (c.accept("ONCE")) {
          proc.mode = ProcMode::Once;
        } else if (c.accept("EVERY")) {
          proc.mode = ProcMode::Every;
          const auto n = c.number("liczba klatek po EVERY");
          if (n < 1 || n != std::floor(n))
            throw LineError("EVERY wymaga dodatniej liczby całkowitej klatek");
          proc.every = static_cast<int>(n);
        }
        c.expect_end();
        if (result.program.find(proc.name))
          error(lineNo, std::format("PROC {} jest już zdefiniowana", proc.name));
        result.program.procs.push_back(std::move(proc));
        current = &result.program.procs.back();
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
