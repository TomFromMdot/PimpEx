#pragma once
#include <raylib.h>
#include <raymath.h>

namespace PimpEx::graphics {
class EngineMouse {
  Vector2 _position;

public:
  Vector2 get_mouse_position();
  void update_mouse_input();
};
}; // namespace PimpEx::graphics
