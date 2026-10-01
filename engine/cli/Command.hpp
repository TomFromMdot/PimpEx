#pragma once

#include <CLI/CLI.hpp>

#include <memory>
#include <vector>

namespace PimpEx::cli {

// Command pattern: every CLI command (init, check, dev...) is its own class.
class ICommand {
public:
  virtual ~ICommand() = default;
  virtual CLI::App *setup(CLI::App &app) = 0;
  // Returns the process exit code.
  virtual int run() = 0;
};

class CommandRegistry {
  struct Entry {
    std::unique_ptr<ICommand> command;
    CLI::App *subcommand = nullptr;
  };
  std::vector<Entry> _entries;

public:
  void add(std::unique_ptr<ICommand> command) {
    _entries.push_back({std::move(command), nullptr});
  }

  int run(CLI::App &app, int argc, char **argv) {
    for (auto &entry : _entries)
      entry.subcommand = entry.command->setup(app);
    app.require_subcommand(1);
    try {
      app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
      return app.exit(e);
    }
    for (auto &entry : _entries)
      if (entry.subcommand && entry.subcommand->parsed())
        return entry.command->run();
    return 0;
  }
};

} // namespace PimpEx::cli
