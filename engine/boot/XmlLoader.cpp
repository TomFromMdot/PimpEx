#include "boot/XmlLoader.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <charconv>
#include <format>
#include <initializer_list>
#include <span>

namespace PimpEx::boot {
namespace {

using namespace scene;

class XmlContext {
  std::string_view _content;
  std::string _file;
  Diagnostics &_diagnostics;

public:
  XmlContext(std::string_view content, std::string file, Diagnostics &diagnostics)
      : _content(content), _file(std::move(file)), _diagnostics(diagnostics) {}

  int line_of(pugi::xml_node node) const {
    const auto offset = static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, node.offset_debug()));
    const auto end = std::min(offset, _content.size());
    return 1 + static_cast<int>(std::count(_content.begin(), _content.begin() + end, '\n'));
  }

  void error(pugi::xml_node node, std::string message) {
    _diagnostics.push_back({Diagnostic::Severity::Error, _file, line_of(node), std::move(message)});
  }
  void warning(pugi::xml_node node, std::string message) {
    _diagnostics.push_back({Diagnostic::Severity::Warning, _file, line_of(node), std::move(message)});
  }

  bool parse(pugi::xml_document &doc) {
    const auto result = doc.load_buffer(_content.data(), _content.size());
    if (result)
      return true;
    const auto offset = std::min(static_cast<std::size_t>(result.offset), _content.size());
    const int line = 1 + static_cast<int>(std::count(_content.begin(), _content.begin() + offset, '\n'));
    _diagnostics.push_back({Diagnostic::Severity::Error, _file, line,
                            std::format("niepoprawny XML: {}", result.description())});
    return false;
  }

  // Ostrzega o atrybutach, których silnik nie zna (literówki!).
  void check_attributes(pugi::xml_node node, std::initializer_list<std::string_view> known) {
    for (auto attr : node.attributes())
      if (std::find(known.begin(), known.end(), attr.name()) == known.end())
        warning(node, std::format("<{}> nie ma atrybutu '{}' – silnik go pominie", node.name(),
                                  attr.name()));
  }

  void read_float(pugi::xml_node node, const char *name, float &out) {
    const auto attr = node.attribute(name);
    if (!attr)
      return;
    const std::string_view text = attr.value();
    float value = 0;
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
      error(node, std::format("atrybut {}=\"{}\" musi być liczbą", name, text));
      return;
    }
    out = value;
  }

  void read_int(pugi::xml_node node, const char *name, int &out) {
    float value = static_cast<float>(out);
    read_float(node, name, value);
    out = static_cast<int>(value);
  }

  void read_bool(pugi::xml_node node, const char *name, bool &out) {
    const auto attr = node.attribute(name);
    if (!attr)
      return;
    const std::string_view text = attr.value();
    if (text == "true" || text == "1")
      out = true;
    else if (text == "false" || text == "0")
      out = false;
    else
      error(node, std::format("atrybut {}=\"{}\" musi być true albo false", name, text));
  }

  void read_color(pugi::xml_node node, const char *name, Rgba &out) {
    const auto attr = node.attribute(name);
    if (!attr)
      return;
    if (auto color = parse_color(attr.value()))
      out = *color;
    else
      error(node, std::format("nieznany kolor {}=\"{}\" (użyj #rrggbb albo nazwy, np. red)",
                              name, attr.value()));
  }
};

// Atrybuty <Actor>, które są skrótami pól Transform i Body.
constexpr std::string_view TransformAttributes[] = {"x", "y", "z", "w", "h", "d", "rx", "ry", "rz",
                                                    "scale", "layer", "visible", "space"};
constexpr std::string_view BodyAttributes[] = {"vx", "vy", "vz", "gravity"};

bool contains(std::span<const std::string_view> list, std::string_view value) {
  return std::find(list.begin(), list.end(), value) != list.end();
}

// Wartości pól komponentu z atrybutów elementu XML (np. <Rect color="red"/>).
void read_component_fields(XmlContext &ctx, pugi::xml_node node, const ComponentDesc &desc,
                           ComponentInit &init, std::span<const std::string_view> skip = {}) {
  for (auto attr : node.attributes()) {
    const std::string_view name = attr.name();
    if (contains(skip, name))
      continue;
    const auto *field = desc.field(name);
    if (!field) {
      std::vector<std::string> names;
      for (const auto &f : desc.fields)
        names.push_back(f.name);
      const auto hint = closest_name(name, names);
      ctx.warning(node, std::format("<{}> nie ma pola '{}'{} – silnik je pominie", node.name(), name,
                                    hint.empty() ? "" : std::format(" (czy chodziło o {}?)", hint)));
      continue;
    }
    if (field->readOnly) {
      ctx.warning(node, std::format("pole {}.{} ustawia silnik – wartość z pliku zostanie pominięta",
                                    desc.name, name));
      continue;
    }
    std::string message;
    if (auto value = parse_field_text(*field, attr.value(), message))
      init.values.emplace_back(field->name, std::move(*value));
    else
      ctx.error(node, message);
  }
}

ActorDef parse_actor(XmlContext &ctx, pugi::xml_node node, const char *what,
                     const ComponentRegistry &registry, SceneMode mode) {
  ActorDef def;
  def.name = node.attribute("name").value();
  def.line = ctx.line_of(node);
  if (def.name.empty())
    ctx.error(node, std::format("<{}> musi mieć atrybut name", what));
  else if (def.name.find_first_of(". /:\"#@") != std::string::npos)
    ctx.error(node, std::format("nazwa '{}' nie może zawierać spacji ani znaków . / : \" # @",
                                def.name));

  // Transform zawsze pierwszy. W 3D domyślny rozmiar to 1 × 1 × 1.
  ComponentInit transform{"Transform", {}, def.line};
  if (mode == SceneMode::Mode3D)
    transform.values = {{"w", 1.0}, {"h", 1.0}, {"d", 1.0}};
  ComponentInit body{"Body", {}, def.line};

  for (auto attr : node.attributes()) {
    const std::string_view name = attr.name();
    if (name == "name")
      continue;
    if (name == "tags") {
      std::string_view list = attr.value();
      while (!list.empty()) {
        const auto comma = list.find(',');
        auto tag = list.substr(0, comma);
        while (!tag.empty() && tag.front() == ' ') tag.remove_prefix(1);
        while (!tag.empty() && tag.back() == ' ') tag.remove_suffix(1);
        if (!tag.empty())
          def.tags.emplace_back(tag);
        list = comma == std::string_view::npos ? std::string_view{} : list.substr(comma + 1);
      }
      continue;
    }
    const bool isBody = contains(BodyAttributes, name);
    const bool isTransform = contains(TransformAttributes, name) || name == "rotation";
    if (!isBody && !isTransform) {
      std::vector<std::string> known = {"name", "tags", "rotation"};
      for (auto a : TransformAttributes) known.emplace_back(a);
      for (auto a : BodyAttributes) known.emplace_back(a);
      const auto hint = closest_name(name, known);
      ctx.warning(node, std::format("<{}> nie ma atrybutu '{}'{} – silnik go pominie", what, name,
                                    hint.empty() ? "" : std::format(" (czy chodziło o {}?)", hint)));
      continue;
    }
    const auto &desc = *registry.find(isBody ? "Body" : "Transform");
    const auto *field = desc.field(name == "rotation" ? "rz" : name);
    std::string message;
    if (auto value = parse_field_text(*field, attr.value(), message))
      (isBody ? body : transform).values.emplace_back(field->name, std::move(*value));
    else
      ctx.error(node, message);
  }
  def.components.push_back(std::move(transform));
  if (!body.values.empty())
    def.components.push_back(std::move(body));

  for (auto child : node.children()) {
    if (child.type() != pugi::node_element)
      continue;
    const std::string_view tag = child.name();

    if (tag == "Var") {
      ctx.check_attributes(child, {"name", "value"});
      const std::string name = child.attribute("name").value();
      if (name.empty() || name.find('.') != std::string::npos) {
        ctx.error(child, "<Var> musi mieć atrybut name (bez kropek)");
        continue;
      }
      float value = 0;
      ctx.read_float(child, "value", value);
      def.vars.values[name] = value;
      continue;
    }

    if (tag == "Behavior") {
      BehaviorRef behavior;
      behavior.name = child.attribute("name").value();
      if (behavior.name.empty()) {
        ctx.error(child, "<Behavior> musi mieć atrybut name (plik scripts/behaviors/<name>.lua)");
        continue;
      }
      for (auto attr : child.attributes())
        if (std::string_view(attr.name()) != "name")
          behavior.params[attr.name()] = attr.value();
      def.behaviors.push_back(std::move(behavior));
      continue;
    }

    const auto *desc = registry.find(tag);
    if (!desc) {
      auto names = registry.names();
      names.insert(names.end(), {"Var", "Behavior"});
      const auto hint = closest_name(tag, names);
      ctx.warning(child, std::format("nieznany komponent <{}>{} – zdefiniuj go w components.xml "
                                     "albo zobacz docs/components.md",
                                     tag, hint.empty() ? "" : std::format(" (czy chodziło o {}?)", hint)));
      continue;
    }

    // <Transform>/<Body> jako element dopisują się do już istniejących.
    ComponentInit *target = nullptr;
    for (auto &c : def.components)
      if (c.component == desc->name)
        target = &c;
    if (!target) {
      def.components.push_back({desc->name, {}, ctx.line_of(child)});
      target = &def.components.back();
    }
    read_component_fields(ctx, child, *desc, *target);
  }
  return def;
}

} // namespace

ProjectConfig parse_project_xml(std::string_view content, const std::string &file,
                                Diagnostics &diagnostics) {
  ProjectConfig config;
  XmlContext ctx(content, file, diagnostics);
  pugi::xml_document doc;
  if (!ctx.parse(doc))
    return config;

  const auto root = doc.child("Project");
  if (!root) {
    diagnostics.push_back({Diagnostic::Severity::Error, file, 1, "brakuje elementu <Project>"});
    return config;
  }
  config.name = root.attribute("name").value();
  config.title = config.name.empty() ? config.title : config.name;

  for (auto child : root.children()) {
    if (child.type() != pugi::node_element)
      continue;
    const std::string_view tag = child.name();
    if (tag == "Window") {
      ctx.check_attributes(child, {"title", "width", "height", "fps", "background"});
      if (auto title = child.attribute("title"))
        config.title = title.value();
      ctx.read_int(child, "width", config.width);
      ctx.read_int(child, "height", config.height);
      ctx.read_int(child, "fps", config.fps);
      ctx.read_color(child, "background", config.background);
      if (config.width <= 0 || config.height <= 0 || config.fps <= 0)
        ctx.error(child, "width, height i fps muszą być większe od zera");
    } else if (tag == "StartScene") {
      ctx.check_attributes(child, {"name"});
      config.startScene = child.attribute("name").value();
    } else if (tag == "HotReload") {
      ctx.check_attributes(child, {"keepState"});
      ctx.read_bool(child, "keepState", config.keepStateOnReload);
    } else {
      ctx.warning(child, std::format("nieznany element <{}> w <Project>", tag));
    }
  }
  if (config.startScene.empty())
    ctx.error(root, "<StartScene name=\"...\"/> musi wskazywać scenę startową");
  return config;
}

SceneData parse_scene_xml(std::string_view content, const std::string &file,
                          const std::string &sceneName, Diagnostics &diagnostics,
                          const ComponentRegistry *components) {
  static const auto builtins = ComponentRegistry::with_builtins();
  const auto &registry = components ? *components : *builtins;

  SceneData data;
  data.name = sceneName;
  XmlContext ctx(content, file, diagnostics);
  pugi::xml_document doc;
  if (!ctx.parse(doc))
    return data;

  const auto root = doc.child("Scene");
  if (!root) {
    diagnostics.push_back({Diagnostic::Severity::Error, file, 1, "brakuje elementu <Scene>"});
    return data;
  }
  ctx.check_attributes(root, {"background", "mode"});
  if (root.attribute("background")) {
    Rgba color;
    ctx.read_color(root, "background", color);
    data.background = color;
  }
  const std::string_view mode = root.attribute("mode").as_string("2d");
  if (mode == "3d")
    data.mode = SceneMode::Mode3D;
  else if (mode != "2d")
    ctx.error(root, std::format("mode=\"{}\" – dozwolone: 2d, 3d", mode));

  auto addActor = [&](pugi::xml_node node) {
    auto def = parse_actor(ctx, node, "Actor", registry, data.mode);
    const bool duplicate = std::any_of(data.actors.begin(), data.actors.end(),
                                       [&](const ActorDef &a) { return a.name == def.name; });
    if (duplicate)
      ctx.error(node, std::format("aktor '{}' jest już zdefiniowany", def.name));
    else if (!def.name.empty())
      data.actors.push_back(std::move(def));
  };
  auto addArchetype = [&](pugi::xml_node node) {
    auto def = parse_actor(ctx, node, "Archetype", registry, data.mode);
    if (data.archetypes.contains(def.name))
      ctx.error(node, std::format("archetyp '{}' jest już zdefiniowany", def.name));
    else if (!def.name.empty())
      data.archetypes.emplace(def.name, std::move(def));
  };

  for (auto child : root.children()) {
    if (child.type() != pugi::node_element)
      continue;
    const std::string_view tag = child.name();
    if (tag == "Actor") {
      addActor(child);
    } else if (tag == "Archetype") {
      addArchetype(child);
    } else if (tag == "Actors" || tag == "Archetypes") {
      const std::string_view inner = tag == "Actors" ? "Actor" : "Archetype";
      for (auto node : child.children()) {
        if (node.type() != pugi::node_element)
          continue;
        if (std::string_view(node.name()) == inner)
          inner == "Actor" ? addActor(node) : addArchetype(node);
        else
          ctx.warning(node, std::format("nieznany element <{}> w <{}>", node.name(), tag));
      }
    } else if (tag == "Resources") {
      for (auto res : child.children()) {
        if (res.type() != pugi::node_element)
          continue;
        const std::string_view kind = res.name();
        if (kind != "Texture" && kind != "Model") {
          ctx.warning(res, std::format("nieznany zasób <{}> (dostępne: Texture, Model)", kind));
          continue;
        }
        ctx.check_attributes(res, {"id", "path"});
        Resource resource{res.attribute("id").value(), res.attribute("path").value()};
        if (resource.id.empty() || resource.path.empty())
          ctx.error(res, std::format("<{}> musi mieć atrybuty id i path", kind));
        else
          (kind == "Texture" ? data.textures : data.models)[resource.id] = resource;
      }
    } else {
      ctx.warning(child, std::format("nieznany element <{}> w <Scene> (dostępne: Actor, "
                                     "Archetype, Resources)",
                                     tag));
    }
  }

  // Sprite i Model muszą wskazywać zasoby z <Resources>.
  auto checkResources = [&](const ActorDef &def) {
    for (const auto &init : def.components)
      for (const auto &[field, value] : init.values) {
        const auto *id = std::get_if<std::string>(&value);
        if (!id)
          continue;
        if (init.component == "Sprite" && field == "texture" && !data.textures.contains(*id))
          diagnostics.push_back({Diagnostic::Severity::Error, file, init.line,
                                 std::format("'{}' używa tekstury '{}', której nie ma w <Resources>",
                                             def.name, *id)});
        if (init.component == "Model" && field == "model" && !data.models.contains(*id))
          diagnostics.push_back({Diagnostic::Severity::Error, file, init.line,
                                 std::format("'{}' używa modelu '{}', którego nie ma w <Resources>",
                                             def.name, *id)});
      }
  };
  for (const auto &def : data.actors)
    checkResources(def);
  for (const auto &[name, def] : data.archetypes)
    checkResources(def);
  return data;
}

} // namespace PimpEx::boot
