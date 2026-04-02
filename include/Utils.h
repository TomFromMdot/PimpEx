#pragma once
#include "EngineXML.h"
#include "Types.h"
#include <filesystem>
#include <iostream>
#include <vector>

namespace PimpEx::Utils {
inline void print_actor_component(const std::filesystem::path &path,
                                  const std::string &actorName) {
  EngineXMLReader reader;
  auto aComp = reader.get_actor_components(path, actorName);
  if (aComp.has_value()) {
    for (const auto comp : *aComp) {
      std::cout << "Component name: " << comp.name << "\n";
      for (const auto &[key, value] : comp.attr) {
        std::cout << "Attr name: " << key << " Attr value: " << value << "\n";
      }
      for (const auto &[key, value] : comp.procedures) {
        std::cout << "Procedure name: " << key << "\n";
        for (const auto &[attrKey, attrVal] : value) {
          std::cout << "Attr name: " << attrKey << " Attr value: " << attrVal
                    << "\n";
        }
      }
    }
  } else {
    std::cout << "Can't find actor\n";
  }
}

inline void print_actors(const std::filesystem::path &path) {
  EngineXMLReader reader2;
  auto actors = reader2.get_all_actors("./test.xml");

  if (actors.has_value()) {
    for (auto actor : *actors) {
      std::cout << actor.name << "\n";
    }
  }
  return;
}

} // namespace PimpEx::Utils
