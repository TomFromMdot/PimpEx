#include "cli/commands/ComponentsCommand.hpp"
#include "boot/ProjectPaths.hpp"
#include "core/Format.hpp"
#include "scene/ComponentDocs.hpp"

#include <iostream>
#include <print>

namespace PimpEx::cli {

CLI::App *ComponentsCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("components", "Lista komponentów: wbudowane i z components.xml projektu.");
  sub->add_option("project", _project, "Katalog projektu (opcjonalnie – dla components.xml)");
  sub->add_flag("--markdown", _markdown, "Pełny opis w Markdown (docs/components.md)");
  return sub;
}

int ComponentsCommand::run() {
  auto registry = scene::ComponentRegistry::with_builtins();
  if (!_project.empty()) {
    const boot::ProjectPaths paths{std::filesystem::absolute(_project).lexically_normal()};
    if (const auto xml = boot::read_file(paths.components_xml())) {
      Diagnostics diagnostics;
      registry->load_custom(*xml, "components.xml", diagnostics);
      for (const auto &d : diagnostics)
        std::println(std::cerr, "{}", d.format());
      if (has_errors(diagnostics))
        return 1;
    }
  }
  if (_markdown) {
    std::print("{}", scene::components_markdown(*registry));
    return 0;
  }
  for (const auto &c : registry->all()) {
    std::println("<{}> – {}", c.name, c.doc);
    for (const auto &f : c.fields) {
      auto def = scene::field_value_to_string(f.defaultValue);
      std::println("    {:<10} {:<18} = {:<8} {}", f.name, scene::field_type_name(f.type),
                   def.empty() ? "\"\"" : def, f.doc);
    }
  }
  return 0;
}

} // namespace PimpEx::cli
