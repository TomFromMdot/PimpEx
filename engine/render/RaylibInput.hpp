#pragma once

#include "vm/Input.hpp"

namespace PimpEx::render {

class RaylibInput : public vm::IInput {
  std::array<bool, vm::KeyCount> _down{};
  std::array<bool, vm::KeyCount> _pressed{};
  std::array<bool, vm::MouseButtonCount> _mouseDown{};
  std::array<bool, vm::MouseButtonCount> _mousePressed{};
  float _mouseX = 0, _mouseY = 0, _mouseDx = 0, _mouseDy = 0;
  bool _hasLastMouse = false;
  bool _ignoreMotion = false;

public:
  void poll();

  // Free cursor with an fps camera: moving to the window's X must not turn the player.
  void set_ignore_motion(bool ignore) { _ignoreMotion = ignore; }

  bool down(vm::Key key) const override { return _down[static_cast<std::size_t>(key)]; }
  bool pressed(vm::Key key) const override { return _pressed[static_cast<std::size_t>(key)]; }
  float mouse_x() const override { return _mouseX; }
  float mouse_y() const override { return _mouseY; }
  float mouse_dx() const override { return _mouseDx; }
  float mouse_dy() const override { return _mouseDy; }
  bool mouse_down(vm::MouseButton button) const override { return _mouseDown[static_cast<std::size_t>(button)]; }
  bool mouse_pressed(vm::MouseButton button) const override {
    return _mousePressed[static_cast<std::size_t>(button)];
  }
};

} // namespace PimpEx::render
