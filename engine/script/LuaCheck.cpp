#include "script/LuaCheck.hpp"
#include "script/LuaErrors.hpp"

#include <sol/sol.hpp>

#include <format>

namespace PimpEx::script {

LuaCheckResult check_scripts(const boot::ProjectPaths &paths, Diagnostics &diagnostics) {
  LuaCheckResult result;
  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

  // Scripts may register systems at the top level – a stub is enough here.
  lua.set_function("system", [](sol::object, sol::object) {});

  std::set<std::string> builtins;
  for (const auto &[key, value] : lua.globals())
    if (key.is<std::string>())
      builtins.insert(key.as<std::string>());

  for (const auto &file : paths.script_files()) {
    const auto source = boot::read_file(file);
    if (!source)
      continue;
    // Runs the script in an empty state (no engine API) to learn its functions.
    // That is why scripts should only define functions and variables at the top level.
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
                             "a behavior must end with `return { on_start = ..., on_update = ... }`"});
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
                               std::format("'{}' will never be called – the engine knows on_start, "
                                           "on_update, on_touch, on_destroy",
                                           name)});
    }
    result.behaviors.insert(file.stem().string());
  }
  return result;
}

} // namespace PimpEx::script
