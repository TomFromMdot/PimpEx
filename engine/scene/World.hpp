#pragma once

#include "scene/SceneData.hpp"

#include <entt/entt.hpp>

#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace PimpEx::scene {

// „Żywa” scena: aktorzy w rejestrze EnTT, dostępni po nazwie.
// Tylko wątek główny dotyka World.
class World {
  entt::registry _registry;
  std::map<std::string, entt::entity> _byName;
  std::map<std::string, ActorDef> _archetypes;
  std::map<std::string, int> _spawnCounters;
  std::optional<Rgba> _background;

  entt::entity create(const ActorDef &def, const std::string &name, bool fromSceneFile,
                      const std::string &archetype);

public:
  // Buduje świat od zera na podstawie scene.xml.
  void load(const SceneData &data);

  // Hot-reload scene.xml. Z `keepState` aktorzy, którzy już istnieją,
  // zachowują pozycję, prędkość i zmienne; zmienia się ich wygląd i rozmiar.
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

  // Odczyt/zapis pól aktora: x, y, w, h, vx, vy, rotation, z, visible albo zmienne.
  std::optional<double> get(const std::string &actor, const std::string &field) const;
  bool set(const std::string &actor, const std::string &field, double value);

  // Czy prostokąty dwóch aktorów na siebie nachodzą.
  bool touching(const std::string &a, const std::string &b) const;

  // Ruch: pozycja += prędkość (raz na klatkę).
  void integrate();

  std::vector<std::string> names() const;
  std::size_t size() const { return _byName.size(); }
  bool has_archetype(const std::string &name) const { return _archetypes.contains(name); }
  const std::optional<Rgba> &background() const { return _background; }

  entt::registry &registry() { return _registry; }
  const entt::registry &registry() const { return _registry; }

  // Tekstowy zrzut stanu – czytelny dla człowieka i agenta AI.
  void dump(std::ostream &out) const;
};

} // namespace PimpEx::scene
