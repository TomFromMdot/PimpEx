#pragma once
#include "./entity/Scene.h"
#include "interfaces/IEngineFileChecker.h"
#include "interfaces/IEngineStorageAPI.h"
#include <entt/entt.hpp>
#include <filesystem>
#include <memory>
#include <utility>

namespace PimpEx {
class EngineStorage : public IEngineStorageAPI {
  std::unique_ptr<IEngineFileChecker> _fileChecker;
  std::map<std::filesystem::path, Entity::Scene> _scenes;
  entt::registry _reg;

private:
  // Method check all nullable pools and throw exceptions.
  void check_storage_condition();

public:
  EngineStorage(std::unique_ptr<IEngineFileChecker> fileChecker)
      : _fileChecker(std::move(fileChecker)) {}

  // USER API
  void reload_storage();
  void load_engine_configuration(const std::filesystem::path &path);
  void load_script_file(const std::filesystem::path &path);
  void load_sprite_file(const std::filesystem::path &path);
  void load_image_file(const std::filesystem::path &path);
  void load_texture_file(const std::filesystem::path &path);
  void load_audio_file(const std::filesystem::path &path);

  void load_scene_resources(const std::filesystem::path &pathToXML);
  void load_scene_actors(const std::filesystem::path &pathToXML);
  void load_scene_controller(const std::filesystem::path &pathToDLL);
};
} // namespace PimpEx
