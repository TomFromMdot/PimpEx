#include "scene/ComponentRegistry.hpp"
#include "core/Format.hpp"
#include "core/Strings.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>

namespace PimpEx::scene {

std::string_view field_type_name(FieldType type) {
  switch (type) {
  case FieldType::Number: return "number";
  case FieldType::Integer: return "integer";
  case FieldType::Bool: return "true/false";
  case FieldType::Text: return "text";
  case FieldType::Color: return "color";
  }
  return "?";
}

const FieldDesc *ComponentDesc::field(std::string_view fieldName) const {
  for (const auto &f : fields)
    if (f.name == fieldName)
      return &f;
  return nullptr;
}

std::string field_value_to_string(const FieldValue &value) {
  if (const auto *number = std::get_if<double>(&value))
    return format_number(*number);
  return std::get<std::string>(value);
}

std::string closest_name(std::string_view word, const std::vector<std::string> &candidates) {
  return closest_match(word, candidates);
}

namespace {

std::optional<double> parse_number(std::string_view text) {
  double value = 0;
  const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || ptr != text.data() + text.size())
    return std::nullopt;
  return value;
}

std::string choices_text(const FieldDesc &field) {
  std::string out;
  for (const auto &c : field.choices)
    out += (out.empty() ? "" : ", ") + c;
  return out;
}

} // namespace

std::optional<FieldValue> parse_field_text(const FieldDesc &field, std::string_view text,
                                           std::string &error) {
  switch (field.type) {
  case FieldType::Number:
  case FieldType::Integer:
    if (auto number = parse_number(text))
      return field.type == FieldType::Integer ? std::round(*number) : *number;
    error = std::format("{}=\"{}\" must be a number", field.name, text);
    return std::nullopt;
  case FieldType::Bool:
    if (text == "true" || text == "1" || text == "tak")
      return 1.0;
    if (text == "false" || text == "0" || text == "nie")
      return 0.0;
    error = std::format("{}=\"{}\" must be true or false", field.name, text);
    return std::nullopt;
  case FieldType::Text:
  case FieldType::Color:
    return coerce_field(field, std::string(text), error);
  }
  return std::nullopt;
}

std::optional<FieldValue> coerce_field(const FieldDesc &field, const FieldValue &value,
                                       std::string &error) {
  const auto *number = std::get_if<double>(&value);
  const auto *text = std::get_if<std::string>(&value);
  switch (field.type) {
  case FieldType::Number:
    if (number)
      return *number;
    return parse_field_text(field, *text, error);
  case FieldType::Integer:
    if (number)
      return std::round(*number);
    return parse_field_text(field, *text, error);
  case FieldType::Bool:
    if (number)
      return *number != 0.0 ? 1.0 : 0.0;
    return parse_field_text(field, *text, error);
  case FieldType::Text: {
    const auto str = number ? format_number(*number) : *text;
    if (!field.choices.empty() &&
        std::find(field.choices.begin(), field.choices.end(), str) == field.choices.end()) {
      error = std::format("{}=\"{}\" – allowed: {}", field.name, str, choices_text(field));
      return std::nullopt;
    }
    return str;
  }
  case FieldType::Color:
    if (text && parse_color(*text))
      return *text;
    error = std::format("{}: unknown color '{}' (use #rrggbb or a name, e.g. red)", field.name,
                        number ? format_number(*number) : *text);
    return std::nullopt;
  }
  return std::nullopt;
}


namespace {

template <typename C> class Binder {
  ComponentDesc _desc;
  using Getter = std::function<FieldValue(const C &)>;
  using Setter = std::function<void(C &, const FieldValue &)>;
  std::shared_ptr<std::map<std::string, std::pair<Getter, Setter>>> _access =
      std::make_shared<std::map<std::string, std::pair<Getter, Setter>>>();

  template <typename M> static FieldType type_of() {
    if constexpr (std::is_same_v<M, float> || std::is_same_v<M, double>)
      return FieldType::Number;
    else if constexpr (std::is_same_v<M, int>)
      return FieldType::Integer;
    else if constexpr (std::is_same_v<M, bool>)
      return FieldType::Bool;
    else if constexpr (std::is_same_v<M, Rgba>)
      return FieldType::Color;
    else
      return FieldType::Text;
  }

  template <typename M> static FieldValue to_value(const M &v) {
    if constexpr (std::is_same_v<M, bool>)
      return v ? 1.0 : 0.0;
    else if constexpr (std::is_arithmetic_v<M>)
      return static_cast<double>(v);
    else if constexpr (std::is_same_v<M, Rgba>)
      return color_to_string(v);
    else
      return v;
  }

  template <typename M> static void from_value(M &out, const FieldValue &v) {
    if constexpr (std::is_same_v<M, bool>)
      out = std::get<double>(v) != 0.0;
    else if constexpr (std::is_arithmetic_v<M>)
      out = static_cast<M>(std::get<double>(v));
    else if constexpr (std::is_same_v<M, Rgba>)
      out = parse_color(std::get<std::string>(v)).value_or(Rgba{});
    else
      out = std::get<std::string>(v);
  }

public:
  Binder(std::string name, std::string group, std::string doc) {
    _desc.name = std::move(name);
    _desc.group = std::move(group);
    _desc.doc = std::move(doc);
  }

  template <typename M>
  Binder &field(std::string name, M C::*member, std::string doc,
                std::vector<std::string> choices = {}, bool readOnly = false) {
    const C defaults{};
    FieldDesc f;
    f.name = name;
    f.type = type_of<M>();
    f.defaultValue = to_value(defaults.*member);
    f.choices = std::move(choices);
    f.doc = std::move(doc);
    f.readOnly = readOnly;
    _desc.fields.push_back(std::move(f));
    (*_access)[name] = {[member](const C &c) { return to_value(c.*member); },
                        [member](C &c, const FieldValue &v) { from_value(c.*member, v); }};
    return *this;
  }

  ComponentDesc build() {
    auto access = _access;
    _desc.has = [](const entt::registry &r, entt::entity e) { return r.all_of<C>(e); };
    _desc.add = [](entt::registry &r, entt::entity e) { (void)r.get_or_emplace<C>(e); };
    _desc.remove = [](entt::registry &r, entt::entity e) { r.remove<C>(e); };
    _desc.get = [access](const entt::registry &r, entt::entity e, const FieldDesc &f) {
      return access->at(f.name).first(r.get<C>(e));
    };
    _desc.set = [access](entt::registry &r, entt::entity e, const FieldDesc &f, const FieldValue &v) {
      access->at(f.name).second(r.get_or_emplace<C>(e), v);
    };
    return std::move(_desc);
  }
};

template <typename C> ComponentDesc color_only(std::string name, std::string group, std::string doc) {
  return Binder<C>(std::move(name), std::move(group), std::move(doc))
      .field("color", &C::color, "color")
      .build();
}

template <typename C> ComponentDesc mesh(std::string name, std::string doc) {
  return Binder<C>(std::move(name), "3D", std::move(doc))
      .field("color", &C::color, "color")
      .field("wire", &C::wire, "true = edges only (wireframe)")
      .build();
}

} // namespace

std::shared_ptr<ComponentRegistry> ComponentRegistry::with_builtins() {
  auto registry = std::make_shared<ComponentRegistry>();

  registry->add(Binder<Transform>("Transform", "general",
                                  "Position, size, rotation and visibility. Every actor has it; "
                                  "its fields can be written directly on <Actor>.")
                    .field("x", &Transform::x, "X position (centre)")
                    .field("y", &Transform::y, "Y position (centre); grows down in 2D, up in 3D")
                    .field("z", &Transform::z, "Z position (3D)")
                    .field("w", &Transform::w, "width")
                    .field("h", &Transform::h, "height")
                    .field("d", &Transform::d, "depth (3D)")
                    .field("rx", &Transform::rx, "rotation around X in degrees (3D)")
                    .field("ry", &Transform::ry, "rotation around Y in degrees (3D)")
                    .field("rz", &Transform::rz, "rotation around Z in degrees (the 2D rotation)")
                    .field("scale", &Transform::scale, "scale (3D, models)")
                    .field("layer", &Transform::layer, "2D drawing order (higher = on top)")
                    .field("visible", &Transform::visible, "whether the actor is drawn")
                    .field("space", &Transform::space,
                           "world = in the world, screen = HUD in screen pixels",
                           {"world", "screen"})
                    .build());

  registry->add(Binder<Body>("Body", "movement",
                             "Velocity and gravity. The engine moves the actor every frame.")
                    .field("vx", &Body::vx, "X velocity per frame")
                    .field("vy", &Body::vy, "Y velocity per frame")
                    .field("vz", &Body::vz, "Z velocity per frame (3D)")
                    .field("gravity", &Body::gravity, "downward acceleration per frame (e.g. 0.5)")
                    .field("grounded", &Body::grounded, "1 when standing on something solid", {}, true)
                    .build());

  registry->add(Binder<Collider>("Collider", "movement",
                                 "Collisions with the actor's box (w, h, d).")
                    .field("kind", &Collider::kind,
                           "solid = cannot be walked through, trigger = only detects touches",
                           {"solid", "trigger"})
                    .build());

  registry->add(color_only<Rect>("Rect", "2D", "Rectangle w × h."));
  registry->add(color_only<Circle>("Circle", "2D", "Circle with diameter w."));
  registry->add(Binder<Sprite>("Sprite", "2D", "Image from <Resources> stretched to w × h.")
                    .field("texture", &Sprite::texture, "texture id from <Resources>")
                    .field("tint", &Sprite::tint, "tint (white = unchanged)")
                    .build());
  registry->add(Binder<Text>("Text", "2D",
                             "Text. {Actor.field} inserts a live value. In 3D scenes "
                             "text usually has space=\"screen\".")
                    .field("value", &Text::value, "the text")
                    .field("size", &Text::size, "font size")
                    .field("color", &Text::color, "color")
                    .build());

  registry->add(mesh<Cube>("Cube", "Box w × h × d."));
  registry->add(mesh<Sphere>("Sphere", "Sphere with diameter w."));
  registry->add(mesh<Cylinder>("Cylinder", "Cylinder: diameter w, height h."));
  registry->add(color_only<Plane>("Plane", "3D", "Flat floor w × d."));
  registry->add(Binder<Model>("Model", "3D", "3D model (.glb, .gltf, .obj) from <Resources>.")
                    .field("model", &Model::model, "model id from <Resources>")
                    .field("tint", &Model::tint, "tint")
                    .build());

  registry->add(Binder<Camera>("Camera", "camera and light",
                               "Camera. Position from the actor's Transform. The first active "
                               "camera in the scene is used.")
                    .field("mode", &Camera::mode,
                           "fixed | follow (2D and 3D), orbit | fps (3D)",
                           {"fixed", "follow", "orbit", "fps"})
                    .field("target", &Camera::target, "actor to follow / look at")
                    .field("zoom", &Camera::zoom, "zoom (2D)")
                    .field("fov", &Camera::fov, "field of view in degrees (3D)")
                    .field("distance", &Camera::distance, "distance from the target (3D follow/orbit)")
                    .field("height", &Camera::height, "height above the target (3D follow/orbit)")
                    .field("active", &Camera::active, "whether this camera is used")
                    .build());
  registry->add(Binder<Light>("Light", "camera and light",
                              "Light in a 3D scene (max 4). Position from Transform.")
                    .field("kind", &Light::kind,
                           "point = bulb, directional = sun (shines from its position to (0, 0, 0)), "
                           "ambient = ambient light (no direction)",
                           {"point", "directional", "ambient"})
                    .field("color", &Light::color, "light color")
                    .field("intensity", &Light::intensity, "brightness")
                    .build());
  return registry;
}

void ComponentRegistry::add(ComponentDesc desc) {
  const auto name = desc.name;
  if (auto it = _byName.find(name); it != _byName.end()) {
    _components[it->second] = std::move(desc);
    return;
  }
  _byName[name] = _components.size();
  _components.push_back(std::move(desc));
}

const ComponentDesc *ComponentRegistry::find(std::string_view name) const {
  const auto it = _byName.find(name);
  return it == _byName.end() ? nullptr : &_components[it->second];
}

std::vector<std::string> ComponentRegistry::names() const {
  std::vector<std::string> out;
  for (const auto &c : _components)
    out.push_back(c.name);
  return out;
}

namespace {

ComponentDesc custom_component(std::string name, std::string doc, std::vector<FieldDesc> fields) {
  ComponentDesc desc;
  desc.name = name;
  desc.doc = std::move(doc);
  desc.group = "user";
  desc.builtin = false;
  desc.fields = std::move(fields);
  const auto defaults = std::make_shared<std::map<std::string, FieldValue>>();
  for (const auto &f : desc.fields)
    (*defaults)[f.name] = f.defaultValue;

  desc.has = [name](const entt::registry &r, entt::entity e) {
    const auto *data = r.try_get<CustomData>(e);
    return data && data->components.contains(name);
  };
  desc.add = [name, defaults](entt::registry &r, entt::entity e) {
    auto &values = r.get_or_emplace<CustomData>(e).components[name];
    for (const auto &[key, value] : *defaults)
      values.try_emplace(key, value);
  };
  desc.remove = [name](entt::registry &r, entt::entity e) {
    if (auto *data = r.try_get<CustomData>(e))
      data->components.erase(name);
  };
  desc.get = [name](const entt::registry &r, entt::entity e, const FieldDesc &f) {
    return r.get<CustomData>(e).components.at(name).at(f.name);
  };
  desc.set = [name, defaults](entt::registry &r, entt::entity e, const FieldDesc &f,
                              const FieldValue &v) {
    auto &values = r.get_or_emplace<CustomData>(e).components[name];
    for (const auto &[key, value] : *defaults)
      values.try_emplace(key, value);
    values[f.name] = v;
  };
  return desc;
}

} // namespace

void ComponentRegistry::load_custom(std::string_view xml, const std::string &file,
                                    Diagnostics &diagnostics) {
  auto lineOf = [&](pugi::xml_node node) {
    const auto offset = std::min(static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, node.offset_debug())),
                                 xml.size());
    return 1 + static_cast<int>(std::count(xml.begin(), xml.begin() + offset, '\n'));
  };
  auto error = [&](pugi::xml_node node, std::string message) {
    diagnostics.push_back({Diagnostic::Severity::Error, file, lineOf(node), std::move(message)});
  };

  pugi::xml_document doc;
  const auto result = doc.load_buffer(xml.data(), xml.size());
  if (!result) {
    diagnostics.push_back({Diagnostic::Severity::Error, file, 1,
                           std::format("invalid XML: {}", result.description())});
    return;
  }
  const auto root = doc.child("Components");
  if (!root) {
    diagnostics.push_back({Diagnostic::Severity::Error, file, 1, "missing <Components> element"});
    return;
  }

  for (auto node : root.children("Component")) {
    const std::string name = node.attribute("name").value();
    if (name.empty() || !(std::isupper(static_cast<unsigned char>(name[0])))) {
      error(node, "<Component> needs a name starting with a capital letter, e.g. Health");
      continue;
    }
    if (name == "Actor" || name == "Archetype" || name == "Var" || name == "Behavior" ||
        (find(name) && find(name)->builtin)) {
      error(node, std::format("the name '{}' is reserved by the engine", name));
      continue;
    }
    std::vector<FieldDesc> fields;
    for (auto fieldNode : node.children("Field")) {
      FieldDesc field;
      field.name = fieldNode.attribute("name").value();
      field.doc = fieldNode.attribute("doc").value();
      const std::string type = fieldNode.attribute("type").as_string("number");
      if (type == "number") field.type = FieldType::Number;
      else if (type == "integer") field.type = FieldType::Integer;
      else if (type == "bool") field.type = FieldType::Bool;
      else if (type == "text") field.type = FieldType::Text;
      else if (type == "color") field.type = FieldType::Color;
      else {
        error(fieldNode, std::format("unknown field type '{}' (number, integer, bool, text, color)", type));
        continue;
      }
      if (field.name.empty() || field.name.find('.') != std::string::npos) {
        error(fieldNode, "<Field> needs a name (without dots)");
        continue;
      }
      field.defaultValue = field.type == FieldType::Text    ? FieldValue{std::string()}
                           : field.type == FieldType::Color ? FieldValue{std::string("white")}
                                                            : FieldValue{0.0};
      if (auto def = fieldNode.attribute("default")) {
        std::string message;
        if (auto value = parse_field_text(field, def.value(), message))
          field.defaultValue = *value;
        else
          error(fieldNode, message);
      }
      fields.push_back(std::move(field));
    }
    add(custom_component(name, node.attribute("doc").value(), std::move(fields)));
  }
}

} // namespace PimpEx::scene
