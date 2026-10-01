#include "boot/XmlLoader.hpp"

#include "core/Strings.hpp"
#include "vm/Parser.hpp"

#include <pugixml.hpp>

#include <algorithm>
#include <charconv>
#include <format>
#include <initializer_list>
#include <map>
#include <set>
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

  Diagnostics &diagnostics() { return _diagnostics; }
  const std::string &file() const { return _file; }

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
                            std::format("invalid XML: {}", result.description())});
    return false;
  }

  // Warns about unknown attributes (typos!).
  void check_attributes(pugi::xml_node node, std::initializer_list<std::string_view> known) {
    for (auto attr : node.attributes())
      if (std::find(known.begin(), known.end(), attr.name()) == known.end())
        warning(node, std::format("<{}> has no attribute '{}' – the engine ignores it", node.name(),
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
      error(node, std::format("attribute {}=\"{}\" must be a number", name, text));
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
      error(node, std::format("attribute {}=\"{}\" must be true or false", name, text));
  }

  void read_color(pugi::xml_node node, const char *name, Rgba &out) {
    const auto attr = node.attribute(name);
    if (!attr)
      return;
    if (auto color = parse_color(attr.value()))
      out = *color;
    else
      error(node, std::format("unknown color {}=\"{}\" (use #rrggbb or a name, e.g. red)",
                              name, attr.value()));
  }
};

// <Actor> attributes that are Transform and Body shortcuts.
constexpr std::string_view TransformAttributes[] = {"x", "y", "z", "w", "h", "d", "rx", "ry", "rz",
                                                    "scale", "layer", "visible", "space"};
constexpr std::string_view BodyAttributes[] = {"vx", "vy", "vz", "gravity"};

bool contains(std::span<const std::string_view> list, std::string_view value) {
  return std::find(list.begin(), list.end(), value) != list.end();
}

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
      ctx.warning(node, std::format("<{}> has no field '{}'{} – the engine ignores it", node.name(), name,
                                    hint.empty() ? "" : std::format(" (did you mean {}?)", hint)));
      continue;
    }
    if (field->readOnly) {
      ctx.warning(node, std::format("the engine sets {}.{} – the value from the file is ignored",
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
    ctx.error(node, std::format("<{}> needs a name attribute", what));
  else if (def.name.find_first_of(". /:\"#@") != std::string::npos)
    ctx.error(node, std::format("the name '{}' cannot contain spaces or . / : \" # @",
                                def.name));

  // Transform always first. The default 3D size is 1 × 1 × 1.
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
      ctx.warning(node, std::format("<{}> has no attribute '{}'{} – the engine ignores it", what, name,
                                    hint.empty() ? "" : std::format(" (did you mean {}?)", hint)));
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
        ctx.error(child, "<Var> needs a name attribute (without dots)");
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
        ctx.error(child, "<Behavior> needs a name attribute (file scripts/behaviors/<name>.lua)");
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
      ctx.warning(child, std::format("unknown component <{}>{} – define it in components.xml "
                                     "or see docs/components.md",
                                     tag, hint.empty() ? "" : std::format(" (did you mean {}?)", hint)));
      continue;
    }

    // <Transform>/<Body> elements extend the attributes already given.
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
    diagnostics.push_back({Diagnostic::Severity::Error, file, 1, "missing <Project> element"});
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
        ctx.error(child, "width, height and fps must be greater than zero");
    } else if (tag == "StartScene") {
      ctx.check_attributes(child, {"name"});
      config.startScene = child.attribute("name").value();
    } else if (tag == "HotReload") {
      ctx.check_attributes(child, {"keepState"});
      ctx.read_bool(child, "keepState", config.keepStateOnReload);
    } else {
      ctx.warning(child, std::format("unknown element <{}> in <Project>", tag));
    }
  }
  if (config.startScene.empty())
    ctx.error(root, "<StartScene name=\"...\"/> must name the start scene");
  return config;
}

namespace {

// <Tilemap cell="2"> <Legend char="#" archetype="Wall"/> <Map>...</Map> </Tilemap>
// Every map character becomes an actor from an archetype (Wall_3_5) or places an
// existing actor (legend actor="Player"). 3D maps lie on x/z, 2D maps on x/y.
void expand_tilemap(XmlContext &ctx, pugi::xml_node node, SceneData &data) {
  const bool is3d = data.mode == SceneMode::Mode3D;
  ctx.check_attributes(node, {"cell", "x", "y", "z"});
  float cell = is3d ? 1.0f : 32.0f, ox = 0, oy = 0, oz = 0;
  ctx.read_float(node, "cell", cell);
  ctx.read_float(node, "x", ox);
  ctx.read_float(node, "y", oy);
  ctx.read_float(node, "z", oz);

  struct Entry {
    std::string archetype, actor;
  };
  std::map<char, Entry> legend;
  pugi::xml_node mapNode;
  for (auto child : node.children()) {
    if (child.type() != pugi::node_element)
      continue;
    const std::string_view tag = child.name();
    if (tag == "Map") {
      mapNode = child;
      continue;
    }
    if (tag != "Legend") {
      ctx.warning(child, std::format("unknown element <{}> in <Tilemap> (available: Legend, Map)", tag));
      continue;
    }
    ctx.check_attributes(child, {"char", "archetype", "actor"});
    const std::string symbol = child.attribute("char").value();
    Entry entry{child.attribute("archetype").value(), child.attribute("actor").value()};
    if (symbol.size() != 1 || symbol == "." || symbol == " ") {
      ctx.error(child, "<Legend char=\"...\"> is one character (not '.' or a space – those are empty cells)");
      continue;
    }
    if (entry.archetype.empty() == entry.actor.empty()) {
      ctx.error(child, "<Legend> needs archetype=\"...\" or actor=\"...\"");
      continue;
    }
    if (!entry.archetype.empty() && !data.archetypes.contains(entry.archetype)) {
      std::vector<std::string> names;
      for (const auto &[name, def] : data.archetypes)
        names.push_back(name);
      const auto hint = closest_name(entry.archetype, names);
      ctx.error(child, std::format("no archetype '{}'{}", entry.archetype,
                                   hint.empty() ? "" : std::format(" (did you mean {}?)", hint)));
      continue;
    }
    if (!entry.actor.empty() &&
        std::none_of(data.actors.begin(), data.actors.end(), [&](const ActorDef &a) { return a.name == entry.actor; })) {
      ctx.error(child, std::format("no actor '{}' (define it with <Actor>)", entry.actor));
      continue;
    }
    if (legend.contains(symbol[0]))
      ctx.error(child, std::format("character '{}' is already in the legend", symbol));
    legend[symbol[0]] = entry;
  }
  if (!mapNode) {
    ctx.error(node, "<Tilemap> needs a <Map> with the character map");
    return;
  }

  // Map rows with their file lines; common indentation removed.
  const std::string_view text = mapNode.child_value();
  const int firstLine = ctx.line_of(mapNode);
  std::vector<std::pair<int, std::string_view>> rows;
  std::size_t start = 0;
  for (int index = 0; start <= text.size(); ++index) {
    auto end = text.find('\n', start);
    if (end == std::string_view::npos)
      end = text.size();
    auto row = text.substr(start, end - start);
    if (!row.empty() && row.back() == '\r')
      row.remove_suffix(1);
    rows.emplace_back(firstLine + index, row);
    start = end + 1;
  }
  auto blank = [](std::string_view s) { return s.find_first_not_of(" \t") == std::string_view::npos; };
  while (!rows.empty() && blank(rows.front().second))
    rows.erase(rows.begin());
  while (!rows.empty() && blank(rows.back().second))
    rows.pop_back();
  std::size_t indent = std::string_view::npos;
  for (const auto &[line, row] : rows)
    if (!blank(row))
      indent = std::min(indent, row.find_first_not_of(" \t"));

  std::set<char> reported;
  std::set<std::string> placed;
  for (std::size_t r = 0; r < rows.size(); ++r) {
    const auto &[line, fullRow] = rows[r];
    const auto row = fullRow.size() > indent ? fullRow.substr(indent) : std::string_view{};
    for (std::size_t c = 0; c < row.size(); ++c) {
      const char symbol = row[c];
      if (symbol == '.' || symbol == ' ' || symbol == '\t')
        continue;
      const auto found = legend.find(symbol);
      if (found == legend.end()) {
        if (reported.insert(symbol).second)
          ctx.diagnostics().push_back({Diagnostic::Severity::Error, ctx.file(), line,
                                       std::format("character '{}' on the map has no <Legend> entry", symbol)});
        continue;
      }
      const float px = ox + static_cast<float>(c) * cell;
      const float pRow = (is3d ? oz : oy) + static_cast<float>(r) * cell;
      const std::pair<std::string, FieldValue> position[] = {{"x", px}, {is3d ? "z" : "y", pRow}};

      if (!found->second.actor.empty()) {
        const auto &name = found->second.actor;
        if (!placed.insert(name).second) {
          ctx.diagnostics().push_back({Diagnostic::Severity::Error, ctx.file(), line,
                                       std::format("actor '{}' is on the map more than once", name)});
          continue;
        }
        auto &def = *std::find_if(data.actors.begin(), data.actors.end(),
                                  [&](const ActorDef &a) { return a.name == name; });
        def.components.front().values.insert(def.components.front().values.end(), std::begin(position),
                                             std::end(position));
        continue;
      }

      ActorDef def = data.archetypes.at(found->second.archetype);
      def.name = std::format("{}_{}_{}", found->second.archetype, c, r);
      def.archetype = found->second.archetype;
      def.fromTilemap = true;
      def.line = line;
      def.components.front().values.insert(def.components.front().values.end(), std::begin(position),
                                           std::end(position));
      if (std::any_of(data.actors.begin(), data.actors.end(), [&](const ActorDef &a) { return a.name == def.name; })) {
        ctx.diagnostics().push_back({Diagnostic::Severity::Error, ctx.file(), line,
                                     std::format("actor '{}' from the map already exists", def.name)});
        continue;
      }
      data.actors.push_back(std::move(def));
    }
  }
}

} // namespace

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
    diagnostics.push_back({Diagnostic::Severity::Error, file, 1, "missing <Scene> element"});
    return data;
  }
  ctx.check_attributes(root, {"background", "mode", "gui"});
  data.gui = split_words(root.attribute("gui").value());
  data.guiLine = ctx.line_of(root);
  if (root.attribute("background")) {
    Rgba color;
    ctx.read_color(root, "background", color);
    data.background = color;
  }
  const std::string_view mode = root.attribute("mode").as_string("2d");
  if (mode == "3d")
    data.mode = SceneMode::Mode3D;
  else if (mode != "2d")
    ctx.error(root, std::format("mode=\"{}\" – allowed: 2d, 3d", mode));

  auto addActor = [&](pugi::xml_node node) {
    auto def = parse_actor(ctx, node, "Actor", registry, data.mode);
    const bool duplicate = std::any_of(data.actors.begin(), data.actors.end(),
                                       [&](const ActorDef &a) { return a.name == def.name; });
    if (duplicate)
      ctx.error(node, std::format("actor '{}' is already defined", def.name));
    else if (!def.name.empty())
      data.actors.push_back(std::move(def));
  };
  auto addArchetype = [&](pugi::xml_node node) {
    auto def = parse_actor(ctx, node, "Archetype", registry, data.mode);
    if (data.archetypes.contains(def.name))
      ctx.error(node, std::format("archetype '{}' is already defined", def.name));
    else if (!def.name.empty())
      data.archetypes.emplace(def.name, std::move(def));
  };

  std::vector<pugi::xml_node> tilemaps; // expanded last, once all archetypes are known
  for (auto child : root.children()) {
    if (child.type() != pugi::node_element)
      continue;
    const std::string_view tag = child.name();
    if (tag == "Actor") {
      addActor(child);
    } else if (tag == "Tilemap") {
      tilemaps.push_back(child);
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
          ctx.warning(node, std::format("unknown element <{}> in <{}>", node.name(), tag));
      }
    } else if (tag == "Resources") {
      for (auto res : child.children()) {
        if (res.type() != pugi::node_element)
          continue;
        const std::string_view kind = res.name();
        if (kind != "Texture" && kind != "Model") {
          ctx.warning(res, std::format("unknown resource <{}> (available: Texture, Model)", kind));
          continue;
        }
        ctx.check_attributes(res, {"id", "path"});
        Resource resource{res.attribute("id").value(), res.attribute("path").value()};
        if (resource.id.empty() || resource.path.empty())
          ctx.error(res, std::format("<{}> needs id and path attributes", kind));
        else
          (kind == "Texture" ? data.textures : data.models)[resource.id] = resource;
      }
    } else {
      ctx.warning(child, std::format("unknown element <{}> in <Scene> (available: Actor, "
                                     "Archetype, Tilemap, Resources)",
                                     tag));
    }
  }
  for (const auto &tilemap : tilemaps)
    expand_tilemap(ctx, tilemap, data);

  auto checkResources = [&](const ActorDef &def) {
    for (const auto &init : def.components)
      for (const auto &[field, value] : init.values) {
        const auto *id = std::get_if<std::string>(&value);
        if (!id)
          continue;
        if (init.component == "Sprite" && field == "texture" && !data.textures.contains(*id))
          diagnostics.push_back({Diagnostic::Severity::Error, file, init.line,
                                 std::format("'{}' uses texture '{}', which is not in <Resources>",
                                             def.name, *id)});
        if (init.component == "Model" && field == "model" && !data.models.contains(*id))
          diagnostics.push_back({Diagnostic::Severity::Error, file, init.line,
                                 std::format("'{}' uses model '{}', which is not in <Resources>",
                                             def.name, *id)});
      }
  };
  for (const auto &def : data.actors)
    checkResources(def);
  for (const auto &[name, def] : data.archetypes)
    checkResources(def);
  return data;
}

gui::GuiScene parse_gui_xml(std::string_view content, const std::string &file, const std::string &name,
                            Diagnostics &diagnostics) {
  using gui::ElementKind;
  gui::GuiScene scene;
  scene.name = name;
  scene.file = file;
  XmlContext ctx(content, file, diagnostics);
  pugi::xml_document doc;
  if (!ctx.parse(doc))
    return scene;
  const auto root = doc.child("Gui");
  if (!root) {
    diagnostics.push_back({Diagnostic::Severity::Error, file, 1, "missing <Gui> element"});
    return scene;
  }
  ctx.check_attributes(root, {"pause", "dim"});
  ctx.read_bool(root, "pause", scene.pause);
  if (root.attribute("dim")) {
    Rgba color;
    ctx.read_color(root, "dim", color);
    scene.dim = color;
  }

  for (auto node : root.children()) {
    if (node.type() != pugi::node_element)
      continue;
    const std::string_view tag = node.name();
    gui::Element e;
    e.line = ctx.line_of(node);
    if (tag == "Panel") {
      e.kind = ElementKind::Panel;
      e.color = {0, 0, 0, 160};
      ctx.check_attributes(node, {"name", "anchor", "x", "y", "w", "h", "color", "border"});
    } else if (tag == "Label") {
      e.kind = ElementKind::Label;
      ctx.check_attributes(node, {"name", "anchor", "x", "y", "w", "h", "text", "size", "color", "align"});
    } else if (tag == "Button") {
      e.kind = ElementKind::Button;
      e.w = 220;
      e.h = 56;
      e.color = {48, 48, 72, 255};
      e.hoverColor = {74, 74, 112, 255};
      ctx.check_attributes(node, {"name", "anchor", "x", "y", "w", "h", "text", "size", "color", "hover",
                                  "text_color", "border", "click", "key"});
    } else if (tag == "Image") {
      e.kind = ElementKind::Image;
      ctx.check_attributes(node, {"name", "anchor", "x", "y", "w", "h", "src", "color"});
    } else {
      ctx.warning(node, std::format("unknown element <{}> in <Gui> (available: Panel, Label, Button, Image)", tag));
      continue;
    }

    e.name = node.attribute("name").value();
    if (const auto anchor = node.attribute("anchor")) {
      if (auto parsed = gui::anchor_from_name(anchor.value()))
        e.anchor = *parsed;
      else
        ctx.error(node, std::format("anchor=\"{}\" – allowed: top-left, top, top-right, left, center, "
                                    "right, bottom-left, bottom, bottom-right",
                                    anchor.value()));
    }
    ctx.read_float(node, "x", e.x);
    ctx.read_float(node, "y", e.y);
    ctx.read_float(node, "w", e.w);
    ctx.read_float(node, "h", e.h);
    e.text = node.attribute("text").value();
    ctx.read_int(node, "size", e.size);
    ctx.read_color(node, "color", e.color);
    ctx.read_color(node, "hover", e.hoverColor);
    ctx.read_color(node, "text_color", e.textColor);
    if (node.attribute("border")) {
      Rgba color;
      ctx.read_color(node, "border", color);
      e.border = color;
    }
    if (const auto align = node.attribute("align")) {
      const std::string_view value = align.value();
      if (value == "left") e.align = gui::Align::Left;
      else if (value == "center") e.align = gui::Align::Center;
      else if (value == "right") e.align = gui::Align::Right;
      else ctx.error(node, std::format("align=\"{}\" – allowed: left, center, right", value));
    }
    if (e.kind == ElementKind::Image) {
      e.image = node.attribute("src").value();
      if (e.image.empty())
        ctx.error(node, "<Image> needs a src attribute (image file relative to the project)");
    }
    if (e.kind == ElementKind::Button) {
      auto parsed = vm::parse_instructions(node.attribute("click").value(), file, e.line);
      diagnostics.insert(diagnostics.end(), parsed.diagnostics.begin(), parsed.diagnostics.end());
      e.click = std::move(parsed.instructions);
      if (const auto key = node.attribute("key")) {
        e.key = vm::key_from_name(key.value());
        if (!e.key)
          ctx.error(node, std::format("unknown key key=\"{}\"", key.value()));
      }
      if (e.click.empty() && !node.attribute("click"))
        ctx.warning(node, "<Button> without click – clicking it does nothing");
    }
    scene.elements.push_back(std::move(e));
  }
  return scene;
}

} // namespace PimpEx::boot
