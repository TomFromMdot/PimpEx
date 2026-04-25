#include "../include/graphics/EngineScene.hpp"
#include <memory>
#include <raylib.h>

void PimpEx::graphics::EngineScene::init_editor_camera() {
  _mainCamera.position = (Vector3){0.0f, 0.0f, 0.0f};
  _mainCamera.target = (Vector3){0.0f, 0.0f, 0.0f};
  _mainCamera.up = (Vector3){0.0f, 1.0f, 0.0f};
  _mainCamera.fovy = 45.f;
  _mainCamera.projection = CAMERA_PERSPECTIVE;
}
