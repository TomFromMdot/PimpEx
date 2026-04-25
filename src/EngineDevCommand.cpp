#include "../include/cli_commands/EngineDevCommand.h"
#include "../include/EnigneCore.h"
#include <CLI/CLI.hpp>
#include <any>
#include <memory>

void PimpEx::cli::commands::EngineDevCommand::setup(std::any hook) {
  auto cli = std::any_cast<std::shared_ptr<CLI::App>>(hook);
  auto sub = cli->add_subcommand("dev");
  sub->add_option_function<std::string>(
      "dev", [this](const std::string &f) { execute(); });
}

void PimpEx::cli::commands::EngineDevCommand::execute() {
  PimpEx::EngineCore core;
}
