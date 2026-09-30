#pragma once

#include "boot/ProjectPaths.hpp"
#include "core/Log.hpp"
#include "scene/Physics.hpp"
#include "scene/SceneData.hpp"
#include "scene/World.hpp"
#include "script/LuaRuntime.hpp"
#include "vm/Parser.hpp"
#include "vm/Vm.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace PimpEx::boot {

// Boot trzyma w pamięci wszystko, czego potrzebuje scena: świat (aktorów),
// program z scene.swap, bibliotekę procedur i skrypty Lua. Tylko Boot
// zapisuje plik .swap – wkleja i wycina z niego procedury w trakcie gry.
// Działa wyłącznie na wątku głównym.
class Boot : public vm::IVmHost, public script::ILuaHost {
public:
  // Wywoływane po każdym zapisie pliku przez silnik (dla watchera).
  using SelfWriteCallback = std::function<void(const fs::path &, std::size_t hash)>;

private:
  struct Request {
    enum class Kind { Add, Remove, Paste, Scene };
    Kind kind;
    std::string value;
    int line = 0;
  };

  ProjectPaths _paths;
  Log &_log;
  scene::ProjectConfig _config;

  std::shared_ptr<const scene::ComponentRegistry> _components = scene::ComponentRegistry::with_builtins();
  std::string _sceneName;
  int _sceneGeneration = 0;
  scene::SceneData _sceneData;
  scene::World _world;

  std::string _swapText;
  vm::Program _program;
  // false = plik scene.swap na dysku ma błędy. Wtedy Boot nie nadpisuje go
  // (zniszczyłby pracę użytkownika), a zmiany robi tylko w pamięci.
  bool _swapFileValid = true;
  std::string _libraryText;
  vm::Program _library;

  vm::Vm _vm;
  std::unique_ptr<script::LuaRuntime> _lua;
  std::vector<Request> _requests;
  const vm::FrameContext *_frame = nullptr;
  std::vector<scene::Contact> _contacts; // styki z ostatniej klatki fizyki
  SelfWriteCallback _onSelfWrite;
  bool _persistSwap = true; // false (pimpex play): zmiany .swap tylko w pamięci

  std::string swap_display() const { return _paths.display(_paths.scene_swap(_sceneName)); }
  void remove_in_memory(const std::string &name);
  bool commit_swap(std::string text, const std::string &reason);
  void load_scripts();

public:
  Boot(ProjectPaths paths, Log &log, unsigned seed = std::random_device{}());
  ~Boot() override;

  // Wczytuje pimpex.xml, skrypty i scenę startową. false = nie da się grać.
  bool start(const std::string &sceneOverride = "");
  bool load_scene(const std::string &name);

  // Wczytuje components.xml (jeśli jest). false = błędy w pliku.
  bool load_components(const std::string *text = nullptr);

  // Jedna klatka logiki: procedury, prośby o zmiany w .swap, ruch.
  void run_frame(const vm::FrameContext &ctx);

  // Wykonuje zamówione zmiany (.swap, scena) od razu – np. z API, gdy gra stoi.
  void apply_requests();

  script::LuaRuntime &lua() { return *_lua; }
  const std::string &swap_text() const { return _swapText; }
  const vm::Program &library() const { return _library; }

  // --- hot-reload (dane przygotowane na wątku zasobów) ----------------------
  void apply_swap(std::string text, vm::ParseResult parsed);
  void apply_library(std::string text, vm::ParseResult parsed);
  void apply_scene_xml(scene::SceneData data, Diagnostics diagnostics);
  void apply_script(const fs::path &path, const std::string &source);
  // components.xml zmienił się: nowy rejestr i ponowne wczytanie scene.xml.
  void apply_components_xml(const std::string &text);

  void set_self_write_callback(SelfWriteCallback callback) { _onSelfWrite = std::move(callback); }

  // Tryb gry (play): Boot nie zapisuje zmian w scene.swap na dysk.
  void set_persist_swap(bool persist) { _persistSwap = persist; }

  // Pliki, które watcher ma obserwować dla bieżącej sceny.
  std::vector<fs::path> watched_files() const;

  // Tekstowy zrzut stanu do .pimpex/state.txt (dla ludzi i agentów AI).
  void write_state(long frame) const;

  const ProjectPaths &paths() const { return _paths; }
  const scene::ProjectConfig &config() const { return _config; }
  // Rejestr komponentów (wbudowane + components.xml). Niezmienny – przy
  // zmianie components.xml powstaje nowy, więc można go czytać z innych wątków.
  std::shared_ptr<const scene::ComponentRegistry> components() const { return _components; }
  const std::string &scene_name() const { return _sceneName; }
  int scene_generation() const { return _sceneGeneration; }
  const scene::SceneData &scene_data() const { return _sceneData; }
  scene::World &world() { return _world; }
  const vm::Program &program() const { return _program; }

  // --- vm::IVmHost i script::ILuaHost ---------------------------------------
  void call_function(const std::string &name, const std::vector<std::string> &args,
                     int line) override;
  void request_proc_add(const std::string &name, int line) override;
  void request_proc_remove(const std::string &name, int line) override;
  void request_proc_paste(const std::string &source) override;
  void request_scene(const std::string &name, int line) override;
  void log(const std::string &message) override;
  void runtime_error(int line, const std::string &message) override;
  void lua_error(const Diagnostic &error) override { _log.diagnostic(error); }

  scene::World &lua_world() override { return _world; }
  const vm::FrameContext *lua_frame() override { return _frame; }
  bool lua_has_proc(const std::string &name) override { return _program.find(name) != nullptr; }
};

} // namespace PimpEx::boot
