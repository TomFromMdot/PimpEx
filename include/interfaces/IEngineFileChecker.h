#pragma once

#include <filesystem>
#include <optional>
namespace PimpEx {
class IEngineFileChecker {
public:
  virtual ~IEngineFileChecker() = default;

  virtual const std::string
  get_file_extension(const std::filesystem::path &path) const = 0;
  virtual bool file_is_xml(const std::filesystem::path &path) const = 0;
  virtual bool file_is_scene_xml(const std::filesystem::path &path) const = 0;
  virtual bool
  file_is_scene_resource_xml(const std::filesystem::path &path) const = 0;
  virtual bool
  file_is_scene_actor_xml(const std::filesystem::path &path) const = 0;
  virtual bool
  file_is_engine_config_xml(const std::filesystem::path &path) const = 0;
  virtual bool check_if_modifed(const std::filesystem::path &path) const = 0;
};
} // namespace PimpEx
