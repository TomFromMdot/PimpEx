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
  <Button name="Resume" anchor="center" y="-20" text="Resume" key="ESCAPE" click="GUI_HIDE pause"/>
  <Button name="Menu" anchor="center" y="50" text="Menu" click="GUI_HIDE pause; SCENE menu"/>
</Gui>)";

// One frame with the mouse at (x, y) and the left button `down`.
std::vector<GuiSystem::Click> frame(GuiSystem &gui, vm::ManualInput &input, float x, float y, bool down) {
  input.set_mouse(x, y);
  input.set_mouse_down(vm::MouseButton::Left, down);
  return gui.update(input, 800, 600);
}

} // namespace

TEST_CASE("gui: loading gui/*.xml") {
  const auto scene = parse("pause", PauseXml);
  CHECK(scene.name == "pause");
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
  CHECK(menu.w == 220); // default button size
  REQUIRE(menu.click.size() == 2);
  CHECK(menu.click[1].op == vm::Op::Scene);
  CHECK(scene.elements[2].key == vm::Key::Escape);
  CHECK(scene.has_buttons());
}

TEST_CASE("gui: GUI file errors have file and line") {
  Diagnostics diagnostics;
  boot::parse_gui_xml(R"(<Gui>
  <Button click="JUMPP 3"/>
  <Label anchor="middle"/>
  <Slider/>
  <Image/>
</Gui>)",
                      "gui/bad.xml", "bad", diagnostics);
  std::string text;
  for (const auto &d : diagnostics)
    text += d.format() + "\n";
  INFO(text);
  CHECK(text.find("gui/bad.xml:2: error: unknown instruction 'JUMPP'") != std::string::npos);
  CHECK(text.find("gui/bad.xml:3: error: anchor=\"middle\"") != std::string::npos);
  CHECK(text.find("gui/bad.xml:4: warning: unknown element <Slider>") != std::string::npos);
  CHECK(text.find("gui/bad.xml:5: error: <Image> needs a src attribute") != std::string::npos);

  Diagnostics noRoot;
  boot::parse_gui_xml("<Scene/>", "gui/x.xml", "x", noRoot);
  CHECK(has_errors(noRoot));
}

TEST_CASE("gui: position from the anchor, element centre at (x, y)") {
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

TEST_CASE("gui: show/hide/toggle by name, ordered bottom to top") {
  GuiSystem gui;
  gui.set(parse("pause", PauseXml));
  gui.set(parse("hud", R"(<Gui><Label text="{Game.score}"/></Gui>)"));
  CHECK_FALSE(gui.show("missing"));
  CHECK(gui.show("pause"));
  CHECK(gui.show("hud"));
  CHECK(gui.shown() == std::vector<std::string>{"pause", "hud"});
  CHECK(gui.show("pause")); // showing again moves it to the top
  CHECK(gui.shown() == std::vector<std::string>{"hud", "pause"});
  CHECK(gui.pauses_game());
  CHECK(gui.wants_mouse());
  CHECK(gui.toggle("pause") == false);
  CHECK_FALSE(gui.pauses_game());
  CHECK_FALSE(gui.wants_mouse()); // the HUD alone has no buttons
  CHECK(gui.toggle("pause") == true);
  CHECK_FALSE(gui.toggle("missing").has_value());
  gui.hide_all();
  CHECK(gui.shown().empty());
}

TEST_CASE("gui: click = press and release on the same button") {
  GuiSystem gui;
  gui.set(parse("pause", PauseXml));
  gui.show("pause");
  vm::ManualInput input;
  // "Resume" is at (400, 280), "Menu" at (400, 350).
  CHECK(frame(gui, input, 400, 280, false).empty());
  CHECK(gui.is_hovered("pause", 2));
  CHECK(frame(gui, input, 400, 280, true).empty());
  CHECK(gui.is_pressed("pause", 2));
  const auto clicks = frame(gui, input, 400, 280, false);
  REQUIRE(clicks.size() == 1);
  CHECK(clicks[0].file == "gui/pause.xml");
  CHECK(clicks[0].actions[0].op == vm::Op::GuiHide);

  // Press on "Resume", release on "Menu" – nothing.
  frame(gui, input, 400, 280, true);
  CHECK(frame(gui, input, 400, 350, false).empty());
  // The panel covers the game: a click on it next to the buttons hits nothing.
  frame(gui, input, 270, 300, true);
  CHECK(frame(gui, input, 270, 300, false).empty());
}

TEST_CASE("gui: a button key acts as a click, panels cover lower GUIs") {
  GuiSystem gui;
  gui.set(parse("pause", PauseXml));
  gui.set(parse("menu", R"(<Gui><Button anchor="center" y="-20" text="Play" click="SCENE game"/></Gui>)"));
  gui.show("menu");
  gui.show("pause");
  vm::ManualInput input;
  input.set_pressed(vm::Key::Escape, true);
  auto clicks = gui.update(input, 800, 600);
  REQUIRE(clicks.size() == 1);
  CHECK(clicks[0].scene == "pause");
  input.set_pressed(vm::Key::Escape, false);

  // "Play" in the menu lies under "Resume" in the pause GUI – the top one wins.
  frame(gui, input, 400, 280, true);
  clicks = frame(gui, input, 400, 280, false);
  REQUIRE(clicks.size() == 1);
  CHECK(clicks[0].scene == "pause");
  gui.hide("pause");
  frame(gui, input, 400, 280, true);
  clicks = frame(gui, input, 400, 280, false);
  REQUIRE(clicks.size() == 1);
  CHECK(clicks[0].scene == "menu");
}
