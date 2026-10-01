#include "boot/ProjectCheck.hpp"
#include "boot/XmlLoader.hpp"
#include "scene/Physics.hpp"
#include "scene/World.hpp"
#include "vm/Parser.hpp"

#include <doctest/doctest.h>

#include <sstream>

using namespace PimpEx;
using namespace PimpEx::scene;

namespace {

const char *SceneXml = R"(<Scene background="black">
  <Actor name="Player" x="100" y="200" w="40" h="20" vx="1">
    <Rect color="#ff0000"/>
    <Var name="lives" value="3"/>
  </Actor>
  <Actor name="Label" x="10" y="10">
    <Text value="Lives: {Player.lives}" size="30" color="żółty"/>
  </Actor>
  <Archetype name="Bullet" w="4" h="8" vy="-5">
    <Circle color="white"/>
  </Archetype>
</Scene>)";

SceneData load_scene(const char *xml, Diagnostics &diagnostics) {
  return boot::parse_scene_xml(xml, "scene.xml", "main", diagnostics);
}

} // namespace

TEST_CASE("colors: hex, English and Polish names") {
  CHECK(parse_color("#fff") == Rgba{255, 255, 255, 255});
  CHECK(parse_color("#102030") == Rgba{16, 32, 48, 255});
  CHECK(parse_color("#10203040") == Rgba{16, 32, 48, 64});
  CHECK(parse_color("red").has_value());
  CHECK(parse_color("zielony").has_value());
  CHECK_FALSE(parse_color("#12").has_value());
  CHECK_FALSE(parse_color("ups").has_value());
}

TEST_CASE("scene.xml: actors, archetypes and variables") {
  Diagnostics diagnostics;
  const auto data = load_scene(SceneXml, diagnostics);
  CHECK(diagnostics.empty());
  REQUIRE(data.actors.size() == 2);
  const auto &player = data.actors[0];
  CHECK(player.name == "Player");
  CHECK(player.line == 2);
  REQUIRE(player.components.size() == 3); // Transform, Body (from vx), Rect
  CHECK(player.components[0].component == "Transform");
  CHECK(player.components[1].component == "Body");
  CHECK(player.components[2].component == "Rect");
  CHECK(player.has("Rect"));
  CHECK(player.vars.values.at("lives") == 3);
  CHECK(data.actors[1].has("Text"));
  CHECK(data.archetypes.contains("Bullet"));
  CHECK(data.background.has_value());
}

TEST_CASE("scene.xml: errors with line numbers") {
  Diagnostics diagnostics;
  load_scene(R"(<Scene>
  <Actor name="A" x="abc"/>
  <Actor name="A"/>
  <Actor x="1"/>
  <Actor name="B"><Sprite texture="nope"/><Foo/></Actor>
</Scene>)",
             diagnostics);
  std::string text;
  for (const auto &d : diagnostics)
    text += d.format() + "\n";
  INFO(text);
  CHECK(text.find("scene.xml:2: error: x=\"abc\" must be a number") != std::string::npos);
  CHECK(text.find("scene.xml:3: error: actor 'A' is already defined") != std::string::npos);
  CHECK(text.find("scene.xml:4: error: <Actor> needs a name attribute") != std::string::npos);
  CHECK(text.find("scene.xml:5: warning: unknown component <Foo>") != std::string::npos);
  CHECK(text.find("scene.xml:5: error: 'B' uses texture 'nope'") != std::string::npos);
}

TEST_CASE("scene.xml: broken XML") {
  Diagnostics diagnostics;
  load_scene("<Scene>\n<Actor name=\"A\">\n</Scene>", diagnostics);
  REQUIRE(diagnostics.size() == 1);
  CHECK(diagnostics[0].message.starts_with("invalid XML"));
}

TEST_CASE("pimpex.xml") {
  Diagnostics diagnostics;
  const auto config = boot::parse_project_xml(
      R"(<Project name="game"><Window title="My game" width="640" height="480" fps="30"/>
<StartScene name="menu"/><HotReload keepState="false"/></Project>)",
      "pimpex.xml", diagnostics);
  CHECK(diagnostics.empty());
  CHECK(config.title == "My game");
  CHECK(config.width == 640);
  CHECK(config.fps == 30);
  CHECK(config.startScene == "menu");
  CHECK_FALSE(config.keepStateOnReload);
}

TEST_CASE("World: fields, SPAWN, DESTROY, TOUCH and movement") {
  Diagnostics diagnostics;
  World world;
  world.load(load_scene(SceneXml, diagnostics));
  CHECK(world.size() == 2);

  CHECK(world.get("Player", "x") == 100);
  CHECK(world.get("Player", "lives") == 3);
  CHECK_FALSE(world.get("Player", "nope").has_value());
  CHECK(world.set("Player", "score", 5));
  CHECK(world.get("Player", "score") == 5);
  CHECK_FALSE(world.set("Nobody", "x", 1));

  const auto b1 = world.spawn("Bullet", "", std::pair{100.f, 200.f});
  const auto b2 = world.spawn("Bullet", "", std::nullopt);
  const auto named = world.spawn("Bullet", "Pocisk", std::nullopt);
  REQUIRE(b1);
  CHECK(*b1 == "Bullet_1");
  CHECK(*b2 == "Bullet_2");
  CHECK(*named == "Pocisk");
  CHECK_FALSE(world.spawn("Nope", "", std::nullopt).has_value());

  CHECK(world.touching("Player", "Bullet_1"));
  CHECK_FALSE(world.touching("Player", "Bullet_2"));

  step_physics(world);
  CHECK(world.get("Player", "x") == 101);
  CHECK(world.get("Bullet_1", "y") == 195);

  CHECK(world.destroy("Bullet_1"));
  CHECK_FALSE(world.exists("Bullet_1"));
  CHECK_FALSE(world.destroy("Bullet_1"));

  std::ostringstream dump;
  world.dump(dump);
  INFO(dump.str());
  CHECK(dump.str().find("Player x=101 y=200 w=40 h=20 Body(vx=1 grounded=0) Rect(color=#ff0000) "
                        "lives=3 score=5") != std::string::npos);
}

TEST_CASE("World: hot-reload keeps state") {
  Diagnostics diagnostics;
  World world;
  world.load(load_scene(SceneXml, diagnostics));
  world.set("Player", "x", 555);
  world.spawn("Bullet", "", std::nullopt);

  const auto changed = load_scene(R"(<Scene>
  <Actor name="Player" x="0" w="80"><Circle color="blue"/><Var name="lives" value="9"/><Var name="ammo" value="10"/></Actor>
  <Actor name="New"/>
  <Archetype name="Bullet"/>
</Scene>)",
                                  diagnostics);
  world.reload(changed, true);
  CHECK(world.get("Player", "x") == 555); // position kept
  CHECK(world.get("Player", "w") == 80);  // size from the file
  CHECK(world.get("Player", "lives") == 3); // old variable value
  CHECK(world.get("Player", "ammo") == 10); // new variable
  CHECK(world.exists("New"));
  CHECK_FALSE(world.exists("Label"));    // removed from the file
  CHECK(world.exists("Bullet_1"));       // SPAWN stays

  world.reload(changed, false);
  CHECK(world.get("Player", "x") == 0);
  CHECK_FALSE(world.exists("Bullet_1"));
}

TEST_CASE("check: references in the program") {
  Diagnostics diagnostics;
  const auto data = load_scene(SceneXml, diagnostics);
  const auto library = vm::parse_program("PROC boost\nEND\n", "library.swap");
  const auto program = vm::parse_program(R"(PROC a
  MOVE Ghost 1 1
  SPAWN Bullet AS Shot
  MOVE Shot 1 1
  SPAWN Rocket
  SET Screen.w 5
  PROC_ADD boost
  PROC_ADD nope
  SCENE menu
  CALL missing
  IF Mouse.x > Player.x : LOG ok
END)",
                                         "scene.swap");
  REQUIRE(program.ok());
  const std::vector<std::string> scenes = {"main"};
  const std::set<std::string> lua = {"exists"};
  boot::check_program_references(
      {program.program, data, &library.program, &scenes, &lua, "scene.swap"}, diagnostics);
  std::string text;
  for (const auto &d : diagnostics)
    text += d.format() + "\n";
  INFO(text);
  CHECK(diagnostics.size() == 6);
  CHECK(text.find("scene.swap:2: warning: actor 'Ghost'") != std::string::npos);
  CHECK(text.find("scene.swap:5: error: no archetype 'Rocket'") != std::string::npos);
  CHECK(text.find("scene.swap:6: error: fields of Screen are read-only") != std::string::npos);
  CHECK(text.find("scene.swap:8: error: no procedure 'nope'") != std::string::npos);
  CHECK(text.find("scene.swap:9: error: no scene 'menu'") != std::string::npos);
  CHECK(text.find("scene.swap:10: error: no Lua function 'missing'") != std::string::npos);
}
