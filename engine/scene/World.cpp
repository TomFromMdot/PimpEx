#include "scene/World.hpp"

#include <cmath>
#include <format>
#include <set>

namespace PimpEx::scene {
namespace {

std::string format_number(double v) {
  if (v == static_cast<long long>(v))
    return std::format("{}", static_cast<long long>(v));
  return std::format("{:.2f}", v);
}

} // namespace

entt::entity World::create(const ActorDef &def, const std::string &name,
                           bool fromSceneFile, const std::string &archetype) {
  const auto e = _registry.create();
  _registry.emplace<Name>(e, name);
  _registry.emplace<Transform>(e, def.transform);
  _registry.emplace<Velocity>(e, def.velocity);
  _registry.emplace<Vars>(e, def.vars);
  _registry.emplace<Origin>(e, fromSceneFile, archetype);
  if (def.shape)
    _registry.emplace<Shape>(e, *def.shape);
  if (def.sprite)
    _registry.emplace<Sprite>(e, *def.sprite);
  if (def.label)
    _registry.emplace<Label>(e, *def.label);
  _byName[name] = e;
  return e;
}

void World::clear() {
  _registry.clear();
  _byName.clear();
  _archetypes.clear();
  _spawnCounters.clear();
  _background.reset();
}

void World::load(const SceneData &data) {
  clear();
  _archetypes = data.archetypes;
  _background = data.background;
  for (const auto &def : data.actors)
    create(def, def.name, true, "");
}

void World::reload(const SceneData &data, bool keepState) {
  if (!keepState) {
    load(data);
    return;
  }
  _archetypes = data.archetypes;
  _background = data.background;

  std::set<std::string> inFile;
  for (const auto &def : data.actors) {
    inFile.insert(def.name);
    const auto it = _byName.find(def.name);
    if (it == _byName.end()) {
      create(def, def.name, true, "");
      continue;
    }
    const auto e = it->second;
    auto &t = _registry.get<Transform>(e);
    t.w = def.transform.w;
    t.h = def.transform.h;
    t.rotation = def.transform.rotation;
    t.z = def.transform.z;
    t.visible = def.transform.visible;
    auto &vars = _registry.get<Vars>(e).values;
    for (const auto &[key, value] : def.vars.values)
      vars.try_emplace(key, value);
    _registry.remove<Shape, Sprite, Label>(e);
    if (def.shape)
      _registry.emplace<Shape>(e, *def.shape);
    if (def.sprite)
      _registry.emplace<Sprite>(e, *def.sprite);
    if (def.label)
      _registry.emplace<Label>(e, *def.label);
    _registry.get<Origin>(e).fromSceneFile = true;
  }

  // Aktorzy usunięci z pliku znikają. Stworzeni przez SPAWN zostają.
  std::vector<std::string> removed;
  for (const auto &[name, e] : _byName)
    if (_registry.get<Origin>(e).fromSceneFile && !inFile.contains(name))
      removed.push_back(name);
  for (const auto &name : removed)
    destroy(name);
}

std::optional<std::string> World::spawn(const std::string &archetype,
                                        const std::string &name,
                                        std::optional<std::pair<float, float>> position) {
  const auto it = _archetypes.find(archetype);
  if (it == _archetypes.end())
    return std::nullopt;

  std::string finalName = name;
  if (finalName.empty() || exists(finalName)) {
    const std::string base = finalName.empty() ? archetype : finalName;
    do {
      finalName = std::format("{}_{}", base, ++_spawnCounters[base]);
    } while (exists(finalName));
  }

  const auto e = create(it->second, finalName, false, archetype);
  if (position) {
    auto &t = _registry.get<Transform>(e);
    t.x = position->first;
    t.y = position->second;
  }
  return finalName;
}

bool World::destroy(const std::string &name) {
  const auto it = _byName.find(name);
  if (it == _byName.end())
    return false;
  _registry.destroy(it->second);
  _byName.erase(it);
  return true;
}

entt::entity World::find(const std::string &name) const {
  const auto it = _byName.find(name);
  return it == _byName.end() ? entt::null : it->second;
}

std::optional<double> World::get(const std::string &actor, const std::string &field) const {
  const auto e = find(actor);
  if (e == entt::null)
    return std::nullopt;
  const auto &t = _registry.get<Transform>(e);
  const auto &v = _registry.get<Velocity>(e);
  if (field == "x") return t.x;
  if (field == "y") return t.y;
  if (field == "w") return t.w;
  if (field == "h") return t.h;
  if (field == "rotation") return t.rotation;
  if (field == "z") return t.z;
  if (field == "visible") return t.visible ? 1.0 : 0.0;
  if (field == "vx") return v.vx;
  if (field == "vy") return v.vy;
  const auto &vars = _registry.get<Vars>(e).values;
  const auto it = vars.find(field);
  if (it == vars.end())
    return std::nullopt;
  return it->second;
}

bool World::set(const std::string &actor, const std::string &field, double value) {
  const auto e = find(actor);
  if (e == entt::null)
    return false;
  auto &t = _registry.get<Transform>(e);
  auto &v = _registry.get<Velocity>(e);
  const auto f = static_cast<float>(value);
  if (field == "x") t.x = f;
  else if (field == "y") t.y = f;
  else if (field == "w") t.w = f;
  else if (field == "h") t.h = f;
  else if (field == "rotation") t.rotation = f;
  else if (field == "z") t.z = static_cast<int>(value);
  else if (field == "visible") t.visible = value != 0.0;
  else if (field == "vx") v.vx = f;
  else if (field == "vy") v.vy = f;
  else _registry.get<Vars>(e).values[field] = value;
  return true;
}

bool World::touching(const std::string &a, const std::string &b) const {
  const auto ea = find(a), eb = find(b);
  if (ea == entt::null || eb == entt::null || ea == eb)
    return false;
  const auto &ta = _registry.get<Transform>(ea);
  const auto &tb = _registry.get<Transform>(eb);
  return std::abs(ta.x - tb.x) * 2 < (ta.w + tb.w) &&
         std::abs(ta.y - tb.y) * 2 < (ta.h + tb.h);
}

void World::integrate() {
  for (auto [e, t, v] : _registry.view<Transform, Velocity>().each()) {
    t.x += v.vx;
    t.y += v.vy;
  }
}

std::vector<std::string> World::names() const {
  std::vector<std::string> out;
  out.reserve(_byName.size());
  for (const auto &[name, e] : _byName)
    out.push_back(name);
  return out;
}

void World::dump(std::ostream &out) const {
  for (const auto &[name, e] : _byName) {
    const auto &t = _registry.get<Transform>(e);
    const auto &v = _registry.get<Velocity>(e);
    out << std::format("{} x={} y={} w={} h={}", name, format_number(t.x),
                       format_number(t.y), format_number(t.w), format_number(t.h));
    if (v.vx != 0 || v.vy != 0)
      out << std::format(" vx={} vy={}", format_number(v.vx), format_number(v.vy));
    if (t.rotation != 0)
      out << " rotation=" << format_number(t.rotation);
    if (!t.visible)
      out << " visible=0";
    for (const auto &[key, value] : _registry.get<Vars>(e).values)
      out << ' ' << key << '=' << format_number(value);
    if (const auto *label = _registry.try_get<Label>(e))
      out << " text=\"" << label->text << '"';
    const auto &origin = _registry.get<Origin>(e);
    if (!origin.archetype.empty())
      out << " archetype=" << origin.archetype;
    out << '\n';
  }
}

} // namespace PimpEx::scene
