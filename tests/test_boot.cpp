#include "boot/Boot.hpp"
#include "boot/FileWatcher.hpp"
#include "boot/SwapFile.hpp"
#include "boot/XmlLoader.hpp"
#include "core/Engine.hpp"

#include <doctest/doctest.h>

#include <fstream>

using namespace PimpEx;
using namespace PimpEx::boot;

namespace {

// Tymczasowy projekt gry w katalogu testowym, sprzątany po teście.
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
    write("scenes/main/scene.swap", "# moje procedury\nPROC tick\n  ADD Game.frames 1\nEND\n");
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

TEST_CASE("SwapFile: wklejanie i wycinanie procedur zachowuje resztę pliku") {
  const std::string text = "# komentarz\nPROC a\n  LOG a\nEND\n\nPROC b\n  LOG b\nEND\n";
  const auto parsed = vm::parse_program(text, "t");
  REQUIRE(parsed.ok());
  CHECK(extract_proc_source(text, *parsed.program.find("b")) == "PROC b\n  LOG b\nEND\n");
  CHECK(remove_proc(text, *parsed.program.find("a")) == "# komentarz\nPROC b\n  LOG b\nEND\n");
  CHECK(remove_proc(text, *parsed.program.find("b")) == "# komentarz\nPROC a\n  LOG a\nEND\n\n");
  CHECK(append_proc("PROC a\nEND", "PROC c\nEND\n") == "PROC a\nEND\n\nPROC c\nEND\n");
  CHECK(append_proc("", "PROC c\nEND\n") == "PROC c\nEND\n");
}

TEST_CASE("FileWatcher: zgłasza zmiany, pomija własne zapisy silnika") {
  TempProject project("watcher");
  const auto file = project.root / "scenes/main/scene.swap";
  std::vector<std::string> changes;
  FileWatcher watcher([&](const fs::path &, std::string content) { changes.push_back(content); });
  watcher.watch({file});

  watcher.poll_once();
  CHECK(changes.empty());

  // Zmiana od użytkownika (czas modyfikacji przesuwamy ręcznie – system
  // plików może mieć zbyt grubą rozdzielczość czasu).
  project.write("scenes/main/scene.swap", "PROC x\nEND\n");
  fs::last_write_time(file, fs::last_write_time(file) + std::chrono::seconds(1));
  watcher.poll_once();
  REQUIRE(changes.size() == 1);
  CHECK(changes[0] == "PROC x\nEND\n");

  // Zapis silnika: najpierw expect, potem plik – brak zdarzenia.
  const std::string own = "PROC y\nEND\n";
  watcher.expect(file, content_hash(own));
  write_file_atomic(file, own);
  fs::last_write_time(file, fs::last_write_time(file) + std::chrono::seconds(2));
  watcher.poll_once();
  CHECK(changes.size() == 1);
}

TEST_CASE("Boot: PROC_ADD i PROC_REMOVE edytują scene.swap na dysku") {
  BootFixture f("proc_add");
  f.project.write("scenes/main/scene.swap", R"(PROC tick
  ADD Game.frames 1
  IF Game.frames == 2 : PROC_ADD boost
  IF Game.frames == 4 : PROC_REMOVE boost
END
)");
  // Wczytujemy scenę od nowa, żeby Boot zobaczył nowy plik.
  REQUIRE(f.boot.load_scene("main"));
  f.step(2);
  CHECK(f.project.read("scenes/main/scene.swap").find("PROC boost") != std::string::npos);
  CHECK(f.boot.program().find("boost"));
  f.step(2);
  CHECK(f.get("Player", "x") == 120); // boost działał w klatkach 3 i 4
  CHECK(f.project.read("scenes/main/scene.swap").find("PROC boost") == std::string::npos);
  CHECK_FALSE(f.boot.program().find("boost"));
}

TEST_CASE("Boot: PROC ONCE wykonuje się raz i znika z pliku") {
  BootFixture f("once");
  f.project.write("scenes/main/scene.swap", "PROC tick\n  ADD Game.frames 1\nEND\n\n"
                                            "PROC hello ONCE\n  ADD Game.hello 1\nEND\n");
  REQUIRE(f.boot.load_scene("main"));
  f.step(5);
  CHECK(f.get("Game", "hello") == 1);
  CHECK(f.get("Game", "frames") == 5);
  CHECK(f.project.read("scenes/main/scene.swap") == "PROC tick\n  ADD Game.frames 1\nEND\n\n");
}

TEST_CASE("Boot: CALL do Lua i swap.paste") {
  BootFixture f("lua");
  f.project.write("scenes/main/scene.swap",
                  "PROC a\n  IF Scene.frame == 0 : CALL on_call Player 7\n"
                  "  IF Scene.frame == 1 : CALL paste_once\nEND\n");
  REQUIRE(f.boot.load_scene("main"));
  f.step();
  CHECK(f.get("Game", "called") == 7);
  CHECK(f.boot.world().exists("Kropka"));
  CHECK(f.get("Kropka", "x") == 100);
  f.step(); // Lua wkleja PROC from_lua ONCE
  CHECK(f.project.read("scenes/main/scene.swap").find("PROC from_lua ONCE") != std::string::npos);
  f.step(); // ONCE się wykonuje i znika
  CHECK(f.get("Game", "pasted") == 1);
  CHECK(f.project.read("scenes/main/scene.swap").find("from_lua") == std::string::npos);
}

TEST_CASE("Boot: hot-reload z błędem zostawia poprzedni program i nie niszczy pliku") {
  BootFixture f("broken");
  f.step();
  CHECK(f.get("Game", "frames") == 1);

  const std::string broken = "PROC tick\n  ADD Game.frames 1\n  PROC_ADD boost\n  MOOVE x\nEND\n";
  f.boot.apply_swap(broken, vm::parse_program(broken, "scene.swap"));
  f.step();
  CHECK(f.get("Game", "frames") == 2); // stary program działa dalej
  CHECK(f.boot.program().procs.size() == 1);

  // Prośba o zmianę pliku, gdy użytkownik ma w nim błąd: tylko w pamięci.
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
  CHECK(f.get("Game", "frames") == 14); // 4 klatki starego programu + 10
}

TEST_CASE("Boot: hot-reload scene.xml i skryptu Lua") {
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

TEST_CASE("Engine: gra bez okna z symulowanymi klawiszami zapisuje stan") {
  TempProject project("engine");
  project.write("scenes/main/scene.swap", R"(PROC ruch
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
  CHECK(state.find("klatka: 30") != std::string::npos);
  CHECK(state.find("Player x=120 y=100") != std::string::npos);
  CHECK(state.find("jumps=2") != std::string::npos);
}

TEST_CASE("Boot: zachowania Lua (on_start/on_update/on_touch/on_destroy), systemy i hot-reload") {
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
  // Nowy Boot, żeby wczytał components.xml i skrypty z dysku.
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
  step(14); // 102 -> 130 i zawraca (dir = -1)
  CHECK(get("Enemy", "x") == 130);
  CHECK(get("Game", "touched_Wall") == 1); // trigger dotknął ściany (135..145 vs 125..135)
  CHECK(get("Game", "starts") == 1);       // on_start tylko raz

  // Hot-reload zachowania: nowe funkcje, stary stan `self` (dir = -1).
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
