#include "cli/commands/InitCommand.hpp"
#include "cli/Templates.hpp"

#include <filesystem>
#include <iostream>
#include <print>

namespace PimpEx::cli {

CLI::App *InitCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("init", "Creates a new game project from the template.");
  sub->add_option("name", _name, "Project name (letters, digits, _ and -)")->required();
  sub->add_option("--dir", _dir, "Directory to create the project in")->capture_default_str();
  sub->add_option("--template", _template, "Template of the first scene (list: pimpex scene list)")
      ->capture_default_str();
  return sub;
}

int InitCommand::run() {
  namespace fs = std::filesystem;
  if (!valid_name(_name)) {
    std::println(std::cerr, "The project name '{}' may contain only letters, digits, _ and -.", _name);
    return 1;
  }
  if (!scene_template_exists(_template)) {
    std::println(std::cerr, "No template '{}'. Available: pimpex scene list", _template);
    return 1;
  }
  const fs::path root = fs::path(_dir) / _name;
  std::error_code ec;
  if (fs::exists(root, ec) && !fs::is_empty(root, ec)) {
    std::println(std::cerr, "Directory {} already exists and is not empty.", root.string());
    return 1;
  }

  for (const auto &result : {apply_project_template(root, _name),
                             apply_scene_template(root, _template, "main", _name)})
    if (!result.error.empty()) {
      std::println(std::cerr, "Error: {}", result.error);
      return 1;
    }

  std::println("Project '{}' ready in {} (scene main from template {})", _name, root.string(), _template);
  std::println("  pimpex check {}   – check the files", root.string());
  std::println("  pimpex dev {}     – run with hot-reload", root.string());
  return 0;
}

} // namespace PimpEx::cli
