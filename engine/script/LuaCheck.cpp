#include "script/LuaCheck.hpp"
#include "script/LuaErrors.hpp"

#include <sol/sol.hpp>

#include <format>

namespace PimpEx::script {

LuaCheckResult check_scripts(const boot::ProjectPaths &paths, Diagnostics &diagnostics) {
  LuaCheckResult result;
  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

  // Na górnym poziomie skryptu wolno rejestrować systemy – tu tylko atrapa.
  lua.set_function("system", [](sol::object, sol::object) {});

  std::set<std::string> builtins;
  for (const auto &[key, value] : lua.globals())
    if (key.is<std::string>())
      builtins.insert(key.as<std::string>());

  for (const auto &file : paths.script_files()) {
    const auto source = boot::read_file(file);
    if (!source)
      continue;
    // Kompilujemy i wykonujemy skrypt w pustym stanie (bez API silnika),
    // żeby poznać zdefiniowane funkcje. Dlatego skrypty powinny na górnym
    // poziomie tylko definiować funkcje i zmienne.
    auto loaded = lua.load(*source, "@" + paths.display(file));
    if (!loaded.valid()) {
      sol::error err = loaded;
      diagnostics.push_back(lua_error_to_diagnostic(err.what(), paths.display(file)));
      result.ok = false;
      continue;
    }
    sol::protected_function chunk = loaded;
    auto run = chunk();
    if (!run.valid()) {
      sol::error err = run;
      diagnostics.push_back(lua_error_to_diagnostic(err.what(), paths.display(file)));
      result.ok = false;
    }
  }

  for (const auto &[key, value] : lua.globals())
    if (key.is<std::string>() && value.get_type() == sol::type::function &&
        !builtins.contains(key.as<std::string>()))
      result.functions.insert(key.as<std::string>());

  // Zachowania: każdy plik musi zwracać tabelę z funkcjami.
  for (const auto &file : paths.behavior_files()) {
    const auto source = boot::read_file(file);
    if (!source)
      continue;
    const auto display = paths.display(file);
    auto loaded = lua.load(*source, "@" + display);
    if (!loaded.valid()) {
      sol::error err = loaded;
      diagnostics.push_back(lua_error_to_diagnostic(err.what(), display));
      result.ok = false;
      continue;
    }
    sol::protected_function chunk = loaded;
    auto run = chunk();
    if (!run.valid()) {
      sol::error err = run;
      diagnostics.push_back(lua_error_to_diagnostic(err.what(), display));
      result.ok = false;
      continue;
    }
    sol::object value = run;
    if (value.get_type() != sol::type::table) {
      diagnostics.push_back({Diagnostic::Severity::Error, display, 0,
                             "zachowanie musi kończyć się `return { on_start = ..., on_update = ... }`"});
      result.ok = false;
      continue;
    }
    for (const auto &[k, v] : value.as<sol::table>()) {
      if (!k.is<std::string>())
        continue;
      const auto name = k.as<std::string>();
      static const std::set<std::string> known = {"on_start", "on_update", "on_touch", "on_destroy"};
      if (v.get_type() == sol::type::function && name.starts_with("on_") && !known.contains(name))
        diagnostics.push_back({Diagnostic::Severity::Warning, display, 0,
                               std::format("'{}' nie zostanie wywołane – silnik zna on_start, "
                                           "on_update, on_touch, on_destroy",
                                           name)});
    }
    result.behaviors.insert(file.stem().string());
  }
  return result;
}

} // namespace PimpEx::script
