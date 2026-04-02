#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>
namespace PimpEx::Types {
using ComponentsBuffer =
    std::map<std::string, std::vector<std::pair<std::string, std::string>>>;
using ComponentProceduresBuffer =
    std::pair<std::string, std::vector<std::pair<std::string, std::string>>>;

using EngineXMLElement =
    std::pair<std::string, std::map<std::string, std::string>>;

struct EngineActorComponent {
  std::string name;
  std::map<std::string, std::string> attr;
  std::map<std::string, std::map<std::string, std::string>> procedures;
};

struct EngineActor {
  std::string name;
  std::vector<EngineActorComponent> components;
};

} // namespace PimpEx::Types
