#include "api/LiveApi.hpp"
#include "boot/ProjectCheck.hpp"
#include "boot/SwapFile.hpp"

#include <httplib.h>

#include <chrono>
#include <format>
#include <random>
#include <unistd.h>

namespace PimpEx::api {
namespace {

constexpr int InvalidParams = -32602;
constexpr int MethodNotFound = -32601;
constexpr int AppError = -32000;

json field_json(const scene::FieldValue &value) {
  if (const auto *number = std::get_if<double>(&value))
    return *number;
  return std::get<std::string>(value);
}

scene::FieldValue field_from_json(const json &value) {
  if (value.is_number())
    return value.get<double>();
  if (value.is_boolean())
    return value.get<bool>() ? 1.0 : 0.0;
  if (value.is_string())
    return value.get<std::string>();
  throw ApiError(InvalidParams, "value musi być liczbą, tekstem albo true/false");
}

std::string str(const json &params, const char *key, bool required = true) {
  if (params.contains(key) && params[key].is_string())
    return params[key].get<std::string>();
  if (required)
    throw ApiError(InvalidParams, std::format("brakuje parametru '{}' (tekst)", key));
  return {};
}

json actor_json(const scene::World &world, const std::string &name, const vm::FrameContext &ctx,
                bool brief) {
  const auto &registry = world.registry();
  const auto e = world.find(name);
  json out = {{"name", name}};
  const auto &t = registry.get<scene::Transform>(e);
  out["x"] = t.x;
  out["y"] = t.y;
  if (world.mode() == scene::SceneMode::Mode3D)
    out["z"] = t.z;
  out["components"] = world.components_of(e);
  if (const auto *tags = registry.try_get<scene::Tags>(e); tags && !tags->values.empty())
    out["tags"] = tags->values;
  if (brief)
    return out;

  json components = json::object();
  for (const auto &desc : world.components().all()) {
    if (!desc.has(registry, e))
      continue;
    json fields = json::object();
    for (const auto &field : desc.fields)
      fields[field.name] = field_json(desc.get(registry, e, field));
    if (desc.name == "Text")
      fields["rendered"] = vm::format_text(world, ctx, std::get<std::string>(desc.get(registry, e, *desc.field("value"))), name);
    components[desc.name] = fields;
  }
  out["components"] = components;
  out["vars"] = registry.get<scene::Vars>(e).values;
  if (const auto *behaviors = registry.try_get<scene::Behaviors>(e)) {
    json list = json::array();
    for (const auto &b : behaviors->list)
      list.push_back({{"name", b.name}, {"params", b.params}});
    out["behaviors"] = list;
  }
  const auto &origin = registry.get<scene::Origin>(e);
  if (!origin.archetype.empty())
    out["archetype"] = origin.archetype;
  return out;
}

json program_json(const vm::Program &program) {
  json list = json::array();
  for (const auto &proc : program.procs) {
    json p = {{"name", proc.name}, {"line", proc.firstLine}};
    if (proc.mode == vm::ProcMode::Once)
      p["mode"] = "ONCE";
    if (proc.mode == vm::ProcMode::Every)
      p["every"] = proc.every;
    if (!proc.forEach.empty())
      p["for"] = proc.forEach;
    list.push_back(p);
  }
  return list;
}

json diagnostics_json(const Diagnostics &diagnostics) {
  json list = json::array();
  for (const auto &d : diagnostics)
    list.push_back({{"severity", d.severity == Diagnostic::Severity::Error ? "error" : "warning"},
                    {"file", d.file},
                    {"line", d.line},
                    {"message", d.message},
                    {"text", d.format()}});
  return list;
}

json registry_json(const scene::ComponentRegistry &registry) {
  json list = json::array();
  for (const auto &c : registry.all()) {
    json fields = json::array();
    for (const auto &f : c.fields) {
      json field = {{"name", f.name},
                    {"type", std::string(scene::field_type_name(f.type))},
                    {"default", field_json(f.defaultValue)},
                    {"doc", f.doc}};
      if (!f.choices.empty())
        field["choices"] = f.choices;
      if (f.readOnly)
        field["readOnly"] = true;
      fields.push_back(field);
    }
    list.push_back({{"name", c.name}, {"group", c.group}, {"builtin", c.builtin}, {"doc", c.doc}, {"fields", fields}});
  }
  return list;
}

struct MethodInfo {
  const char *name;
  const char *params;
  const char *doc;
};

constexpr MethodInfo Methods[] = {
    {"ping", "", "czy gra działa; zwraca scenę i klatkę"},
    {"state.get", "selector?, brief?", "stan sceny: klatka, pauza, procedury, aktorzy (filtr: Komponent albo @tag)"},
    {"actors.list", "selector?", "nazwy aktorów (wszyscy, z komponentem albo z @tagiem)"},
    {"actor.get", "name", "wszystkie komponenty, zmienne i tagi aktora"},
    {"actor.set", "name, field, value", "ustawia pole: \"x\", \"vx\", \"Health.hp\", \"Rect.color\", zmienną"},
    {"actor.spawn", "archetype, name?, x?, y?, z?", "tworzy aktora z archetypu"},
    {"actor.destroy", "name", "usuwa aktora"},
    {"actor.add_component", "name, component, values?", "dodaje komponent (z wartościami pól)"},
    {"actor.remove_component", "name, component", "usuwa komponent"},
    {"swap.get", "", "tekst scene.swap, lista procedur i biblioteki"},
    {"swap.paste", "source", "wkleja PROC ... END do scene.swap (ta sama nazwa = podmiana)"},
    {"swap.add", "name", "wkleja PROC z library.swap"},
    {"swap.remove", "name", "usuwa PROC z scene.swap"},
    {"scene.load", "name", "przełącza scenę"},
    {"scene.list", "", "sceny projektu"},
    {"input.press", "keys, frames?", "trzyma klawisze (np. [\"SPACE\"]) przez N klatek"},
    {"game.pause", "", "zatrzymuje logikę (rysowanie trwa)"},
    {"game.resume", "", "wznawia logikę"},
    {"game.step", "frames?", "wykonuje N klatek logiki podczas pauzy"},
    {"game.quit", "", "zamyka grę"},
    {"screenshot", "", "PNG ostatniej klatki (base64); tylko z oknem"},
    {"log.tail", "lines?", "ostatnie linie runtime.log"},
    {"components.list", "", "schemat komponentów (wbudowane + components.xml)"},
    {"lua.eval", "code", "wykonuje kod Lua w grze i zwraca wynik"},
    {"check", "", "pimpex check dla całego projektu"},
};

} // namespace

json method_list() {
  json list = json::array();
  for (const auto &m : Methods)
    list.push_back({{"method", m.name}, {"params", m.params}, {"doc", m.doc}});
  return list;
}

json dispatch(const std::shared_ptr<ApiRequest> &request, boot::Boot &boot, GameControl &control, Log &log) {
  const auto &method = request->method;
  const auto &params = request->params;
  auto &world = boot.world();
  vm::ManualInput noInput;
  const vm::FrameContext ctx{control.input ? static_cast<const vm::IInput &>(*control.input) : noInput,
                             boot.config().width, boot.config().height, control.frame};
  auto requireActor = [&](const std::string &name) {
    if (!world.exists(name))
      throw ApiError(AppError, std::format("aktor '{}' nie istnieje", name));
  };

  if (method == "ping")
    return {{"pong", true}, {"scene", boot.scene_name()}, {"frame", control.frame}, {"paused", control.paused}};

  if (method == "state.get") {
    const auto selector = str(params, "selector", false);
    const bool brief = params.value("brief", false);
    json actors = json::array();
    for (const auto &name : world.query(selector))
      actors.push_back(actor_json(world, name, ctx, brief));
    return {{"scene", boot.scene_name()},
            {"mode", world.mode() == scene::SceneMode::Mode3D ? "3d" : "2d"},
            {"frame", control.frame},
            {"paused", control.paused},
            {"procs", program_json(boot.program())},
            {"actors", actors}};
  }
  if (method == "actors.list")
    return world.query(str(params, "selector", false));
  if (method == "actor.get") {
    const auto name = str(params, "name");
    requireActor(name);
    return actor_json(world, name, ctx, false);
  }
  if (method == "actor.set") {
    const auto name = str(params, "name");
    if (!params.contains("value"))
      throw ApiError(InvalidParams, "brakuje parametru 'value'");
    if (auto error = world.set_field(name, str(params, "field"), field_from_json(params["value"])))
      throw ApiError(AppError, *error);
    return actor_json(world, name, ctx, false);
  }
  if (method == "actor.spawn") {
    std::optional<std::pair<float, float>> position;
    if (params.contains("x") && params.contains("y"))
      position = std::pair{params["x"].get<float>(), params["y"].get<float>()};
    const auto created = world.spawn(str(params, "archetype"), str(params, "name", false), position);
    if (!created)
      throw ApiError(AppError, std::format("nie ma archetypu '{}'", str(params, "archetype")));
    if (params.contains("z"))
      world.set(*created, "z", params["z"].get<double>());
    return actor_json(world, *created, ctx, false);
  }
  if (method == "actor.destroy") {
    const auto name = str(params, "name");
    requireActor(name);
    world.destroy(name);
    return {{"destroyed", name}};
  }
  if (method == "actor.add_component") {
    const auto name = str(params, "name");
    const auto component = str(params, "component");
    if (auto error = world.add_component(name, component))
      throw ApiError(AppError, *error);
    if (params.contains("values") && params["values"].is_object())
      for (const auto &[field, value] : params["values"].items())
        if (auto error = world.set_field(name, component + "." + field, field_from_json(value)))
          throw ApiError(AppError, *error);
    return actor_json(world, name, ctx, false);
  }
  if (method == "actor.remove_component") {
    const auto name = str(params, "name");
    requireActor(name);
    if (!world.remove_component(name, str(params, "component")))
      throw ApiError(AppError, "nie można usunąć tego komponentu");
    return actor_json(world, name, ctx, false);
  }

  if (method == "swap.get") {
    json library = json::array();
    for (const auto &proc : boot.library().procs)
      library.push_back(proc.name);
    return {{"file", boot.paths().display(boot.paths().scene_swap(boot.scene_name()))},
            {"text", boot.swap_text()},
            {"procs", program_json(boot.program())},
            {"library", library}};
  }
  if (method == "swap.paste") {
    const auto source = str(params, "source");
    const auto parsed = vm::parse_program(source, "swap.paste");
    if (!parsed.ok() || parsed.program.procs.empty())
      throw ApiError(AppError, "kod musi zawierać poprawne PROC ... END: " +
                                   (parsed.diagnostics.empty() ? std::string("brak PROC")
                                                               : parsed.diagnostics.front().format()));
    boot.request_proc_paste(source);
    boot.apply_requests();
    return {{"procs", program_json(boot.program())}};
  }
  if (method == "swap.add" || method == "swap.remove") {
    const auto name = str(params, "name");
    if (method == "swap.add") {
      if (!boot.library().find(name))
        throw ApiError(AppError, std::format("nie ma procedury '{}' w library.swap", name));
      boot.request_proc_add(name, 0);
    } else {
      if (!boot.program().find(name))
        throw ApiError(AppError, std::format("nie ma procedury '{}' w scene.swap", name));
      boot.request_proc_remove(name, 0);
    }
    boot.apply_requests();
    return {{"procs", program_json(boot.program())}};
  }
  if (method == "scene.load") {
    const auto name = str(params, "name");
    if (!boot.load_scene(name))
      throw ApiError(AppError, std::format("nie mogę wczytać sceny '{}' – szczegóły w log.tail", name));
    return {{"scene", name}};
  }
  if (method == "scene.list")
    return boot.paths().scene_names();

  if (method == "input.press") {
    if (!params.contains("keys") || !params["keys"].is_array())
      throw ApiError(InvalidParams, "keys musi być listą, np. [\"SPACE\"]");
    const long frames = params.value("frames", 1L);
    for (const auto &k : params["keys"]) {
      const auto key = vm::key_from_name(k.get<std::string>());
      if (!key)
        throw ApiError(InvalidParams, std::format("nieznany klawisz '{}'", k.get<std::string>()));
      control.input->press(*key, frames);
    }
    return {{"frame", control.frame}};
  }
  if (method == "game.pause" || method == "game.resume") {
    control.paused = method == "game.pause";
    return {{"paused", control.paused}, {"frame", control.frame}};
  }
  if (method == "game.step") {
    control.stepFrames += std::max(1L, params.value("frames", 1L));
    return {{"queued", control.stepFrames}, {"frame", control.frame}};
  }
  if (method == "game.quit") {
    control.quit = true;
    return {{"quit", true}};
  }
  if (method == "screenshot") {
    if (!control.canScreenshot)
      throw ApiError(AppError, "zrzut ekranu wymaga okna (gra działa z --headless)");
    control.screenshots.push_back(request);
    return nullptr; // odpowiedź po narysowaniu klatki
  }
  if (method == "log.tail") {
    const auto lines = params.value("lines", 50);
    return log.tail(static_cast<std::size_t>(std::clamp(lines, 1, 500)));
  }
  if (method == "components.list")
    return registry_json(world.components());
  if (method == "lua.eval") {
    const auto result = boot.lua().eval(str(params, "code"));
    if (!result.ok)
      throw ApiError(AppError, "Lua: " + result.output);
    boot.apply_requests(); // kod mógł wołać swap.* albo scene()
    return {{"output", result.output}};
  }
  if (method == "check") {
    boot::CheckSummary summary;
    const auto diagnostics = boot::check_project(boot.paths(), summary);
    return {{"ok", !has_errors(diagnostics)}, {"diagnostics", diagnostics_json(diagnostics)}};
  }
  throw ApiError(MethodNotFound, std::format("nieznana metoda '{}' – lista: GET /methods", method));
}

// --- serwer HTTP --------------------------------------------------------------------------

LiveServer::LiveServer(ApiMailbox &mailbox, std::filesystem::path infoFile)
    : _mailbox(mailbox), _infoFile(std::move(infoFile)) {}

LiveServer::~LiveServer() { stop(); }

int LiveServer::start(const std::string &projectRoot, int port) {
  std::random_device random;
  for (int i = 0; i < 4; ++i)
    _token += std::format("{:08x}", random());

  _server = std::make_unique<httplib::Server>();
  auto authorized = [this](const httplib::Request &req) {
    return req.get_header_value("Authorization") == "Bearer " + _token;
  };

  _server->Get("/methods", [](const httplib::Request &, httplib::Response &res) {
    res.set_content(method_list().dump(2), "application/json");
  });
  _server->Post("/rpc", [this, authorized](const httplib::Request &req, httplib::Response &res) {
    json id = nullptr;
    auto reply = [&](json body) {
      body["jsonrpc"] = "2.0";
      body["id"] = id;
      res.set_content(body.dump(), "application/json");
    };
    if (!authorized(req)) {
      res.status = 401;
      reply({{"error", {{"code", -32001}, {"message", "brak tokenu – nagłówek Authorization: Bearer <token> z .pimpex/api.json"}}}});
      return;
    }
    json message;
    try {
      message = json::parse(req.body);
    } catch (const json::parse_error &e) {
      reply({{"error", {{"code", -32700}, {"message", std::string("niepoprawny JSON: ") + e.what()}}}});
      return;
    }
    id = message.value("id", json(nullptr));
    auto request = std::make_shared<ApiRequest>();
    request->method = message.value("method", "");
    request->params = message.value("params", json::object());
    auto future = request->reply.get_future();
    _mailbox.post(request);
    if (future.wait_for(std::chrono::seconds(10)) != std::future_status::ready) {
      reply({{"error", {{"code", AppError}, {"message", "gra nie odpowiada (czy okno nie jest zamrożone?)"}}}});
      return;
    }
    reply(future.get());
  });

  _port = port > 0 && _server->bind_to_port("127.0.0.1", port) ? port : _server->bind_to_any_port("127.0.0.1");
  if (_port <= 0)
    return 0;
  _thread = std::jthread([this] { _server->listen_after_bind(); });

  std::error_code ec;
  std::filesystem::create_directories(_infoFile.parent_path(), ec);
  const json info = {{"url", std::format("http://127.0.0.1:{}", _port)},
                     {"port", _port},
                     {"token", _token},
                     {"pid", static_cast<long>(::getpid())},
                     {"project", projectRoot}};
  boot::write_file_atomic(_infoFile, info.dump(2));
  return _port;
}

void LiveServer::stop() {
  if (_server)
    _server->stop();
  if (_thread.joinable())
    _thread.join();
  if (_port > 0) {
    std::error_code ec;
    std::filesystem::remove(_infoFile, ec);
    _port = 0;
  }
}

} // namespace PimpEx::api
