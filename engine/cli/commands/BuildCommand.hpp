#pragma once

#include "cli/Command.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex build [project] [--out dir] – a package to send to friends:
// the engine binary, the game files and a launcher script.
class BuildCommand : public ICommand {
  std::string _project = ".";
  std::string _out = "dist";

public:
  CLI::App *setup(CLI::App &app) override;
  int run() override;
};

} // namespace PimpEx::cli
