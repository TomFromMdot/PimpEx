#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace PimpEx::vm {

// Klawisze dostępne w opcodach (KEY / PRESSED) i w Lua.
// Silnik nie zależy tu od raylib – mapowanie jest w render/RaylibInput.
enum class Key {
  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
  Space, Enter, Escape, Shift, Left, Right, Up, Down,
  Count
};

constexpr std::size_t KeyCount = static_cast<std::size_t>(Key::Count);

inline constexpr std::array<std::string_view, KeyCount> KeyNames = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "SPACE", "ENTER", "ESCAPE", "SHIFT", "LEFT", "RIGHT", "UP", "DOWN"};

inline std::string_view key_name(Key key) {
  return KeyNames[static_cast<std::size_t>(key)];
}

// Przyciski myszki: `Mouse.left` w .swap, kliknięcia w GUI.
enum class MouseButton { Left, Right, Middle, Count };

constexpr std::size_t MouseButtonCount = static_cast<std::size_t>(MouseButton::Count);

// Nazwa klawisza bez względu na wielkość liter: "w", "W", "space"...
inline std::optional<Key> key_from_name(std::string_view name) {
  std::string upper(name);
  for (auto &c : upper)
    if (c >= 'a' && c <= 'z')
      c = static_cast<char>(c - 'a' + 'A');
  for (std::size_t i = 0; i < KeyCount; ++i)
    if (KeyNames[i] == upper)
      return static_cast<Key>(i);
  return std::nullopt;
}

} // namespace PimpEx::vm
