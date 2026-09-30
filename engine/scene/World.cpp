#include "scene/World.hpp"
#include "core/Format.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <set>

namespace PimpEx::scene {
namespace {

// Skróty pól: "x" zamiast "Transform.x", "vx" zamiast "Body.vx".
struct Alias {
  std::string_view name, component, field;
};
constexpr Alias Aliases[] = {
    {"x", "Transform", "x"},         {"y", "Transform", "y"},       {"z", "Transform", "z"},
    {"w", "Transform", "w"},         {"h", "Transform", "h"},       {"d", "Transform", "d"},
    {"rx", "Transform", "rx"},       {"ry", "Transform", "ry"},     {"rz", "Transform", "rz"},
    {"rotation", "Transform", "rz"}, {"scale", "Transform", "scale"}, {"layer", "Transform", "layer"},
    {"visible", "Transform", "visible"}, {"space", "Transform", "space"},
    {"vx", "Body", "vx"},            {"vy", "Body", "vy"},          {"vz", "Body", "vz"},
    {"gravity", "Body", "gravity"},  {"grounded", "Body", "grounded"},
};

} // namespace

World::World() : _components(ComponentRegistry::with_builtins()) {}
World::World(std::shared_ptr<const ComponentRegistry> components)
    : _components(std::move(components)) {}

void World::set_components(std::shared_ptr<const ComponentRegistry> components) {
  _components = std::move(components);
}

void World::apply_components(entt::entity e, const ActorDef &def) {
  (void)_registry.get_or_emplace<Transform>(e);
  for (const auto &init : def.components) {
    const auto *desc = _components->find(init.component);
    if (!desc)
      continue; // loader już to zgłosił
    desc->add(_registry, e);
    for (const auto &[fieldName, value] : init.values)
      if (const auto *field = desc->field(fieldName))
        desc->set(_registry, e, *field, value);
  }
  _registry.emplace_or_replace<Vars>(e, def.vars);
  _registry.emplace_or_replace<Tags>(e, Tags{def.tags});
  if (!def.behaviors.empty())
    _registry.emplace_or_replace<Behaviors>(e, Behaviors{def.behaviors});
}

void World::strip_components(entt::entity e) {
  for (const auto &desc : _components->all())
    if (desc.has(_registry, e))
      desc.remove(_registry, e);
  _registry.remove<Vars, Tags, Behaviors, CustomData>(e);
}

entt::entity World::create(const ActorDef &def, const std::string &name, bool fromSceneFile,
                           const std::string &archetype) {
  const auto e = _registry.create();
  _registry.emplace<Name>(e, name);
  _registry.emplace<Origin>(e, fromSceneFile, archetype);
  apply_components(e, def);
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
  _mode = data.mode;
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
  _mode = data.mode;

  std::set<std::string> inFile;
  for (const auto &def : data.actors) {
    inFile.insert(def.name);
    const auto it = _byName.find(def.name);
    if (it == _byName.end()) {
      create(def, def.name, true, "");
      continue;
    }
    const auto e = it->second;

    // Stan, który zostaje: pozycja, prędkość, zmienne, dane komponentów użytkownika.
    const auto oldTransform = _registry.get<Transform>(e);
    const auto oldBody = _registry.try_get<Body>(e) ? std::optional(_registry.get<Body>(e)) : std::nullopt;
    const auto oldVars = _registry.get<Vars>(e).values;
    const auto oldCustom = _registry.try_get<CustomData>(e) ? _registry.get<CustomData>(e).components
                                                             : decltype(CustomData::components){};

    strip_components(e);
    apply_components(e, def);

    auto &t = _registry.get<Transform>(e);
    t.x = oldTransform.x;
    t.y = oldTransform.y;
    t.z = oldTransform.z;
    if (auto *body = _registry.try_get<Body>(e); body && oldBody) {
      body->vx = oldBody->vx;
      body->vy = oldBody->vy;
      body->vz = oldBody->vz;
    }
    auto &vars = _registry.get<Vars>(e).values;
    for (const auto &[key, value] : oldVars)
      vars[key] = value;
    if (auto *custom = _registry.try_get<CustomData>(e))
      for (auto &[component, fields] : custom->components)
        if (const auto old = oldCustom.find(component); old != oldCustom.end())
          for (const auto &[field, value] : old->second)
            if (fields.contains(field))
              fields[field] = value;
    _registry.get<Origin>(e).fromSceneFile = true;
  }

  std::vector<std::string> removed;
  for (const auto &[name, e] : _byName)
    if (_registry.get<Origin>(e).fromSceneFile && !inFile.contains(name))
      removed.push_back(name);
  for (const auto &name : removed)
    destroy(name);
}

std::optional<std::string> World::spawn(const std::string &archetype, const std::string &name,
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

std::string World::name_of(entt::entity e) const {
  const auto *name = _registry.try_get<Name>(e);
  return name ? name->value : std::string();
}

// --- pola --------------------------------------------------------------------------------

std::optional<World::FieldRef> World::resolve(std::string_view path, std::string *error) const {
  const auto dot = path.find('.');
  std::string_view componentName, fieldName;
  if (dot == std::string_view::npos) {
    for (const auto &alias : Aliases)
      if (alias.name == path) {
        componentName = alias.component;
        fieldName = alias.field;
      }
    if (componentName.empty())
      return FieldRef{nullptr, nullptr, std::string(path)}; // zmienna
  } else {
    componentName = path.substr(0, dot);
    fieldName = path.substr(dot + 1);
  }

  const auto *component = _components->find(componentName);
  if (!component) {
    if (error) {
      const auto hint = closest_name(componentName, _components->names());
      *error = std::format("nie ma komponentu '{}'{}", componentName,
                           hint.empty() ? "" : std::format(" (czy chodziło o {}?)", hint));
    }
    return std::nullopt;
  }
  const auto *field = component->field(fieldName);
  if (!field) {
    if (error) {
      std::vector<std::string> names;
      for (const auto &f : component->fields)
        names.push_back(f.name);
      const auto hint = closest_name(fieldName, names);
      *error = std::format("komponent {} nie ma pola '{}'{}", componentName, fieldName,
                           hint.empty() ? "" : std::format(" (czy chodziło o {}?)", hint));
    }
    return std::nullopt;
  }
  return FieldRef{component, field, {}};
}

std::optional<FieldValue> World::get_field(const std::string &actor, std::string_view path,
                                           std::string *error) const {
  const auto e = find(actor);
  if (e == entt::null) {
    if (error)
      *error = std::format("aktor '{}' nie istnieje", actor);
    return std::nullopt;
  }
  const auto ref = resolve(path, error);
  if (!ref)
    return std::nullopt;
  if (!ref->component) {
    const auto &vars = _registry.get<Vars>(e).values;
    const auto it = vars.find(ref->var);
    if (it == vars.end())
      return std::nullopt; // zmienna jeszcze nieustawiona
    return it->second;
  }
  if (!ref->component->has(_registry, e)) {
    // Brak Body przy "vx" itp. – wartość domyślna, jak dla świeżego komponentu.
    if (path.find('.') == std::string_view::npos)
      return ref->field->defaultValue;
    if (error)
      *error = std::format("aktor '{}' nie ma komponentu {}", actor, ref->component->name);
    return std::nullopt;
  }
  return ref->component->get(_registry, e, *ref->field);
}

std::optional<double> World::get(const std::string &actor, std::string_view path) const {
  const auto value = get_field(actor, path);
  if (!value)
    return std::nullopt;
  if (const auto *number = std::get_if<double>(&*value))
    return *number;
  return std::nullopt;
}

std::optional<std::string> World::set_field(const std::string &actor, std::string_view path,
                                            const FieldValue &value) {
  const auto e = find(actor);
  if (e == entt::null)
    return std::format("aktor '{}' nie istnieje", actor);
  std::string error;
  const auto ref = resolve(path, &error);
  if (!ref)
    return error;
  if (!ref->component) {
    const auto *number = std::get_if<double>(&value);
    if (!number)
      return std::format("zmienna '{}' może przechowywać tylko liczby", ref->var);
    _registry.get<Vars>(e).values[ref->var] = *number;
    return std::nullopt;
  }
  if (ref->field->readOnly)
    return std::format("pole {}.{} jest tylko do odczytu", ref->component->name, ref->field->name);
  const auto coerced = coerce_field(*ref->field, value, error);
  if (!coerced)
    return error;
  if (!ref->component->has(_registry, e))
    ref->component->add(_registry, e);
  ref->component->set(_registry, e, *ref->field, *coerced);
  return std::nullopt;
}

// --- komponenty i zapytania ------------------------------------------------------------------

bool World::has_component(entt::entity e, std::string_view component) const {
  const auto *desc = _components->find(component);
  return desc && _registry.valid(e) && desc->has(_registry, e);
}

std::optional<std::string> World::add_component(const std::string &actor, const std::string &component) {
  const auto e = find(actor);
  if (e == entt::null)
    return std::format("aktor '{}' nie istnieje", actor);
  const auto *desc = _components->find(component);
  if (!desc)
    return std::format("nie ma komponentu '{}'", component);
  desc->add(_registry, e);
  return std::nullopt;
}

bool World::remove_component(const std::string &actor, const std::string &component) {
  const auto e = find(actor);
  const auto *desc = _components->find(component);
  if (e == entt::null || !desc || component == "Transform")
    return false;
  desc->remove(_registry, e);
  return true;
}

std::vector<std::string> World::components_of(entt::entity e) const {
  std::vector<std::string> out;
  for (const auto &desc : _components->all())
    if (desc.has(_registry, e))
      out.push_back(desc.name);
  return out;
}

bool World::has_tag(entt::entity e, std::string_view tag) const {
  const auto *tags = _registry.try_get<Tags>(e);
  return tags && std::find(tags->values.begin(), tags->values.end(), tag) != tags->values.end();
}

bool World::matches(entt::entity e, std::string_view selector) const {
  if (selector.empty())
    return true;
  if (selector.front() == '@')
    return has_tag(e, selector.substr(1));
  return has_component(e, selector);
}

std::vector<std::string> World::query(std::string_view selector) const {
  std::vector<std::string> out;
  for (const auto &[name, e] : _byName)
    if (matches(e, selector))
      out.push_back(name);
  return out;
}

bool World::touching(entt::entity a, entt::entity b) const {
  if (a == entt::null || b == entt::null || a == b)
    return false;
  const auto &ta = _registry.get<Transform>(a);
  const auto &tb = _registry.get<Transform>(b);
  return std::abs(ta.x - tb.x) * 2 < (ta.w + tb.w) && std::abs(ta.y - tb.y) * 2 < (ta.h + tb.h) &&
         std::abs(ta.z - tb.z) * 2 < (ta.d + tb.d);
}

bool World::touching(const std::string &a, const std::string &b) const {
  return touching(find(a), find(b));
}

std::vector<std::string> World::names() const {
  std::vector<std::string> out;
  out.reserve(_byName.size());
  for (const auto &[name, e] : _byName)
    out.push_back(name);
  return out;
}

void World::dump(std::ostream &out, const TextFormatter &formatText) const {
  const bool is3d = _mode == SceneMode::Mode3D;
  for (const auto &[name, e] : _byName) {
    const auto &t = _registry.get<Transform>(e);
    out << name << " x=" << format_number(t.x) << " y=" << format_number(t.y);
    if (is3d || t.z != 0)
      out << " z=" << format_number(t.z);
    out << " w=" << format_number(t.w) << " h=" << format_number(t.h);
    if (is3d)
      out << " d=" << format_number(t.d);

    // Pozostałe komponenty: tylko pola różne od domyślnych.
    for (const auto &desc : _components->all()) {
      if (!desc.has(_registry, e))
        continue;
      std::string fields;
      for (const auto &field : desc.fields) {
        if (desc.name == "Transform" && (field.name == "x" || field.name == "y" || field.name == "z" ||
                                         field.name == "w" || field.name == "h" || field.name == "d"))
          continue;
        auto value = desc.get(_registry, e, field);
        if (desc.builtin && value == field.defaultValue && field.name != "grounded")
          continue;
        auto text = field_value_to_string(value);
        if (desc.name == "Text" && field.name == "value" && formatText)
          text = formatText(text, name);
        if (field.type == FieldType::Text)
          text = "\"" + text + "\"";
        fields += (fields.empty() ? "" : " ") + field.name + "=" + text;
      }
      if (desc.name == "Transform") {
        if (!fields.empty())
          out << ' ' << fields;
        continue;
      }
      out << ' ' << desc.name;
      if (!fields.empty())
        out << '(' << fields << ')';
    }

    for (const auto &[key, value] : _registry.get<Vars>(e).values)
      out << ' ' << key << '=' << format_number(value);
    if (const auto *tags = _registry.try_get<Tags>(e); tags && !tags->values.empty()) {
      out << " tags=";
      for (std::size_t i = 0; i < tags->values.size(); ++i)
        out << (i ? "," : "") << tags->values[i];
    }
    if (const auto *behaviors = _registry.try_get<Behaviors>(e))
      for (const auto &b : behaviors->list)
        out << " behavior=" << b.name;
    const auto &origin = _registry.get<Origin>(e);
    if (!origin.archetype.empty())
      out << " archetype=" << origin.archetype;
    out << '\n';
  }
}

} // namespace PimpEx::scene
