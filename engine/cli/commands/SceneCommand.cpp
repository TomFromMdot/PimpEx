#include "cli/commands/SceneCommand.hpp"
#include "boot/ProjectPaths.hpp"
#include "boot/XmlLoader.hpp"
#include "cli/Templates.hpp"

#include <iostream>
#include <print>

namespace PimpEx::cli {

CLI::App *SceneCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("scene", "Sceny: nowa scena z szablonu albo lista szablonów.");
  sub->require_subcommand(1);
  _new = sub->add_subcommand("new", "Tworzy scenes/<nazwa>/ z szablonu (i brakujące zachowania).");
  _new->add_option("name", _name, "Nazwa sceny (litery, cyfry, _ i -)")->required();
  _new->add_option("--template", _template, "Szablon sceny")->capture_default_str();
  _new->add_option("--project", _project, "Katalog projektu")->capture_default_str();
  _list = sub->add_subcommand("list", "Lista szablonów scen.");
  return sub;
}

int SceneCommand::run() {
  if (_list->parsed()) {
    for (const auto &t : scene_templates())
      std::println("{:<14} {}", t.name, t.about);
    return 0;
  }

  if (!valid_name(_name)) {
    std::println(std::cerr, "Nazwa sceny '{}' może zawierać tylko litery, cyfry, _ i -.", _name);
    return 1;
  }
  const boot::ProjectPaths paths{std::filesystem::absolute(_project).lexically_normal()};
  const auto projectXml = boot::read_file(paths.project_xml());
  if (!projectXml) {
    std::println(std::cerr, "{} nie jest projektem PimpEx (brak pimpex.xml).", paths.root.string());
    return 1;
  }
  Diagnostics ignored;
  const auto config = boot::parse_project_xml(*projectXml, "pimpex.xml", ignored);

  const auto result = apply_scene_template(paths.root, _template, _name, config.name);
  if (!result.error.empty()) {
    std::println(std::cerr, "Błąd: {}", result.error);
    return 1;
  }
  std::println("Scena '{}' z szablonu {}:", _name, _template);
  for (const auto &file : result.created)
    std::println("  + {}", file);
  std::println("Przełącz na nią z .swap: SCENE {}  (albo <StartScene name=\"{}\"/> w pimpex.xml)", _name,
               _name);
  return 0;
}

} // namespace PimpEx::cli
