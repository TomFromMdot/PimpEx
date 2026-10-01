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

// Aktorzy specjalni, których nie ma w scene.xml, ale da się z nich czytać.
bool is_builtin_actor(const std::string &name);

struct ProgramCheckInput {
  const vm::Program &program;
  const scene::SceneData &scene;
  const vm::Program *library = nullptr;             // library.swap (jeśli jest)
  const std::vector<std::string> *sceneNames = nullptr; // do sprawdzenia SCENE
  const std::set<std::string> *luaFunctions = nullptr;  // do sprawdzenia CALL
  std::string file;
  const scene::ComponentRegistry *components = nullptr; // null = tylko wbudowane
  const std::set<std::string> *guiNames = nullptr;      // do sprawdzenia GUI_SHOW/HIDE/TOGGLE
};

// Sprawdza, czy program odwołuje się do istniejących aktorów, archetypów,
// procedur z biblioteki, scen i funkcji Lua.
void check_program_references(const ProgramCheckInput &input, Diagnostics &diagnostics);

struct CheckSummary {
  int scenes = 0;
  int procs = 0;
  int actors = 0;
  int scripts = 0;
  int components = 0; // komponenty użytkownika z components.xml
  int guis = 0;       // sceny GUI z gui/*.xml
};

// Pełna walidacja projektu bez uruchamiania gry (`pimpex check`).
Diagnostics check_project(const ProjectPaths &paths, CheckSummary &summary);

} // namespace PimpEx::boot
