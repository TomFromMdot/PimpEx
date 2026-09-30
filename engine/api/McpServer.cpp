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

// Błąd narzędzia – zwracany agentowi jako isError, a nie błąd protokołu.
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

  // --- offline ---
  add("pimpex_check",
      "Sprawdza cały projekt gry PimpEx (scene.xml, scene.swap, Lua, components.xml) bez "
      "uruchamiania. Zwraca błędy w formacie plik:linia: opis. Używaj po każdej zmianie plików.",
      schema());
  add("pimpex_run",
      "Uruchamia grę bez okna na N klatek (60 = 1 s) z zaplanowanymi klawiszami i zwraca zrzut "
      "stanu (aktorzy, pola, procedury) oraz dziennik. Nie zmienia plików projektu.",
      schema({{"frames", prop("integer", "ile klatek (domyślnie 120)")},
              {"input", prop("string", "klawisze: KLAWISZ:od-do albo KLAWISZ:klatka, np. \"RIGHT:0-60 SPACE:10\"")},
              {"scene", prop("string", "scena startowa (domyślnie z pimpex.xml)")}}));
  add("pimpex_docs",
      "Dokumentacja silnika PimpEx. Tematy: opcodes (język .swap), components (komponenty ECS, "
      "także z components.xml projektu), behaviors (zachowania Lua i systemy), lua-api, "
      "scene-xml (pliki sceny, 3D), api (API na żywo).",
      schema({{"topic", {{"type", "string"},
                         {"enum", {"opcodes", "components", "behaviors", "lua-api", "scene-xml", "api"}},
                         {"description", "temat"}}}},
             {"topic"}));
  add("pimpex_scene_new",
      "Tworzy nową scenę scenes/<name>/ z szablonu: empty2d, topdown2d, platformer2d, empty3d, "
      "arena3d, starter2d. Dodaje brakujące zachowania do scripts/behaviors/.",
      schema({{"name", prop("string", "nazwa sceny (litery, cyfry, _ i -)")},
              {"template", prop("string", "szablon (domyślnie empty2d)")}},
             {"name"}));

  // --- na żywo ---
  const char *live = " Wymaga uruchomionej gry: `pimpex dev .` (albo `pimpex dev . --headless`).";
  auto liveDesc = [&](const char *text) { return std::string(text) + live; };
  add("live_state", liveDesc("Stan działającej gry: scena, klatka, procedury i aktorzy z komponentami.").c_str(),
      schema({{"selector", prop("string", "filtr: Komponent (np. Health) albo @tag (np. @enemy)")},
              {"brief", prop("boolean", "tylko nazwy, pozycje i lista komponentów")}}));
  add("live_actor", liveDesc("Wszystkie komponenty, zmienne, tagi i zachowania jednego aktora.").c_str(),
      schema({{"name", prop("string", "nazwa aktora")}}, {"name"}));
  add("live_set",
      liveDesc("Ustawia pole aktora w działającej grze: x, vx, Health.hp, Rect.color, Text.value, zmienna.").c_str(),
      schema({{"name", prop("string", "aktor")},
              {"field", prop("string", "pole")},
              {"value", {{"description", "liczba, tekst albo true/false"}}}},
             {"name", "field", "value"}));
  add("live_spawn", liveDesc("Tworzy aktora z archetypu.").c_str(),
      schema({{"archetype", prop("string", "archetyp z scene.xml")},
              {"name", prop("string", "nazwa (opcjonalnie)")},
              {"x", prop("number", "x")},
              {"y", prop("number", "y")},
              {"z", prop("number", "z (3D)")}},
             {"archetype"}));
  add("live_destroy", liveDesc("Usuwa aktora.").c_str(), schema({{"name", prop("string", "aktor")}}, {"name"}));
  add("live_swap_paste",
      liveDesc("Wkleja kod .swap (PROC ... END) do działającej sceny; PROC o tej samej nazwie jest "
               "podmieniana. PROC nazwa ONCE wykona się raz i zniknie.")
          .c_str(),
      schema({{"source", prop("string", "kod .swap")}}, {"source"}));
  add("live_press", liveDesc("Wciska klawisze na N klatek (np. [\"SPACE\"], [\"LEFT\",\"UP\"]).").c_str(),
      schema({{"keys", {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", "klawisze"}}},
              {"frames", prop("integer", "ile klatek trzymać (domyślnie 1)")}},
             {"keys"}));
  add("live_control", liveDesc("Sterowanie grą: pause, resume, step (z frames), quit.").c_str(),
      schema({{"action", {{"type", "string"}, {"enum", {"pause", "resume", "step", "quit"}}}},
              {"frames", prop("integer", "dla step: ile klatek")}},
             {"action"}));
  add("live_screenshot", liveDesc("Zrzut ekranu działającej gry (wymaga okna, nie --headless).").c_str(),
      schema());
  add("live_log", liveDesc("Ostatnie linie dziennika gry (błędy, LOG, zmiany w .swap).").c_str(),
      schema({{"lines", prop("integer", "ile linii (domyślnie 40)")}}));
  add("live_lua", liveDesc("Wykonuje kod Lua w działającej grze i zwraca wynik, np. get(\"Player\",\"x\").").c_str(),
      schema({{"code", prop("string", "kod Lua")}}, {"code"}));
  add("live_scene", liveDesc("Przełącza działającą grę na inną scenę.").c_str(),
      schema({{"name", prop("string", "scena")}}, {"name"}));
  return tools;
}

// --- połączenie z działającą grą ---------------------------------------------------------

json live_call(const fs::path &root, const std::string &method, json params) {
  const auto info = boot::read_file(root / ".pimpex" / "api.json");
  if (!info)
    throw ToolError("Gra nie działa – uruchom `pimpex dev .` (albo `pimpex dev . --headless`) "
                    "w katalogu projektu.");
  json api;
  try {
    api = json::parse(*info);
  } catch (const json::parse_error &) {
    throw ToolError("Uszkodzony .pimpex/api.json – uruchom grę ponownie.");
  }
  httplib::Client client("127.0.0.1", api.value("port", 0));
  client.set_connection_timeout(2);
  client.set_read_timeout(15);
  const json request = {{"jsonrpc", "2.0"}, {"id", 1}, {"method", method}, {"params", std::move(params)}};
  const auto response = client.Post("/rpc", {{"Authorization", "Bearer " + api.value("token", "")}},
                                    request.dump(), "application/json");
  if (!response)
    throw ToolError("Nie mogę połączyć się z grą (może została zamknięta?) – uruchom `pimpex dev .`.");
  const auto body = json::parse(response->body);
  if (body.contains("error"))
    throw ToolError(body["error"].value("message", "błąd API"));
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
  throw ToolError(std::format("nie ma tematu '{}'", topic));
}

json call_tool(const fs::path &root, const std::string &name, const json &args) {
  if (name == "pimpex_check") {
    boot::CheckSummary summary;
    const auto diagnostics = boot::check_project(boot::ProjectPaths{root, {}}, summary);
    std::string text;
    for (const auto &d : diagnostics)
      text += d.format() + "\n";
    text += has_errors(diagnostics)
                ? "Są błędy – popraw je i sprawdź ponownie."
                : std::format("OK – sceny: {}, procedury: {}, aktorzy: {}, skrypty: {}, komponenty użytkownika: {}.",
                              summary.scenes, summary.procs, summary.actors, summary.scripts, summary.components);
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
    // Osobny katalog – nie mieszamy z dziennikiem gry uruchomionej w `pimpex dev`.
    options.runtimeDir = root / ".pimpex" / "mcp-run";
    const int code = Engine(root, options).run();
    const auto state = boot::read_file(options.runtimeDir / "state.txt").value_or("(brak state.txt)");
    const auto log = boot::read_file(options.runtimeDir / "runtime.log").value_or("");
    return {{"content", {text_content(std::format("{}\n--- runtime.log ---\n{}", state, log))}},
            {"isError", code != 0}};
  }
  if (name == "pimpex_docs")
    return {{"content", {text_content(doc_text(root, args.value("topic", "")))}}};
  if (name == "pimpex_scene_new") {
    const auto sceneName = args.value("name", "");
    if (!cli::valid_name(sceneName))
      throw ToolError("nazwa sceny może zawierać tylko litery, cyfry, _ i -");
    const auto result =
        cli::apply_scene_template(root, args.value("template", "empty2d"), sceneName, root.filename().string());
    if (!result.error.empty())
      throw ToolError(result.error);
    std::string text = std::format("Scena '{}' utworzona:\n", sceneName);
    for (const auto &file : result.created)
      text += "  + " + file + "\n";
    return {{"content", {text_content(text)}}};
  }

  // Na żywo.
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
      throw ToolError("action: pause, resume, step albo quit");
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
  throw ToolError(std::format("nieznane narzędzie '{}'", name));
}

} // namespace

McpServer::McpServer(fs::path projectRoot) : _root(std::move(projectRoot)) {}

std::optional<json> McpServer::handle(const json &message) {
  if (!message.contains("id"))
    return std::nullopt; // powiadomienie (np. notifications/initialized)
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
                    "PimpEx: silnik gier, w którym gra to pliki tekstowe (scene.xml, scene.swap, Lua). "
                    "Zacznij od pimpex_docs (opcodes, components, behaviors). Po zmianach plików: "
                    "pimpex_check, potem pimpex_run z input, żeby sprawdzić rozgrywkę. Jeśli gra działa "
                    "w `pimpex dev`, narzędzia live_* czytają i zmieniają ją na żywo."}});
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
      return result({{"content", {text_content(std::string("Błąd: ") + e.what())}}, {"isError", true}});
    }
  }
  return json{{"jsonrpc", "2.0"},
              {"id", id},
              {"error", {{"code", -32601}, {"message", std::format("nieznana metoda '{}'", method)}}}};
}

int McpServer::run() {
  // stdout należy do protokołu. Wszystko inne (print z Lua, raylib, dziennik)
  // kierujemy na stderr, a odpowiedzi piszemy do zachowanej kopii stdout.
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
