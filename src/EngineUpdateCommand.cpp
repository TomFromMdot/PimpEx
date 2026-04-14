#include "../include/cli_commands/EngineUpdateCommand.h"
#include "CLI/CLI.hpp"
#include <CLI/CLI.hpp>
#include <any>
#include <memory>
#include <string>

void PimpEx::cli::commands::EngineUpdateCommand::setup(std::any hook) {
  auto cli = std::any_cast<std::shared_ptr<CLI::App>>(hook);
  auto sub = cli->add_subcommand("update", "Update engine data");
  sub->require_option(1);
  sub->add_option_function<std::string>(
      "update", [this](const std::string &s) { execute(); });
}

void PimpEx::cli::commands::EngineUpdateCommand::execute() {
  std::cout << "update\n";
}
