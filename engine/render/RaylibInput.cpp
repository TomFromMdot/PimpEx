#include "render/RaylibInput.hpp"

#include <raylib.h>

#include <cmath>

namespace PimpEx::render {
namespace {

int raylib_key(vm::Key key) {
  using vm::Key;
  const auto index = static_cast<int>(key);
  if (key <= Key::Z)
    return KEY_A + index;
  if (key <= Key::Num9)
    return KEY_ZERO + (index - static_cast<int>(Key::Num0));
  switch (key) {
  case Key::Space: return KEY_SPACE;
  case Key::Enter: return KEY_ENTER;
  case Key::Escape: return KEY_ESCAPE;
  case Key::Shift: return KEY_LEFT_SHIFT;
  case Key::Left: return KEY_LEFT;
  case Key::Right: return KEY_RIGHT;
  case Key::Up: return KEY_UP;
  case Key::Down: return KEY_DOWN;
  default: return KEY_NULL;
  }
}

} // namespace

void RaylibInput::poll() {
  for (std::size_t i = 0; i < vm::KeyCount; ++i) {
    const auto key = raylib_key(static_cast<vm::Key>(i));
    _down[i] = IsKeyDown(key);
    _pressed[i] = IsKeyPressed(key);
  }
  const auto shift = static_cast<std::size_t>(vm::Key::Shift);
  _down[shift] = _down[shift] || IsKeyDown(KEY_RIGHT_SHIFT);
  _pressed[shift] = _pressed[shift] || IsKeyPressed(KEY_RIGHT_SHIFT);

  // Mouse motion is the position difference between frames. With a locked
  // cursor GetMouseDelta assumes the cursor is re-centred, which is not true
  // under WSLg, and the player would spin forever.
  const auto mouse = GetMousePosition();
  if (_hasLastMouse) {
    _mouseDx = mouse.x - _mouseX;
    _mouseDy = mouse.y - _mouseY;
    // A cursor jump (locking, returning to the window) is not player motion.
    if (std::abs(_mouseDx) > 200 || std::abs(_mouseDy) > 200)
      _mouseDx = _mouseDy = 0;
  }
  if (_ignoreMotion)
    _mouseDx = _mouseDy = 0;
  _hasLastMouse = IsWindowFocused();
  _mouseX = mouse.x;
  _mouseY = mouse.y;

  static constexpr int buttons[] = {MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE};
  for (std::size_t i = 0; i < vm::MouseButtonCount; ++i) {
    _mouseDown[i] = IsMouseButtonDown(buttons[i]);
    _mousePressed[i] = IsMouseButtonPressed(buttons[i]);
  }
}

} // namespace PimpEx::render
