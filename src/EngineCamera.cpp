#include "../include/graphics/EngineCamera.hpp"
#include <raylib.h>

void PimpEx::graphics::EngineCamera::camera_input() {
  auto sideway = (IsKeyDown(KEY_A) - IsKeyDown(KEY_D));
  auto forawd = (IsKeyDown(KEY_W) - IsKeyDown(KEY_S));
  _inputAxis = (Vector2){(float)sideway, (float)-forawd};
}

void PimpEx::graphics::EngineCamera::update() { camera_input(); }
