#pragma once

#include "Types.h"
#include "interfaces/IEngineXMLReader.h"
#include "interfaces/IEnigneXMLWriter.h"
#include <complex>
#include <filesystem>
#include <glm/detail/qualifier.hpp>
#include <iostream>
#include <optional>
#include <print>
#include <tinyxml2.h>
#include <vector>

namespace PimpEx::Utils {
class EngineXMLReader : public IEngineXMLReader {
  tinyxml2::XMLDocument _doc;
  std::filesystem::path _path;
  tinyxml2::XMLElement *_pre;
  /*
   * @brief Mehtod open xml file check if all is good then return 0 if all ok
   * @param path - Path to file.
   * @return true if something go wrong.*/
  bool open_and_check_error(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
      std::println("Path not exists: {}", path.c_str());
      return true;
    }
    if (std::filesystem::is_directory(path)) {
      std::println("Path return directory not file: {}", path.c_str());
      return true;
    }
    auto result = _doc.LoadFile(path.c_str());
    if (result != tinyxml2::XML_SUCCESS) {
      std::cout << "XML ERROR" << _doc.ErrorStr() << "\n";
      return true;
    }
    return false;
  }

public:
  EngineXMLReader() {};
  ~EngineXMLReader() {};

  void open_xml(const std::filesystem::path &path) override;

  tinyxml2::XMLElement *
  find_actor_element(const std::filesystem::path &path) override;

  tinyxml2::XMLElement *
  find_actor_element(const std::filesystem::path &path,
                     const std::string &actorName) override;

  tinyxml2::XMLElement *
  find_actor_component_element(const std::filesystem::path &path,
                               const std::string &actorName) override;

  tinyxml2::XMLElement *
  find_actor_component_element(const std::filesystem::path &path,
                               const std::string &actorName,
                               const std::string &compName) override;

  tinyxml2::XMLElement *
  find_actor_in_scene(const std::filesystem::path &path,
                      const std::string &sceneName,
                      const std::string &actorName) override;

  tinyxml2::XMLElement *
  find_scene_element(const std::filesystem::path &path) override;

  tinyxml2::XMLElement *
  find_scene_element(const std::filesystem::path &path,
                     const std::string &sceneName) override;

  tinyxml2::XMLElement *
  find_scene_resource_element(const std::filesystem::path &path,
                              const std::string &sceneName) override;

  tinyxml2::XMLElement *
  find_scene_resource_element(const std::filesystem::path &path,
                              const std::string &sceneName,
                              const std::string &resourceName) override;

  tinyxml2::XMLElement *
  find_engine_config_element(const std::filesystem::path &path) override;

  tinyxml2::XMLElement *
  find_engine_config_element(const std::filesystem::path &path,
                             const std::string &configName) override;

  std::optional<std::vector<Types::EngineActor>>
  get_all_actors(const std::filesystem::path &path);
  std::optional<std::vector<Types::EngineActorComponent>>
  get_actor_components(const std::filesystem::path &path,
                       const std::string &actorName) override;
};

class EngineXMLWriter : public IEngineXMLWriter {
  tinyxml2::XMLDocument doc;

public:
  EngineXMLWriter() {}
  ~EngineXMLWriter() {}
  bool save_actor_element(const std::filesystem::path &path,
                          void *actor) override;
  bool save_scene_element(const std::filesystem::path &path,
                          void *scene) override;

  bool save_resource_element(const std::filesystem::path &path,
                             void *resource) override;

  bool save_engine_config_element(const std::filesystem::path &path,
                                  void *config) override;

  bool save_engine_config_value_element(const std::filesystem::path &path,
                                        const std::string &configName,
                                        void *configValue) override;

  bool make_actors_bundle(const std::filesystem::path &path) override;

  bool make_scenes_bundle(const std::filesystem::path &path) override;

  bool make_resources_bundle(const std::filesystem::path &path) override;

  bool save_actor_component(const std::filesystem::path &path,
                            const std::string &actorName,
                            const void *compBuffer) override;

  bool save_component_procedures(const std::filesystem::path &path,
                                 const std::string &actorName,
                                 const void *proceduresBuffer) override;
};
}; // namespace PimpEx::Utils
