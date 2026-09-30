#pragma once

#include "vm/Input.hpp"

namespace PimpEx::render {

// Stan klawiatury i myszki odczytany z raylib raz na klatkę.
class RaylibInput : public vm::IInput {
  std::array<bool, vm::KeyCount> _down{};
  std::array<bool, vm::KeyCount> _pressed{};
  float _mouseX = 0, _mouseY = 0, _mouseDx = 0, _mouseDy = 0;
  bool _hasLastMouse = false;

public:
  void poll();

  bool down(vm::Key key) const override { return _down[static_cast<std::size_t>(key)]; }
  bool pressed(vm::Key key) const override { return _pressed[static_cast<std::size_t>(key)]; }
  float mouse_x() const override { return _mouseX; }
  float mouse_y() const override { return _mouseY; }
  float mouse_dx() const override { return _mouseDx; }
  float mouse_dy() const override { return _mouseDy; }
};

} // namespace PimpEx::render
