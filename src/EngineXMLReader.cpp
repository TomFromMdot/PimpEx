#include "../include/EngineXML.h"
#include "../include/Types.h"
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <print>
#include <vector>

PimpEx::Utils::EngineXMLReader::EngineXMLReader(const std::string &path) {
  _filePath = path;
}
PimpEx::Utils::EngineXMLReader::~EngineXMLReader() {}

std::optional<
    const std::map<std::string, std::shared_ptr<PimpEx::Types::EngineResource>>>
PimpEx::Utils::EngineXMLReader::get_scene_resources(
    const std::string &sceneName) {
  if (file_checker())
    return std::nullopt;
  // //Scene[@name="NAZWA_SCENY"]/Resources/Resource
  std::map<std::string, std::shared_ptr<Types::EngineResource>> mapResult;
  // MAKE SOME GUARD FOR STRING INJECTION
  std::string query =
      std::format("/Project/Scene[@id='{}']/Resources", sceneName);

  try {
    pugi::xpath_node xPathResult = _doc.select_node(query.c_str());
    pugi::xml_node resourceContainer = xPathResult.node();
    for (pugi::xml_node resourceNode = resourceContainer.child("Resource");
         resourceNode; resourceNode = resourceNode.next_sibling("Resource")) {
      auto engineResourcePtr = std::make_shared<Types::EngineResource>();
      engineResourcePtr->name = resourceNode.attribute("id").value();
      engineResourcePtr->path = resourceNode.attribute("path").value();
      engineResourcePtr->type = engineResourcePtr->get_type_from_string(
          resourceNode.attribute("type").value());
      mapResult[engineResourcePtr->name] = std::move(engineResourcePtr);
    }
  } catch (const pugi::xpath_exception &e) {
    std::cerr << "Error with parsing by xPath\nError: " << e.what() << "\n";
    return std::nullopt;
  }
  return mapResult;
}

std::optional<
    const std::map<std::string, std::shared_ptr<PimpEx::Types::EngineActor>>>
PimpEx::Utils::EngineXMLReader::get_scene_actors(const std::string &sceneName) {
  // //Scene[@name="NAZWA_SCENY"]/Actors/Actor
  if (file_checker())
    return std::nullopt;
  std::map<std::string, std::shared_ptr<Types::EngineActor>> mapResult;

  std::string query = std::format("/Project/Scene[@id='{}']/Actors", sceneName);
  auto result = _doc.select_node(query.c_str());

  std::cout << query << "\n";
  if (!result)
    return std::nullopt;
  pugi::xml_node actorContainer = result.node();
  for (pugi::xml_node actor = actorContainer.child("Actor"); actor;
       actor = actor.next_sibling("Actor")) {
    auto engineActor = std::make_shared<Types::EngineActor>();
    std::string actorId = actor.attribute("id").value();
    std::string actorName = actor.attribute("name").value();
    std::println("Find Actor: {} with id: {}", actorName, actorId);
    engineActor->name = actorName;
    engineActor->id = actorId;
    auto actorComponents =
        get_scene_actor_components(sceneName, engineActor->id);
    if (actorComponents.has_value())
      engineActor->components = std::move(*actorComponents);
    mapResult[engineActor->name] = std::move(engineActor);
  }
  return mapResult;
}

std::optional<std::vector<PimpEx::Types::EngineActorComponent>>
PimpEx::Utils::EngineXMLReader::get_scene_actor_components(
    const std::string &sceneName, const std::string &actorName) {
  std::vector<Types::EngineActorComponent> comps;
  // //Scene[@id="NAZWA_SCENY"]/Actors/Actor[@id="NAZWA_AKTORA"]/Components/Component

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
      aComp.property = get_properties(component);
      aComp.onStartProcedures = get_procedures(component);
      comps.push_back(std::move(aComp));
    }
  } catch (const pugi::xpath_exception &e) {
    std::cout << "Błąd xpath: " << e.what() << "\n";
    return std::nullopt;
  }
  return comps;
}

const std::map<std::string, std::string>
PimpEx::Utils::EngineXMLReader::get_attrs(pugi::xml_node element) {
  std::map<std::string, std::string> mapResult;
  for (pugi::xml_attribute attr = element.first_attribute(); attr;
       attr = attr.next_attribute()) {
    mapResult[attr.name()] = attr.value();
  }
  return mapResult;
}

const std::vector<PimpEx::Types::EngineComponentProperty>
PimpEx::Utils::EngineXMLReader::get_properties(pugi::xml_node component) {
  std::vector<PimpEx::Types::EngineComponentProperty> vectorResult;
  for (auto prop = component.child("Property"); prop;
       prop = component.next_sibling("Property")) {
    Types::EngineComponentProperty property;
    property.name = prop.attribute("name").value();
    vectorResult.push_back(property);
  }
  return vectorResult;
}

const std::vector<std::string>
PimpEx::Utils::EngineXMLReader::get_procedures(pugi::xml_node component) {
  std::vector<std::string> vectorResult;
  auto onStartNode = component.child("OnStart");
  for (auto p = onStartNode.child("Procedure"); p;
       p = onStartNode.next_sibling("Procedure")) {
    vectorResult.push_back(p.attribute("name").value());
  }
  return vectorResult;
}
