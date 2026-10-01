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
  std::vector<std::string> choices; // allowed text values (empty = any)
  std::string doc;
  bool readOnly = false; // e.g. Body.grounded – set by the engine
};

// A component's name (= XML tag), fields and entity operations. XML, .swap,
// Lua, `check`, the state dump and the API all work through this, so
// components from components.xml behave exactly like built-in ones.
struct ComponentDesc {
  std::string name;
  std::string doc;
  std::string group; // "general", "2D", "3D", "user"...
  bool builtin = true;
  std::vector<FieldDesc> fields;

  std::function<bool(const entt::registry &, entt::entity)> has;
  std::function<void(entt::registry &, entt::entity)> add; // with default values
  std::function<void(entt::registry &, entt::entity)> remove;
  std::function<FieldValue(const entt::registry &, entt::entity, const FieldDesc &)> get;
  std::function<void(entt::registry &, entt::entity, const FieldDesc &, const FieldValue &)> set;

  const FieldDesc *field(std::string_view fieldName) const;
};

// XML text ("4", "true", "#ff0000") -> field value; nullopt and `error` on mismatch.
std::optional<FieldValue> parse_field_text(const FieldDesc &field, std::string_view text,
                                           std::string &error);

// Fits a value from .swap / Lua / API to the field type (e.g. 1 -> true).
std::optional<FieldValue> coerce_field(const FieldDesc &field, const FieldValue &value,
                                       std::string &error);

std::string field_value_to_string(const FieldValue &value);

// Typo hint: "Heath" -> "Health".
std::string closest_name(std::string_view word, const std::vector<std::string> &candidates);

class ComponentRegistry {
  std::vector<ComponentDesc> _components;
  std::map<std::string, std::size_t, std::less<>> _byName;

public:
  static std::shared_ptr<ComponentRegistry> with_builtins();

  void add(ComponentDesc desc);
  const ComponentDesc *find(std::string_view name) const;
  const std::vector<ComponentDesc> &all() const { return _components; }
  std::vector<std::string> names() const;

  void load_custom(std::string_view xml, const std::string &file, Diagnostics &diagnostics);
};

} // namespace PimpEx::scene
