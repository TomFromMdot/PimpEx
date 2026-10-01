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

// To, co skrypty Lua mogą zrobić ze sceną. Implementuje Boot.
class ILuaHost {
public:
  virtual ~ILuaHost() = default;
  virtual scene::World &lua_world() = 0;
  virtual const vm::FrameContext *lua_frame() = 0; // null poza klatką
  virtual bool lua_has_proc(const std::string &name) = 0;
  virtual gui::GuiSystem &lua_gui() = 0;
  virtual void request_proc_add(const std::string &name, int line) = 0;
  virtual void request_proc_remove(const std::string &name, int line) = 0;
  virtual void request_proc_paste(const std::string &source) = 0;
  virtual void request_scene(const std::string &name, int line) = 0;
  virtual void log(const std::string &message) = 0;
  virtual void lua_error(const Diagnostic &error) = 0;
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

  // --- zachowania (scripts/behaviors/<name>.lua) ----------------------------------
  // Plik zwraca tabelę z on_start/on_update/on_touch/on_destroy. Ponowne
  // wczytanie podmienia funkcje, a istniejące `self` zostają.
  std::optional<Diagnostic> load_behavior(const std::string &name, const std::string &displayName,
                                          std::string_view source);
  bool has_behavior(const std::string &name) const;

  // Raz na klatkę: tworzy `self` dla nowych aktorów z <Behavior> (on_start),
  // woła on_destroy dla zniszczonych, potem on_update i systemy z system().
  void update(scene::World &world);

  // on_touch(self, other) dla aktorów z zachowaniami, którzy się stykają.
  void dispatch_contacts(scene::World &world, const std::vector<std::pair<std::string, std::string>> &contacts);

  // Zmiana sceny: wszystkie `self` znikają (bez on_destroy).
  void reset_instances();

  // Wykonuje dowolny kod Lua w stanie gry (API: lua.eval). Zwraca wynik
  // jako tekst (wartości oddzielone tabulatorem) albo błąd.
  struct EvalResult {
    bool ok = true;
    std::string output;
  };
  EvalResult eval(std::string_view code);
};

} // namespace PimpEx::script
