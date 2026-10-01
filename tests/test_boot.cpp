#include "boot/Boot.hpp"
#include "boot/FileWatcher.hpp"
#include "boot/ProjectCheck.hpp"
#include "boot/SwapFile.hpp"
#include "boot/XmlLoader.hpp"
#include "core/Engine.hpp"

#include <doctest/doctest.h>

#include <fstream>

using namespace PimpEx;
using namespace PimpEx::boot;

namespace {

// A temporary game project, removed after the test.
struct TempProject {
  fs::path root;

  explicit TempProject(const std::string &name) {
    root = fs::temp_directory_path() / ("pimpex_test_" + name);
    fs::remove_all(root);
    write("pimpex.xml", R"(<Project name="t"><Window width="800" height="600"/>
<StartScene name="main"/></Project>)");
    write("scenes/main/scene.xml", R"(<Scene>
  <Actor name="Player" x="100" y="100" w="20" h="20"/>
  <Actor name="Game" w="0" h="0"/>
  <Archetype name="Dot" w="2" h="2"/>
</Scene>)");
    write("scenes/main/scene.swap", "# my procs\nPROC tick\n  ADD Game.frames 1\nEND\n");
    write("scenes/main/library.swap", "PROC boost\n  ADD Player.x 10\nEND\n");
    write("scripts/game.lua", R"(
function on_call(who, amount)
  set("Game", "called", amount)
  spawn("Dot", "Kropka", get(who, "x"), 5)
end
function paste_once()
  swap.paste("PROC from_lua ONCE\n  SET Game.pasted 1\nEND\n")
end
)");
  }
  ~TempProject() { fs::remove_all(root); }

  void write(const std::string &rel, const std::string &content) const {
    fs::create_directories((root / rel).parent_path());
    std::ofstream(root / rel) << content;
  }
  std::string read(const std::string &rel) const { return read_file(root / rel).value_or(""); }
  ProjectPaths paths() const { return ProjectPaths{root}; }
};

struct BootFixture {
  TempProject project;
  Log log;
  Boot boot;
  vm::ManualInput input;
  long frame = 0;

  explicit BootFixture(const std::string &name)
      : project(name), boot((log.set_quiet(true), project.paths()), log, 1) {
    REQUIRE(boot.start());
  }
  void step(int frames = 1) {
    for (int i = 0; i < frames; ++i) {
      const vm::FrameContext ctx{input, 800, 600, frame++};
      boot.run_frame(ctx);
    }
  }
  double get(const char *actor, const char *field) { return *boot.world().get(actor, field); }
};

} // namespace

TEST_CASE("SwapFile: pasting and cutting procs keeps the rest of the file") {
  const std::string text = "# comment\nPROC a\n  LOG a\nEND\n\nPROC b\n  LOG b\nEND\n";
  const auto parsed = vm::parse_program(text, "t");
  REQUIRE(parsed.ok());
  CHECK(extract_proc_source(text, *parsed.program.find("b")) == "PROC b\n  LOG b\nEND\n");
  CHECK(remove_proc(text, *parsed.program.find("a")) == "# comment\nPROC b\n  LOG b\nEND\n");
  CHECK(remove_proc(text, *parsed.program.find("b")) == "# comment\nPROC a\n  LOG a\nEND\n\n");
  CHECK(append_proc("PROC a\nEND", "PROC c\nEND\n") == "PROC a\nEND\n\nPROC c\nEND\n");
  CHECK(append_proc("", "PROC c\nEND\n") == "PROC c\nEND\n");
}

TEST_CASE("FileWatcher: reports changes, skips the engine's own writes") {
  TempProject project("watcher");
  const auto file = project.root / "scenes/main/scene.swap";
  std::vector<std::string> changes;
  FileWatcher watcher([&](const fs::path &, std::string content) { changes.push_back(content); });
  watcher.watch({file});

  watcher.poll_once();
  CHECK(changes.empty());

  // A user change (the mtime is moved by hand – the file system clock may be coarse).
  project.write("scenes/main/scene.swap", "PROC x\nEND\n");
  fs::last_write_time(file, fs::last_write_time(file) + std::chrono::seconds(1));
  watcher.poll_once();
  REQUIRE(changes.size() == 1);
  CHECK(changes[0] == "PROC x\nEND\n");

  // An engine write: expect first, then the file – no event.
  const std::string own = "PROC y\nEND\n";
  watcher.expect(file, content_hash(own));
  write_file_atomic(file, own);
  fs::last_write_time(file, fs::last_write_time(file) + std::chrono::seconds(2));
  watcher.poll_once();
  CHECK(changes.size() == 1);
}

TEST_CASE("Boot: PROC_ADD and PROC_REMOVE edit scene.swap on disk") {
  BootFixture f("proc_add");
  f.project.write("scenes/main/scene.swap", R"(PROC tick
  ADD Game.frames 1
  IF Game.frames == 2 : PROC_ADD boost
  IF Game.frames == 4 : PROC_REMOVE boost
END
)");
  // Reload the scene so Boot sees the new file.
  REQUIRE(f.boot.load_scene("main"));
  f.step(2);
  CHECK(f.project.read("scenes/main/scene.swap").find("PROC boost") != std::string::npos);
  CHECK(f.boot.program().find("boost"));
  f.step(2);
  CHECK(f.get("Player", "x") == 120); // boost ran in frames 3 and 4
  CHECK(f.project.read("scenes/main/scene.swap").find("PROC boost") == std::string::npos);
  CHECK_FALSE(f.boot.program().find("boost"));
}

TEST_CASE("Boot: PROC ONCE runs once and leaves the file") {
  BootFixture f("once");
  f.project.write("scenes/main/scene.swap", "PROC tick\n  ADD Game.frames 1\nEND\n\n"
                                            "PROC hello ONCE\n  ADD Game.hello 1\nEND\n");
  REQUIRE(f.boot.load_scene("main"));
  f.step(5);
  CHECK(f.get("Game", "hello") == 1);
  CHECK(f.get("Game", "frames") == 5);
  CHECK(f.project.read("scenes/main/scene.swap") == "PROC tick\n  ADD Game.frames 1\nEND\n\n");
}

TEST_CASE("Boot: CALL into Lua and swap.paste") {
  BootFixture f("lua");
  f.project.write("scenes/main/scene.swap",
                  "PROC a\n  IF Scene.frame == 0 : CALL on_call Player 7\n"
                  "  IF Scene.frame == 1 : CALL paste_once\nEND\n");
  REQUIRE(f.boot.load_scene("main"));
  f.step();
  CHECK(f.get("Game", "called") == 7);
  CHECK(f.boot.world().exists("Kropka"));
  CHECK(f.get("Kropka", "x") == 100);
  f.step(); // Lua pastes PROC from_lua ONCE
  CHECK(f.project.read("scenes/main/scene.swap").find("PROC from_lua ONCE") != std::string::npos);
  f.step(); // ONCE runs and disappears
  CHECK(f.get("Game", "pasted") == 1);
  CHECK(f.project.read("scenes/main/scene.swap").find("from_lua") == std::string::npos);
}

TEST_CASE("Boot: a broken hot-reload keeps the old program and the file") {
  BootFixture f("broken");
  f.step();
  CHECK(f.get("Game", "frames") == 1);

  const std::string broken = "PROC tick\n  ADD Game.frames 1\n  PROC_ADD boost\n  MOOVE x\nEND\n";
  f.boot.apply_swap(broken, vm::parse_program(broken, "scene.swap"));
  f.step();
  CHECK(f.get("Game", "frames") == 2); // the old program keeps running
  CHECK(f.boot.program().procs.size() == 1);

  // A file change request while the file has errors: memory only.
  f.boot.request_proc_add("boost", 0);
  f.step();
  CHECK(f.boot.program().find("boost"));
  f.project.write("scenes/main/scene.swap", broken);
  f.boot.request_proc_remove("boost", 0);
  f.step();
  CHECK(f.project.read("scenes/main/scene.swap") == broken);

  const std::string fixed = "PROC tick\n  ADD Game.frames 10\nEND\n";
  f.boot.apply_swap(fixed, vm::parse_program(fixed, "scene.swap"));
  f.step();
  CHECK(f.get("Game", "frames") == 14); // 4 frames of the old program + 10
}

TEST_CASE("Boot: hot-reload of scene.xml and a Lua script") {
  BootFixture f("reload");
  f.boot.world().set("Player", "x", 300);
  Diagnostics diagnostics;
  f.boot.apply_scene_xml(parse_scene_xml(R"(<Scene>
  <Actor name="Player" x="100" y="100" w="50" h="20"/>
  <Actor name="Game" w="0" h="0"/>
</Scene>)",
                                         "scene.xml", "main", diagnostics),
                         diagnostics);
  CHECK(f.get("Player", "x") == 300);
  CHECK(f.get("Player", "w") == 50);

  f.boot.apply_script(f.project.root / "scripts/game.lua",
                      "function on_call() set('Game', 'v2', 1) end");
  f.boot.call_function("on_call", {}, 1);
  CHECK(f.get("Game", "v2") == 1);
}

TEST_CASE("Engine: a headless run with scripted keys writes the state") {
  TempProject project("engine");
  project.write("scenes/main/scene.swap", R"(PROC movement
  IF KEY RIGHT : MOVE Player 2 0
  IF PRESSED SPACE : ADD Game.jumps 1
END
)");
  EngineOptions options;
  options.headless = true;
  options.frames = 30;
  options.quiet = true;
  options.watch = false;
  options.input = "RIGHT:0-9 SPACE:3 SPACE:20";
  CHECK(Engine(project.root, options).run() == 0);
  const auto state = project.read(".pimpex/state.txt");
  INFO(state);
  CHECK(state.find("frame: 30") != std::string::npos);
  CHECK(state.find("Player x=120 y=100") != std::string::npos);
  CHECK(state.find("jumps=2") != std::string::npos);
}

TEST_CASE("Boot: Lua behaviors (on_start/on_update/on_touch/on_destroy), systems and hot-reload") {
  BootFixture f("behaviors");
  f.project.write("components.xml", R"(<Components><Component name="Health">
  <Field name="hp" default="1"/></Component></Components>)");
  f.project.write("scripts/behaviors/patrol.lua", R"(
return {
  on_start = function(self)
    self.dir = 1
    set("Game", "starts", (get("Game", "starts") or 0) + 1)
  end,
  on_update = function(self)
    self:move(self.speed * self.dir, 0)
    if self.x >= 130 then self.dir = -1 end
  end,
  on_touch = function(self, other)
    set("Game", "touched_" .. other, 1)
  end,
  on_destroy = function(self)
    set("Game", "destroyed", 1)
  end,
}
)");
  f.project.write("scripts/systems.lua", R"(
system("Health", function(e) e:set("Health.hp", e:get("Health.hp") + 1) end)
system("@hero", function(e) e.y = e.y + 1 end)
)");
  f.project.write("scenes/main/scene.xml", R"(<Scene>
  <Actor name="Enemy" x="100" y="100" w="10" h="10">
    <Collider kind="trigger"/>
    <Behavior name="patrol" speed="2"/>
  </Actor>
  <Actor name="Wall" x="140" y="100" w="10" h="10"><Collider/></Actor>
  <Actor name="Player" y="0" tags="hero"><Health/></Actor>
  <Actor name="Game" w="0" h="0"/>
</Scene>)");
  f.project.write("scenes/main/scene.swap", "PROC a\nEND\n");
  // A new Boot so it loads components.xml and scripts from disk.
  Log log;
  log.set_quiet(true);
  Boot boot(f.project.paths(), log, 1);
  REQUIRE(boot.start());
  vm::ManualInput input;
  auto step = [&](int n) {
    for (int i = 0; i < n; ++i) {
      static long frame = 0;
      const vm::FrameContext ctx{input, 800, 600, frame++};
      boot.run_frame(ctx);
    }
  };
  auto get = [&](const char *a, const char *field) { return boot.world().get(a, field).value_or(-999); };

  step(1);
  CHECK(get("Game", "starts") == 1);
  CHECK(get("Enemy", "x") == 102);
  CHECK(get("Player", "Health.hp") == 2);
  CHECK(get("Player", "y") == 1);
  step(14); // 102 -> 130 and turns back (dir = -1)
  CHECK(get("Enemy", "x") == 130);
  CHECK(get("Game", "touched_Wall") == 1); // the trigger touched the wall (135..145 vs 125..135)
  CHECK(get("Game", "starts") == 1);       // on_start only once

  // Behavior hot-reload: new functions, old `self` state (dir = -1).
  boot.apply_script(f.project.root / "scripts/behaviors/patrol.lua", R"(
return {
  on_update = function(self) self:move(10 * self.dir, 0) end,
  on_destroy = function(self) set("Game", "destroyed", 1) end,
}
)");
  step(1);
  CHECK(get("Enemy", "x") == 120);

  boot.world().destroy("Enemy");
  step(1);
  CHECK(get("Game", "destroyed") == 1);
}

TEST_CASE("Lua: self:sees, self:look, self:face and distance") {
  BootFixture f("sees");
  f.project.write("scripts/behaviors/watcher.lua", R"(
return {
  on_update = function(self)
    set("Game", "sees", self:sees("Player") and 1 or 0)
    local hit, dist = self:look(50)
    set("Game", "look_" .. (hit or "nic"), dist or 0)
    self:face("Player")
    set("Game", "dist", distance(self.name, "Player"))
  end,
}
)");
  f.project.write("scenes/main/scene.xml", R"(<Scene mode="3d">
  <Actor name="Guard" y="1"><Collider/><Behavior name="watcher"/></Actor>
  <Actor name="Player" x="10" y="1"><Collider/></Actor>
  <Actor name="Wall" z="-4" y="1" w="4" h="2" d="1"><Collider/></Actor>
  <Actor name="Game" w="0" h="0"/>
</Scene>)");
  f.project.write("scenes/main/scene.swap", "PROC a\nEND\n");
  Log log;
  log.set_quiet(true);
  Boot boot(f.project.paths(), log, 1);
  REQUIRE(boot.start());
  vm::ManualInput input;
  const vm::FrameContext ctx{input, 800, 600, 0};
  boot.run_frame(ctx);
  auto get = [&](const char *a, const char *field) { return boot.world().get(a, field).value_or(-999); };
  CHECK(get("Game", "sees") == 1);
  CHECK(get("Game", "look_Wall") == doctest::Approx(3.5)); // ry=0 looks towards -z
  CHECK(get("Guard", "ry") == doctest::Approx(-90));       // turned to the player (+x)
  CHECK(get("Game", "dist") == doctest::Approx(10));

  // A wall between the guard and the player – no longer visible.
  boot.world().set("Wall", "x", 5);
  boot.world().set("Wall", "z", 0);
  boot.run_frame(ctx);
  CHECK(get("Game", "sees") == 0);
}

TEST_CASE("Boot: GUI – <Scene gui>, clicks run instructions, pause stops the logic") {
  TempProject project("gui");
  project.write("gui/hud.xml", R"(<Gui><Label text="Klatki: {Game.frames}"/></Gui>)");
  project.write("gui/pause.xml", R"(<Gui pause="true">
  <Button anchor="center" text="Resume" key="ESCAPE" click="GUI_HIDE pause; ADD Game.resumed 1"/>
  <Button anchor="center" y="80" text="Quit" click="SCENE game_over"/>
</Gui>)");
  project.write("scenes/main/scene.xml", R"(<Scene gui="hud">
  <Actor name="Game" w="0" h="0"/>
</Scene>)");
  project.write("scenes/main/scene.swap",
                "PROC tick\n  ADD Game.frames 1\n  IF PRESSED ESCAPE : GUI_TOGGLE pause\nEND\n");
  project.write("scenes/game_over/scene.xml", R"(<Scene><Actor name="Title" w="0" h="0"/></Scene>)");
  project.write("scenes/game_over/scene.swap", "");

  Log log;
  log.set_quiet(true);
  Boot boot(project.paths(), log, 1);
  REQUIRE(boot.start());
  CHECK(boot.gui().shown() == std::vector<std::string>{"hud"});

  vm::ManualInput input;
  long frame = 0;
  auto step = [&] {
    const vm::FrameContext ctx{input, 800, 600, frame++};
    boot.run_frame(ctx);
    input.set_pressed(vm::Key::Escape, false);
  };
  step();
  CHECK(*boot.world().get("Game", "frames") == 1);

  // ESC in a procedure shows the pause – the logic stops.
  input.set_pressed(vm::Key::Escape, true);
  step();
  CHECK(boot.gui().is_shown("pause"));
  step();
  step();
  CHECK(*boot.world().get("Game", "frames") == 2);

  // ESC = "Resume" (key="ESCAPE"): the pause goes away and does not come back in the same frame.
  input.set_pressed(vm::Key::Escape, true);
  step();
  CHECK_FALSE(boot.gui().is_shown("pause"));
  CHECK(*boot.world().get("Game", "resumed") == 1);
  step();
  CHECK(*boot.world().get("Game", "frames") == 3);

  // A click on "Quit" changes the scene and the <Scene gui> GUIs hide.
  REQUIRE(boot.gui().show("pause"));
  input.set_mouse(400, 380);
  input.set_mouse_down(vm::MouseButton::Left, true);
  step();
  input.set_mouse_down(vm::MouseButton::Left, false);
  step();
  CHECK(boot.scene_name() == "game_over");
  CHECK(boot.gui().shown().empty());
}

TEST_CASE("Boot: GUI from Lua and unknown GUI errors") {
  BootFixture f("gui_lua");
  f.project.write("gui/menu.xml", R"(<Gui><Button text="x" click="GUI_SHOW missing"/></Gui>)");
  f.project.write("scripts/ui.lua", R"(
function show_menu() gui.show("menu"); set("Game", "shown", gui.shown("menu") and 1 or 0) end
function toggle_menu() set("Game", "state", gui.toggle("menu") and 1 or 0) end
function bad() gui.show("missing") end
)");
  REQUIRE(f.boot.start());
  f.project.write("scenes/main/scene.swap", "PROC a ONCE\n  CALL show_menu\n  CALL toggle_menu\n  GUI_SHOW nope\nEND\n");
  REQUIRE(f.boot.load_scene("main"));
  f.step();
  CHECK(f.get("Game", "shown") == 1);
  CHECK(f.get("Game", "state") == 0);
  CHECK_FALSE(f.boot.gui().is_shown("menu"));
  auto logText = [&] {
    std::string text;
    for (const auto &line : f.log.tail(50))
      text += line + "\n";
    return text;
  };
  const auto errors = logText();
  INFO(errors);
  CHECK(errors.find("scenes/main/scene.swap:4: error: no GUI 'nope' (file gui/nope.xml)") != std::string::npos);
  CHECK(f.boot.lua().eval("bad()").output.find("gui.show: no GUI 'missing'") != std::string::npos);

  // An error in a click points at the GUI file, not scene.swap.
  f.boot.gui().show("menu");
  f.input.set_mouse(50, 10);
  f.input.set_mouse_down(vm::MouseButton::Left, true);
  f.step();
  f.input.set_mouse_down(vm::MouseButton::Left, false);
  f.step();
  CHECK(logText().find("gui/menu.xml:1: error: no GUI 'missing'") != std::string::npos);
}

TEST_CASE("Engine: a click from --input CLICK@x/y and GUI in state.txt") {
  TempProject project("gui_engine");
  project.write("gui/menu.xml",
                R"(<Gui><Button anchor="center" text="Graj" click="SET Game.started 1; GUI_HIDE menu"/></Gui>)");
  project.write("gui/hud.xml", R"(<Gui><Label text="hud"/></Gui>)");
  project.write("scenes/main/scene.xml", R"(<Scene gui="hud menu"><Actor name="Game" w="0" h="0"/></Scene>)");
  EngineOptions options;
  options.headless = true;
  options.frames = 20;
  options.quiet = true;
  options.watch = false;
  options.input = "CLICK@400/300:5";
  CHECK(Engine(project.root, options).run() == 0);
  const auto state = project.read(".pimpex/state.txt");
  INFO(state);
  CHECK(state.find("started=1") != std::string::npos);
  CHECK(state.find("gui: hud\n") != std::string::npos);

  vm::ScriptedInput input;
  CHECK(input.parse("CLICK@400:5").find("CLICK@x/y") != std::string::npos);
}

TEST_CASE("check: GUI – unknown GUIs, scenes and functions in clicks") {
  TempProject project("gui_check");
  project.write("gui/menu.xml", R"(<Gui>
  <Button text="a" click="SCENE missing"/>
  <Button text="b" click="GUI_SHOW nope; CALL missing_fn"/>
  <Button text="c" click="SCENE main; GUI_HIDE menu"/>
</Gui>)");
  project.write("scenes/main/scene.xml", R"(<Scene gui="menu hud">
  <Actor name="Game" w="0" h="0"/>
</Scene>)");
  project.write("scenes/main/scene.swap", "PROC a\n  IF PRESSED ESCAPE : GUI_TOGGLE pause\nEND\n");
  CheckSummary summary;
  const auto diagnostics = check_project(project.paths(), summary);
  std::string text;
  for (const auto &d : diagnostics)
    text += d.format() + "\n";
  INFO(text);
  CHECK(summary.guis == 1);
  CHECK(text.find("gui/menu.xml:2: error: no scene 'missing'") != std::string::npos);
  CHECK(text.find("gui/menu.xml:3: error: no GUI 'nope'") != std::string::npos);
  CHECK(text.find("gui/menu.xml:3: error: no Lua function 'missing_fn'") != std::string::npos);
  CHECK(text.find("gui/menu.xml:4") == std::string::npos);
  CHECK(text.find("scenes/main/scene.xml:1: error: <Scene gui>: no GUI 'hud'") != std::string::npos);
  CHECK(text.find("scenes/main/scene.swap:2: error: no GUI 'pause'") != std::string::npos);
}
