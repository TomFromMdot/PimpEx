#include "vm/Parser.hpp"

#include <doctest/doctest.h>

using namespace PimpEx;
using namespace PimpEx::vm;

namespace {
std::string errors_of(const ParseResult &r) {
  std::string out;
  for (const auto &d : r.diagnostics)
    out += d.format() + "\n";
  return out;
}
} // namespace

TEST_CASE("parser: prosta PROC z ruchem i warunkiem") {
  const auto r = parse_program(R"(# ruch gracza
PROC move_player
  IF KEY W : MOVE Player 0 -2
  IF key s : move Player 0 2
END
)",
                               "scene.swap");
  INFO(errors_of(r));
  REQUIRE(r.ok());
  REQUIRE(r.program.procs.size() == 1);
  const auto &proc = r.program.procs[0];
  CHECK(proc.name == "move_player");
  CHECK(proc.mode == ProcMode::Loop);
  CHECK(proc.firstLine == 2);
  CHECK(proc.lastLine == 5);
  REQUIRE(proc.body.size() == 2);
  const auto &move = proc.body[0];
  CHECK(move.op == Op::Move);
  CHECK(move.line == 3);
  CHECK(move.target == "Player");
  REQUIRE(move.guard.size() == 1);
  CHECK(move.guard[0].kind == Condition::Kind::KeyDown);
  CHECK(move.guard[0].key == Key::W);
  CHECK(move.values[1].number == doctest::Approx(-2));
  CHECK(proc.body[1].guard[0].key == Key::S);
}

TEST_CASE("parser: tryby ONCE i EVERY") {
  const auto r = parse_program("PROC a ONCE\nEND\nPROC b EVERY 30\nEND\n", "t");
  INFO(errors_of(r));
  REQUIRE(r.ok());
  CHECK(r.program.procs[0].mode == ProcMode::Once);
  CHECK(r.program.procs[1].mode == ProcMode::Every);
  CHECK(r.program.procs[1].every == 30);
}

TEST_CASE("parser: wszystkie instrukcje") {
  const auto r = parse_program(R"(PROC all
  SET Ball.vx -Ball.vx
  ADD Game.score 1
  MUL Ball.vy 1.1
  SPAWN Bullet AS b1 AT Player.x Player.y
  SPAWN Bullet
  DESTROY Enemy
  FOLLOW Enemy Player 1.5
  TEXT Score "Wynik: {Game.score}"
  LOG "Cześć!"
  LOG bez cudzysłowu też działa
  CALL on_hit Enemy 3
  PROC_ADD shoot
  PROC_REMOVE shoot
  SCENE menu
  IF TOUCH Ball Paddle AND NOT KEY SPACE : SET Ball.vy RANDOM 1 3
  IF Ball.x > 780 : IF CHANCE 50 : SET Ball.vx -4
  IF EXISTS Enemy AND PRESSED 1 : LOG hit
END)",
                               "t");
  INFO(errors_of(r));
  REQUIRE(r.ok());
  const auto &b = r.program.procs[0].body;
  REQUIRE(b.size() == 17);
  CHECK(b[0].op == Op::Set);
  CHECK(b[0].ref.actor == "Ball");
  CHECK(b[0].ref.field == "vx");
  CHECK(b[0].values[0].kind == Value::Kind::Ref);
  CHECK(b[0].values[0].negate);
  CHECK(b[3].other == "b1");
  CHECK(b[3].values.size() == 2);
  CHECK(b[4].values.empty());
  CHECK(b[7].text == "Wynik: {Game.score}");
  CHECK(b[9].text == "bez cudzysłowu też działa");
  CHECK(b[10].args == std::vector<std::string>{"Enemy", "3"});
  CHECK(b[14].guard.size() == 2);
  CHECK(b[14].guard[1].negate);
  CHECK(b[14].values[0].kind == Value::Kind::Random);
  CHECK(b[15].guard.size() == 2);
  CHECK(b[15].guard[0].op == CompareOp::Greater);
  CHECK(b[16].guard[1].key == Key::Num1);
}

TEST_CASE("parser: błędy mają numer linii i podpowiedź") {
  const auto r = parse_program(R"(PROC a
  MOOVE Player 1 1
  MOVE Player 1
  IF KEY W MOVE Player 1 1
  SET Player 5
END
MOVE Player 1 1
END
PROC b
)",
                               "scene.swap");
  REQUIRE_FALSE(r.ok());
  const auto text = errors_of(r);
  INFO(text);
  CHECK(r.diagnostics.size() == 7);
  CHECK(text.find("scene.swap:2: błąd: nieznana instrukcja 'MOOVE'. Czy chodziło o MOVE?") !=
        std::string::npos);
  CHECK(text.find("scene.swap:3:") != std::string::npos);
  CHECK(text.find("scene.swap:4: błąd: po warunku IF potrzebny jest dwukropek") !=
        std::string::npos);
  CHECK(text.find("scene.swap:5:") != std::string::npos);
  CHECK(text.find("scene.swap:7: błąd: instrukcja poza PROC") != std::string::npos);
  CHECK(text.find("scene.swap:8: błąd: END bez PROC") != std::string::npos);
  CHECK(text.find("scene.swap:9: błąd: PROC b nie ma END") != std::string::npos);
}

TEST_CASE("parser: powtórzona nazwa PROC i zły klawisz") {
  const auto r = parse_program("PROC a\nIF KEY FOO : LOG x\nEND\nPROC a\nEND\n", "t");
  CHECK(r.diagnostics.size() == 2);
}

TEST_CASE("parser: niezamknięty cudzysłów") {
  const auto r = parse_program("PROC a\nLOG \"ups\nEND\n", "t");
  REQUIRE(r.diagnostics.size() == 1);
  CHECK(r.diagnostics[0].line == 2);
}
