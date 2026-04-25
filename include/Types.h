#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <raylib.h>

namespace PimpEx::Types {
using ComponentsBuffer =
    std::map<std::string, std::vector<std::pair<std::string, std::string>>>;
using ComponentProceduresBuffer =
    std::pair<std::string, std::vector<std::pair<std::string, std::string>>>;

using EngineXMLElement =
    std::pair<std::string, std::map<std::string, std::string>>;

struct EngineGraphicsWindowResolution {
  int x, y;
  std::string aspekt;
};

// Main wątek do przetwarzania grafiki.
// Drugi wątek powinien być odpowiedzialny za odczytanie danych,
// przemielenie ich na następne dane gotowe do odbioru przez Main wątek.
// głównym zamysłem jest odciążenie głównej pętli gry od ciężkich operacji.
// Adaptery wymagają czasu a same dane wymagają procesu pieczenia w piecu zanim
// ciasto będzie gotowe.

struct EngineGraphicsWindowOptions {
private:
  bool isFullscreen = false;
  int exitKey = KEY_ESCAPE;
  std::map<std::string, Types::EngineGraphicsWindowResolution> Resolution;

public:
  EngineGraphicsWindowOptions &
  add_resolution(EngineGraphicsWindowResolution res);
};

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
  std::string id;
  std::string name;
  std::vector<EngineActorComponent> components;
};

struct EngineResource {
  enum class Type { Model, Texture, Audio, Error };
  std::string name;
  Type type;
  std::filesystem::path path;
  const std::string get_string_type(EngineResource::Type type) const;
  const Type get_type_from_string(const std::string &type) const;
};

struct EngineScene {
  std::map<std::string, std::shared_ptr<EngineResource>> resources;
  std::map<std::string, std::shared_ptr<EngineActor>> actors;

  const std::optional<std::weak_ptr<EngineResource>>
  get_resouce(const std::string &resourceId) const;
};

struct EngineProject {
  std::map<std::string, std::shared_ptr<EngineScene>> scenes;
};
} // namespace PimpEx::Types
