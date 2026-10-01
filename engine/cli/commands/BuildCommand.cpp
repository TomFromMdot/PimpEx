#include "cli/commands/BuildCommand.hpp"
#include "boot/ProjectCheck.hpp"
#include "boot/XmlLoader.hpp"

#include <fstream>
#include <iostream>
#include <print>

namespace PimpEx::cli {
namespace {

namespace fs = std::filesystem;

// Copies the project without .pimpex (logs and state dumps).
void copy_project(const fs::path &from, const fs::path &to) {
  for (auto it = fs::recursive_directory_iterator(from); it != fs::recursive_directory_iterator();
       ++it) {
    const auto rel = it->path().lexically_relative(from);
    if (rel.begin()->string() == ".pimpex") {
      it.disable_recursion_pending();
      continue;
    }
    if (it->is_directory())
      fs::create_directories(to / rel);
    else if (it->is_regular_file())
      fs::copy_file(it->path(), to / rel, fs::copy_options::overwrite_existing);
  }
}

} // namespace

CLI::App *BuildCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("build", "Packages the game with the engine into --out.");
  sub->add_option("project", _project, "Project directory")->capture_default_str();
  sub->add_option("--out", _out, "Output directory")->capture_default_str();
  return sub;
}

int BuildCommand::run() {
  const boot::ProjectPaths paths{fs::absolute(_project).lexically_normal()};
  boot::CheckSummary summary;
  const auto diagnostics = boot::check_project(paths, summary);
  for (const auto &d : diagnostics)
    std::println(std::cerr, "{}", d.format());
  if (has_errors(diagnostics)) {
    std::println(std::cerr, "Fix the errors first – a game with errors is not packaged.");
    return 1;
  }

  Diagnostics ignored;
  const auto config = boot::parse_project_xml(boot::read_file(paths.project_xml()).value_or(""),
                                              "pimpex.xml", ignored);
  const auto name = config.name.empty() ? paths.root.filename().string() : config.name;
  const fs::path target = fs::absolute(_out) / name;

  std::error_code ec;
  const auto self = fs::read_symlink("/proc/self/exe", ec);
  if (ec) {
    std::println(std::cerr, "Cannot find the engine binary: {}", ec.message());
    return 1;
  }
  if (target.lexically_relative(paths.root).begin()->string() != "..") {
    std::println(std::cerr, "--out cannot be inside the project.");
    return 1;
  }

  try {
    fs::remove_all(target);
    fs::create_directories(target / "game");
    copy_project(paths.root, target / "game");
    fs::copy_file(self, target / "pimpex", fs::copy_options::overwrite_existing);

    const auto launcher = target / "play.sh";
    std::ofstream(launcher) << "#!/bin/sh\n"
                               "# Starts the game " << name << "\n"
                               "cd \"$(dirname \"$0\")\" && exec ./pimpex play game \"$@\"\n";
    fs::permissions(launcher,
                    fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add);
  } catch (const fs::filesystem_error &e) {
    std::println(std::cerr, "Packaging failed: {}", e.what());
    return 1;
  }

  std::println("Done: {}", target.string());
  std::println("  {}/play.sh – start the game", target.string());
  return 0;
}

} // namespace PimpEx::cli
