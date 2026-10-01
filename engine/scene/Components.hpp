#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Built-in components. Each is described in ComponentRegistry, so XML, .swap,
// Lua and the API reach them by field name without extra C++.
namespace PimpEx::scene {

struct Rgba {
  std::uint8_t r = 255, g = 255, b = 255, a = 255;
  bool operator==(const Rgba &) const = default;
};

// "#rgb", "#rrggbb", "#rrggbbaa" or a name: white, red, zielony...
std::optional<Rgba> parse_color(std::string_view text);
std::string color_to_string(const Rgba &color);

// A field value as seen from outside: a number or text.
using FieldValue = std::variant<double, std::string>;

struct Name {
  std::string value;
};

// Every actor has one. In 2D `z` usually stays 0 and `layer` sets the drawing order.
struct Transform {
  float x = 0, y = 0, z = 0;
  float w = 32, h = 32, d = 32;
  float rx = 0, ry = 0, rz = 0; // degrees; 2D uses rz
  float scale = 1;
  int layer = 0;
  bool visible = true;
  std::string space = "world"; // "screen" = HUD in screen pixels
};

// <Var> variables: Player.lives, Game.score...
struct Vars {
  std::map<std::string, double> values;
};

struct Tags {
  std::vector<std::string> values;
};

// Where the actor came from – needed when scene.xml is hot-reloaded.
struct Origin {
  bool fromSceneFile = false;
  std::string archetype;
  bool fromTilemap = false; // a <Tilemap> tile (e.g. a wall)
};

// Data of the components defined in components.xml.
struct CustomData {
  std::map<std::string, std::map<std::string, FieldValue>> components;
};

// Velocity in pixels (2D) or units (3D) per frame.
struct Body {
  float vx = 0, vy = 0, vz = 0;
  float gravity = 0;
  bool grounded = false; // set by physics
};

struct Collider {
  std::string kind = "solid"; // or "trigger"
};

struct Rect {
  Rgba color;
};
struct Circle {
  Rgba color;
};
struct Sprite {
  std::string texture; // id from <Resources>
  Rgba tint;
};
struct Text {
  std::string value; // may contain {Actor.field}
  int size = 20;
  Rgba color;
};

struct Cube {
  Rgba color;
  bool wire = false;
};
struct Sphere {
  Rgba color;
  bool wire = false;
};
struct Cylinder {
  Rgba color;
  bool wire = false;
};
struct Plane {
  Rgba color;
};
struct Model {
  std::string model; // id from <Resources>
  Rgba tint;
};

struct Camera {
  // 2D: fixed | follow;  3D: fixed | follow | orbit | fps
  std::string mode = "fixed";
  std::string target;
  float zoom = 1;     // 2D
  float fov = 60;     // 3D
  float distance = 8; // 3D follow/orbit
  float height = 4;   // 3D follow/orbit
  bool active = true;
};

struct Light {
  std::string kind = "point"; // point | directional | ambient
  Rgba color;
  float intensity = 1;
};

struct BehaviorRef {
  std::string name;                          // scripts/behaviors/<name>.lua
  std::map<std::string, std::string> params; // <Behavior> attributes
};
struct Behaviors {
  std::vector<BehaviorRef> list;
};

} // namespace PimpEx::scene
