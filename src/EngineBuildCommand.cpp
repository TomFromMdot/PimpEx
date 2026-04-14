#include "../include/cli_commands/EngineBuildCommand.h"
#include "CLI/CLI.hpp"
#include <CLI/CLI.hpp>
#include <any>
#include <memory>

void PimpEx::cli::commands::EngineBuildCommand::setup(std::any hook) {
  auto cli = std::any_cast<std::shared_ptr<CLI::App>>(hook);
  auto sub = cli->add_subcommand("build", "Build and release project.");
  sub->require_option(1);
  sub->add_option_function<std::string>(
      "build", [this](const std::string &f) { execute(); });
}

void PimpEx::cli::commands::EngineBuildCommand::execute() {
  std::cout << "build\n";
}
