#include "cli/commands/CheckCommand.hpp"
#include "boot/ProjectCheck.hpp"

#include <iostream>
#include <print>

namespace PimpEx::cli {

CLI::App *CheckCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("check", "Checks the project files (XML, .swap, Lua, GUI) without running the game.");
  sub->add_option("project", _project, "Project directory")->capture_default_str();
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
    std::println(std::cerr, "Errors: {}, warnings: {}.", errors, warnings);
    return 1;
  }
  std::println("OK – scenes: {}, procs: {}, actors: {}, scripts: {}, GUI: {}, warnings: {}.",
               summary.scenes, summary.procs, summary.actors, summary.scripts, summary.guis, warnings);
  return 0;
}

} // namespace PimpEx::cli
