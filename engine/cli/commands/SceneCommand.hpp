#pragma once

#include "cli/Command.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex scene new <nazwa> [--template szablon] [--project katalog]
// pimpex scene list
class SceneCommand : public ICommand {
  CLI::App *_new = nullptr;
  CLI::App *_list = nullptr;
  std::string _name;
  std::string _template = "empty2d";
  std::string _project = ".";

public:
  CLI::App *setup(CLI::App &app) override;
  int run() override;
};

} // namespace PimpEx::cli
