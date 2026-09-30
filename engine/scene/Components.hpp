#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Komponenty wbudowane w silnik. Każdy jest opisany w ComponentRegistry,
// więc XML, .swap, Lua i API widzą je przez nazwy pól, bez kodu C++.
namespace PimpEx::scene {

struct Rgba {
  std::uint8_t r = 255, g = 255, b = 255, a = 255;
  bool operator==(const Rgba &) const = default;
};

// "#rgb", "#rrggbb", "#rrggbbaa" albo nazwa: white, red, zielony...
std::optional<Rgba> parse_color(std::string_view text);
std::string color_to_string(const Rgba &color);

// Wartość pola widziana z zewnątrz: liczba albo tekst.
using FieldValue = std::variant<double, std::string>;

// --- zawsze obecne ------------------------------------------------------------

struct Name {
  std::string value;
};

// Pozycja (środek), rozmiar, obrót i widoczność. Każdy aktor ją ma.
// W 2D `z` zwykle zostaje 0, a kolejność rysowania ustala `layer`.
struct Transform {
  float x = 0, y = 0, z = 0;
  float w = 32, h = 32, d = 32;
  float rx = 0, ry = 0, rz = 0; // obrót w stopniach; w 2D liczy się rz
  float scale = 1;
  int layer = 0;
  bool visible = true;
  std::string space = "world"; // "screen" = HUD w pikselach ekranu
};

// Zmienne użytkownika z <Var>: Player.lives, Game.score...
struct Vars {
  std::map<std::string, double> values;
};

struct Tags {
  std::vector<std::string> values;
};

// Skąd aktor się wziął – potrzebne przy hot-reloadzie scene.xml.
struct Origin {
  bool fromSceneFile = false;
  std::string archetype;
};

// Dane komponentów zdefiniowanych przez użytkownika w components.xml.
struct CustomData {
  std::map<std::string, std::map<std::string, FieldValue>> components;
};

// --- ruch i kolizje -------------------------------------------------------------

// Prędkość (piksele albo jednostki na klatkę) i grawitacja.
struct Body {
  float vx = 0, vy = 0, vz = 0;
  float gravity = 0;
  bool grounded = false; // stoi na czymś solidnym (ustawia fizyka)
};

struct Collider {
  std::string kind = "solid"; // solid = nie da się przejść, trigger = tylko wykrywa
};

// --- wygląd 2D -------------------------------------------------------------------

struct Rect {
  Rgba color;
};
struct Circle {
  Rgba color;
};
struct Sprite {
  std::string texture; // id z <Resources>
  Rgba tint;
};
struct Text {
  std::string value; // może zawierać {Aktor.pole}
  int size = 20;
  Rgba color;
};

// --- wygląd 3D -------------------------------------------------------------------

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
  std::string model; // id z <Resources>
  Rgba tint;
};

// --- kamera i światło ---------------------------------------------------------------

struct Camera {
  // 2D: fixed | follow;  3D: fixed | follow | orbit | fps
  std::string mode = "fixed";
  std::string target; // aktor, za którym kamera podąża / na który patrzy
  float zoom = 1;     // 2D
  float fov = 60;     // 3D
  float distance = 8; // 3D follow/orbit: odległość od celu
  float height = 4;   // 3D follow/orbit: wysokość nad celem
  bool active = true;
};

struct Light {
  std::string kind = "point"; // point | directional
  Rgba color;
  float intensity = 1;
};

// --- zachowania Lua ---------------------------------------------------------------------

struct BehaviorRef {
  std::string name;                          // plik scripts/behaviors/<name>.lua
  std::map<std::string, std::string> params; // atrybuty z <Behavior>
};
struct Behaviors {
  std::vector<BehaviorRef> list;
};

} // namespace PimpEx::scene
