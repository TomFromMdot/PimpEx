#include "scene/Components.hpp"

#include <array>
#include <format>
#include <utility>

namespace PimpEx::scene {
namespace {

int hex_digit(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

constexpr std::array<std::pair<std::string_view, Rgba>, 26> NamedColors = {{
    {"white", {255, 255, 255, 255}},
    {"black", {0, 0, 0, 255}},
    {"red", {230, 41, 55, 255}},
    {"green", {0, 228, 48, 255}},
    {"blue", {0, 121, 241, 255}},
    {"yellow", {253, 249, 0, 255}},
    {"orange", {255, 161, 0, 255}},
    {"purple", {200, 122, 255, 255}},
    {"pink", {255, 109, 194, 255}},
    {"gray", {130, 130, 130, 255}},
    {"brown", {127, 106, 79, 255}},
    {"cyan", {0, 220, 230, 255}},
    {"gold", {255, 203, 0, 255}},
    {"transparent", {0, 0, 0, 0}},
    // Polish names for the youngest creators.
    {"biały", {255, 255, 255, 255}},
    {"czarny", {0, 0, 0, 255}},
    {"czerwony", {230, 41, 55, 255}},
    {"zielony", {0, 228, 48, 255}},
    {"niebieski", {0, 121, 241, 255}},
    {"żółty", {253, 249, 0, 255}},
    {"pomarańczowy", {255, 161, 0, 255}},
    {"fioletowy", {200, 122, 255, 255}},
    {"różowy", {255, 109, 194, 255}},
    {"szary", {130, 130, 130, 255}},
    {"brązowy", {127, 106, 79, 255}},
    {"złoty", {255, 203, 0, 255}},
}};

} // namespace

std::optional<Rgba> parse_color(std::string_view text) {
  for (const auto &[name, color] : NamedColors)
    if (name == text)
      return color;

  if (text.empty() || text[0] != '#')
    return std::nullopt;
  text.remove_prefix(1);

  std::array<int, 8> digits{};
  for (std::size_t i = 0; i < text.size() && i < digits.size(); ++i) {
    digits[i] = hex_digit(text[i]);
    if (digits[i] < 0)
      return std::nullopt;
  }

  auto byte = [&](int hi, int lo) {
    return static_cast<std::uint8_t>(digits[hi] * 16 + digits[lo]);
  };
  switch (text.size()) {
  case 3:
    return Rgba{byte(0, 0), byte(1, 1), byte(2, 2), 255};
  case 6:
    return Rgba{byte(0, 1), byte(2, 3), byte(4, 5), 255};
  case 8:
    return Rgba{byte(0, 1), byte(2, 3), byte(4, 5), byte(6, 7)};
  default:
    return std::nullopt;
  }
}

std::string color_to_string(const Rgba &color) {
  for (const auto &[name, named] : NamedColors)
    if (named == color)
      return std::string(name);
  if (color.a == 255)
    return std::format("#{:02x}{:02x}{:02x}", color.r, color.g, color.b);
  return std::format("#{:02x}{:02x}{:02x}{:02x}", color.r, color.g, color.b, color.a);
}

} // namespace PimpEx::scene
