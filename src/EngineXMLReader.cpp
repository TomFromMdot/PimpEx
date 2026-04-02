#include "../include/EngineXML.h"
#include <cctype>
#include <cstring>
#include <filesystem>
#include <glm/detail/qualifier.hpp>
#include <iostream>
#include <map>
#include <optional>
#include <print>
#include <tinyxml2.h>
#include <vector>

// ╭────────────────────────────────────────────────╮
// │ THIS FILE REPRESENTS SOURCE FILE FOR XMLREADER │
// ╰────────────────────────────────────────────────╯

void PimpEx::Utils::EngineXMLReader::open_xml(
    const std::filesystem::path &path) {
  open_and_check_error(path);
}

tinyxml2::XMLElement *PimpEx::Utils::EngineXMLReader::find_actor_element(
    const std::filesystem::path &path) {
  if (open_and_check_error(path))
    return nullptr;
  return _doc.FirstChildElement("Actor");
}

tinyxml2::XMLElement *PimpEx::Utils::EngineXMLReader::find_actor_element(
    const std::filesystem::path &path, const std::string &actorName) {
  std::cout << "Looking for: " << actorName << "\n";
  if (open_and_check_error(path)) {
    return nullptr;
  }

  std::string attrName;
  tinyxml2::XMLElement *root = _doc.FirstChildElement("Actors");
  tinyxml2::XMLElement *element;
  tinyxml2::XMLElement *result;
  for (element = root->FirstChildElement("Actor"); element != nullptr;
       element = element->NextSiblingElement("Actor")) {
    if (_pre &&
        std::strcmp(element->Attribute("name"), _pre->Attribute("name"))) {
      std::cout << "D";
      break;
    }
    _pre = element;
    attrName = element->Attribute("name");
    std::cout << "Actor name: " << attrName << "\n";
    if (attrName == actorName) {
      result = element;
      break;
    }
  }
  return result;
}

tinyxml2::XMLElement *
PimpEx::Utils::EngineXMLReader::find_actor_component_element(
    const std::filesystem::path &path, const std::string &actorName) {
  auto element = find_actor_element(path, actorName);
  if (!element)
    return nullptr;

  auto component = element->FirstChildElement();
  if (!component)
    return nullptr;
  return component;
}

tinyxml2::XMLElement *
PimpEx::Utils::EngineXMLReader::find_actor_component_element(
    const std::filesystem::path &path, const std::string &actorName,
    const std::string &compName) {
  auto element = find_actor_element(path, actorName);
  if (!element)
    return nullptr;
  auto component = element->FirstChildElement(compName.c_str());

  if (!component) {
    std::println("Can't find component: {}", compName);
    return nullptr;
  }
  return component;
}

tinyxml2::XMLElement *PimpEx::Utils::EngineXMLReader::find_actor_in_scene(

    const std::filesystem::path &path, const std::string &sceneName,
    const std::string &actorName) {}

tinyxml2::XMLElement *PimpEx::Utils::EngineXMLReader::find_scene_element(
    const std::filesystem::path &path) {
  if (open_and_check_error(path))
    return nullptr;
  return _doc.FirstChildElement("Scene");
}

tinyxml2::XMLElement *PimpEx::Utils::EngineXMLReader::find_scene_element(
    const std::filesystem::path &path, const std::string &sceneName) {
  if (open_and_check_error(path))
    return nullptr;
  std::string attrName;
  tinyxml2::XMLElement *element;
  for (element = _doc.FirstChildElement("Scene"); element != nullptr;
       element = _doc.NextSiblingElement("Scene")) {
    attrName = element->Attribute("name");
    if (attrName == sceneName)
      return element;
  }

  return nullptr;
}

tinyxml2::XMLElement *
PimpEx::Utils::EngineXMLReader::find_scene_resource_element(
    const std::filesystem::path &path, const std::string &sceneName) {}

tinyxml2::XMLElement *
PimpEx::Utils::EngineXMLReader::find_scene_resource_element(
    const std::filesystem::path &path, const std::string &sceneName,
    const std::string &resourceName) {}

tinyxml2::XMLElement *
PimpEx::Utils::EngineXMLReader::find_engine_config_element(
    const std::filesystem::path &path) {
  if (open_and_check_error(path))
    return nullptr;
  return _doc.FirstChildElement("EngineConfiguration");
}

tinyxml2::XMLElement *
PimpEx::Utils::EngineXMLReader::find_engine_config_element(
    const std::filesystem::path &path, const std::string &configName) {
  if (open_and_check_error(path))
    return nullptr;
  std::string attrName;
  tinyxml2::XMLElement *element;
  for (element = _doc.FirstChildElement("EngineConfiguration");
       element != nullptr;
       element = _doc.NextSiblingElement("EngineConfiguration")) {
    attrName = element->Attribute("name");
    std::transform(attrName.begin(), attrName.end(), attrName.begin(),
                   ::tolower);
    if (attrName == configName)
      return element;
  }
  return nullptr;
}

std::optional<std::vector<PimpEx::Types::EngineActor>>
PimpEx::Utils::EngineXMLReader::get_all_actors(
    const std::filesystem::path &path) {
  if (open_and_check_error(path))
    return std::nullopt;

  std::vector<Types::EngineActor> result;
  tinyxml2::XMLElement *root = _doc.FirstChildElement("Actors");
  for (tinyxml2::XMLElement *element = root->FirstChildElement("Actor");
       element != nullptr; element = root->NextSiblingElement("Actor")) {
    Types::EngineActor a;
    a.name = element->Attribute("name");
    auto components = get_actor_components(path, a.name);
    if (components.has_value())
      a.components = std::move(*components);
    result.push_back(a);
  }
  return result;
}

std::optional<std::vector<PimpEx::Types::EngineActorComponent>>
PimpEx::Utils::EngineXMLReader::get_actor_components(
    const std::filesystem::path &path, const std::string &actorName) {
  std::vector<Types::EngineActorComponent> result;

  auto actor = find_actor_element(path, actorName);
  if (!actor)
    return std::nullopt;
  // Iteracja po każdym komponencie
  for (tinyxml2::XMLElement *component = actor->FirstChildElement();
       component != nullptr; component = actor->NextSiblingElement()) {
    Types::EngineActorComponent c;
    c.name = component->Name();
    // Iteracja po każdym atrybucie wyciągając klucz i wartoś
    for (const auto *attr = component->FirstAttribute(); attr != nullptr;
         attr = attr->Next()) {
      c.attr.insert({attr->Name(), attr->Value()});
    }
    // Iteracja po każdej procedurze w componencie
    for (auto *procedure = component->FirstChildElement(); procedure != nullptr;
         component->NextSiblingElement()) {
      std::map<std::string, std::string> pData;
      // Iteracja po każdym atrybucie wyciągając klucz i wartość procedury
      for (const auto *attr = procedure->FirstAttribute(); attr != nullptr;
           attr = attr->Next()) {
        pData.insert({attr->Name(), attr->Value()});
      }
      c.procedures.insert({procedure->Name(), pData});
    }
    result.push_back(c);
  }
  return result;
}
