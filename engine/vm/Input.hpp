#pragma once

#include "vm/Keys.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace PimpEx::vm {

// Stan klawiatury i myszki w bieżącej klatce.
// VM nie wie nic o raylib – dostaje tylko ten interfejs.
class IInput {
public:
  virtual ~IInput() = default;
  virtual bool down(Key key) const = 0;    // klawisz trzymany
  virtual bool pressed(Key key) const = 0; // wciśnięty w tej klatce
  virtual float mouse_x() const = 0;
  virtual float mouse_y() const = 0;
  // Ruch myszy od poprzedniej klatki (kamera fps).
  virtual float mouse_dx() const { return 0; }
  virtual float mouse_dy() const { return 0; }
};

// Input sterowany ręcznie – dla testów i trybu bez okna (--headless).
class ManualInput : public IInput {
  std::array<bool, KeyCount> _down{};
  std::array<bool, KeyCount> _pressed{};
  float _mouseX = 0, _mouseY = 0;

public:
  void set_down(Key key, bool value) { _down[static_cast<std::size_t>(key)] = value; }
  void set_pressed(Key key, bool value) { _pressed[static_cast<std::size_t>(key)] = value; }
  void set_mouse(float x, float y) {
    _mouseX = x;
    _mouseY = y;
  }

  bool down(Key key) const override { return _down[static_cast<std::size_t>(key)]; }
  bool pressed(Key key) const override { return _pressed[static_cast<std::size_t>(key)]; }
  float mouse_x() const override { return _mouseX; }
  float mouse_y() const override { return _mouseY; }
};

// Klawisze wciśnięte „z zewnątrz” (API input.press) nałożone na prawdziwy input.
class OverlayInput : public IInput {
  const IInput *_base = nullptr;
  std::array<long, KeyCount> _downUntil{}; // klatka, do której klawisz jest trzymany
  std::array<long, KeyCount> _pressedAt{};
  long _frame = 0;

public:
  OverlayInput() {
    _downUntil.fill(-1);
    _pressedAt.fill(-1);
  }
  void set_base(const IInput *base) { _base = base; }
  void set_frame(long frame) { _frame = frame; }
  // Trzyma klawisz przez `frames` klatek, zaczynając od następnej.
  void press(Key key, long frames) {
    const auto i = static_cast<std::size_t>(key);
    _pressedAt[i] = _frame;
    _downUntil[i] = _frame + std::max(1L, frames) - 1;
  }
  bool down(Key key) const override {
    const auto i = static_cast<std::size_t>(key);
    return (_base && _base->down(key)) || _frame <= _downUntil[i];
  }
  bool pressed(Key key) const override {
    const auto i = static_cast<std::size_t>(key);
    return (_base && _base->pressed(key)) || _frame == _pressedAt[i];
  }
  float mouse_x() const override { return _base ? _base->mouse_x() : 0; }
  float mouse_y() const override { return _base ? _base->mouse_y() : 0; }
  float mouse_dx() const override { return _base ? _base->mouse_dx() : 0; }
  float mouse_dy() const override { return _base ? _base->mouse_dy() : 0; }
};

// Zaplanowane wciśnięcia klawiszy dla trybu --headless, np. "RIGHT:0-60 SPACE:10".
// KLAWISZ:od-do trzyma klawisz w klatkach od..do, KLAWISZ:n wciska go na jedną klatkę.
class ScriptedInput : public ManualInput {
  struct Press {
    Key key;
    long from, to;
  };
  std::vector<Press> _presses;

public:
  // Zwraca opis błędu albo pusty tekst.
  std::string parse(std::string_view spec);
  void update(long frame);
};

} // namespace PimpEx::vm
