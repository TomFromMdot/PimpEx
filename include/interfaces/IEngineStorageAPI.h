#pragma once

#include <filesystem>
namespace PimpEx {
class IEngineStorageAPI {
public:
  virtual ~IEngineStorageAPI() = default;
  virtual void reload_storage() = 0;
  virtual void load_engine_configuration(const std::filesystem::path &path) = 0;
  virtual void load_script_file(const std::filesystem::path &path) = 0;
  virtual void load_sprite_file(const std::filesystem::path &path) = 0;
  virtual void load_image_file(const std::filesystem::path &path) = 0;
  virtual void load_texture_file(const std::filesystem::path &path) = 0;
  virtual void load_audio_file(const std::filesystem::path &path) = 0;
  virtual void load_scene_resources(const std::filesystem::path &pathToXML) = 0;
  virtual void load_scene_actors(const std::filesystem::path &pathToXML) = 0;
  virtual void
  load_scene_controller(const std::filesystem::path &pathToDLL) = 0;
};
} // namespace PimpEx
