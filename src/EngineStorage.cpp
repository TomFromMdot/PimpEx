#include "../include/storage/EngineSceneStorage.hpp"
#include <cstdio>
#include <memory>

void PimpEx::storage::EnigneSceneStorage::add_scene(
    const std::string &sceneName,
    std::shared_ptr<graphics::EngineScene> scenePtr) {
  _scenes[sceneName] = std::move(scenePtr);
}

std::shared_ptr<PimpEx::graphics::EngineScene>
PimpEx::storage::EnigneSceneStorage::get_scene(const std::string &sceneName) {
  return _scenes.find(sceneName)->second;
}
