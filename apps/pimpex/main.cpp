#include "cli/Command.hpp"

int main(int argc, char **argv) {
  CLI::App app{"PimpEx – silnik gier, w którym wszystko jest tekstem."};
  app.set_version_flag("--version", "PimpEx 0.1.0");
  argv = app.ensure_utf8(argv);

  PimpEx::cli::CommandRegistry commands;
  return commands.run(app, argc, argv);
}
