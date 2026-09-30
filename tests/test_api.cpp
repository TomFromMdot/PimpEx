#include "api/LiveApi.hpp"

#include <doctest/doctest.h>

#include <fstream>

using namespace PimpEx;
using api::json;

namespace {

struct ApiFixture {
  std::filesystem::path root = std::filesystem::temp_directory_path() / "pimpex_test_api";
  Log log;
  std::unique_ptr<boot::Boot> boot;
  vm::OverlayInput input;
  api::GameControl control;

  void write(const std::string &rel, const std::string &content) {
    std::filesystem::create_directories((root / rel).parent_path());
    std::ofstream(root / rel) << content;
  }

  ApiFixture() {
    std::filesystem::remove_all(root);
    write("pimpex.xml", R"(<Project name="t"><StartScene name="main"/></Project>)");
    write("scenes/main/scene.xml", R"(<Scene>
  <Actor name="Player" x="10" y="20" tags="hero"><Rect color="red"/></Actor>
  <Actor name="Game" w="0" h="0"/>
  <Archetype name="Dot" w="2" h="2"><Circle color="white"/></Archetype>
</Scene>)");
    write("scenes/main/scene.swap", "PROC tick\n  IF KEY SPACE : ADD Game.jumps 1\nEND\n");
    write("scenes/main/library.swap", "PROC boost\n  ADD Player.x 1\nEND\n");
    write("scenes/other/scene.xml", "<Scene/>");
    write("scenes/other/scene.swap", "");
    log.set_quiet(true);
    boot = std::make_unique<boot::Boot>(boot::ProjectPaths{root}, log, 1);
    REQUIRE(boot->start());
    control.input = &input;
  }
  ~ApiFixture() { std::filesystem::remove_all(root); }

  json call(const std::string &method, json params = json::object()) {
    auto request = std::make_shared<api::ApiRequest>();
    request->method = method;
    request->params = std::move(params);
    return api::dispatch(request, *boot, control, log);
  }
  void step(int n = 1) {
    for (int i = 0; i < n; ++i) {
      input.set_frame(control.frame);
      const vm::FrameContext ctx{input, 800, 600, control.frame++};
      boot->run_frame(ctx);
    }
  }
};

} // namespace

TEST_CASE("API: stan, aktorzy i pola") {
  ApiFixture f;
  const auto state = f.call("state.get");
  CHECK(state["scene"] == "main");
  CHECK(state["actors"].size() == 2);
  CHECK(state["procs"][0]["name"] == "tick");

  const auto player = f.call("actor.get", {{"name", "Player"}});
  CHECK(player["components"]["Rect"]["color"] == "red");
  CHECK(player["tags"][0] == "hero");
  CHECK(f.call("actors.list", {{"selector", "@hero"}}) == json{"Player"});

  f.call("actor.set", {{"name", "Player"}, {"field", "x"}, {"value", 99}});
  f.call("actor.set", {{"name", "Player"}, {"field", "Rect.color"}, {"value", "blue"}});
  const auto updated = f.call("actor.get", {{"name", "Player"}});
  CHECK(updated["x"] == 99);
  CHECK(updated["components"]["Rect"]["color"] == "blue");

  CHECK_THROWS_WITH_AS(f.call("actor.set", {{"name", "Player"}, {"field", "Rect.colr"}, {"value", "x"}}),
                       doctest::Contains("czy chodziło o color?"), api::ApiError);
  CHECK_THROWS_AS(f.call("actor.get", {{"name", "Nope"}}), api::ApiError);
  CHECK_THROWS_AS(f.call("no.such.method"), api::ApiError);
}

TEST_CASE("API: spawn, komponenty, destroy") {
  ApiFixture f;
  const auto dot = f.call("actor.spawn", {{"archetype", "Dot"}, {"x", 5}, {"y", 6}});
  CHECK(dot["name"] == "Dot_1");
  CHECK(dot["x"] == 5);
  f.call("actor.add_component", {{"name", "Dot_1"}, {"component", "Body"}, {"values", {{"vx", 2}}}});
  f.step();
  CHECK(f.boot->world().get("Dot_1", "x") == 7);
  f.call("actor.remove_component", {{"name", "Dot_1"}, {"component", "Body"}});
  CHECK_FALSE(f.boot->world().has_component(f.boot->world().find("Dot_1"), "Body"));
  f.call("actor.destroy", {{"name", "Dot_1"}});
  CHECK_FALSE(f.boot->world().exists("Dot_1"));
}

TEST_CASE("API: swap.paste / add / remove zmieniają plik od razu") {
  ApiFixture f;
  f.call("swap.paste", {{"source", "PROC hello ONCE\n  ADD Game.hello 1\nEND\n"}});
  CHECK(f.boot->program().find("hello"));
  f.step(2);
  CHECK(f.boot->world().get("Game", "hello") == 1);
  CHECK_FALSE(f.boot->program().find("hello"));

  f.call("swap.add", {{"name", "boost"}});
  CHECK(f.call("swap.get")["text"].get<std::string>().find("PROC boost") != std::string::npos);
  f.call("swap.remove", {{"name", "boost"}});
  CHECK_FALSE(f.boot->program().find("boost"));
  CHECK_THROWS_AS(f.call("swap.paste", {{"source", "PROC x\n MOOVE a 1 1\nEND"}}), api::ApiError);
}

TEST_CASE("API: input.press, pauza i krok, Lua, sceny, check") {
  ApiFixture f;
  f.call("input.press", {{"keys", {"SPACE"}}, {"frames", 3}});
  f.step(5);
  CHECK(f.boot->world().get("Game", "jumps") == 3);

  f.call("game.pause");
  CHECK(f.control.paused);
  f.call("game.step", {{"frames", 2}});
  CHECK(f.control.stepFrames == 2);
  f.call("game.resume");
  CHECK_FALSE(f.control.paused);

  CHECK(f.call("lua.eval", {{"code", "get('Player', 'x') + 1"}})["output"] == "11");
  CHECK_THROWS_WITH_AS(f.call("lua.eval", {{"code", "nope("}}), doctest::Contains("Lua"), api::ApiError);

  CHECK(f.call("scene.list") == json{"main", "other"});
  f.call("scene.load", {{"name", "other"}});
  CHECK(f.boot->scene_name() == "other");

  CHECK(f.call("check")["ok"] == true);
  CHECK(f.call("components.list").size() == 14); // wbudowane
  CHECK_THROWS_AS(f.call("screenshot"), api::ApiError); // bez okna
}

TEST_CASE("API: swap.paste odrzuca kod z błędnymi odwołaniami") {
  ApiFixture f;
  CHECK_THROWS_WITH_AS(f.call("swap.paste", {{"source", "PROC x ONCE\n  SPAWN Nope\nEND"}}),
                       doctest::Contains("swap.paste:2: błąd: nie ma archetypu 'Nope'"), api::ApiError);
  const auto ok = f.call("swap.paste", {{"source", "PROC y ONCE\n  MOVE Ghost 1 1\nEND"}});
  REQUIRE(ok["warnings"].size() == 1); // Ghost nie istnieje – tylko ostrzeżenie
  CHECK(ok["warnings"][0]["severity"] == "warning");
}
