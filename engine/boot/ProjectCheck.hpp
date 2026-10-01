#pragma once

#include "boot/ProjectPaths.hpp"
#include "core/Diagnostic.hpp"
#include "scene/ComponentRegistry.hpp"
#include "scene/SceneData.hpp"
#include "vm/Program.hpp"

#include <set>
#include <string>
#include <vector>

namespace PimpEx::boot {

bool is_builtin_actor(const std::string &name);

struct ProgramCheckInput {
  const vm::Program &program;
  const scene::SceneData &scene;
  const vm::Program *library = nullptr;             // library.swap, if any
  const std::vector<std::string> *sceneNames = nullptr; // for SCENE
  const std::set<std::string> *luaFunctions = nullptr;  // for CALL
  std::string file;
  const scene::ComponentRegistry *components = nullptr; // null = built-ins only
  const std::set<std::string> *guiNames = nullptr;      // for GUI_SHOW/HIDE/TOGGLE
};

// Checks references to actors, archetypes, library procs, scenes, Lua functions and GUIs.
void check_program_references(const ProgramCheckInput &input, Diagnostics &diagnostics);

struct CheckSummary {
  int scenes = 0;
  int procs = 0;
  int actors = 0;
  int scripts = 0;
  int components = 0; // from components.xml
  int guis = 0;
};

// Validates the whole project without running it (`pimpex check`).
Diagnostics check_project(const ProjectPaths &paths, CheckSummary &summary);

} // namespace PimpEx::boot
