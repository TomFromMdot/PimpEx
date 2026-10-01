#pragma once

#include "scene/Components.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Data loaded from XML – not yet live in the EnTT registry.
namespace PimpEx::scene {

// One actor component from XML with validated field values.
struct ComponentInit {
  std::string component;
  std::vector<std::pair<std::string, FieldValue>> values;
  int line = 0;
};

struct ActorDef {
  std::string name;
  int line = 0;
  std::vector<std::string> tags;
  std::vector<ComponentInit> components; // always starts with Transform
  Vars vars;
  std::vector<BehaviorRef> behaviors;
  std::string archetype;    // <Tilemap> actors: their archetype
  bool fromTilemap = false;

  bool has(std::string_view component) const {
    for (const auto &c : components)
      if (c.component == component)
        return true;
    return false;
  }
};

struct Resource {
  std::string id;
  std::string path; // relative to the project root
};

enum class SceneMode { Mode2D, Mode3D };

struct SceneData {
  std::string name;
  SceneMode mode = SceneMode::Mode2D;
  std::optional<Rgba> background;
  std::vector<ActorDef> actors;
  std::map<std::string, ActorDef> archetypes;
  std::map<std::string, Resource> textures;
  std::map<std::string, Resource> models;
  std::vector<std::string> gui; // <Scene gui="hud pause">: shown when the scene loads
  int guiLine = 0;
};

// pimpex.xml
struct ProjectConfig {
  std::string name;
  std::string title = "PimpEx";
  int width = 800;
  int height = 600;
  int fps = 60;
  Rgba background{16, 16, 24, 255};
  std::string startScene = "main";
  bool keepStateOnReload = true; // hot-reloading scene.xml keeps positions
};

} // namespace PimpEx::scene
