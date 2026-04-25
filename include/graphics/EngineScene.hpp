#pragma once

#include <entt/entity/fwd.hpp>
#include <memory>
#include <raylib.h>

#include "../ecs/Transform.h"

namespace PimpEx::graphics {
class EngineScene {
  std::shared_ptr<entt::registry> _entityReg;
  std::shared_ptr<Camera3D> _mainCamera;

public:
  std::shared_ptr<Camera3D> get_camera();
  void init_editor_camera();
};
}; // namespace PimpEx::graphics
