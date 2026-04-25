#pragma once

#include "./EngineScene.hpp"

#include <memory>

namespace PimpEx::graphics {
class EngineRenderer {
  std::shared_ptr<EngineScene> _currentScene;

public:
  // Narazie coś tego
  void set_target_scene(std::shared_ptr<EngineScene> scenePtr);
  void render();
};
}; // namespace PimpEx::graphics
