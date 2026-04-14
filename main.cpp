#include <iostream>
#include <memory>
#include <unistd.h>

#include "./include/EngineCLI.h"
#include "include/cli_commands/EngineBuildCommand.h"
#include "include/cli_commands/EngineInitCommand.h"
#include "include/cli_commands/EngineUpdateCommand.h"

constexpr std::string pimpexDataPath = "./data/";

auto init_commands(PimpEx::cli::EngineCommandBuffer &cBuffer) -> void {
  cBuffer.init_commands();
}
int main(int argc, char **argv) {
  std::cout << "Hello world\n";
  std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>();
  argv = app->ensure_utf8(argv);
  app->require_subcommand(1);
  PimpEx::cli::EngineCommandBuffer cliCommandBuffer(app);

  cliCommandBuffer.register_command(
      PimpEx::cli::EngineCommandBuffer::Command::Init,
      std::make_shared<PimpEx::cli::commands::EngineInitCommand>(
          pimpexDataPath));

  cliCommandBuffer.register_command(
      PimpEx::cli::EngineCommandBuffer::Command::Update,
      std::make_shared<PimpEx::cli::commands::EngineUpdateCommand>());

  cliCommandBuffer.register_command(
      PimpEx::cli::EngineCommandBuffer::Command::Build,
      std::make_shared<PimpEx::cli::commands::EngineBuildCommand>());

  app->name("PimpEx - The Game Engine");
  cliCommandBuffer.init_commands();
  CLI11_PARSE(*app, argc, argv);
  return 0;
}
