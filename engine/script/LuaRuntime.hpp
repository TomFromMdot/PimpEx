#pragma once

#include "core/Diagnostic.hpp"
#include "gui/Gui.hpp"
#include "scene/World.hpp"
#include "vm/Vm.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace PimpEx::script {

// What Lua scripts can do with the scene. Implemented by Boot.
class ILuaHost {
public:
  virtual ~ILuaHost() = default;
  virtual scene::World &lua_world() = 0;
  virtual const vm::FrameContext *lua_frame() = 0; // null outside a frame
  virtual bool lua_has_proc(const std::string &name) = 0;
  virtual gui::GuiSystem &lua_gui() = 0;
  virtual void request_proc_add(const std::string &name, int line) = 0;
  virtual void request_proc_remove(const std::string &name, int line) = 0;
  virtual void request_proc_paste(const std::string &source) = 0;
  virtual void request_scene(const std::string &name, int line) = 0;
  virtual void log(const std::string &message) = 0;
  virtual void lua_error(const Diagnostic &error) = 0;
};

// One Lua state per project. Globals survive script reloads and scene changes.
class LuaRuntime {
  struct Impl;
  std::unique_ptr<Impl> _impl;

public:
  explicit LuaRuntime(ILuaHost &host);
  ~LuaRuntime();

  std::optional<Diagnostic> load_script(const std::string &displayName, std::string_view source);

  bool has_function(const std::string &name) const;

  // Arguments that look like numbers are passed as numbers.
  std::optional<Diagnostic> call(const std::string &name, const std::vector<std::string> &args);

  // The file returns a table with on_start/on_update/on_touch/on_destroy.
  // Reloading swaps the functions; existing `self` instances stay.
  std::optional<Diagnostic> load_behavior(const std::string &name, const std::string &displayName,
                                          std::string_view source);
  bool has_behavior(const std::string &name) const;

  // Once per frame: on_start for new <Behavior> actors, on_destroy for removed
  // ones, then on_update and system() systems.
  void update(scene::World &world);

  void dispatch_contacts(scene::World &world, const std::vector<std::pair<std::string, std::string>> &contacts);

  // Scene change: all `self` instances go away (without on_destroy).
  void reset_instances();

  // Any Lua code in the game state (API lua.eval); results as tab-separated text.
  struct EvalResult {
    bool ok = true;
    std::string output;
  };
  EvalResult eval(std::string_view code);
};

} // namespace PimpEx::script
