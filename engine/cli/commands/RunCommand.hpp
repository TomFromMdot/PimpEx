#pragma once

#include "cli/Command.hpp"
#include "core/Engine.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex dev  [project] – the game with hot-reload (making mode)
// pimpex play [project] – just the game, no file watching
class RunCommand : public ICommand {
  bool _dev;
  std::string _project = ".";
  EngineOptions _options;

public:
  explicit RunCommand(bool dev) : _dev(dev) {
    _options.watch = dev;
    _options.api = dev;
  }

  CLI::App *setup(CLI::App &app) override;
  int run() override;
};

} // namespace PimpEx::cli
