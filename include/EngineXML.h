#pragma once

#include "Types.h"
#include "interfaces/IEngineXMLReader.h"
#include "interfaces/IEnigneXMLWriter.h"
#include <filesystem>
#include <glm/detail/qualifier.hpp>
#include <iostream>
#include <optional>
#include <pugixml.hpp>
#include <vector>

namespace PimpEx::Utils {
class EngineXMLReader : public IEngineXMLReader {
  std::filesystem::path _filePath;
  pugi::xml_document _doc;
  pugi::xml_parse_result _result;

private:
  bool file_checker() {
    _result = _doc.load_file(_filePath.c_str());
    if (!_result) {
      std::cerr << "Can't parse file: " << _filePath
                << "\nError: " << _result.description() << "\n";
      std::cerr << "Błąd offsetL: " << _result.offset << "(Okolice znaku)\n";
      if (_result.status == pugi::status_file_not_found) {
        std::cerr << "Nie znaleziono pliku\n";
      }
      return true;
    }
    return false;
  }

  std::optional<std::vector<Types::EngineActorComponent>>
  get_scene_actor_components(const std::string &sceneName,
                             const std::string &actorName) override;

  const std::vector<Types::EngineComponentProperty>
  get_properties(pugi::xml_node component);

  const std::vector<std::string> get_procedures(pugi::xml_node component);

  const std::map<std::string, std::string> get_attrs(pugi::xml_node element);

public:
  explicit EngineXMLReader(const std::string &path);
  ~EngineXMLReader();

  std::optional<
      const std::map<std::string, std::shared_ptr<Types::EngineResource>>>
  get_scene_resources(const std::string &sceneName) override;

  std::optional<
      const std::map<std::string, std::shared_ptr<Types::EngineActor>>>
  get_scene_actors(const std::string &sceneName) override;
};

class EngineXMLWriter : public IEngineXMLWriter {
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
