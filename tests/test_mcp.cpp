#include "api/McpServer.hpp"

#include <doctest/doctest.h>

#include <fstream>

using nlohmann::json;

TEST_CASE("MCP: initialize, tools/list, tools/call") {
  const auto root = std::filesystem::temp_directory_path() / "pimpex_test_mcp";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root / "scenes/main");
  std::ofstream(root / "pimpex.xml") << R"(<Project name="t"><StartScene name="main"/></Project>)";
  std::ofstream(root / "scenes/main/scene.xml") << R"(<Scene><Actor name="P" x="0" vx="2"/></Scene>)";
  std::ofstream(root / "scenes/main/scene.swap") << "PROC a\n  IF KEY RIGHT : MOVE P 1 0\nEND\n";

  PimpEx::api::McpServer server(root);
  const auto init = server.handle({{"jsonrpc", "2.0"}, {"id", 1}, {"method", "initialize"},
                                   {"params", {{"protocolVersion", "2025-03-26"}}}});
  REQUIRE(init);
  CHECK((*init)["result"]["protocolVersion"] == "2025-03-26");
  CHECK((*init)["result"]["serverInfo"]["name"] == "pimpex");
  CHECK_FALSE(server.handle({{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}}));

  const auto list = server.handle({{"jsonrpc", "2.0"}, {"id", 2}, {"method", "tools/list"}});
  CHECK((*list)["result"]["tools"].size() == 16);

  auto call = [&](const char *name, json args) {
    return (*server.handle({{"jsonrpc", "2.0"}, {"id", 3}, {"method", "tools/call"},
                            {"params", {{"name", name}, {"arguments", std::move(args)}}}}))["result"];
  };
  const auto check = call("pimpex_check", json::object());
  CHECK(check["isError"] == false);
  CHECK(check["content"][0]["text"].get<std::string>().starts_with("OK"));

  const auto run = call("pimpex_run", {{"frames", 10}, {"input", "RIGHT:0-4"}});
  CHECK(run["isError"] == false);
  CHECK(run["content"][0]["text"].get<std::string>().find("P x=25 y=0") != std::string::npos); // 10*2 + 5
  CHECK_FALSE(std::filesystem::exists(root / ".pimpex/state.txt")); // osobny katalog mcp-run

  const auto docs = call("pimpex_docs", {{"topic", "components"}});
  CHECK(docs["content"][0]["text"].get<std::string>().find("### `<Camera>`") != std::string::npos);

  const auto live = call("live_state", json::object());
  CHECK(live["isError"] == true); // gra nie działa
  CHECK(live["content"][0]["text"].get<std::string>().find("pimpex dev") != std::string::npos);

  const auto scene = call("pimpex_scene_new", {{"name", "poziom2"}, {"template", "platformer2d"}});
  CHECK(std::filesystem::exists(root / "scenes/poziom2/scene.xml"));
  CHECK(std::filesystem::exists(root / "scripts/behaviors/platformer_move.lua"));

  const auto unknown = server.handle({{"jsonrpc", "2.0"}, {"id", 9}, {"method", "nope"}});
  CHECK((*unknown)["error"]["code"] == -32601);
  std::filesystem::remove_all(root);
}
