#pragma once

#include "cli/Command.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex check [project]
class CheckCommand : public ICommand {
  std::string _project = ".";

public:
  CLI::App *setup(CLI::App &app) override;
  int run() override;
};

} // namespace PimpEx::cli
