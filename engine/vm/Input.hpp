#pragma once

#include "vm/Keys.hpp"

#include <array>

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

} // namespace PimpEx::vm
