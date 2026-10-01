#include "cli/Command.hpp"
#include "cli/commands/BuildCommand.hpp"
#include "cli/commands/CheckCommand.hpp"
#include "cli/commands/ComponentsCommand.hpp"
#include "cli/commands/InitCommand.hpp"
#include "cli/commands/McpCommand.hpp"
#include "cli/commands/RunCommand.hpp"
#include "cli/commands/SceneCommand.hpp"

int main(int argc, char **argv) {
  CLI::App app{"PimpEx – a game engine where everything is text."};
  app.set_version_flag("--version", "PimpEx 0.2.0");
  argv = app.ensure_utf8(argv);

  PimpEx::cli::CommandRegistry commands;
  commands.add(std::make_unique<PimpEx::cli::InitCommand>());
  commands.add(std::make_unique<PimpEx::cli::CheckCommand>());
  commands.add(std::make_unique<PimpEx::cli::RunCommand>(true));
  commands.add(std::make_unique<PimpEx::cli::RunCommand>(false));
  commands.add(std::make_unique<PimpEx::cli::SceneCommand>());
  commands.add(std::make_unique<PimpEx::cli::ComponentsCommand>());
  commands.add(std::make_unique<PimpEx::cli::BuildCommand>());
  commands.add(std::make_unique<PimpEx::cli::McpCommand>());
  return commands.run(app, argc, argv);
}
