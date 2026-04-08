#pragma once

#include <filesystem>
#include <map>
#include <memory>
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

struct EngineComponentProperty {
  std::string name;
  std::map<std::string, std::string> attrs;
};

struct EngineActorComponent {
  std::string name;
  std::vector<EngineComponentProperty> property;
  std::vector<std::string> onStartProcedures;
};

struct EngineActor {
  std::string name;
  std::vector<EngineActorComponent> components;
};

struct EngineResource {
  enum class Type { Model, Texture, Audio };
  std::string name;
  Type type;
  std::filesystem::path path;
};

struct EngineScene {
  std::map<std::string, std::shared_ptr<EngineResource>> resources;
  std::map<std::string, std::shared_ptr<EngineActor>> actors;
};

struct EngineProject {
  std::map<std::string, std::shared_ptr<EngineScene>> scenes;
};
} // namespace PimpEx::Types
