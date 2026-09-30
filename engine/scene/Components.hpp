#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

// Komponenty aktorów przechowywane w rejestrze EnTT.
namespace PimpEx::scene {

struct Rgba {
  std::uint8_t r = 255, g = 255, b = 255, a = 255;
  bool operator==(const Rgba &) const = default;
};

// "#rgb", "#rrggbb", "#rrggbbaa" albo nazwa: white, red, ...
std::optional<Rgba> parse_color(std::string_view text);

struct Name {
  std::string value;
};

// Pozycja (środek), rozmiar i widoczność. Każdy aktor ją ma.
struct Transform {
  float x = 0, y = 0;
  float w = 32, h = 32;
  float rotation = 0;
  int z = 0; // kolejność rysowania – większe z rysuje się na wierzchu
  bool visible = true;
};

// Prędkość w pikselach na klatkę. Silnik dodaje ją do pozycji co klatkę.
struct Velocity {
  float vx = 0, vy = 0;
};

struct Shape {
  enum class Kind { Rect, Circle };
  Kind kind = Kind::Rect;
  Rgba color;
};

struct Sprite {
  std::string texture; // id zasobu z <Resources>
  Rgba tint;
};

// Napis rysowany na aktorze. `text` może zawierać {Aktor.pole}.
struct Label {
  std::string text;
  int size = 20;
  Rgba color;
};

// Zmienne użytkownika: Player.lives, Game.score...
struct Vars {
  std::map<std::string, double> values;
};

// Skąd aktor się wziął – potrzebne przy hot-reloadzie scene.xml.
struct Origin {
  bool fromSceneFile = false;
  std::string archetype;
};

} // namespace PimpEx::scene
