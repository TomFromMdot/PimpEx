#include "cli/commands/SceneCommand.hpp"
#include "boot/ProjectPaths.hpp"
#include "boot/XmlLoader.hpp"
#include "cli/Templates.hpp"

#include <iostream>
#include <print>

namespace PimpEx::cli {

CLI::App *SceneCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("scene", "Scenes: a new scene from a template or the template list.");
  sub->require_subcommand(1);
  _new = sub->add_subcommand("new", "Creates scenes/<name>/ from a template (and missing behaviors).");
  _new->add_option("name", _name, "Scene name (letters, digits, _ and -)")->required();
  _new->add_option("--template", _template, "Scene template")->capture_default_str();
  _new->add_option("--project", _project, "Project directory")->capture_default_str();
  _list = sub->add_subcommand("list", "Lists scene templates.");
  return sub;
}

int SceneCommand::run() {
  if (_list->parsed()) {
    for (const auto &t : scene_templates())
      std::println("{:<14} {}", t.name, t.about);
    return 0;
  }

  if (!valid_name(_name)) {
    std::println(std::cerr, "The scene name '{}' may contain only letters, digits, _ and -.", _name);
    return 1;
  }
  const boot::ProjectPaths paths{std::filesystem::absolute(_project).lexically_normal()};
  const auto projectXml = boot::read_file(paths.project_xml());
  if (!projectXml) {
    std::println(std::cerr, "{} is not a PimpEx project (no pimpex.xml).", paths.root.string());
    return 1;
  }
  Diagnostics ignored;
  const auto config = boot::parse_project_xml(*projectXml, "pimpex.xml", ignored);

  const auto result = apply_scene_template(paths.root, _template, _name, config.name);
  if (!result.error.empty()) {
    std::println(std::cerr, "Error: {}", result.error);
    return 1;
  }
  std::println("Scene '{}' from template {}:", _name, _template);
  for (const auto &file : result.created)
    std::println("  + {}", file);
  std::println("Switch to it from .swap: SCENE {}  (or <StartScene name=\"{}\"/> in pimpex.xml)", _name,
               _name);
  return 0;
}

} // namespace PimpEx::cli
