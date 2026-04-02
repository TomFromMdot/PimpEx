#pragma once

#include "interfaces/IEngineFileChecker.h"
#include <filesystem>
namespace PimpEx {
class EngineFileChecker : public IEngineFileChecker {
public:
  const std::string
  get_file_extension(const std::filesystem::path &path) const override;
  bool file_is_xml(const std::filesystem::path &path) const override;
  bool file_is_scene_xml(const std::filesystem::path &path) const override;
  bool
  file_is_scene_resource_xml(const std::filesystem::path &path) const override;
  bool
  file_is_scene_actor_xml(const std::filesystem::path &path) const override;
  bool
  file_is_engine_config_xml(const std::filesystem::path &path) const override;
  bool check_if_modifed(const std::filesystem::path &path) const override;
};
} // namespace PimpEx
