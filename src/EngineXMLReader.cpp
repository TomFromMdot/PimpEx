#include "../include/EngineXML.h"
#include "../include/Types.h"
#include <algorithm>
#include <optional>
#include <vector>

namespace PimpEx::Utils {
EngineXMLReader::EngineXMLReader(const std::string &path) { _filePath = path; }
EngineXMLReader::~EngineXMLReader() {}
std::optional<
    const std::map<std::string, std::shared_ptr<Types::EngineResource>>>
EngineXMLReader::get_scene_resources(const std::string &sceneName) {
  if (file_checker())
    return std::nullopt;

  // //Scene[@name="NAZWA_SCENY"]/Resources/Resource
}

std::optional<const std::map<std::string, std::shared_ptr<Types::EngineActor>>>
EngineXMLReader::get_scene_actors(const std::string &sceneName) {
  // //Scene[@name="NAZWA_SCENY"]/Actors/Actor
}

std::optional<std::vector<Types::EngineActorComponent>>
EngineXMLReader::get_scene_actor_components(const std::string &sceneName,
                                            const std::string &actorName) {
  std::vector<Types::EngineActorComponent> comps;
  // //Scene[@id="NAZWA_SCENY"]/Actors/Actor[@id="NAZWA_AKTORA"]/Components/Component
  if (file_checker())
    return std::nullopt;
  std::string queryString = "/Project/Scene[@id='" + sceneName +
                            "']/Actors/Actor[@id='" + actorName +
                            "']/Components/Component";
  std::cout << "Query: " << queryString << "\n";
  try {
    pugi::xpath_node_set result = _doc.select_nodes(queryString.c_str());
    for (auto comp : result) {
      pugi::xml_node component = comp.node();
      Types::EngineActorComponent aComp;
      aComp.name = component.attribute("type").value();
      auto p = component.select_nodes("Property");

      for (const auto e : p) {
        Types::EngineComponentProperty props;
        pugi::xml_node node = e.node();
        props.name = node.attribute("name").value();
        aComp.property.push_back(std::move(props));
      }

      auto onStart = component.select_nodes("OnStart");

      for (const auto &e : onStart) {
        pugi::xml_node node = e.node();
        for (const auto &d : node) {
          std::string pName = d.attribute("name").value();
          aComp.onStartProcedures.push_back(std::move(pName));
        }
      }
      comps.push_back(std::move(aComp));
    }
  } catch (const pugi::xpath_exception &e) {
    std::cout << "Błąd xpath: " << e.what() << "\n";
    return std::nullopt;
  }
  return comps;
}
} // namespace PimpEx::Utils
