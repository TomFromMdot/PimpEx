#pragma once

#include <map>
#include <memory>
#include <string>

#include "../graphics/EngineScene.hpp"

namespace PimpEx::storage {
class EnigneSceneStorage {
  std::map<std::string, std::shared_ptr<graphics::EngineScene>> _scenes;

public:
  void add_scene(const std::string &sceneName,
                 std::shared_ptr<graphics::EngineScene> scenePtr);

  std::shared_ptr<graphics::EngineScene>
  get_scene(const std::string &sceneName);
};
} // namespace PimpEx::storage
