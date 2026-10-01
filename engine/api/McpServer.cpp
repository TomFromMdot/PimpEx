#include "api/McpServer.hpp"
#include "boot/ProjectCheck.hpp"
#include "cli/EmbeddedTemplate.hpp"
#include "cli/Templates.hpp"
#include "core/Engine.hpp"
#include "scene/ComponentDocs.hpp"

#include <httplib.h>

#include <cstdio>
#include <format>
#include <iostream>
#include <unistd.h>

namespace PimpEx::api {
namespace {

namespace fs = std::filesystem;
using json = nlohmann::json;

constexpr const char *ServerVersion = "0.2.0";
constexpr const char *LatestProtocol = "2025-06-18";

// Returned to the agent as isError, not as a protocol error.
struct ToolError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

json text_content(const std::string &text) { return {{"type", "text"}, {"text", text}}; }

json schema(json properties = json::object(), std::vector<std::string> required = {}) {
  json s = {{"type", "object"}, {"properties", std::move(properties)}};
  if (!required.empty())
    s["required"] = required;
  return s;
}

json prop(const char *type, const char *description) {
  return {{"type", type}, {"description", description}};
}

json tools_list() {
  json tools = json::array();
  auto add = [&](const char *name, const char *description, json inputSchema) {
    tools.push_back({{"name", name}, {"description", description}, {"inputSchema", std::move(inputSchema)}});
  };

  add("pimpex_check",
      "Checks the whole PimpEx project (scene.xml, scene.swap, Lua, components.xml, gui/) "
      "without running it. Errors come as file:line: message. Use after every file change.",
      schema());
  add("pimpex_run",
      "Runs the game without a window for N frames (60 = 1 s) with scripted input and returns "
      "the state dump (actors, fields, procs) and the log. Does not change project files.",
      schema({{"frames", prop("integer", "frame count (default 120)")},
              {"input", prop("string", "keys: KEY:from-to or KEY:frame, clicks: CLICK@x/y:frame, e.g. \"RIGHT:0-60 SPACE:10 CLICK@400/300:20\"")},
              {"scene", prop("string", "start scene (default from pimpex.xml)")}}));
  add("pimpex_docs",
      "PimpEx engine docs. Topics: opcodes (the .swap language), components (ECS components, "
      "including the project's components.xml), behaviors (Lua behaviors and systems), lua-api, "
      "scene-xml (scene files, 3D), gui (menus, pause, buttons), api (live API).",
      schema({{"topic", {{"type", "string"},
                         {"enum", {"opcodes", "components", "behaviors", "lua-api", "scene-xml", "gui", "api"}},
                         {"description", "topic"}}}},
             {"topic"}));
  add("pimpex_scene_new",
      "Creates scenes/<name>/ from a template: empty2d, topdown2d, platformer2d, empty3d, "
      "arena3d, starter2d. Adds missing behaviors to scripts/behaviors/.",
      schema({{"name", prop("string", "scene name (letters, digits, _ and -)")},
              {"template", prop("string", "template (default empty2d)")}},
             {"name"}));

  const char *live = " Needs a running game: `pimpex dev .` (or `pimpex dev . --headless`).";
  auto liveDesc = [&](const char *text) { return std::string(text) + live; };
  add("live_state", liveDesc("State of the running game: scene, frame, procs and actors with components.").c_str(),
      schema({{"selector", prop("string", "filter: Component (e.g. Health) or @tag (e.g. @enemy)")},
              {"brief", prop("boolean", "names, positions and component lists only")}}));
  add("live_actor", liveDesc("All components, variables, tags and behaviors of one actor.").c_str(),
      schema({{"name", prop("string", "actor name")}}, {"name"}));
  add("live_set",
      liveDesc("Sets an actor field in the running game: x, vx, Health.hp, Rect.color, Text.value, a variable.").c_str(),
      schema({{"name", prop("string", "actor")},
              {"field", prop("string", "field")},
              {"value", {{"description", "number, text or true/false"}}}},
             {"name", "field", "value"}));
  add("live_spawn", liveDesc("Creates an actor from an archetype.").c_str(),
      schema({{"archetype", prop("string", "archetype from scene.xml")},
              {"name", prop("string", "name (optional)")},
              {"x", prop("number", "x")},
              {"y", prop("number", "y")},
              {"z", prop("number", "z (3D)")}},
             {"archetype"}));
  add("live_destroy", liveDesc("Removes an actor.").c_str(), schema({{"name", prop("string", "actor")}}, {"name"}));
  add("live_swap_paste",
      liveDesc("Pastes .swap code (PROC ... END) into the running scene; a PROC with the same name is "
               "replaced. PROC name ONCE runs once and disappears.")
          .c_str(),
      schema({{"source", prop("string", "the .swap code")}}, {"source"}));
  add("live_press", liveDesc("Holds keys for N frames (e.g. [\"SPACE\"], [\"LEFT\",\"UP\"]).").c_str(),
      schema({{"keys", {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", "keys"}}},
              {"frames", prop("integer", "frames to hold (default 1)")}},
             {"keys"}));
  add("live_control", liveDesc("Game control: pause, resume, step (with frames), quit.").c_str(),
      schema({{"action", {{"type", "string"}, {"enum", {"pause", "resume", "step", "quit"}}}},
              {"frames", prop("integer", "for step: frame count")}},
             {"action"}));
  add("live_screenshot", liveDesc("Screenshot of the running game (needs a window, not --headless).").c_str(),
      schema());
  add("live_log", liveDesc("Last lines of the game log (errors, LOG, .swap changes).").c_str(),
      schema({{"lines", prop("integer", "line count (default 40)")}}));
  add("live_lua", liveDesc("Runs Lua code in the running game and returns the result, e.g. get(\"Player\",\"x\").").c_str(),
      schema({{"code", prop("string", "Lua code")}}, {"code"}));
  add("live_scene", liveDesc("Switches the running game to another scene.").c_str(),
      schema({{"name", prop("string", "scene")}}, {"name"}));
  return tools;
}

json live_call(const fs::path &root, const std::string &method, json params) {
  const auto info = boot::read_file(root / ".pimpex" / "api.json");
  if (!info)
    throw ToolError("The game is not running – start `pimpex dev .` (or `pimpex dev . --headless`) "
                    "in the project directory.");
  json api;
  try {
    api = json::parse(*info);
  } catch (const json::parse_error &) {
    throw ToolError("Broken .pimpex/api.json – restart the game.");
  }
  httplib::Client client("127.0.0.1", api.value("port", 0));
  client.set_connection_timeout(2);
  client.set_read_timeout(15);
  const json request = {{"jsonrpc", "2.0"}, {"id", 1}, {"method", method}, {"params", std::move(params)}};
  const auto response = client.Post("/rpc", {{"Authorization", "Bearer " + api.value("token", "")}},
                                    request.dump(), "application/json");
  if (!response)
    throw ToolError("Cannot connect to the game (was it closed?) – start `pimpex dev .`.");
  const auto body = json::parse(response->body);
  if (body.contains("error"))
    throw ToolError(body["error"].value("message", "API error"));
  return body["result"];
}

std::string doc_text(const fs::path &root, const std::string &topic) {
  if (topic == "components") {
    auto registry = scene::ComponentRegistry::with_builtins();
    if (const auto xml = boot::read_file(root / "components.xml")) {
      Diagnostics ignored;
      registry->load_custom(*xml, "components.xml", ignored);
    }
    return scene::components_markdown(*registry);
  }
  const auto key = std::format("project/docs/{}.md", topic);
  for (const auto &file : cli::embedded_template())
    if (file.path == key)
      return std::string(file.content);
  throw ToolError(std::format("no topic '{}'", topic));
}

json call_tool(const fs::path &root, const std::string &name, const json &args) {
  if (name == "pimpex_check") {
    boot::CheckSummary summary;
    const auto diagnostics = boot::check_project(boot::ProjectPaths{root, {}}, summary);
    std::string text;
    for (const auto &d : diagnostics)
      text += d.format() + "\n";
    text += has_errors(diagnostics)
                ? "There are errors – fix them and check again."
                : std::format("OK – scenes: {}, procs: {}, actors: {}, scripts: {}, GUI: {}, user components: {}.",
                              summary.scenes, summary.procs, summary.actors, summary.scripts, summary.guis,
                              summary.components);
    return {{"content", {text_content(text)}}, {"isError", has_errors(diagnostics)}};
  }
  if (name == "pimpex_run") {
    EngineOptions options;
    options.headless = true;
    options.watch = false;
    options.api = false;
    options.quiet = true;
    options.frames = args.value("frames", 120L);
    options.input = args.value("input", "");
    options.scene = args.value("scene", "");
    options.stateEvery = 0;
    // A separate directory – do not mix with the log of a game running in `pimpex dev`.
    options.runtimeDir = root / ".pimpex" / "mcp-run";
    const int code = Engine(root, options).run();
    const auto state = boot::read_file(options.runtimeDir / "state.txt").value_or("(no state.txt)");
    const auto log = boot::read_file(options.runtimeDir / "runtime.log").value_or("");
    return {{"content", {text_content(std::format("{}\n--- runtime.log ---\n{}", state, log))}},
            {"isError", code != 0}};
  }
  if (name == "pimpex_docs")
    return {{"content", {text_content(doc_text(root, args.value("topic", "")))}}};
  if (name == "pimpex_scene_new") {
    const auto sceneName = args.value("name", "");
    if (!cli::valid_name(sceneName))
      throw ToolError("a scene name may contain only letters, digits, _ and -");
    const auto result =
        cli::apply_scene_template(root, args.value("template", "empty2d"), sceneName, root.filename().string());
    if (!result.error.empty())
      throw ToolError(result.error);
    std::string text = std::format("Scene '{}' created:\n", sceneName);
    for (const auto &file : result.created)
      text += "  + " + file + "\n";
    return {{"content", {text_content(text)}}};
  }

  auto live = [&](const std::string &method, json params) {
    return json{{"content", {text_content(live_call(root, method, std::move(params)).dump(2))}}};
  };
  if (name == "live_state")
    return live("state.get", {{"selector", args.value("selector", "")}, {"brief", args.value("brief", false)}});
  if (name == "live_actor")
    return live("actor.get", {{"name", args.value("name", "")}});
  if (name == "live_set")
    return live("actor.set", {{"name", args.value("name", "")}, {"field", args.value("field", "")},
                              {"value", args.value("value", json(nullptr))}});
  if (name == "live_spawn") {
    json params = {{"archetype", args.value("archetype", "")}};
    for (const char *key : {"name", "x", "y", "z"})
      if (args.contains(key))
        params[key] = args[key];
    return live("actor.spawn", params);
  }
  if (name == "live_destroy")
    return live("actor.destroy", {{"name", args.value("name", "")}});
  if (name == "live_swap_paste")
    return live("swap.paste", {{"source", args.value("source", "")}});
  if (name == "live_press")
    return live("input.press", {{"keys", args.value("keys", json::array())}, {"frames", args.value("frames", 1)}});
  if (name == "live_control") {
    const auto action = args.value("action", "");
    if (action == "step")
      return live("game.step", {{"frames", args.value("frames", 1)}});
    if (action != "pause" && action != "resume" && action != "quit")
      throw ToolError("action: pause, resume, step or quit");
    return live("game." + action, json::object());
  }
  if (name == "live_screenshot") {
    const auto result = live_call(root, "screenshot", json::object());
    return {{"content", {{{"type", "image"}, {"data", result.value("png_base64", "")}, {"mimeType", "image/png"}}}}};
  }
  if (name == "live_log") {
    const auto lines = live_call(root, "log.tail", {{"lines", args.value("lines", 40)}});
    std::string text;
    for (const auto &line : lines)
      text += line.get<std::string>() + "\n";
    return {{"content", {text_content(text.empty() ? "(pusto)" : text)}}};
  }
  if (name == "live_lua")
    return live("lua.eval", {{"code", args.value("code", "")}});
  if (name == "live_scene")
    return live("scene.load", {{"name", args.value("name", "")}});
  throw ToolError(std::format("unknown tool '{}'", name));
}

} // namespace

McpServer::McpServer(fs::path projectRoot) : _root(std::move(projectRoot)) {}

std::optional<json> McpServer::handle(const json &message) {
  if (!message.contains("id"))
    return std::nullopt; // a notification (e.g. notifications/initialized)
  const auto id = message["id"];
  const auto method = message.value("method", "");
  const auto params = message.value("params", json::object());
  auto result = [&](json value) { return json{{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(value)}}; };

  if (method == "initialize") {
    const auto requested = params.value("protocolVersion", LatestProtocol);
    const bool known = requested == "2025-06-18" || requested == "2025-03-26" || requested == "2024-11-05";
    return result({{"protocolVersion", known ? requested : LatestProtocol},
                   {"capabilities", {{"tools", json::object()}}},
                   {"serverInfo", {{"name", "pimpex"}, {"version", ServerVersion}}},
                   {"instructions",
                    "PimpEx: a game engine where the game is text files (scene.xml, scene.swap, Lua, gui/). "
                    "Start with pimpex_docs (opcodes, components, behaviors, gui). After file changes: "
                    "pimpex_check, then pimpex_run with input to test the gameplay. If the game runs "
                    "in `pimpex dev`, the live_* tools read and change it live."}});
  }
  if (method == "ping")
    return result(json::object());
  if (method == "tools/list")
    return result({{"tools", tools_list()}});
  if (method == "tools/call") {
    try {
      return result(call_tool(_root, params.value("name", ""), params.value("arguments", json::object())));
    } catch (const ToolError &e) {
      return result({{"content", {text_content(e.what())}}, {"isError", true}});
    } catch (const std::exception &e) {
      return result({{"content", {text_content(std::string("Error: ") + e.what())}}, {"isError", true}});
    }
  }
  return json{{"jsonrpc", "2.0"},
              {"id", id},
              {"error", {{"code", -32601}, {"message", std::format("unknown method '{}'", method)}}}};
}

int McpServer::run() {
  // stdout belongs to the protocol. Everything else (Lua print, raylib, the log)
  // goes to stderr; responses go to a saved copy of stdout.
  std::fflush(stdout);
  const int protocolFd = ::dup(STDOUT_FILENO);
  ::dup2(STDERR_FILENO, STDOUT_FILENO);
  FILE *protocol = ::fdopen(protocolFd, "w");
  if (!protocol)
    return 1;

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty())
      continue;
    std::optional<json> response;
    try {
      response = handle(json::parse(line));
    } catch (const json::parse_error &e) {
      response = json{{"jsonrpc", "2.0"}, {"id", nullptr}, {"error", {{"code", -32700}, {"message", e.what()}}}};
    }
    if (response) {
      const auto text = response->dump();
      std::fwrite(text.data(), 1, text.size(), protocol);
      std::fputc('\n', protocol);
      std::fflush(protocol);
    }
  }
  std::fclose(protocol);
  return 0;
}

} // namespace PimpEx::api
