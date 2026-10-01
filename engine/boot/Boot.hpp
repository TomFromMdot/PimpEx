#pragma once

#include "boot/ProjectPaths.hpp"
#include "core/Log.hpp"
#include "gui/Gui.hpp"
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

// Boot holds everything a scene needs in memory: the world, the scene.swap
// program, the procedure library, Lua and the GUI. Only Boot writes .swap
// files – it pastes and cuts procedures while the game runs. Main thread only.
class Boot : public vm::IVmHost, public script::ILuaHost {
public:
  // Called after every file the engine writes (for the watcher).
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
  // false = scene.swap on disk has errors. Boot then changes the program only
  // in memory instead of overwriting the user's work.
  bool _swapFileValid = true;
  std::string _libraryText;
  vm::Program _library;

  vm::Vm _vm;
  gui::GuiSystem _gui;
  std::string _guiActionFile; // GUI file whose button is running (for diagnostics)
  std::unique_ptr<script::LuaRuntime> _lua;
  std::vector<Request> _requests;
  const vm::FrameContext *_frame = nullptr;
  std::vector<scene::Contact> _contacts;
  SelfWriteCallback _onSelfWrite;
  bool _persistSwap = true; // false (pimpex play): .swap changes stay in memory

  std::string swap_display() const { return _paths.display(_paths.scene_swap(_sceneName)); }
  void remove_in_memory(const std::string &name);
  bool commit_swap(std::string text, const std::string &reason);
  void load_scripts();
  void load_gui();
  void show_scene_gui();

public:
  Boot(ProjectPaths paths, Log &log, unsigned seed = std::random_device{}());
  ~Boot() override;

  // Loads pimpex.xml, scripts, GUI and the start scene. false = cannot play.
  bool start(const std::string &sceneOverride = "");
  bool load_scene(const std::string &name);

  // false = components.xml has errors.
  bool load_components(const std::string *text = nullptr);

  // One logic frame: GUI clicks, procedures, Lua, physics, .swap requests.
  // A visible GUI with pause="true" stops procedures, Lua and physics.
  void run_frame(const vm::FrameContext &ctx);

  // Applies queued .swap/scene requests now – e.g. from the API while paused.
  void apply_requests();

  script::LuaRuntime &lua() { return *_lua; }
  const std::string &swap_text() const { return _swapText; }
  const vm::Program &library() const { return _library; }

  // Hot-reload (data prepared on the asset thread).
  void apply_swap(std::string text, vm::ParseResult parsed);
  void apply_library(std::string text, vm::ParseResult parsed);
  void apply_scene_xml(scene::SceneData data, Diagnostics diagnostics);
  void apply_script(const fs::path &path, const std::string &source);
  // New registry and a reload of scene.xml.
  void apply_components_xml(const std::string &text);
  void apply_gui_xml(gui::GuiScene scene, Diagnostics diagnostics);

  void set_self_write_callback(SelfWriteCallback callback) { _onSelfWrite = std::move(callback); }

  void set_persist_swap(bool persist) { _persistSwap = persist; }

  std::vector<fs::path> watched_files() const;

  // Writes .pimpex/state.txt for people and AI agents.
  void write_state(long frame) const;

  const ProjectPaths &paths() const { return _paths; }
  const scene::ProjectConfig &config() const { return _config; }
  // Immutable – a components.xml change creates a new one, so other threads may read it.
  std::shared_ptr<const scene::ComponentRegistry> components() const { return _components; }
  const std::string &scene_name() const { return _sceneName; }
  int scene_generation() const { return _sceneGeneration; }
  const scene::SceneData &scene_data() const { return _sceneData; }
  gui::GuiSystem &gui() { return _gui; }
  const gui::GuiSystem &gui() const { return _gui; }
  scene::World &world() { return _world; }
  const vm::Program &program() const { return _program; }

  // vm::IVmHost and script::ILuaHost
  void call_function(const std::string &name, const std::vector<std::string> &args,
                     int line) override;
  void request_proc_add(const std::string &name, int line) override;
  void request_proc_remove(const std::string &name, int line) override;
  void request_proc_paste(const std::string &source) override;
  void request_scene(const std::string &name, int line) override;
  void request_gui(vm::Op op, const std::string &name, int line) override;
  void log(const std::string &message) override;
  void runtime_error(int line, const std::string &message) override;
  void lua_error(const Diagnostic &error) override { _log.diagnostic(error); }

  scene::World &lua_world() override { return _world; }
  const vm::FrameContext *lua_frame() override { return _frame; }
  bool lua_has_proc(const std::string &name) override { return _program.find(name) != nullptr; }
  gui::GuiSystem &lua_gui() override { return _gui; }
};

} // namespace PimpEx::boot
