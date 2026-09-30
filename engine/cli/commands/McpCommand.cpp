#include "cli/commands/McpCommand.hpp"
#include "api/McpServer.hpp"

#include <filesystem>

namespace PimpEx::cli {

CLI::App *McpCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("mcp", "Serwer MCP (stdio) dla agentów AI – narzędzia do sprawdzania, "
                                        "uruchamiania i sterowania grą.");
  sub->add_option("project", _project, "Katalog projektu")->capture_default_str();
  return sub;
}

int McpCommand::run() {
  api::McpServer server(std::filesystem::absolute(_project).lexically_normal());
  return server.run();
}

} // namespace PimpEx::cli
