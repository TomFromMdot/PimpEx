#pragma once
#include "../interfaces/IEngineCommand.h"
#include "CLI/CLI.hpp"

#include <CLI/CLI.hpp>
#include <any>
#include <filesystem>
#include <memory>
#include <string>

namespace PimpEx::cli::commands {
class EngineInitCommand : public interfaces::IEngineCommand {
  std::string _name;
  std::filesystem::path _dir;
  std::weak_ptr<CLI::App> _cli;

  void create_project_structure(const std::string &project_name);

public:
  EngineInitCommand(const std::filesystem::path &pathToProject)
      : _dir(pathToProject) {}

  void setup(std::any hook) override;
  void execute() override;
};
}; // namespace PimpEx::cli::commands
