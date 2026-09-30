#include "boot/XmlLoader.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <charconv>
#include <format>
#include <initializer_list>

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

ActorDef parse_actor(XmlContext &ctx, pugi::xml_node node, const char *what) {
  ActorDef def;
  def.name = node.attribute("name").value();
  def.line = ctx.line_of(node);
  if (def.name.empty())
    ctx.error(node, std::format("<{}> musi mieć atrybut name", what));
  else if (def.name.find_first_of(". /:\"#") != std::string::npos)
    ctx.error(node, std::format("nazwa '{}' nie może zawierać spacji ani znaków . / : \" #",
                                def.name));

  ctx.check_attributes(node, {"name", "x", "y", "w", "h", "vx", "vy", "rotation", "z", "visible"});
  ctx.read_float(node, "x", def.transform.x);
  ctx.read_float(node, "y", def.transform.y);
  ctx.read_float(node, "w", def.transform.w);
  ctx.read_float(node, "h", def.transform.h);
  ctx.read_float(node, "rotation", def.transform.rotation);
  ctx.read_int(node, "z", def.transform.z);
  ctx.read_bool(node, "visible", def.transform.visible);
  ctx.read_float(node, "vx", def.velocity.vx);
  ctx.read_float(node, "vy", def.velocity.vy);

  for (auto child : node.children()) {
    if (child.type() != pugi::node_element)
      continue;
    const std::string_view tag = child.name();
    if (tag == "Rect" || tag == "Circle") {
      Shape shape;
      shape.kind = tag == "Rect" ? Shape::Kind::Rect : Shape::Kind::Circle;
      ctx.check_attributes(child, {"color"});
      ctx.read_color(child, "color", shape.color);
      def.shape = shape;
    } else if (tag == "Sprite") {
      Sprite sprite;
      ctx.check_attributes(child, {"texture", "tint"});
      sprite.texture = child.attribute("texture").value();
      if (sprite.texture.empty())
        ctx.error(child, "<Sprite> musi mieć atrybut texture (id z <Resources>)");
      ctx.read_color(child, "tint", sprite.tint);
      def.sprite = sprite;
    } else if (tag == "Text") {
      Label label;
      ctx.check_attributes(child, {"value", "size", "color"});
      label.text = child.attribute("value").value();
      ctx.read_int(child, "size", label.size);
      ctx.read_color(child, "color", label.color);
      def.label = label;
    } else if (tag == "Var") {
      ctx.check_attributes(child, {"name", "value"});
      const std::string name = child.attribute("name").value();
      if (name.empty()) {
        ctx.error(child, "<Var> musi mieć atrybut name");
        continue;
      }
      float value = 0;
      ctx.read_float(child, "value", value);
      def.vars.values[name] = value;
    } else {
      ctx.warning(child, std::format("nieznany element <{}> w <{}> (dostępne: Rect, Circle, "
                                     "Sprite, Text, Var)",
                                     tag, what));
    }
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
                          const std::string &sceneName, Diagnostics &diagnostics) {
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
  ctx.check_attributes(root, {"background"});
  if (root.attribute("background")) {
    Rgba color;
    ctx.read_color(root, "background", color);
    data.background = color;
  }

  auto addActor = [&](pugi::xml_node node) {
    auto def = parse_actor(ctx, node, "Actor");
    const bool duplicate = std::any_of(data.actors.begin(), data.actors.end(),
                                       [&](const ActorDef &a) { return a.name == def.name; });
    if (duplicate)
      ctx.error(node, std::format("aktor '{}' jest już zdefiniowany", def.name));
    else if (!def.name.empty())
      data.actors.push_back(std::move(def));
  };
  auto addArchetype = [&](pugi::xml_node node) {
    auto def = parse_actor(ctx, node, "Archetype");
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
      for (auto inner : child.children())
        if (inner.type() == pugi::node_element) {
          if (std::string_view(inner.name()) == (tag == "Actors" ? "Actor" : "Archetype"))
            tag == "Actors" ? addActor(inner) : addArchetype(inner);
          else
            ctx.warning(inner, std::format("nieznany element <{}> w <{}>", inner.name(), tag));
        }
    } else if (tag == "Resources") {
      for (auto res : child.children()) {
        if (res.type() != pugi::node_element)
          continue;
        if (std::string_view(res.name()) != "Texture") {
          ctx.warning(res, std::format("nieznany zasób <{}> (dostępne: Texture)", res.name()));
          continue;
        }
        ctx.check_attributes(res, {"id", "path"});
        TextureResource texture{res.attribute("id").value(), res.attribute("path").value()};
        if (texture.id.empty() || texture.path.empty())
          ctx.error(res, "<Texture> musi mieć atrybuty id i path");
        else
          data.textures[texture.id] = texture;
      }
    } else {
      ctx.warning(child, std::format("nieznany element <{}> w <Scene> (dostępne: Actor, "
                                     "Archetype, Resources)",
                                     tag));
    }
  }

  auto checkSprite = [&](const ActorDef &def) {
    if (def.sprite && !data.textures.contains(def.sprite->texture))
      diagnostics.push_back({Diagnostic::Severity::Error, file, def.line,
                             std::format("'{}' używa tekstury '{}', której nie ma w <Resources>",
                                         def.name, def.sprite->texture)});
  };
  for (const auto &def : data.actors)
    checkSprite(def);
  for (const auto &[name, def] : data.archetypes)
    checkSprite(def);
  return data;
}

} // namespace PimpEx::boot
