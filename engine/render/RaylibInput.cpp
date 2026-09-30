#include "render/RaylibInput.hpp"

#include <raylib.h>

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
  // Prawy Shift też liczy się jako SHIFT.
  const auto shift = static_cast<std::size_t>(vm::Key::Shift);
  _down[shift] = _down[shift] || IsKeyDown(KEY_RIGHT_SHIFT);
  _pressed[shift] = _pressed[shift] || IsKeyPressed(KEY_RIGHT_SHIFT);

  const auto mouse = GetMousePosition();
  _mouseX = mouse.x;
  _mouseY = mouse.y;
  const auto delta = GetMouseDelta();
  _mouseDx = delta.x;
  _mouseDy = delta.y;
}

} // namespace PimpEx::render
