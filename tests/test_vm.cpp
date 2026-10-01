#include "boot/XmlLoader.hpp"
#include "vm/Parser.hpp"
#include "vm/Vm.hpp"

#include <doctest/doctest.h>

using namespace PimpEx;
using namespace PimpEx::vm;

namespace {

struct FakeHost : IVmHost {
  std::vector<std::string> calls, added, removed, scenes, guis, logs, errors;

  void call_function(const std::string &name, const std::vector<std::string> &args,
                     int) override {
    std::string text = name;
    for (const auto &a : args)
      text += " " + a;
    calls.push_back(text);
  }
  void request_proc_add(const std::string &name, int) override { added.push_back(name); }
  void request_proc_remove(const std::string &name, int) override { removed.push_back(name); }
  void request_scene(const std::string &name, int) override { scenes.push_back(name); }
  void request_gui(Op op, const std::string &name, int) override {
    guis.push_back((op == Op::GuiShow ? "show " : op == Op::GuiHide ? "hide " : "toggle ") + name);
  }
  void log(const std::string &message) override { logs.push_back(message); }
  void runtime_error(int line, const std::string &message) override {
    errors.push_back(std::to_string(line) + ": " + message);
  }
};

struct Fixture {
  scene::World world;
  ManualInput input;
  FakeHost host;
  Vm vm{42};
  Program program;
  long frame = 0;

  Fixture() {
    Diagnostics diagnostics;
    world.load(boot::parse_scene_xml(R"(<Scene>
  <Actor name="Player" x="100" y="100" w="20" h="20"><Var name="speed" value="3"/></Actor>
  <Actor name="Enemy" x="200" y="100" w="20" h="20"/>
  <Actor name="Game" w="0" h="0"/>
  <Archetype name="Bullet" w="4" h="4" vy="-5"/>
</Scene>)",
                                     "scene.xml", "main", diagnostics));
    REQUIRE(diagnostics.empty());
  }

  void load(const char *source) {
    auto parsed = parse_program(source, "scene.swap");
    std::string text;
    for (const auto &d : parsed.diagnostics)
      text += d.format() + "\n";
    INFO(text);
    REQUIRE(parsed.ok());
    program = std::move(parsed.program);
  }

  void step(int frames = 1) {
    for (int i = 0; i < frames; ++i) {
      FrameContext ctx{input, 800, 600, frame++};
      vm.run_frame(program, world, ctx, host);
    }
  }

  double get(const char *actor, const char *field) { return *world.get(actor, field); }
};

} // namespace

TEST_CASE("VM: MOVE only while the key is held") {
  Fixture f;
  f.load(R"(PROC movement
  IF KEY RIGHT : MOVE Player Player.speed 0
  IF KEY UP : MOVE Player 0 -Player.speed
END)");
  f.step();
  CHECK(f.get("Player", "x") == 100);
  f.input.set_down(Key::Right, true);
  f.step(2);
  CHECK(f.get("Player", "x") == 106);
  f.input.set_down(Key::Up, true);
  f.step();
  CHECK(f.get("Player", "y") == 97);
}

TEST_CASE("VM: SET, ADD, MUL and comparisons") {
  Fixture f;
  f.load(R"(PROC a
  ADD Game.score 2
  IF Game.score >= 6 : SET Game.done 1
  IF Game.score == 4 : MUL Player.speed -1
END)");
  f.step(3);
  CHECK(f.get("Game", "score") == 6);
  CHECK(f.get("Game", "done") == 1);
  CHECK(f.get("Player", "speed") == -3);
}

TEST_CASE("VM: EVERY and ONCE") {
  Fixture f;
  f.load(R"(PROC co_drugi EVERY 2
  ADD Game.a 1
END
PROC raz ONCE
  ADD Game.b 1
END)");
  f.step(4);
  CHECK(f.get("Game", "a") == 2);
  // ONCE runs and asks Boot to remove it – the program here does not change,
  // so count the requests.
  CHECK(f.host.removed.size() == 4);
  CHECK(f.host.removed[0] == "raz");
}

TEST_CASE("VM: SPAWN, DESTROY, FOLLOW, TOUCH, EXISTS") {
  Fixture f;
  f.load(R"(PROC a
  IF NOT EXISTS Shot : SPAWN Bullet AS Shot AT Player.x Player.y
  FOLLOW Enemy Player 60
  IF TOUCH Enemy Player : DESTROY Enemy
END)");
  f.step();
  CHECK(f.world.exists("Shot"));
  CHECK(f.get("Shot", "x") == 100);
  CHECK(f.get("Enemy", "x") == 140);
  f.step(); // reached the player (no overshoot), touched it and vanished
  CHECK_FALSE(f.world.exists("Enemy"));
  f.step();
  CHECK(f.host.errors.size() == 1); // FOLLOW on a missing actor – reported once
}

TEST_CASE("VM: host – CALL, PROC_ADD, PROC_REMOVE, SCENE, LOG") {
  Fixture f;
  f.load(R"(PROC a
  CALL on_hit Enemy 2
  PROC_ADD turbo
  PROC_REMOVE turbo
  SCENE menu
  LOG "Player x={Player.x}, screen {Screen.w}x{Screen.h}, frame {Scene.frame}"
END)");
  f.step();
  CHECK(f.host.calls == std::vector<std::string>{"on_hit Enemy 2"});
  CHECK(f.host.added == std::vector<std::string>{"turbo"});
  CHECK(f.host.removed == std::vector<std::string>{"turbo"});
  CHECK(f.host.scenes == std::vector<std::string>{"menu"});
  CHECK(f.host.logs == std::vector<std::string>{"Player x=100, screen 800x600, frame 0"});
}

TEST_CASE("VM: runtime errors are reported once") {
  Fixture f;
  f.load(R"(PROC a
  MOVE Ghost 1 1
  SET Screen.w 5
  SPAWN Nope
END)");
  f.step(5);
  REQUIRE(f.host.errors.size() == 3);
  CHECK(f.host.errors[0] == "2: actor 'Ghost' does not exist");
  CHECK(f.host.errors[1] == "3: fields of Screen are read-only");
  CHECK(f.host.errors[2] == "4: no archetype 'Nope'");
}

TEST_CASE("VM: TEXT and format_text") {
  Fixture f;
  f.load(R"(PROC a
  SET Game.score 7.5
  TEXT Game "Wynik: {score} / {Player.speed} / {Nope.x}"
END)");
  f.step();
  const auto e = f.world.find("Game");
  const auto &label = f.world.registry().get<scene::Text>(e);
  ManualInput input;
  FrameContext ctx{input, 800, 600, 0};
  CHECK(format_text(f.world, ctx, label.value, "Game") == "Wynik: 7.50 / 3 / ?");
}

TEST_CASE("VM: RANDOM and CHANCE stay in range") {
  Fixture f;
  f.load(R"(PROC a
  SET Game.r RANDOM 5 10
  IF Game.r < 5 : SET Game.bad 1
  IF Game.r > 10 : SET Game.bad 1
  IF CHANCE 50 : ADD Game.hits 1
END)");
  f.step(200);
  CHECK_FALSE(f.world.get("Game", "bad").has_value());
  const double hits = f.get("Game", "hits");
  CHECK(hits > 50);
  CHECK(hits < 150);
}

TEST_CASE("VM: PROC FOR – systems over components and tags, SELF and OTHER") {
  Diagnostics diagnostics;
  auto registry = scene::ComponentRegistry::with_builtins();
  registry->load_custom(R"(<Components><Component name="Health">
  <Field name="hp" default="3"/></Component></Components>)",
                        "components.xml", diagnostics);
  scene::World world(registry);
  world.load(boot::parse_scene_xml(R"(<Scene>
  <Actor name="Player" x="0" y="0" w="10" h="10" tags="hero"><Health hp="5"/></Actor>
  <Actor name="Orc1" x="100" y="0" w="10" h="10" tags="enemy"><Health/></Actor>
  <Actor name="Orc2" x="5" y="0" w="10" h="10" tags="enemy"><Health hp="1"/></Actor>
  <Actor name="Coin" x="0" y="5" w="4" h="4" tags="coin"/>
</Scene>)",
                                   "scene.xml", "main", diagnostics, registry.get()));
  REQUIRE(diagnostics.empty());

  auto parsed = parse_program(R"(PROC regeneracja FOR Health EVERY 2
  ADD SELF.Health.hp 1
END
PROC wrogowie FOR @enemy
  MOVE SELF -1 0
  IF TOUCH SELF @hero : ADD OTHER.Health.hp -1
  IF SELF.Health.hp <= 0 : DESTROY SELF
END
PROC zbieranie
  IF TOUCH Player @coin : SET Player.Text.value "zebrano"
  IF TOUCH Player @coin : DESTROY OTHER
  IF NOT EXISTS @coin : SET Player.label "?"
END
)",
                               "scene.swap");
  std::string text;
  for (const auto &d : parsed.diagnostics)
    text += d.format() + "\n";
  INFO(text);
  REQUIRE(parsed.ok());
  CHECK(parsed.program.procs[0].forEach == "Health");
  CHECK(parsed.program.procs[0].mode == ProcMode::Every);
  CHECK(parsed.program.procs[1].forEach == "@enemy");

  ManualInput input;
  FakeHost host;
  Vm vm(1);
  for (long frame = 0; frame < 2; ++frame) {
    FrameContext ctx{input, 800, 600, frame};
    vm.run_frame(parsed.program, world, ctx, host);
  }
  // Frame 0: regeneration (+1 to all), Orc2 touches the player (-1 to the player).
  // Frame 1: Orc2 touches the player again. Orc1 walks left.
  CHECK(world.get("Orc1", "x") == 98);
  CHECK(world.get("Player", "Health.hp") == 4); // 5 + 1 - 1 - 1
  CHECK_FALSE(world.exists("Coin"));
  REQUIRE(host.errors.size() == 1); // SET Player.label "?" – variables hold numbers only
  CHECK(host.errors[0].find("only hold numbers") != std::string::npos);
  CHECK(std::get<std::string>(*world.get_field("Player", "Text.value")) == "zebrano");
}

TEST_CASE("VM: SELF outside FOR and MOVE in 3D") {
  Fixture f;
  f.load(R"(PROC a
  MOVE SELF 1 1
  MOVE Player 1 2 3
END)");
  f.step();
  CHECK(f.get("Player", "x") == 101);
  CHECK(f.get("Player", "z") == 3);
  REQUIRE(f.host.errors.size() == 1);
  CHECK(f.host.errors[0] == "2: SELF only works in PROC ... FOR");
}

TEST_CASE("VM: GUI_* goes to the host, Mouse.left reads the button") {
  Fixture f;
  f.load("PROC gui\n  IF Mouse.left == 1 : GUI_SHOW pause\n  GUI_TOGGLE hud\n  GUI_HIDE menu\nEND\n");
  f.step();
  CHECK(f.host.guis == std::vector<std::string>{"toggle hud", "hide menu"});
  f.input.set_mouse_down(MouseButton::Left, true);
  f.step();
  CHECK(f.host.guis.size() == 5);
  CHECK(f.host.guis[2] == "show pause");
}

TEST_CASE("VM: run_instructions runs instructions outside a PROC") {
  Fixture f;
  const auto parsed = parse_instructions("SET Game.level 3; MOVE Player 5 0; SCENE game_over", "gui/menu.xml", 2);
  REQUIRE(parsed.ok());
  FrameContext ctx{f.input, 800, 600, 0};
  f.vm.run_instructions(parsed.instructions, f.world, ctx, f.host);
  CHECK(f.get("Game", "level") == 3);
  CHECK(f.get("Player", "x") == 105);
  CHECK(f.host.scenes == std::vector<std::string>{"game_over"});
}
