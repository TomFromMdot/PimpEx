#include "boot/ProjectCheck.hpp"
#include "boot/XmlLoader.hpp"
#include "script/LuaCheck.hpp"
#include "vm/Parser.hpp"
#include "vm/Vm.hpp"

#include <algorithm>
#include <format>

namespace PimpEx::boot {

bool is_builtin_actor(const std::string &name) { return vm::is_builtin_actor(name); }

void check_program_references(const ProgramCheckInput &input, Diagnostics &diagnostics) {
  static const auto builtins = scene::ComponentRegistry::with_builtins();
  const auto &registry = input.components ? *input.components : *builtins;

  std::set<std::string> known, tags;
  for (const auto &actor : input.scene.actors) {
    known.insert(actor.name);
    tags.insert(actor.tags.begin(), actor.tags.end());
  }
  for (const auto &[name, def] : input.scene.archetypes)
    tags.insert(def.tags.begin(), def.tags.end());

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
  auto warn = [&](int line, std::string message) {
    if (reported.insert({line, message}).second)
      diagnostics.push_back({Diagnostic::Severity::Warning, input.file, line, std::move(message)});
  };
  auto error = [&](int line, std::string message) {
    if (reported.insert({line, message}).second)
      diagnostics.push_back({Diagnostic::Severity::Error, input.file, line, std::move(message)});
  };

  const vm::Proc *currentProc = nullptr;
  // Aktor albo grupa (@tag / Komponent – tylko tam, gdzie wolno).
  auto checkActor = [&](int line, const std::string &actor, bool allowGroup = false) {
    if (actor.empty() || known.contains(actor) || is_builtin_actor(actor))
      return;
    if (actor == "SELF") {
      if (currentProc && currentProc->forEach.empty())
        error(line, "SELF działa tylko w PROC ... FOR Komponent / FOR @tag");
      return;
    }
    if (actor == "OTHER")
      return;
    if (actor.starts_with('@')) {
      if (!allowGroup)
        error(line, std::format("'{}' – tagu można użyć tylko w TOUCH, EXISTS i FOR", actor));
      else if (!tags.contains(actor.substr(1)))
        warn(line, std::format("żaden aktor ani archetyp w scene.xml nie ma tagu '{}'", actor.substr(1)));
      return;
    }
    if (allowGroup && registry.find(actor))
      return; // TOUCH Player Health – dowolny aktor z komponentem
    if (input.scene.archetypes.contains(actor))
      return; // np. FOLLOW po archetypie – nazwa nadana automatycznie
    warn(line, std::format("aktor '{}' nie istnieje w scene.xml (chyba że powstanie przez SPAWN)", actor));
  };
  // Pole: "x", "score" (zmienna) albo "Komponent.pole".
  auto checkField = [&](int line, const std::string &field) {
    const auto dot = field.find('.');
    if (dot == std::string::npos)
      return;
    const auto componentName = field.substr(0, dot);
    const auto fieldName = field.substr(dot + 1);
    const auto *component = registry.find(componentName);
    if (!component) {
      const auto hint = scene::closest_name(componentName, registry.names());
      error(line, std::format("nie ma komponentu '{}'{}", componentName,
                              hint.empty() ? "" : std::format(" (czy chodziło o {}?)", hint)));
      return;
    }
    if (!component->field(fieldName)) {
      std::vector<std::string> names;
      for (const auto &f : component->fields)
        names.push_back(f.name);
      const auto hint = scene::closest_name(fieldName, names);
      error(line, std::format("komponent {} nie ma pola '{}'{}", componentName, fieldName,
                              hint.empty() ? "" : std::format(" (czy chodziło o {}?)", hint)));
    }
  };
  auto checkValue = [&](int line, const vm::Value &value) {
    if (value.kind == vm::Value::Kind::Ref) {
      checkActor(line, value.ref.actor);
      checkField(line, value.ref.field);
    }
  };

  for (const auto &proc : input.program.procs) {
    currentProc = &proc;
    if (!proc.forEach.empty()) {
      if (proc.forEach.starts_with('@'))
        checkActor(proc.firstLine, proc.forEach, true);
      else if (!registry.find(proc.forEach)) {
        const auto hint = scene::closest_name(proc.forEach, registry.names());
        error(proc.firstLine, std::format("FOR {}: nie ma takiego komponentu{} (tag pisz jako @{})",
                                          proc.forEach,
                                          hint.empty() ? "" : std::format(" – czy chodziło o {}?", hint),
                                          proc.forEach));
      }
    }
    for (const auto &instr : proc.body) {
      const int line = instr.line;
      for (const auto &cond : instr.guard) {
        const bool group = cond.kind == vm::Condition::Kind::Touch || cond.kind == vm::Condition::Kind::Exists;
        checkActor(line, cond.a, group);
        checkActor(line, cond.b, group);
        checkValue(line, cond.left);
        checkValue(line, cond.right);
      }
      for (const auto &value : instr.values)
        checkValue(line, value);

      switch (instr.op) {
      case vm::Op::Move:
      case vm::Op::Destroy:
      case vm::Op::Text:
        checkActor(line, instr.target);
        break;
      case vm::Op::Follow:
        checkActor(line, instr.target);
        checkActor(line, instr.other);
        break;
      case vm::Op::Set:
      case vm::Op::Add:
      case vm::Op::Mul:
        if (is_builtin_actor(instr.ref.actor))
          error(line, std::format("pola {} są tylko do odczytu", instr.ref.actor));
        else
          checkActor(line, instr.ref.actor);
        checkField(line, instr.ref.field);
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
          error(line, std::format("nie ma sceny '{}' (katalog scenes/{})", instr.target, instr.target));
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

  // Komponenty użytkownika.
  auto registry = scene::ComponentRegistry::with_builtins();
  if (const auto componentsXml = read_file(paths.components_xml())) {
    registry->load_custom(*componentsXml, paths.display(paths.components_xml()), diagnostics);
    for (const auto &c : registry->all())
      summary.components += c.builtin ? 0 : 1;
  }

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
    const auto data =
        parse_scene_xml(*xml, paths.display(xmlPath), sceneName, diagnostics, registry.get());
    summary.actors += static_cast<int>(data.actors.size());

    // Każde <Behavior name="x"> potrzebuje pliku scripts/behaviors/x.lua.
    auto checkBehaviors = [&](const scene::ActorDef &def) {
      for (const auto &behavior : def.behaviors)
        if (!fs::exists(paths.behavior_file(behavior.name)))
          diagnostics.push_back({Diagnostic::Severity::Error, paths.display(xmlPath), def.line,
                                 std::format("'{}': nie ma zachowania '{}' (plik {})", def.name,
                                             behavior.name,
                                             paths.display(paths.behavior_file(behavior.name)))});
    };
    for (const auto &def : data.actors)
      checkBehaviors(def);
    for (const auto &[name, def] : data.archetypes)
      checkBehaviors(def);

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
                              luaFunctions, paths.display(swapPath), registry.get()},
                             diagnostics);
    if (library)
      check_program_references({*library, data, &*library, &sceneNames, luaFunctions,
                                paths.display(paths.library_swap(sceneName)), registry.get()},
                               diagnostics);
  }
  return diagnostics;
}

} // namespace PimpEx::boot
