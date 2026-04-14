#include "../include/cli_commands/EngineInitCommand.h"
#include "CLI/CLI.hpp"
#include <any>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <vector>

void PimpEx::cli::commands::EngineInitCommand::create_project_structure(
    const std::string &project_name) {
  namespace fs = std::filesystem;
  try {

    std::filesystem::path path =
        std::format("{}{}/", _dir.c_str(), _name.c_str());
    fs::path scene = path / "scene";
    fs::path engineConfig = path / "config";
    fs::path scripts = scene / "scripts";
    fs::path resources = scene / "resources";

    fs::create_directories(scripts);
    fs::create_directories(resources);
    fs::create_directories(engineConfig);

    std::vector<fs::path> files = {path / "pimpex.xml", scene / "main.cpp",
                                   scene / "main.h", scene / "scene.xml"};

    for (const auto &filepath : files) {
      if (!fs::exists(filepath)) {
        std::ofstream file(filepath);
      }
    }

  } catch (const fs::filesystem_error &e) {
    std::cerr << "Błąd systemu plików" << e.what() << "\n";
  }
}

void PimpEx::cli::commands::EngineInitCommand::setup(std::any hook) {
  auto cli = std::any_cast<std::shared_ptr<CLI::App>>(hook);

  auto sub = cli->add_subcommand("init", "Init new project.");
  sub->require_option(1);
  sub->add_option_function<std::string>("init", [this](const std::string &f) {
    _name = f;
    execute();
  });
}

void PimpEx::cli::commands::EngineInitCommand::execute() {

  if (_name.empty())
    return;
  create_project_structure(_name);
}
