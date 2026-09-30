#include "boot/Boot.hpp"
#include "boot/SwapFile.hpp"
#include "boot/XmlLoader.hpp"

#include <format>

namespace PimpEx::boot {

Boot::Boot(ProjectPaths paths, Log &log, unsigned seed)
    : _paths(std::move(paths)), _log(log), _vm(seed) {}

Boot::~Boot() = default;

bool Boot::start() {
  const auto projectXml = read_file(_paths.project_xml());
  if (!projectXml) {
    _log.error(std::format("Nie ma pliku {} – to nie jest projekt PimpEx.",
                           _paths.project_xml().string()));
    return false;
  }
  Diagnostics diagnostics;
  _config = parse_project_xml(*projectXml, _paths.display(_paths.project_xml()), diagnostics);
  _log.diagnostics(diagnostics);
  if (has_errors(diagnostics))
    return false;

  _lua = std::make_unique<script::LuaRuntime>(*this);
  load_scripts();
  return load_scene(_config.startScene);
}

void Boot::load_scripts() {
  for (const auto &file : _paths.script_files())
    if (const auto source = read_file(file))
      apply_script(file, *source);
}

bool Boot::load_scene(const std::string &name) {
  const auto xmlPath = _paths.scene_xml(name);
  const auto xml = read_file(xmlPath);
  if (!xml) {
    _log.error(std::format("Scena '{}' nie istnieje (brak {}).", name, _paths.display(xmlPath)));
    return false;
  }
  Diagnostics diagnostics;
  auto data = parse_scene_xml(*xml, _paths.display(xmlPath), name, diagnostics);
  _log.diagnostics(diagnostics);
  if (has_errors(diagnostics))
    return false;

  _sceneName = name;
  ++_sceneGeneration;
  _sceneData = std::move(data);
  _world.load(_sceneData);
  _requests.clear();

  _libraryText.clear();
  _library = {};
  if (auto library = read_file(_paths.library_swap(name))) {
    auto parsed = vm::parse_program(*library, _paths.display(_paths.library_swap(name)));
    apply_library(std::move(*library), std::move(parsed));
  }

  auto swap = read_file(_paths.scene_swap(name)).value_or("");
  auto parsed = vm::parse_program(swap, swap_display());
  _swapText.clear();
  _program = {};
  apply_swap(std::move(swap), std::move(parsed));

  _log.info(std::format("Scena '{}': aktorzy {}, procedury {}.", name, _world.size(),
                        _program.procs.size()));
  return true;
}

void Boot::run_frame(const vm::FrameContext &ctx) {
  _frame = &ctx;
  _vm.run_frame(_program, _world, ctx, *this);
  apply_requests();
  _world.integrate();
  _frame = nullptr;
}

// --- prośby z VM i Lua: zmiany pliku .swap ----------------------------------

void Boot::request_proc_add(const std::string &name, int line) {
  _requests.push_back({Request::Kind::Add, name, line});
}
void Boot::request_proc_remove(const std::string &name, int line) {
  _requests.push_back({Request::Kind::Remove, name, line});
}
void Boot::request_proc_paste(const std::string &source) {
  _requests.push_back({Request::Kind::Paste, source, 0});
}
void Boot::request_scene(const std::string &name, int line) {
  _requests.push_back({Request::Kind::Scene, name, line});
}

bool Boot::commit_swap(std::string text, const std::string &reason) {
  auto parsed = vm::parse_program(text, swap_display());
  if (!parsed.ok()) {
    // Nie powinno się zdarzyć – wklejamy tylko sparsowane procedury.
    _log.diagnostics(parsed.diagnostics);
    return false;
  }
  const auto path = _paths.scene_swap(_sceneName);
  // Najpierw uprzedzamy watchera, potem zapisujemy – żeby nie wziął
  // naszego własnego zapisu za zmianę od użytkownika.
  if (_onSelfWrite)
    _onSelfWrite(path, content_hash(text));
  if (_persistSwap && !write_file_atomic(path, text))
    _log.error(std::format("Nie mogę zapisać {} – zmiana działa tylko w pamięci.", swap_display()));
  _swapText = std::move(text);
  _program = std::move(parsed.program);
  _vm.reset_reports();
  _log.info(reason);
  return true;
}

void Boot::remove_in_memory(const std::string &name) {
  std::erase_if(_program.procs, [&](const vm::Proc &p) { return p.name == name; });
  _log.info(std::format("boot: PROC {} zmieniona tylko w pamięci – {} ma błędy", name,
                        swap_display()));
}

void Boot::apply_requests() {
  auto requests = std::move(_requests);
  _requests.clear();
  for (const auto &request : requests) {
    switch (request.kind) {
    case Request::Kind::Add: {
      if (_program.find(request.value))
        break; // już jest w scene.swap
      const auto *proc = _library.find(request.value);
      if (!proc) {
        runtime_error(request.line, std::format("nie ma procedury '{}' w library.swap", request.value));
        break;
      }
      if (!_swapFileValid) {
        remove_in_memory(request.value);
        _program.procs.push_back(*proc);
        break;
      }
      commit_swap(append_proc(_swapText, extract_proc_source(_libraryText, *proc)),
                  std::format("boot: wklejono PROC {} do {}", request.value, swap_display()));
      break;
    }
    case Request::Kind::Remove: {
      const auto *proc = _program.find(request.value);
      if (!proc)
        break;
      if (!_swapFileValid) {
        remove_in_memory(request.value);
        break;
      }
      commit_swap(remove_proc(_swapText, *proc),
                  std::format("boot: usunięto PROC {} z {}", request.value, swap_display()));
      break;
    }
    case Request::Kind::Paste: {
      auto parsed = vm::parse_program(request.value, "swap.paste");
      if (!parsed.ok() || parsed.program.procs.empty()) {
        _log.error("swap.paste: tekst musi zawierać poprawną PROC ... END");
        _log.diagnostics(parsed.diagnostics);
        break;
      }
      if (!_swapFileValid) {
        for (auto &proc : parsed.program.procs) {
          remove_in_memory(proc.name);
          _program.procs.push_back(std::move(proc));
        }
        break;
      }
      std::string text = _swapText;
      for (const auto &proc : parsed.program.procs)
        if (const auto *existing = vm::parse_program(text, "").program.find(proc.name))
          text = remove_proc(text, *existing); // wklejona wersja zastępuje starą
      commit_swap(append_proc(text, request.value),
                  std::format("boot: wklejono kod z Lua do {}", swap_display()));
      break;
    }
    case Request::Kind::Scene:
      if (!load_scene(request.value))
        runtime_error(request.line, std::format("nie mogę wczytać sceny '{}'", request.value));
      return; // po zmianie sceny pozostałe prośby dotyczą starej sceny
    }
  }
}

// --- hot-reload ---------------------------------------------------------------

void Boot::apply_swap(std::string text, vm::ParseResult parsed) {
  if (!parsed.ok()) {
    _swapFileValid = false;
    _log.diagnostics(parsed.diagnostics);
    _log.error(std::format("{} ma błędy – działa poprzednia wersja procedur.", swap_display()));
    return;
  }
  const bool reload = !_swapText.empty() || !_program.procs.empty();
  _swapFileValid = true;
  _swapText = std::move(text);
  _program = std::move(parsed.program);
  _vm.reset_reports();
  if (reload)
    _log.info(std::format("hot-reload: {} (procedury: {})", swap_display(), _program.procs.size()));
}

void Boot::apply_library(std::string text, vm::ParseResult parsed) {
  const auto display = _paths.display(_paths.library_swap(_sceneName));
  if (!parsed.ok()) {
    _log.diagnostics(parsed.diagnostics);
    _log.error(std::format("{} ma błędy – działa poprzednia wersja biblioteki.", display));
    return;
  }
  _libraryText = std::move(text);
  _library = std::move(parsed.program);
}

void Boot::apply_scene_xml(scene::SceneData data, Diagnostics diagnostics) {
  _log.diagnostics(diagnostics);
  if (has_errors(diagnostics)) {
    _log.error(std::format("{} ma błędy – scena się nie zmienia.",
                           _paths.display(_paths.scene_xml(_sceneName))));
    return;
  }
  _sceneData = std::move(data);
  _world.reload(_sceneData, _config.keepStateOnReload);
  _log.info(std::format("hot-reload: {} (aktorzy: {})",
                        _paths.display(_paths.scene_xml(_sceneName)), _world.size()));
}

void Boot::apply_script(const fs::path &path, const std::string &source) {
  if (auto error = _lua->load_script(_paths.display(path), source))
    _log.diagnostic(*error);
}

// --- reszta hosta ---------------------------------------------------------------

void Boot::call_function(const std::string &name, const std::vector<std::string> &args, int line) {
  if (!_lua->has_function(name)) {
    runtime_error(line, std::format("nie ma funkcji Lua '{}' w scripts/", name));
    return;
  }
  if (auto error = _lua->call(name, args))
    _log.diagnostic(*error);
}

void Boot::log(const std::string &message) { _log.info("LOG: " + message); }

void Boot::runtime_error(int line, const std::string &message) {
  _log.diagnostic({Diagnostic::Severity::Error, swap_display(), line, message});
}

std::vector<fs::path> Boot::watched_files() const {
  std::vector<fs::path> files = {_paths.scene_swap(_sceneName), _paths.library_swap(_sceneName),
                                 _paths.scene_xml(_sceneName)};
  for (const auto &script : _paths.script_files())
    files.push_back(script);
  return files;
}

void Boot::write_state(long frame) const {
  std::ostringstream out;
  out << "# PimpEx – stan sceny. Plik nadpisywany w trakcie gry.\n";
  out << "scena: " << _sceneName << '\n';
  out << "klatka: " << frame << '\n';
  out << "procedury:";
  for (const auto &proc : _program.procs)
    out << ' ' << proc.name;
  out << "\naktorzy: " << _world.size() << '\n';
  vm::ManualInput noInput;
  const vm::FrameContext ctx{noInput, _config.width, _config.height, frame};
  _world.dump(out, [&](const std::string &text, const std::string &self) {
    return vm::format_text(_world, ctx, text, self);
  });
  std::error_code ec;
  fs::create_directories(_paths.runtime_dir(), ec);
  write_file_atomic(_paths.state_file(), out.str());
}

} // namespace PimpEx::boot
