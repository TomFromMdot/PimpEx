#pragma once

#include "scene/Components.hpp"
#include "vm/Input.hpp"
#include "vm/Program.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace PimpEx::gui {

enum class Anchor {
  TopLeft,
  Top,
  TopRight,
  Left,
  Center,
  Right,
  BottomLeft,
  Bottom,
  BottomRight
};

enum class ElementKind { Panel, Label, Button, Image };

enum class Align { Left, Center, Right };

struct Element {
  ElementKind kind = ElementKind::Panel;
  std::string name; // opcjonalny – do komunikatów i zrzutu stanu
  int line = 0;
  Anchor anchor = Anchor::TopLeft;
  float x = 0, y = 0, w = 0, h = 0;

  std::string text; // Label, Button; może zawierać {Aktor.pole}
  int size = 24;
  Align align = Align::Center;
  scene::Rgba color;      // Panel, Button: tło; Label: tekst; Image: odcień
  scene::Rgba textColor;  // Button
  scene::Rgba hoverColor; // Button: tło pod kursorem
  std::optional<scene::Rgba> border;
  std::string image; // Image: plik względem katalogu projektu

  std::vector<vm::Instr> click;
  std::optional<vm::Key> key;
};

struct GuiScene {
  std::string name;   // = nazwa pliku bez .xml
  std::string file;   // do komunikatów o błędach
  bool pause = false; // gdy widoczna, logika gry (VM, Lua, fizyka) stoi
  std::optional<scene::Rgba> dim; // przyciemnienie całego okna pod GUI
  std::vector<Element> elements;  // kolejność = kolejność rysowania

  bool has_buttons() const;
};

struct Rect {
  float x = 0, y = 0, w = 0, h = 0; // lewy górny róg i rozmiar

  bool contains(float px, float py) const {
    return px >= x && px < x + w && py >= y && py < y + h;
  }
};

Rect layout(const Element &element, int screenWidth, int screenHeight);

std::optional<Anchor> anchor_from_name(std::string_view name);

class GuiSystem {
public:
  struct Click {
    std::string scene;
    std::string file;
    std::vector<vm::Instr> actions;
  };

private:
  struct Hit {
    std::string scene;
    std::size_t index = 0;
    bool operator==(const Hit &) const = default;
  };

  std::map<std::string, GuiScene> _scenes;
  std::vector<std::string> _shown; // od spodu do wierzchu
  std::optional<Hit> _hovered;
  std::optional<Hit> _pressed; // przycisk, na którym wciśnięto mysz
  bool _wasDown = false;

  std::optional<Hit> hit_test(float x, float y, int screenWidth,
                              int screenHeight) const;

public:
  void set(GuiScene scene);
  void clear();
  const GuiScene *find(const std::string &name) const;
  std::vector<std::string> names() const;

  bool show(const std::string &name);
  void hide(const std::string &name);
  std::optional<bool> toggle(const std::string &name);
  void hide_all();
  bool is_shown(const std::string &name) const;
  const std::vector<std::string> &shown() const { return _shown; }

  bool pauses_game() const;
  bool wants_mouse() const;

  std::vector<Click> update(const vm::IInput &input, int screenWidth,
                            int screenHeight);

  bool is_hovered(const std::string &scene, std::size_t index) const;
  bool is_pressed(const std::string &scene, std::size_t index) const;
};

} // namespace PimpEx::gui
