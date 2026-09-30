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

// „Żywa” scena: aktorzy (GameObjecty) w rejestrze EnTT, dostępni po nazwie.
// Pola czyta się i zapisuje ścieżkami: "x", "vx", "Rect.color", "Health.hp",
// "score" (zmienna). Tylko wątek główny dotyka World.
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
  // Pole znalezione po ścieżce.
  struct FieldRef {
    const ComponentDesc *component = nullptr; // null = zmienna z <Var>
    const FieldDesc *field = nullptr;
    std::string var;
  };

  World();
  explicit World(std::shared_ptr<const ComponentRegistry> components);

  // Rejestr komponentów (wbudowane + components.xml).
  void set_components(std::shared_ptr<const ComponentRegistry> components);
  const ComponentRegistry &components() const { return *_components; }

  // Buduje świat od zera na podstawie scene.xml.
  void load(const SceneData &data);

  // Hot-reload scene.xml. Z `keepState` aktorzy, którzy już istnieją,
  // zachowują pozycję, prędkość, zmienne i dane komponentów użytkownika;
  // resztę (wygląd, rozmiar, nowe komponenty) biorą z pliku.
  // Aktorzy usunięci z pliku znikają, a stworzeni przez SPAWN zostają.
  void reload(const SceneData &data, bool keepState);

  void clear();

  // SPAWN: tworzy aktora z archetypu. Zwraca nazwę nowego aktora
  // albo nullopt, jeśli archetypu nie ma.
  std::optional<std::string> spawn(const std::string &archetype, const std::string &name,
                                   std::optional<std::pair<float, float>> position);

  bool destroy(const std::string &name);
  bool exists(const std::string &name) const { return _byName.contains(name); }
  entt::entity find(const std::string &name) const;
  std::string name_of(entt::entity e) const;

  // --- pola -------------------------------------------------------------------------
  // "x" / "vx" / "rotation" – skróty do Transform i Body; "Comp.pole";
  // inna pojedyncza nazwa – zmienna. Zwraca nullopt i opis w `error`.
  std::optional<FieldRef> resolve(std::string_view path, std::string *error = nullptr) const;

  std::optional<FieldValue> get_field(const std::string &actor, std::string_view path,
                                      std::string *error = nullptr) const;
  // Wartość liczbowa (bool = 0/1). Tekst albo brak pola = nullopt.
  std::optional<double> get(const std::string &actor, std::string_view path) const;

  // Zwraca opis błędu albo nullopt. Brakujący komponent zostaje dodany.
  std::optional<std::string> set_field(const std::string &actor, std::string_view path,
                                       const FieldValue &value);
  bool set(const std::string &actor, std::string_view path, double value) {
    return !set_field(actor, path, value).has_value();
  }

  // --- komponenty i zapytania ---------------------------------------------------------
  bool has_component(entt::entity e, std::string_view component) const;
  std::optional<std::string> add_component(const std::string &actor, const std::string &component);
  bool remove_component(const std::string &actor, const std::string &component);
  std::vector<std::string> components_of(entt::entity e) const;

  bool has_tag(entt::entity e, std::string_view tag) const;
  // "Health" – aktorzy z komponentem, "@enemy" – z tagiem, "" – wszyscy.
  bool matches(entt::entity e, std::string_view selector) const;
  std::vector<std::string> query(std::string_view selector) const;

  // Czy prostokąty (prostopadłościany) dwóch aktorów na siebie nachodzą.
  bool touching(const std::string &a, const std::string &b) const;
  bool touching(entt::entity a, entt::entity b) const;

  // Kafel z <Tilemap> bez ruchu i zachowań (np. ściana). W zrzucie stanu
  // i w API jest tylko liczony, żeby setki ścian nie zasłaniały reszty.
  bool is_static_tile(entt::entity e) const;

  // Promień od (x, y, z) w kierunku (dx, dy, dz): pierwszy aktor z solidnym
  // Colliderem w odległości do `maxDistance`. `ignore` – np. sam strzelający.
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

  // Zamienia szablon napisu ({Aktor.pole}) na tekst – podaje go VM.
  using TextFormatter = std::function<std::string(const std::string &text, const std::string &self)>;

  // Tekstowy zrzut stanu – czytelny dla człowieka i agenta AI.
  void dump(std::ostream &out, const TextFormatter &formatText = {}) const;
};

} // namespace PimpEx::scene
