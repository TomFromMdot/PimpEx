#include "boot/ProjectCheck.hpp"
#include "boot/XmlLoader.hpp"
#include "script/LuaCheck.hpp"
#include "vm/Parser.hpp"

#include <algorithm>
#include <format>

namespace PimpEx::boot {

bool is_builtin_actor(const std::string &name) {
  return name == "Screen" || name == "Mouse" || name == "Scene";
}

void check_program_references(const ProgramCheckInput &input, Diagnostics &diagnostics) {
  std::set<std::string> known;
  for (const auto &actor : input.scene.actors)
    known.insert(actor.name);

  // Nazwy nadawane przez SPAWN ... AS nazwa też są znane.
  auto collectSpawnNames = [&](const vm::Program &program) {
    for (const auto &proc : program.procs)
      for (const auto &instr : proc.body)
        if (instr.op == vm::Op::Spawn && !instr.other.empty())
          known.insert(instr.other);
  };
  collectSpawnNames(input.program);
  if (input.library)
    collectSpawnNames(*input.library);

  std::set<std::pair<int, std::string>> reported;
  auto warnActor = [&](int line, const std::string &actor) {
    if (actor.empty() || known.contains(actor) || is_builtin_actor(actor))
      return;
    if (input.scene.archetypes.contains(actor))
      return; // np. FOLLOW po archetypie – nazwa nadana automatycznie
    if (!reported.insert({line, actor}).second)
      return;
    diagnostics.push_back({Diagnostic::Severity::Warning, input.file, line,
                           std::format("aktor '{}' nie istnieje w scene.xml (chyba że powstanie "
                                       "przez SPAWN)",
                                       actor)});
  };
  auto error = [&](int line, std::string message) {
    diagnostics.push_back({Diagnostic::Severity::Error, input.file, line, std::move(message)});
  };
  auto checkValue = [&](int line, const vm::Value &value) {
    if (value.kind == vm::Value::Kind::Ref)
      warnActor(line, value.ref.actor);
  };

  for (const auto &proc : input.program.procs) {
    for (const auto &instr : proc.body) {
      const int line = instr.line;
      for (const auto &cond : instr.guard) {
        warnActor(line, cond.a);
        warnActor(line, cond.b);
        checkValue(line, cond.left);
        checkValue(line, cond.right);
      }
      for (const auto &value : instr.values)
        checkValue(line, value);

      switch (instr.op) {
      case vm::Op::Move:
      case vm::Op::Destroy:
      case vm::Op::Text:
        warnActor(line, instr.target);
        break;
      case vm::Op::Follow:
        warnActor(line, instr.target);
        warnActor(line, instr.other);
        break;
      case vm::Op::Set:
      case vm::Op::Add:
      case vm::Op::Mul:
        if (is_builtin_actor(instr.ref.actor))
          error(line, std::format("pola {} są tylko do odczytu", instr.ref.actor));
        else
          warnActor(line, instr.ref.actor);
        break;
      case vm::Op::Spawn:
        if (!input.scene.archetypes.contains(instr.target))
          error(line, std::format("nie ma archetypu '{}' w scene.xml", instr.target));
        break;
      case vm::Op::ProcAdd:
        if (!input.library || !input.library->find(instr.target))
          error(line, std::format("nie ma procedury '{}' w library.swap", instr.target));
        break;
      case vm::Op::Scene:
        if (input.sceneNames &&
            std::find(input.sceneNames->begin(), input.sceneNames->end(), instr.target) ==
                input.sceneNames->end())
          error(line, std::format("nie ma sceny '{}' (katalog scenes/{})", instr.target,
                                  instr.target));
        break;
      case vm::Op::Call:
        if (input.luaFunctions && !input.luaFunctions->contains(instr.target))
          error(line, std::format("nie ma funkcji Lua '{}' w katalogu scripts/", instr.target));
        break;
      case vm::Op::Log:
      case vm::Op::ProcRemove:
        break;
      }
    }
  }
}

Diagnostics check_project(const ProjectPaths &paths, CheckSummary &summary) {
  Diagnostics diagnostics;
  auto missing = [&](const fs::path &path, std::string message) {
    diagnostics.push_back(
        {Diagnostic::Severity::Error, paths.display(path), 0, std::move(message)});
  };

  const auto projectXml = read_file(paths.project_xml());
  if (!projectXml) {
    missing(paths.project_xml(), "brak pliku – to nie jest projekt PimpEx (użyj `pimpex init`)");
    return diagnostics;
  }
  const auto config =
      parse_project_xml(*projectXml, paths.display(paths.project_xml()), diagnostics);

  const auto sceneNames = paths.scene_names();
  if (std::find(sceneNames.begin(), sceneNames.end(), config.startScene) == sceneNames.end())
    missing(paths.project_xml(),
            std::format("scena startowa '{}' nie istnieje (brak katalogu scenes/{})",
                        config.startScene, config.startScene));

  // Lua: składnia skryptów i lista zdefiniowanych funkcji (dla CALL).
  const auto lua = script::check_scripts(paths, diagnostics);
  summary.scripts = static_cast<int>(paths.script_files().size());

  for (const auto &sceneName : sceneNames) {
    ++summary.scenes;
    const auto xmlPath = paths.scene_xml(sceneName);
    const auto xml = read_file(xmlPath);
    if (!xml) {
      missing(xmlPath, "brak pliku scene.xml");
      continue;
    }
    const auto data = parse_scene_xml(*xml, paths.display(xmlPath), sceneName, diagnostics);
    summary.actors += static_cast<int>(data.actors.size());

    std::optional<vm::Program> library;
    if (const auto libraryText = read_file(paths.library_swap(sceneName))) {
      auto parsed = vm::parse_program(*libraryText, paths.display(paths.library_swap(sceneName)));
      diagnostics.insert(diagnostics.end(), parsed.diagnostics.begin(), parsed.diagnostics.end());
      library = std::move(parsed.program);
    }

    const auto swapPath = paths.scene_swap(sceneName);
    const auto swap = read_file(swapPath);
    if (!swap) {
      missing(swapPath, "brak pliku scene.swap");
      continue;
    }
    auto parsed = vm::parse_program(*swap, paths.display(swapPath));
    diagnostics.insert(diagnostics.end(), parsed.diagnostics.begin(), parsed.diagnostics.end());
    summary.procs += static_cast<int>(parsed.program.procs.size());

    const std::set<std::string> *luaFunctions = lua.ok ? &lua.functions : nullptr;
    check_program_references({parsed.program, data, library ? &*library : nullptr, &sceneNames,
                              luaFunctions, paths.display(swapPath)},
                             diagnostics);
    if (library)
      check_program_references({*library, data, &*library, &sceneNames, luaFunctions,
                                paths.display(paths.library_swap(sceneName))},
                               diagnostics);
  }
  return diagnostics;
}

} // namespace PimpEx::boot
