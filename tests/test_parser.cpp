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

TEST_CASE("parser: a simple PROC with movement and a condition") {
  const auto r = parse_program(R"(# player movement
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

TEST_CASE("parser: ONCE and EVERY modes") {
  const auto r = parse_program("PROC a ONCE\nEND\nPROC b EVERY 30\nEND\n", "t");
  INFO(errors_of(r));
  REQUIRE(r.ok());
  CHECK(r.program.procs[0].mode == ProcMode::Once);
  CHECK(r.program.procs[1].mode == ProcMode::Every);
  CHECK(r.program.procs[1].every == 30);
}

TEST_CASE("parser: all instructions") {
  const auto r = parse_program(R"(PROC all
  SET Ball.vx -Ball.vx
  ADD Game.score 1
  MUL Ball.vy 1.1
  SPAWN Bullet AS b1 AT Player.x Player.y
  SPAWN Bullet
  DESTROY Enemy
  FOLLOW Enemy Player 1.5
  TEXT Score "Wynik: {Game.score}"
  LOG "Hello!"
  LOG works without quotes too
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
  CHECK(b[9].text == "works without quotes too");
  CHECK(b[10].args == std::vector<std::string>{"Enemy", "3"});
  CHECK(b[14].guard.size() == 2);
  CHECK(b[14].guard[1].negate);
  CHECK(b[14].values[0].kind == Value::Kind::Random);
  CHECK(b[15].guard.size() == 2);
  CHECK(b[15].guard[0].op == CompareOp::Greater);
  CHECK(b[16].guard[1].key == Key::Num1);
}

TEST_CASE("parser: errors have a line number and a hint") {
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
  CHECK(text.find("scene.swap:2: error: unknown instruction 'MOOVE'. Did you mean MOVE?") !=
        std::string::npos);
  CHECK(text.find("scene.swap:3:") != std::string::npos);
  CHECK(text.find("scene.swap:4: error: the IF condition needs a colon") !=
        std::string::npos);
  CHECK(text.find("scene.swap:5:") != std::string::npos);
  CHECK(text.find("scene.swap:7: error: instruction outside a PROC") != std::string::npos);
  CHECK(text.find("scene.swap:8: error: END without PROC") != std::string::npos);
  CHECK(text.find("scene.swap:9: error: PROC b has no END") != std::string::npos);
}

TEST_CASE("parser: duplicate PROC name and a bad key") {
  const auto r = parse_program("PROC a\nIF KEY FOO : LOG x\nEND\nPROC a\nEND\n", "t");
  CHECK(r.diagnostics.size() == 2);
}

TEST_CASE("parser: unclosed quote") {
  const auto r = parse_program("PROC a\nLOG \"ups\nEND\n", "t");
  REQUIRE(r.diagnostics.size() == 1);
  CHECK(r.diagnostics[0].line == 2);
}

TEST_CASE("parser: FOR, @tags, component fields and text in SET") {
  const auto r = parse_program(R"(PROC s FOR @enemy EVERY 10
  IF TOUCH SELF @coin : SET SELF.Health.hp 3
  SET Player.Text.value "Hej {Game.score}"
  SPAWN Box AT 1 2 3
END
PROC t ONCE FOR Health
END
PROC u FOR
END
PROC v FOR a.b
END
PROC w NOW
END)",
                               "t");
  INFO(errors_of(r));
  REQUIRE(r.diagnostics.size() == 3);
  CHECK(r.diagnostics[0].line == 8);
  CHECK(r.diagnostics[1].line == 10);
  CHECK(r.diagnostics[2].message.find("unknown word 'NOW'") != std::string::npos);
  const auto &s = r.program.procs[0];
  CHECK(s.forEach == "@enemy");
  CHECK(s.every == 10);
  CHECK(s.body[0].guard[0].b == "@coin");
  CHECK(s.body[0].ref.actor == "SELF");
  CHECK(s.body[0].ref.field == "Health.hp");
  CHECK(s.body[1].values[0].kind == Value::Kind::Text);
  CHECK(s.body[1].values[0].text == "Hej {Game.score}");
  CHECK(s.body[2].values.size() == 3);
  CHECK(r.program.procs[1].mode == ProcMode::Once);
  CHECK(r.program.procs[1].forEach == "Health");
}

TEST_CASE("parser: GUI_SHOW, GUI_HIDE and GUI_TOGGLE") {
  const auto r = parse_program("PROC menu\n  IF PRESSED ESCAPE : GUI_TOGGLE pause\n  GUI_SHOW hud\n  GUI_HIDE menu\nEND\n",
                               "scene.swap");
  INFO(errors_of(r));
  REQUIRE(r.ok());
  const auto &body = r.program.procs[0].body;
  REQUIRE(body.size() == 3);
  CHECK(body[0].op == Op::GuiToggle);
  CHECK(body[0].target == "pause");
  CHECK(body[1].op == Op::GuiShow);
  CHECK(body[2].op == Op::GuiHide);
  CHECK_FALSE(parse_program("PROC a\n  GUI_SHOW\nEND\n", "s").ok());
}

TEST_CASE("parser: instructions separated by semicolons (GUI button click)") {
  const auto r = parse_instructions(R"(SET Game.level 2; TEXT Hud "a;b" ; GUI_HIDE menu; SCENE level)", "gui/menu.xml", 7);
  REQUIRE(r.ok());
  REQUIRE(r.instructions.size() == 4);
  CHECK(r.instructions[0].op == Op::Set);
  CHECK(r.instructions[1].text == "a;b");
  CHECK(r.instructions[2].op == Op::GuiHide);
  CHECK(r.instructions[3].op == Op::Scene);
  CHECK(r.instructions[3].line == 7);
  CHECK(parse_instructions("", "f", 1).instructions.empty());

  const auto bad = parse_instructions("SCENE level; JUMPP 3", "gui/menu.xml", 4);
  REQUIRE_FALSE(bad.ok());
  CHECK(bad.diagnostics[0].format().starts_with("gui/menu.xml:4: error: unknown instruction 'JUMPP'"));
  CHECK_FALSE(parse_instructions("PROC x", "f", 1).ok());
}
