#pragma once

#include "boot/ProjectPaths.hpp"
#include "core/Diagnostic.hpp"

#include <set>
#include <string>

namespace PimpEx::script {

struct LuaCheckResult {
  bool ok = true;                  // wszystkie skrypty się wczytały
  std::set<std::string> functions; // globalne funkcje zdefiniowane w scripts/*.lua
};

// Wczytuje skrypty w osobnym, pustym stanie Lua: sprawdza składnię
// i zbiera nazwy funkcji, które da się wywołać przez CALL.
LuaCheckResult check_scripts(const boot::ProjectPaths &paths, Diagnostics &diagnostics);

} // namespace PimpEx::script
