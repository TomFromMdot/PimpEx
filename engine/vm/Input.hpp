#pragma once

#include "vm/Keys.hpp"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace PimpEx::vm {

// Stan klawiatury i myszki w bieżącej klatce.
// VM nie wie nic o raylib – dostaje tylko ten interfejs.
class IInput {
public:
  virtual ~IInput() = default;
  virtual bool down(Key key) const = 0;    // klawisz trzymany
  virtual bool pressed(Key key) const = 0; // wciśnięty w tej klatce
  virtual float mouse_x() const = 0;
  virtual float mouse_y() const = 0;
  // Ruch myszy od poprzedniej klatki (kamera fps).
  virtual float mouse_dx() const { return 0; }
  virtual float mouse_dy() const { return 0; }
};

// Input sterowany ręcznie – dla testów i trybu bez okna (--headless).
class ManualInput : public IInput {
  std::array<bool, KeyCount> _down{};
  std::array<bool, KeyCount> _pressed{};
  float _mouseX = 0, _mouseY = 0;

public:
  void set_down(Key key, bool value) { _down[static_cast<std::size_t>(key)] = value; }
  void set_pressed(Key key, bool value) { _pressed[static_cast<std::size_t>(key)] = value; }
  void set_mouse(float x, float y) {
    _mouseX = x;
    _mouseY = y;
  }

  bool down(Key key) const override { return _down[static_cast<std::size_t>(key)]; }
  bool pressed(Key key) const override { return _pressed[static_cast<std::size_t>(key)]; }
  float mouse_x() const override { return _mouseX; }
  float mouse_y() const override { return _mouseY; }
};

// Zaplanowane wciśnięcia klawiszy dla trybu --headless, np. "RIGHT:0-60 SPACE:10".
// KLAWISZ:od-do trzyma klawisz w klatkach od..do, KLAWISZ:n wciska go na jedną klatkę.
class ScriptedInput : public ManualInput {
  struct Press {
    Key key;
    long from, to;
  };
  std::vector<Press> _presses;

public:
  // Zwraca opis błędu albo pusty tekst.
  std::string parse(std::string_view spec);
  void update(long frame);
};

} // namespace PimpEx::vm
