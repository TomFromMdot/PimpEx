#include "boot/XmlLoader.hpp"
#include "gui/Gui.hpp"

#include <doctest/doctest.h>

using namespace PimpEx;
using namespace PimpEx::gui;

namespace {

GuiScene parse(const std::string &name, const char *xml) {
  Diagnostics diagnostics;
  auto scene = boot::parse_gui_xml(xml, "gui/" + name + ".xml", name, diagnostics);
  std::string text;
  for (const auto &d : diagnostics)
    text += d.format() + "\n";
  INFO(text);
  REQUIRE_FALSE(has_errors(diagnostics));
  return scene;
}

constexpr const char *PauseXml = R"(<Gui pause="true" dim="#00000080">
  <Panel anchor="center" w="300" h="260"/>
  <Label anchor="center" y="-90" text="Pauza" size="40"/>
  <Button name="Wroc" anchor="center" y="-20" text="Wróć" key="ESCAPE" click="GUI_HIDE pauza"/>
  <Button name="Menu" anchor="center" y="50" text="Menu" click="GUI_HIDE pauza; SCENE menu"/>
</Gui>)";

// Klatka z myszką w (x, y) i lewym przyciskiem w stanie `down`.
std::vector<GuiSystem::Click> frame(GuiSystem &gui, vm::ManualInput &input, float x, float y, bool down) {
  input.set_mouse(x, y);
  input.set_mouse_down(vm::MouseButton::Left, down);
  return gui.update(input, 800, 600);
}

} // namespace

TEST_CASE("gui: wczytywanie pliku gui/*.xml") {
  const auto scene = parse("pauza", PauseXml);
  CHECK(scene.name == "pauza");
  CHECK(scene.pause);
  REQUIRE(scene.dim);
  CHECK(scene.dim->a == 0x80);
  REQUIRE(scene.elements.size() == 4);
  CHECK(scene.elements[0].kind == ElementKind::Panel);
  CHECK(scene.elements[1].kind == ElementKind::Label);
  CHECK(scene.elements[1].text == "Pauza");
  const auto &menu = scene.elements[3];
  CHECK(menu.kind == ElementKind::Button);
  CHECK(menu.line == 5);
  CHECK(menu.w == 220); // domyślny rozmiar przycisku
  REQUIRE(menu.click.size() == 2);
  CHECK(menu.click[1].op == vm::Op::Scene);
  CHECK(scene.elements[2].key == vm::Key::Escape);
  CHECK(scene.has_buttons());
}

TEST_CASE("gui: błędy w pliku GUI mają plik i linię") {
  Diagnostics diagnostics;
  boot::parse_gui_xml(R"(<Gui>
  <Button click="SKOK 3"/>
  <Label anchor="środek"/>
  <Slider/>
  <Image/>
</Gui>)",
                      "gui/zle.xml", "zle", diagnostics);
  std::string text;
  for (const auto &d : diagnostics)
    text += d.format() + "\n";
  INFO(text);
  CHECK(text.find("gui/zle.xml:2: błąd: nieznana instrukcja 'SKOK'") != std::string::npos);
  CHECK(text.find("gui/zle.xml:3: błąd: anchor=\"środek\"") != std::string::npos);
  CHECK(text.find("gui/zle.xml:4: ostrzeżenie: nieznany element <Slider>") != std::string::npos);
  CHECK(text.find("gui/zle.xml:5: błąd: <Image> musi mieć atrybut src") != std::string::npos);

  Diagnostics noRoot;
  boot::parse_gui_xml("<Scene/>", "gui/x.xml", "x", noRoot);
  CHECK(has_errors(noRoot));
}

TEST_CASE("gui: pozycja liczona od kotwicy, środek elementu w (x, y)") {
  Element e;
  e.w = 100;
  e.h = 40;
  e.x = 10;
  e.y = 20;
  auto r = layout(e, 800, 600);
  CHECK(r.x == -40);
  CHECK(r.y == 0);
  e.anchor = Anchor::Center;
  r = layout(e, 800, 600);
  CHECK(r.x == 360);
  CHECK(r.y == 300);
  e.anchor = Anchor::BottomRight;
  e.x = -60;
  e.y = -30;
  r = layout(e, 1024, 768);
  CHECK(r.x == 914);
  CHECK(r.y == 718);
  CHECK(r.contains(914, 718));
  CHECK_FALSE(r.contains(1014, 718));
}

TEST_CASE("gui: show/hide/toggle po nazwie, kolejność od spodu do wierzchu") {
  GuiSystem gui;
  gui.set(parse("pauza", PauseXml));
  gui.set(parse("hud", R"(<Gui><Label text="{Game.score}"/></Gui>)"));
  CHECK_FALSE(gui.show("nie_ma"));
  CHECK(gui.show("pauza"));
  CHECK(gui.show("hud"));
  CHECK(gui.shown() == std::vector<std::string>{"pauza", "hud"});
  CHECK(gui.show("pauza")); // ponowne show przenosi na wierzch
  CHECK(gui.shown() == std::vector<std::string>{"hud", "pauza"});
  CHECK(gui.pauses_game());
  CHECK(gui.wants_mouse());
  CHECK(gui.toggle("pauza") == false);
  CHECK_FALSE(gui.pauses_game());
  CHECK_FALSE(gui.wants_mouse()); // sam HUD nie ma przycisków
  CHECK(gui.toggle("pauza") == true);
  CHECK_FALSE(gui.toggle("nie_ma").has_value());
  gui.hide_all();
  CHECK(gui.shown().empty());
}

TEST_CASE("gui: kliknięcie = wciśnięcie i puszczenie na tym samym przycisku") {
  GuiSystem gui;
  gui.set(parse("pauza", PauseXml));
  gui.show("pauza");
  vm::ManualInput input;
  // „Wróć” jest w (400, 280), „Menu” w (400, 350).
  CHECK(frame(gui, input, 400, 280, false).empty());
  CHECK(gui.is_hovered("pauza", 2));
  CHECK(frame(gui, input, 400, 280, true).empty());
  CHECK(gui.is_pressed("pauza", 2));
  const auto clicks = frame(gui, input, 400, 280, false);
  REQUIRE(clicks.size() == 1);
  CHECK(clicks[0].file == "gui/pauza.xml");
  CHECK(clicks[0].actions[0].op == vm::Op::GuiHide);

  // Wciśnięcie na „Wróć”, puszczenie na „Menu” – nic.
  frame(gui, input, 400, 280, true);
  CHECK(frame(gui, input, 400, 350, false).empty());
  // Panel zasłania grę: klik w panel obok przycisków nie trafia w nic.
  frame(gui, input, 270, 300, true);
  CHECK(frame(gui, input, 270, 300, false).empty());
}

TEST_CASE("gui: klawisz przycisku działa jak kliknięcie, panel zasłania niższe GUI") {
  GuiSystem gui;
  gui.set(parse("pauza", PauseXml));
  gui.set(parse("menu", R"(<Gui><Button anchor="center" y="-20" text="Graj" click="SCENE gra"/></Gui>)"));
  gui.show("menu");
  gui.show("pauza");
  vm::ManualInput input;
  input.set_pressed(vm::Key::Escape, true);
  auto clicks = gui.update(input, 800, 600);
  REQUIRE(clicks.size() == 1);
  CHECK(clicks[0].scene == "pauza");
  input.set_pressed(vm::Key::Escape, false);

  // „Graj” z menu leży pod „Wróć” z pauzy – trafia wierzch.
  frame(gui, input, 400, 280, true);
  clicks = frame(gui, input, 400, 280, false);
  REQUIRE(clicks.size() == 1);
  CHECK(clicks[0].scene == "pauza");
  gui.hide("pauza");
  frame(gui, input, 400, 280, true);
  clicks = frame(gui, input, 400, 280, false);
  REQUIRE(clicks.size() == 1);
  CHECK(clicks[0].scene == "menu");
}
