#pragma once

#include "scene/Components.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Dane wczytane z plików XML – jeszcze nie „żywe” w rejestrze EnTT.
namespace PimpEx::scene {

// Jeden komponent aktora z XML z już sprawdzonymi wartościami pól.
struct ComponentInit {
  std::string component;
  std::vector<std::pair<std::string, FieldValue>> values;
  int line = 0;
};

// Opis aktora (albo archetypu) z scene.xml.
struct ActorDef {
  std::string name;
  int line = 0;
  std::vector<std::string> tags;
  std::vector<ComponentInit> components; // zawsze zaczyna się od Transform
  Vars vars;
  std::vector<BehaviorRef> behaviors;
  std::string archetype;    // aktor z <Tilemap>: z jakiego archetypu powstał
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
  std::string path; // względem katalogu projektu
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
  std::vector<std::string> gui; // <Scene gui="hud pauza">: GUI widoczne po wczytaniu sceny
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
  bool keepStateOnReload = true; // hot-reload scene.xml nie resetuje pozycji
};

} // namespace PimpEx::scene
