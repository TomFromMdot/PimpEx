#include "script/LuaCheck.hpp"
#include "script/LuaErrors.hpp"

#include <sol/sol.hpp>

namespace PimpEx::script {

LuaCheckResult check_scripts(const boot::ProjectPaths &paths, Diagnostics &diagnostics) {
  LuaCheckResult result;
  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

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
  return result;
}

} // namespace PimpEx::script
