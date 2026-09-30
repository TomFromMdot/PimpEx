#pragma once

#include "cli/Command.hpp"

#include <string>

namespace PimpEx::cli {

// pimpex mcp [projekt] – serwer MCP (stdio) dla agentów AI.
class McpCommand : public ICommand {
  std::string _project = ".";

public:
  CLI::App *setup(CLI::App &app) override;
  int run() override;
};

} // namespace PimpEx::cli
