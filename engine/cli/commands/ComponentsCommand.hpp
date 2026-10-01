#pragma once

#include "cli/Command.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex components [project] [--markdown]
class ComponentsCommand : public ICommand {
  std::string _project;
  bool _markdown = false;

public:
  CLI::App *setup(CLI::App &app) override;
  int run() override;
};

} // namespace PimpEx::cli
