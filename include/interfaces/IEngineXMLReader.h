#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <tinyxml2.h>
#include <utility>
#include <vector>

#include "../Types.h"

namespace PimpEx {
class IEngineXMLReader {
public:
  virtual ~IEngineXMLReader() = default;

  virtual void open_xml(const std::filesystem::path &path) = 0;

  virtual tinyxml2::XMLElement *
  find_actor_element(const std::filesystem::path &path) = 0;

  virtual tinyxml2::XMLElement *
  find_actor_element(const std::filesystem::path &path,
                     const std::string &actorName) = 0;

  virtual tinyxml2::XMLElement *
  find_actor_component_element(const std::filesystem::path &path,
                               const std::string &actorName) = 0;

  virtual tinyxml2::XMLElement *
  find_actor_component_element(const std::filesystem::path &path,
                               const std::string &actorName,
                               const std::string &compName) = 0;

  virtual tinyxml2::XMLElement *
  find_actor_in_scene(const std::filesystem::path &path,
                      const std::string &sceneName,
                      const std::string &actorName) = 0;

  virtual tinyxml2::XMLElement *
  find_scene_element(const std::filesystem::path &path) = 0;

  virtual tinyxml2::XMLElement *
  find_scene_element(const std::filesystem::path &path,
                     const std::string &sceneName) = 0;

  virtual tinyxml2::XMLElement *
  find_scene_resource_element(const std::filesystem::path &path,
                              const std::string &sceneName) = 0;

  virtual tinyxml2::XMLElement *
  find_scene_resource_element(const std::filesystem::path &path,
                              const std::string &sceneName,
                              const std::string &resourceName) = 0;

  virtual tinyxml2::XMLElement *
  find_engine_config_element(const std::filesystem::path &path) = 0;

  virtual tinyxml2::XMLElement *
  find_engine_config_element(const std::filesystem::path &path,
                             const std::string &configName) = 0;

  virtual std::optional<std::vector<Types::EngineActorComponent>>
  get_actor_components(const std::filesystem::path &path,
                       const std::string &actorName) = 0;
};

} // namespace PimpEx
