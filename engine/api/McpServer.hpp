#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>

// Serwer MCP (Model Context Protocol) po stdio: `pimpex mcp [projekt]`.
// Agent AI (np. Claude Code) dostaje narzędzia do sprawdzania, uruchamiania
// i sterowania grą – offline oraz na żywo przez API działającego `pimpex dev`.
namespace PimpEx::api {

class McpServer {
  std::filesystem::path _root;

public:
  explicit McpServer(std::filesystem::path projectRoot);

  // Obsługuje jedną wiadomość JSON-RPC. Zwraca odpowiedź albo nullopt
  // (powiadomienia nie dostają odpowiedzi).
  std::optional<nlohmann::json> handle(const nlohmann::json &message);

  // Pętla stdio: wiadomość w linii na wejściu, odpowiedź w linii na wyjściu.
  int run();
};

} // namespace PimpEx::api
