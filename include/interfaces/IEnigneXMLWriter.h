#pragma once

#include <filesystem>
namespace PimpEx {
class IEngineXMLWriter {
  // DOCUMENT XML
public:
  virtual ~IEngineXMLWriter() = default;

  virtual bool save_actor_element(const std::filesystem::path &path,
                                  void *actor) = 0;
  virtual bool save_scene_element(const std::filesystem::path &path,
                                  void *scene) = 0;

  virtual bool save_resource_element(const std::filesystem::path &path,
                                     void *resource) = 0;

  virtual bool save_engine_config_element(const std::filesystem::path &path,
                                          void *config) = 0;
  virtual bool
  save_engine_config_value_element(const std::filesystem::path &path,
                                   const std::string &configName,
                                   void *configValue) = 0;
  virtual bool make_actors_bundle(const std::filesystem::path &path) = 0;

  virtual bool make_scenes_bundle(const std::filesystem::path &path) = 0;

  virtual bool make_resources_bundle(const std::filesystem::path &path) = 0;

  virtual bool save_actor_component(const std::filesystem::path &path,
                                    const std::string &actorName,
                                    const void *compBuffer) = 0;
  virtual bool save_component_procedures(const std::filesystem::path &path,
                                         const std::string &actorName,
                                         const void *proceduresBuffer) = 0;
};
} // namespace PimpEx
