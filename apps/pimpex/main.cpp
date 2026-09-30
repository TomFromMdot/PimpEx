#include "cli/Command.hpp"
#include "cli/commands/CheckCommand.hpp"
#include "cli/commands/InitCommand.hpp"
#include "cli/commands/RunCommand.hpp"

int main(int argc, char **argv) {
  CLI::App app{"PimpEx – silnik gier, w którym wszystko jest tekstem."};
  app.set_version_flag("--version", "PimpEx 0.1.0");
  argv = app.ensure_utf8(argv);

  PimpEx::cli::CommandRegistry commands;
  commands.add(std::make_unique<PimpEx::cli::InitCommand>());
  commands.add(std::make_unique<PimpEx::cli::CheckCommand>());
  commands.add(std::make_unique<PimpEx::cli::RunCommand>(true));
  commands.add(std::make_unique<PimpEx::cli::RunCommand>(false));
  return commands.run(app, argc, argv);
}
