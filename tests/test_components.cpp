#include "boot/XmlLoader.hpp"
#include "scene/Physics.hpp"
#include "scene/World.hpp"

#include <doctest/doctest.h>

#include <sstream>

using namespace PimpEx;
using namespace PimpEx::scene;

namespace {

std::string text_of(const Diagnostics &diagnostics) {
  std::string out;
  for (const auto &d : diagnostics)
    out += d.format() + "\n";
  return out;
}

std::shared_ptr<ComponentRegistry> registry_with_health(Diagnostics &diagnostics) {
  auto registry = ComponentRegistry::with_builtins();
  registry->load_custom(R"(<Components>
  <Component name="Health" doc="Punkty życia">
    <Field name="hp" type="number" default="3"/>
    <Field name="max" type="integer" default="5"/>
    <Field name="shield" type="bool" default="false"/>
    <Field name="owner" type="text"/>
  </Component>
</Components>)",
                        "components.xml", diagnostics);
  return registry;
}

World load(const std::shared_ptr<ComponentRegistry> &registry, const char *xml, Diagnostics &diagnostics) {
  World world(registry);
  world.load(boot::parse_scene_xml(xml, "scene.xml", "main", diagnostics, registry.get()));
  return world;
}

} // namespace

TEST_CASE("rejestr: komponenty wbudowane mają pola z typami i wartościami domyślnymi") {
  const auto registry = ComponentRegistry::with_builtins();
  for (const char *name : {"Transform", "Body", "Collider", "Rect", "Circle", "Sprite", "Text", "Cube",
                           "Sphere", "Cylinder", "Plane", "Model", "Camera", "Light"})
    CHECK_MESSAGE(registry->find(name) != nullptr, name);
  const auto *camera = registry->find("Camera");
  REQUIRE(camera);
  const auto *mode = camera->field("mode");
  REQUIRE(mode);
  CHECK(mode->type == FieldType::Text);
  CHECK(std::get<std::string>(mode->defaultValue) == "fixed");
  std::string error;
  CHECK_FALSE(coerce_field(*mode, std::string("drone"), error).has_value());
  CHECK(error.find("dozwolone: fixed, follow, orbit, fps") != std::string::npos);
  CHECK(registry->find("Body")->field("grounded")->readOnly);
}

TEST_CASE("components.xml: komponent użytkownika w XML, polach i zapytaniach") {
  Diagnostics diagnostics;
  const auto registry = registry_with_health(diagnostics);
  REQUIRE(diagnostics.empty());
  auto world = load(registry, R"(<Scene>
  <Actor name="Player" tags="hero"><Health hp="10" shield="true"/></Actor>
  <Actor name="Orc" tags="enemy, big"><Health/></Actor>
  <Actor name="Rock"/>
</Scene>)",
                    diagnostics);
  INFO(text_of(diagnostics));
  CHECK(diagnostics.empty());

  CHECK(world.get("Player", "Health.hp") == 10);
  CHECK(world.get("Player", "Health.shield") == 1);
  CHECK(world.get("Orc", "Health.hp") == 3); // domyślne
  CHECK(world.get("Orc", "Health.max") == 5);
  CHECK(world.query("Health") == std::vector<std::string>{"Orc", "Player"});
  CHECK(world.query("@enemy") == std::vector<std::string>{"Orc"});
  CHECK(world.query("@big") == std::vector<std::string>{"Orc"});
  CHECK(world.has_tag(world.find("Player"), "hero"));

  CHECK_FALSE(world.set_field("Orc", "Health.hp", 1.0).has_value());
  CHECK(world.get("Orc", "Health.hp") == 1);
  CHECK_FALSE(world.set_field("Rock", "Health.hp", 7.0).has_value()); // komponent dodany
  CHECK(world.has_component(world.find("Rock"), "Health"));
  CHECK(world.get("Rock", "Health.max") == 5);

  const auto error = world.set_field("Orc", "Heath.hp", 1.0);
  REQUIRE(error);
  CHECK(error->find("czy chodziło o Health?") != std::string::npos);
  const auto fieldError = world.set_field("Orc", "Health.pk", 1.0);
  REQUIRE(fieldError);
  CHECK(fieldError->find("czy chodziło o hp?") != std::string::npos);

  std::ostringstream dump;
  world.dump(dump);
  INFO(dump.str());
  CHECK(dump.str().find("Player x=0 y=0 w=32 h=32 Health(hp=10 max=5 shield=1 owner=\"\") tags=hero") !=
        std::string::npos);
}

TEST_CASE("components.xml: błędy definicji") {
  Diagnostics diagnostics;
  auto registry = ComponentRegistry::with_builtins();
  registry->load_custom(R"(<Components>
  <Component name="health"/>
  <Component name="Rect"/>
  <Component name="Ammo"><Field name="n" type="float"/><Field name="k" default="abc"/></Component>
</Components>)",
                        "components.xml", diagnostics);
  const auto text = text_of(diagnostics);
  INFO(text);
  CHECK(text.find("components.xml:2: błąd: <Component> musi mieć name zaczynające się wielką literą") !=
        std::string::npos);
  CHECK(text.find("components.xml:3: błąd: nazwa 'Rect' jest zajęta przez silnik") != std::string::npos);
  CHECK(text.find("components.xml:4: błąd: nieznany typ pola 'float'") != std::string::npos);
  CHECK(text.find("k=\"abc\" musi być liczbą") != std::string::npos);
}

TEST_CASE("scene.xml: literówki w komponentach i polach mają podpowiedzi") {
  Diagnostics diagnostics;
  boot::parse_scene_xml(R"(<Scene>
  <Actor name="A" colr="red"><Rectt color="red"/><Circle colour="red"/><Camera mode="drone"/></Actor>
</Scene>)",
                        "scene.xml", "main", diagnostics);
  const auto text = text_of(diagnostics);
  INFO(text);
  CHECK(text.find("nieznany komponent <Rectt> (czy chodziło o Rect?)") != std::string::npos);
  CHECK(text.find("<Circle> nie ma pola 'colour' (czy chodziło o color?)") != std::string::npos);
  CHECK(text.find("mode=\"drone\" – dozwolone: fixed, follow, orbit, fps") != std::string::npos);
  CHECK(text.find("<Actor> nie ma atrybutu 'colr'") != std::string::npos);
}

TEST_CASE("scena 3D: domyślny rozmiar 1, pola 3D i zasoby modeli") {
  Diagnostics diagnostics;
  World world;
  world.load(boot::parse_scene_xml(R"(<Scene mode="3d">
  <Resources><Model id="tree" path="assets/tree.glb"/></Resources>
  <Actor name="Box" x="1" y="2" z="3" ry="45"><Cube color="red"/></Actor>
  <Actor name="Tree" scale="2"><Model model="tree"/></Actor>
  <Actor name="Cam" z="10"><Camera mode="orbit" target="Box" distance="6"/></Actor>
  <Actor name="Bad"><Model model="nope"/></Actor>
</Scene>)",
                                   "scene.xml", "main", diagnostics));
  const auto text = text_of(diagnostics);
  INFO(text);
  CHECK(diagnostics.size() == 1);
  CHECK(text.find("'Bad' używa modelu 'nope'") != std::string::npos);
  CHECK(world.mode() == SceneMode::Mode3D);
  CHECK(world.get("Box", "w") == 1);
  CHECK(world.get("Box", "d") == 1);
  CHECK(world.get("Box", "z") == 3);
  CHECK(world.get("Box", "ry") == 45);
  CHECK(world.get("Tree", "scale") == 2);
  CHECK(std::get<std::string>(*world.get_field("Cam", "Camera.mode")) == "orbit");
  CHECK(world.get("Cam", "Camera.distance") == 6);
}

TEST_CASE("fizyka: grawitacja, lądowanie na solidzie i styki z triggerem") {
  Diagnostics diagnostics;
  World world;
  world.load(boot::parse_scene_xml(R"(<Scene>
  <Actor name="Player" x="100" y="100" w="20" h="20" gravity="1"><Collider/></Actor>
  <Actor name="Floor" x="100" y="200" w="400" h="20"><Collider/></Actor>
  <Actor name="Coin" x="100" y="180" w="10" h="10"><Collider kind="trigger"/></Actor>
</Scene>)",
                                   "scene.xml", "main", diagnostics));
  REQUIRE(diagnostics.empty());
  bool touchedCoin = false;
  for (int i = 0; i < 60; ++i)
    for (const auto &contact : step_physics(world)) {
      const auto a = world.name_of(contact.a), b = world.name_of(contact.b);
      if ((a == "Player" && b == "Coin") || (a == "Coin" && b == "Player"))
        touchedCoin = true;
    }
  CHECK(world.get("Player", "y") == 180); // stoi na podłodze: 200 - 10 - 10
  CHECK(world.get("Player", "vy") == 0);
  CHECK(world.get("Player", "grounded") == 1);
  CHECK(touchedCoin); // trigger nie zatrzymuje, ale jest w stykach
  CHECK(world.set_field("Player", "grounded", 0.0).has_value()); // tylko do odczytu

  // 3D: w dół to -y.
  World world3d;
  world3d.load(boot::parse_scene_xml(R"(<Scene mode="3d">
  <Actor name="Ball" y="5" gravity="0.1"><Collider/></Actor>
  <Actor name="Ground" y="-0.5" w="20" h="1" d="20"><Collider/></Actor>
</Scene>)",
                                     "scene.xml", "main", diagnostics));
  for (int i = 0; i < 100; ++i)
    step_physics(world3d);
  CHECK(world3d.get("Ball", "y") == doctest::Approx(0.5));
  CHECK(world3d.get("Ball", "grounded") == 1);
}
