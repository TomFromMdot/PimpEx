#include "gui/Gui.hpp"

#include <algorithm>
#include <array>

namespace PimpEx::gui {
namespace {

constexpr std::array<std::pair<std::string_view, Anchor>, 9> AnchorNames = {{
    {"top-left", Anchor::TopLeft},
    {"top", Anchor::Top},
    {"top-right", Anchor::TopRight},
    {"left", Anchor::Left},
    {"center", Anchor::Center},
    {"right", Anchor::Right},
    {"bottom-left", Anchor::BottomLeft},
    {"bottom", Anchor::Bottom},
    {"bottom-right", Anchor::BottomRight},
}};

// Buttons and panels hide what is below them; labels and images do not.
bool blocks_mouse(ElementKind kind) { return kind == ElementKind::Button || kind == ElementKind::Panel; }

} // namespace

std::optional<Anchor> anchor_from_name(std::string_view name) {
  for (const auto &[text, anchor] : AnchorNames)
    if (text == name)
      return anchor;
  return std::nullopt;
}

bool GuiScene::has_buttons() const {
  return std::any_of(elements.begin(), elements.end(),
                     [](const Element &e) { return e.kind == ElementKind::Button; });
}

Rect layout(const Element &element, int screenWidth, int screenHeight) {
  const auto w = static_cast<float>(screenWidth), h = static_cast<float>(screenHeight);
  float ax = 0, ay = 0;
  switch (element.anchor) {
  case Anchor::TopLeft: break;
  case Anchor::Top: ax = w / 2; break;
  case Anchor::TopRight: ax = w; break;
  case Anchor::Left: ay = h / 2; break;
  case Anchor::Center: ax = w / 2; ay = h / 2; break;
  case Anchor::Right: ax = w; ay = h / 2; break;
  case Anchor::BottomLeft: ay = h; break;
  case Anchor::Bottom: ax = w / 2; ay = h; break;
  case Anchor::BottomRight: ax = w; ay = h; break;
  }
  return {ax + element.x - element.w / 2, ay + element.y - element.h / 2, element.w, element.h};
}

void GuiSystem::set(GuiScene scene) {
  // Element indices may have changed – reset mouse state for this scene.
  if (_hovered && _hovered->scene == scene.name)
    _hovered.reset();
  if (_pressed && _pressed->scene == scene.name)
    _pressed.reset();
  auto name = scene.name;
  _scenes.insert_or_assign(std::move(name), std::move(scene));
}

void GuiSystem::clear() {
  _scenes.clear();
  _shown.clear();
  _hovered.reset();
  _pressed.reset();
}

const GuiScene *GuiSystem::find(const std::string &name) const {
  const auto it = _scenes.find(name);
  return it == _scenes.end() ? nullptr : &it->second;
}

std::vector<std::string> GuiSystem::names() const {
  std::vector<std::string> out;
  for (const auto &[name, scene] : _scenes)
    out.push_back(name);
  return out;
}

bool GuiSystem::show(const std::string &name) {
  if (!_scenes.contains(name))
    return false;
  std::erase(_shown, name);
  _shown.push_back(name);
  return true;
}

void GuiSystem::hide(const std::string &name) {
  std::erase(_shown, name);
  if (_hovered && _hovered->scene == name)
    _hovered.reset();
  if (_pressed && _pressed->scene == name)
    _pressed.reset();
}

std::optional<bool> GuiSystem::toggle(const std::string &name) {
  if (!_scenes.contains(name))
    return std::nullopt;
  if (is_shown(name)) {
    hide(name);
    return false;
  }
  show(name);
  return true;
}

void GuiSystem::hide_all() {
  _shown.clear();
  _hovered.reset();
  _pressed.reset();
}

bool GuiSystem::is_shown(const std::string &name) const {
  return std::find(_shown.begin(), _shown.end(), name) != _shown.end();
}

bool GuiSystem::pauses_game() const {
  return std::any_of(_shown.begin(), _shown.end(), [&](const std::string &name) {
    const auto *scene = find(name);
    return scene && scene->pause;
  });
}

bool GuiSystem::wants_mouse() const {
  return std::any_of(_shown.begin(), _shown.end(), [&](const std::string &name) {
    const auto *scene = find(name);
    return scene && scene->has_buttons();
  });
}

std::optional<GuiSystem::Hit> GuiSystem::hit_test(float x, float y, int screenWidth, int screenHeight) const {
  // Top first: last shown scene, last drawn element.
  for (auto it = _shown.rbegin(); it != _shown.rend(); ++it) {
    const auto *scene = find(*it);
    if (!scene)
      continue;
    for (std::size_t i = scene->elements.size(); i-- > 0;) {
      const auto &element = scene->elements[i];
      if (!blocks_mouse(element.kind) || !layout(element, screenWidth, screenHeight).contains(x, y))
        continue;
      if (element.kind == ElementKind::Button)
        return Hit{scene->name, i};
      return std::nullopt;
    }
    if (scene->pause)
      return std::nullopt; // a pause GUI covers everything below
  }
  return std::nullopt;
}

std::vector<GuiSystem::Click> GuiSystem::update(const vm::IInput &input, int screenWidth, int screenHeight) {
  std::vector<Click> clicks;
  auto clickOf = [&](const Hit &hit) {
    const auto &scene = _scenes.at(hit.scene);
    clicks.push_back({scene.name, scene.file, scene.elements[hit.index].click});
  };

  _hovered = hit_test(input.mouse_x(), input.mouse_y(), screenWidth, screenHeight);
  const bool down = input.mouse_down(vm::MouseButton::Left);
  const bool pressed = input.mouse_pressed(vm::MouseButton::Left) || (down && !_wasDown);
  if (pressed)
    _pressed = _hovered;
  const bool released = (_wasDown || pressed) && !down;
  if (released) {
    if (_pressed && _pressed == _hovered)
      clickOf(*_pressed);
    _pressed.reset();
  }
  _wasDown = down;

  // Button keys (key="ESCAPE"): the topmost match wins.
  for (std::size_t k = 0; k < vm::KeyCount; ++k) {
    const auto key = static_cast<vm::Key>(k);
    if (!input.pressed(key))
      continue;
    bool done = false;
    for (auto it = _shown.rbegin(); it != _shown.rend() && !done; ++it) {
      const auto &scene = _scenes.at(*it);
      for (std::size_t i = scene.elements.size(); i-- > 0 && !done;)
        if (scene.elements[i].kind == ElementKind::Button && scene.elements[i].key == key) {
          clickOf({scene.name, i});
          done = true;
        }
    }
  }
  return clicks;
}

bool GuiSystem::is_hovered(const std::string &scene, std::size_t index) const {
  return _hovered && _hovered->scene == scene && _hovered->index == index;
}

bool GuiSystem::is_pressed(const std::string &scene, std::size_t index) const {
  return _pressed && _pressed->scene == scene && _pressed->index == index && _hovered == _pressed;
}

} // namespace PimpEx::gui
