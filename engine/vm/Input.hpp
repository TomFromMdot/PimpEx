#pragma once

#include "vm/Keys.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace PimpEx::vm {

// Keyboard and mouse state for the current frame. Keeps the VM free of raylib.
class IInput {
public:
  virtual ~IInput() = default;
  virtual bool down(Key key) const = 0;    // held
  virtual bool pressed(Key key) const = 0; // went down this frame
  virtual float mouse_x() const = 0;
  virtual float mouse_y() const = 0;
  virtual float mouse_dx() const { return 0; }
  virtual float mouse_dy() const { return 0; }
  virtual bool mouse_down(MouseButton) const { return false; }
  virtual bool mouse_pressed(MouseButton) const { return false; }
};

class ManualInput : public IInput {
  std::array<bool, KeyCount> _down{};
  std::array<bool, KeyCount> _pressed{};
  std::array<bool, MouseButtonCount> _mouseDown{};
  std::array<bool, MouseButtonCount> _mousePressed{};
  float _mouseX = 0, _mouseY = 0;

public:
  void set_down(Key key, bool value) { _down[static_cast<std::size_t>(key)] = value; }
  void set_pressed(Key key, bool value) { _pressed[static_cast<std::size_t>(key)] = value; }
  void set_mouse(float x, float y) {
    _mouseX = x;
    _mouseY = y;
  }
  void set_mouse_down(MouseButton button, bool value) { _mouseDown[static_cast<std::size_t>(button)] = value; }
  void set_mouse_pressed(MouseButton button, bool value) {
    _mousePressed[static_cast<std::size_t>(button)] = value;
  }

  bool down(Key key) const override { return _down[static_cast<std::size_t>(key)]; }
  bool pressed(Key key) const override { return _pressed[static_cast<std::size_t>(key)]; }
  float mouse_x() const override { return _mouseX; }
  float mouse_y() const override { return _mouseY; }
  bool mouse_down(MouseButton button) const override { return _mouseDown[static_cast<std::size_t>(button)]; }
  bool mouse_pressed(MouseButton button) const override {
    return _mousePressed[static_cast<std::size_t>(button)];
  }
};

// Keys pressed from outside (API input.press) layered over the real input.
class OverlayInput : public IInput {
  const IInput *_base = nullptr;
  std::array<long, KeyCount> _downUntil{};
  std::array<long, KeyCount> _pressedAt{};
  long _frame = 0;

public:
  OverlayInput() {
    _downUntil.fill(-1);
    _pressedAt.fill(-1);
  }
  void set_base(const IInput *base) { _base = base; }
  void set_frame(long frame) { _frame = frame; }
  // Holds the key for `frames` frames, starting with the next one.
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
  bool mouse_down(MouseButton button) const override { return _base && _base->mouse_down(button); }
  bool mouse_pressed(MouseButton button) const override { return _base && _base->mouse_pressed(button); }
};

// Scripted input for --headless runs, e.g. "RIGHT:0-60 SPACE:10 CLICK@400/300:20".
// KEY:from-to holds a key, KEY:n taps it for one frame, CLICK@x/y:n moves the
// cursor to (x, y) and holds the left mouse button.
class ScriptedInput : public ManualInput {
  struct Press {
    Key key;
    long from, to;
  };
  struct Click {
    float x, y;
    long from, to;
  };
  std::vector<Press> _presses;
  std::vector<Click> _clicks;

public:
  // Returns an error message, or an empty string.
  std::string parse(std::string_view spec);
  void update(long frame);
};

} // namespace PimpEx::vm
