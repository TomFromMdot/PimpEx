#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>

// MCP (Model Context Protocol) server over stdio: `pimpex mcp [project]`.
// Gives AI agents tools to check, run and drive the game – offline and live
// through the API of a running `pimpex dev`.
namespace PimpEx::api {

class McpServer {
  std::filesystem::path _root;

public:
  explicit McpServer(std::filesystem::path projectRoot);

  // nullopt for notifications, which get no response.
  std::optional<nlohmann::json> handle(const nlohmann::json &message);

  // One JSON message per line in, one per line out.
  int run();
};

} // namespace PimpEx::api
