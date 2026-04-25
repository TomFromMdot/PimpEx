#pragma once

#include <memory>
#include <raylib.h>
#include <raymath.h>
namespace PimpEx::graphics {
class EngineCamera {
  bool _isMain = false;
  std::unique_ptr<Camera3D> _camera;
  Vector3 _position = {0.0f, 0.0f, 0.0f};
  Vector3 _target = {0.0f, 0.0f, 10.f};
  Vector3 _up = {0.0f, 1.0f, 0.0f};
  float _fov = 45.f;
  CameraProjection _projection = CAMERA_PERSPECTIVE;
  Vector2 _inputAxis;

public:
  EngineCamera(bool isMain = false) : _isMain(isMain) {}
  virtual void camera_input();
  virtual void update();
};
} // namespace PimpEx::graphics
