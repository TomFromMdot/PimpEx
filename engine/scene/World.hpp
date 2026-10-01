#pragma once

#include "scene/ComponentRegistry.hpp"
#include "scene/SceneData.hpp"

#include <entt/entt.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace PimpEx::scene {

// The live scene: actors in an EnTT registry, found by name. Fields are paths:
// "x", "vx", "Rect.color", "Health.hp", "score" (a variable). Main thread only.
class World {
  std::shared_ptr<const ComponentRegistry> _components;
  entt::registry _registry;
  std::map<std::string, entt::entity> _byName;
  std::map<std::string, ActorDef> _archetypes;
  std::map<std::string, int> _spawnCounters;
  std::optional<Rgba> _background;
  SceneMode _mode = SceneMode::Mode2D;

  entt::entity create(const ActorDef &def, const std::string &name, bool fromSceneFile,
                      const std::string &archetype);
  void apply_components(entt::entity e, const ActorDef &def);
  void strip_components(entt::entity e);

public:
  struct FieldRef {
    const ComponentDesc *component = nullptr; // null = a <Var> variable
    const FieldDesc *field = nullptr;
    std::string var;
  };

  World();
  explicit World(std::shared_ptr<const ComponentRegistry> components);

  void set_components(std::shared_ptr<const ComponentRegistry> components);
  const ComponentRegistry &components() const { return *_components; }

  void load(const SceneData &data);

  // Hot-reload of scene.xml. With `keepState` existing actors keep position,
  // velocity, variables and user component data; the rest (looks, size, new
  // components) comes from the file. Actors removed from the file disappear,
  // spawned ones stay.
  void reload(const SceneData &data, bool keepState);

  void clear();

  // Returns the new actor's name, or nullopt without such an archetype.
  std::optional<std::string> spawn(const std::string &archetype, const std::string &name,
                                   std::optional<std::pair<float, float>> position);

  bool destroy(const std::string &name);
  bool exists(const std::string &name) const { return _byName.contains(name); }
  entt::entity find(const std::string &name) const;
  std::string name_of(entt::entity e) const;

  // "x" / "vx" / "rotation" – Transform and Body shortcuts; "Comp.field";
  // any other single name is a variable. nullopt and `error` on failure.
  std::optional<FieldRef> resolve(std::string_view path, std::string *error = nullptr) const;

  std::optional<FieldValue> get_field(const std::string &actor, std::string_view path,
                                      std::string *error = nullptr) const;
  // Numeric value (bool = 0/1); nullopt for text or a missing field.
  std::optional<double> get(const std::string &actor, std::string_view path) const;

  // Returns an error message or nullopt. A missing component is added.
  std::optional<std::string> set_field(const std::string &actor, std::string_view path,
                                       const FieldValue &value);
  bool set(const std::string &actor, std::string_view path, double value) {
    return !set_field(actor, path, value).has_value();
  }

  bool has_component(entt::entity e, std::string_view component) const;
  std::optional<std::string> add_component(const std::string &actor, const std::string &component);
  bool remove_component(const std::string &actor, const std::string &component);
  std::vector<std::string> components_of(entt::entity e) const;

  bool has_tag(entt::entity e, std::string_view tag) const;
  // "Health" – has the component, "@enemy" – has the tag, "" – everyone.
  bool matches(entt::entity e, std::string_view selector) const;
  std::vector<std::string> query(std::string_view selector) const;

  bool touching(const std::string &a, const std::string &b) const;
  bool touching(entt::entity a, entt::entity b) const;

  // A <Tilemap> tile without movement or behaviors (e.g. a wall). The state
  // dump and the API only count these so hundreds of walls do not hide the rest.
  bool is_static_tile(entt::entity e) const;

  // First actor with a solid Collider within `maxDistance`. `ignore` is e.g. the shooter.
  struct RayHit {
    entt::entity entity = entt::null;
    float distance = 0;
  };
  std::optional<RayHit> raycast(float x, float y, float z, float dx, float dy, float dz, float maxDistance,
                                entt::entity ignore = entt::null) const;

  std::vector<std::string> names() const;
  std::size_t size() const { return _byName.size(); }
  bool has_archetype(const std::string &name) const { return _archetypes.contains(name); }
  const std::optional<Rgba> &background() const { return _background; }
  SceneMode mode() const { return _mode; }

  entt::registry &registry() { return _registry; }
  const entt::registry &registry() const { return _registry; }

  // Expands {Actor.field} templates; supplied by the VM.
  using TextFormatter = std::function<std::string(const std::string &text, const std::string &self)>;

  // Human- and agent-readable state dump.
  void dump(std::ostream &out, const TextFormatter &formatText = {}) const;
};

} // namespace PimpEx::scene
