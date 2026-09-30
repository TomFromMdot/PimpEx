#pragma once

#include "cli/Command.hpp"
#include "core/Engine.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex dev  [projekt] – gra z hot-reloadem (tryb tworzenia)
// pimpex play [projekt] – sama gra, bez obserwowania plików
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
