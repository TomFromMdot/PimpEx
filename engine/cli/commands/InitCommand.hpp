#pragma once

#include "cli/Command.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex init <name> [--dir dir] [--template template]
class InitCommand : public ICommand {
  std::string _name;
  std::string _dir = ".";
  std::string _template = "starter2d";

public:
  CLI::App *setup(CLI::App &app) override;
  int run() override;
};

} // namespace PimpEx::cli
