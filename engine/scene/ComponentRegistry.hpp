#pragma once

#include "core/Diagnostic.hpp"
#include "scene/Components.hpp"

#include <entt/entt.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace PimpEx::scene {

enum class FieldType { Number, Integer, Bool, Text, Color };

std::string_view field_type_name(FieldType type);

struct FieldDesc {
  std::string name;
  FieldType type = FieldType::Number;
  FieldValue defaultValue = 0.0;
  std::vector<std::string> choices; // dozwolone wartości tekstu (puste = dowolny)
  std::string doc;
  bool readOnly = false; // np. Body.grounded – ustawia silnik
};

// Opis jednego komponentu: nazwa (= tag w XML), pola i operacje na encji.
// Dzięki temu XML, .swap, Lua, `check`, zrzut stanu i API działają dla
// każdego komponentu tak samo – także dla komponentów z components.xml.
struct ComponentDesc {
  std::string name;
  std::string doc;
  std::string group; // "ogólne", "2D", "3D", "użytkownika"...
  bool builtin = true;
  std::vector<FieldDesc> fields;

  std::function<bool(const entt::registry &, entt::entity)> has;
  std::function<void(entt::registry &, entt::entity)> add; // z wartościami domyślnymi
  std::function<void(entt::registry &, entt::entity)> remove;
  std::function<FieldValue(const entt::registry &, entt::entity, const FieldDesc &)> get;
  std::function<void(entt::registry &, entt::entity, const FieldDesc &, const FieldValue &)> set;

  const FieldDesc *field(std::string_view fieldName) const;
};

// Zamiana tekstu z XML ("4", "true", "#ff0000") na wartość pola.
// Zwraca nullopt i wypełnia `error`, gdy tekst nie pasuje do typu.
std::optional<FieldValue> parse_field_text(const FieldDesc &field, std::string_view text,
                                           std::string &error);

// Dopasowanie wartości z .swap / Lua / API do typu pola (np. 1 -> true).
std::optional<FieldValue> coerce_field(const FieldDesc &field, const FieldValue &value,
                                       std::string &error);

std::string field_value_to_string(const FieldValue &value);

// Podpowiedź literówki: "Heath" -> "Health".
std::string closest_name(std::string_view word, const std::vector<std::string> &candidates);

class ComponentRegistry {
  std::vector<ComponentDesc> _components;
  std::map<std::string, std::size_t, std::less<>> _byName;

public:
  // Rejestr z komponentami wbudowanymi w silnik.
  static std::shared_ptr<ComponentRegistry> with_builtins();

  void add(ComponentDesc desc);
  const ComponentDesc *find(std::string_view name) const;
  const std::vector<ComponentDesc> &all() const { return _components; }
  std::vector<std::string> names() const;

  // Wczytuje components.xml (komponenty użytkownika).
  void load_custom(std::string_view xml, const std::string &file, Diagnostics &diagnostics);
};

} // namespace PimpEx::scene
