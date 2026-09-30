#pragma once

#include "cli/Command.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex build [projekt] [--out katalog] – pakuje grę do wysłania znajomym:
// binarka silnika + pliki gry + skrypt uruchamiający.
class BuildCommand : public ICommand {
  std::string _project = ".";
  std::string _out = "dist";

public:
  CLI::App *setup(CLI::App &app) override;
  int run() override;
};

} // namespace PimpEx::cli
