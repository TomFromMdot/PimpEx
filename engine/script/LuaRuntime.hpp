#pragma once

#include "core/Diagnostic.hpp"
#include "scene/World.hpp"
#include "vm/Vm.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace PimpEx::script {

// To, co skrypty Lua mogą zrobić ze sceną. Implementuje Boot.
class ILuaHost {
public:
  virtual ~ILuaHost() = default;
  virtual scene::World &lua_world() = 0;
  virtual const vm::FrameContext *lua_frame() = 0; // null poza klatką
  virtual bool lua_has_proc(const std::string &name) = 0;
  virtual void request_proc_add(const std::string &name, int line) = 0;
  virtual void request_proc_remove(const std::string &name, int line) = 0;
  virtual void request_proc_paste(const std::string &source) = 0;
  virtual void request_scene(const std::string &name, int line) = 0;
  virtual void log(const std::string &message) = 0;
};

// Jeden stan Lua na cały projekt. Zmienne globalne przetrwają
// przeładowanie skryptu i zmianę sceny.
class LuaRuntime {
  struct Impl;
  std::unique_ptr<Impl> _impl;

public:
  explicit LuaRuntime(ILuaHost &host);
  ~LuaRuntime();

  // Wykonuje skrypt (definiuje/podmienia funkcje). Błąd -> Diagnostic.
  std::optional<Diagnostic> load_script(const std::string &displayName, std::string_view source);

  bool has_function(const std::string &name) const;

  // Wywołuje funkcję globalną. Argumenty wyglądające jak liczby są liczbami.
  // Zwraca opis błędu albo nullopt.
  std::optional<Diagnostic> call(const std::string &name, const std::vector<std::string> &args);
};

} // namespace PimpEx::script
