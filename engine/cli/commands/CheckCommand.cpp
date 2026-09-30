#include "cli/commands/CheckCommand.hpp"
#include "boot/ProjectCheck.hpp"

#include <iostream>
#include <print>

namespace PimpEx::cli {

CLI::App *CheckCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("check", "Sprawdza pliki projektu (XML, .swap, Lua) bez uruchamiania gry.");
  sub->add_option("project", _project, "Katalog projektu")->capture_default_str();
  return sub;
}

int CheckCommand::run() {
  boot::ProjectPaths paths{std::filesystem::absolute(_project).lexically_normal()};
  boot::CheckSummary summary;
  const auto diagnostics = boot::check_project(paths, summary);

  int errors = 0, warnings = 0;
  for (const auto &d : diagnostics) {
    std::println(std::cerr, "{}", d.format());
    d.severity == Diagnostic::Severity::Error ? ++errors : ++warnings;
  }
  if (errors > 0) {
    std::println(std::cerr, "Znaleziono błędy: {}, ostrzeżenia: {}.", errors, warnings);
    return 1;
  }
  std::println("OK – sceny: {}, procedury: {}, aktorzy: {}, skrypty: {}, ostrzeżenia: {}.",
               summary.scenes, summary.procs, summary.actors, summary.scripts, warnings);
  return 0;
}

} // namespace PimpEx::cli
