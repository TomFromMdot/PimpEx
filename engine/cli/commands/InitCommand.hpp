#pragma once

#include "cli/Command.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex init <nazwa> [--dir katalog] – tworzy nowy projekt gry z szablonu.
class InitCommand : public ICommand {
  std::string _name;
  std::string _dir = ".";

public:
  CLI::App *setup(CLI::App &app) override;
  int run() override;
};

} // namespace PimpEx::cli
