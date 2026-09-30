#pragma once

#include "scene/Components.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

// Dane wczytane z plików XML – jeszcze nie „żywe” w rejestrze EnTT.
namespace PimpEx::scene {

// Opis aktora (albo archetypu) z scene.xml.
struct ActorDef {
  std::string name;
  int line = 0;
  Transform transform;
  Velocity velocity;
  std::optional<Shape> shape;
  std::optional<Sprite> sprite;
  std::optional<Label> label;
  Vars vars;
};

struct TextureResource {
  std::string id;
  std::string path; // względem katalogu projektu
};

struct SceneData {
  std::string name;
  std::optional<Rgba> background;
  std::vector<ActorDef> actors;
  std::map<std::string, ActorDef> archetypes;
  std::map<std::string, TextureResource> textures;
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
