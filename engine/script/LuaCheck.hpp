#pragma once

#include "boot/ProjectPaths.hpp"
#include "core/Diagnostic.hpp"

#include <set>
#include <string>

namespace PimpEx::script {

struct LuaCheckResult {
  bool ok = true;                  // all scripts loaded
  std::set<std::string> functions; // globals defined in scripts/*.lua
  std::set<std::string> behaviors; // valid scripts/behaviors/
};

// Loads scripts in a separate empty Lua state: checks syntax and collects the
// function names CALL can use.
LuaCheckResult check_scripts(const boot::ProjectPaths &paths, Diagnostics &diagnostics);

} // namespace PimpEx::script
