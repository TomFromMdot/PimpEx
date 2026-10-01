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

  // Names given by SPAWN ... AS are known too.
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
  // An actor or, where allowed, a group (@tag / Component).
  auto checkActor = [&](int line, const std::string &actor, bool allowGroup = false) {
    if (actor.empty() || known.contains(actor) || is_builtin_actor(actor))
      return;
    if (actor == "SELF") {
      if (currentProc && currentProc->forEach.empty())
        error(line, "SELF only works in PROC ... FOR Component / FOR @tag");
      return;
    }
    if (actor == "OTHER")
      return;
    if (actor.starts_with('@')) {
      if (!allowGroup)
        error(line, std::format("'{}' – tags only work in TOUCH, EXISTS and FOR", actor));
      else if (!tags.contains(actor.substr(1)))
        warn(line, std::format("no actor or archetype in scene.xml has the tag '{}'", actor.substr(1)));
      return;
    }
    if (allowGroup && registry.find(actor))
      return; // TOUCH Player Health – any actor with the component
    if (input.scene.archetypes.contains(actor))
      return; // e.g. FOLLOW an archetype – the name is generated
    warn(line, std::format("actor '{}' is not in scene.xml (unless SPAWN creates it)", actor));
  };
  // "x", "score" (a variable) or "Component.field".
  auto checkField = [&](int line, const std::string &field) {
    const auto dot = field.find('.');
    if (dot == std::string::npos)
      return;
    const auto componentName = field.substr(0, dot);
    const auto fieldName = field.substr(dot + 1);
    const auto *component = registry.find(componentName);
    if (!component) {
      const auto hint = scene::closest_name(componentName, registry.names());
      error(line, std::format("no component '{}'{}", componentName,
                              hint.empty() ? "" : std::format(" (did you mean {}?)", hint)));
      return;
    }
    if (!component->field(fieldName)) {
      std::vector<std::string> names;
      for (const auto &f : component->fields)
        names.push_back(f.name);
      const auto hint = scene::closest_name(fieldName, names);
      error(line, std::format("component {} has no field '{}'{}", componentName, fieldName,
                              hint.empty() ? "" : std::format(" (did you mean {}?)", hint)));
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
        error(proc.firstLine, std::format("FOR {}: no such component{} (write tags as @{})",
                                          proc.forEach,
                                          hint.empty() ? "" : std::format(" – did you mean {}?", hint),
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
          error(line, std::format("fields of {} are read-only", instr.ref.actor));
        else
          checkActor(line, instr.ref.actor);
        checkField(line, instr.ref.field);
        break;
      case vm::Op::Spawn:
        if (!input.scene.archetypes.contains(instr.target))
          error(line, std::format("no archetype '{}' in scene.xml", instr.target));
        break;
      case vm::Op::ProcAdd:
        if (!input.library || !input.library->find(instr.target))
          error(line, std::format("no procedure '{}' in library.swap", instr.target));
        break;
      case vm::Op::Scene:
        if (input.sceneNames &&
            std::find(input.sceneNames->begin(), input.sceneNames->end(), instr.target) ==
                input.sceneNames->end())
          error(line, std::format("no scene '{}' (directory scenes/{})", instr.target, instr.target));
        break;
      case vm::Op::Call:
        if (input.luaFunctions && !input.luaFunctions->contains(instr.target))
          error(line, std::format("no Lua function '{}' in scripts/", instr.target));
        break;
      case vm::Op::GuiShow:
      case vm::Op::GuiHide:
      case vm::Op::GuiToggle:
        if (input.guiNames && !input.guiNames->contains(instr.target))
          error(line, std::format("no GUI '{}' (file gui/{}.xml)", instr.target, instr.target));
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
    missing(paths.project_xml(), "missing file – this is not a PimpEx project (use `pimpex init`)");
    return diagnostics;
  }
  const auto config =
      parse_project_xml(*projectXml, paths.display(paths.project_xml()), diagnostics);

  const auto sceneNames = paths.scene_names();
  if (std::find(sceneNames.begin(), sceneNames.end(), config.startScene) == sceneNames.end())
    missing(paths.project_xml(),
            std::format("start scene '{}' does not exist (no directory scenes/{})",
                        config.startScene, config.startScene));

  auto registry = scene::ComponentRegistry::with_builtins();
  if (const auto componentsXml = read_file(paths.components_xml())) {
    registry->load_custom(*componentsXml, paths.display(paths.components_xml()), diagnostics);
    for (const auto &c : registry->all())
      summary.components += c.builtin ? 0 : 1;
  }

  const auto lua = script::check_scripts(paths, diagnostics);
  summary.scripts = static_cast<int>(paths.script_files().size());
  const std::set<std::string> *luaFunctions = lua.ok ? &lua.functions : nullptr;

  // GUI names first – a button may show another GUI.
  std::vector<gui::GuiScene> guis;
  std::set<std::string> guiNames;
  for (const auto &file : paths.gui_files()) {
    if (const auto text = read_file(file)) {
      guis.push_back(parse_gui_xml(*text, paths.display(file), file.stem().string(), diagnostics));
      guiNames.insert(guis.back().name);
    }
  }
  summary.guis = static_cast<int>(guis.size());
  for (const auto &scene : guis)
    for (const auto &element : scene.elements)
      for (const auto &instr : element.click) {
        auto report = [&](std::string message) {
          diagnostics.push_back({Diagnostic::Severity::Error, scene.file, instr.line, std::move(message)});
        };
        if (instr.op == vm::Op::Scene &&
            std::find(sceneNames.begin(), sceneNames.end(), instr.target) == sceneNames.end())
          report(std::format("no scene '{}' (directory scenes/{})", instr.target, instr.target));
        if (instr.op == vm::Op::Call && luaFunctions && !luaFunctions->contains(instr.target))
          report(std::format("no Lua function '{}' in scripts/", instr.target));
        if ((instr.op == vm::Op::GuiShow || instr.op == vm::Op::GuiHide || instr.op == vm::Op::GuiToggle) &&
            !guiNames.contains(instr.target))
          report(std::format("no GUI '{}' (file gui/{}.xml)", instr.target, instr.target));
        if (instr.op == vm::Op::ProcAdd)
          report("PROC_ADD does not work in GUI – a button does not know which scene's library.swap to use");
      }

  for (const auto &sceneName : sceneNames) {
    ++summary.scenes;
    const auto xmlPath = paths.scene_xml(sceneName);
    const auto xml = read_file(xmlPath);
    if (!xml) {
      missing(xmlPath, "missing scene.xml");
      continue;
    }
    const auto data =
        parse_scene_xml(*xml, paths.display(xmlPath), sceneName, diagnostics, registry.get());
    summary.actors += static_cast<int>(data.actors.size());
    for (const auto &name : data.gui)
      if (!guiNames.contains(name))
        diagnostics.push_back({Diagnostic::Severity::Error, paths.display(xmlPath), data.guiLine,
                               std::format("<Scene gui>: no GUI '{}' (file gui/{}.xml)", name, name)});

    auto checkBehaviors = [&](const scene::ActorDef &def) {
      for (const auto &behavior : def.behaviors)
        if (!fs::exists(paths.behavior_file(behavior.name)))
          diagnostics.push_back({Diagnostic::Severity::Error, paths.display(xmlPath), def.line,
                                 std::format("'{}': no behavior '{}' (file {})", def.name,
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
      missing(swapPath, "missing scene.swap");
      continue;
    }
    auto parsed = vm::parse_program(*swap, paths.display(swapPath));
    diagnostics.insert(diagnostics.end(), parsed.diagnostics.begin(), parsed.diagnostics.end());
    summary.procs += static_cast<int>(parsed.program.procs.size());

    check_program_references({parsed.program, data, library ? &*library : nullptr, &sceneNames,
                              luaFunctions, paths.display(swapPath), registry.get(), &guiNames},
                             diagnostics);
    if (library)
      check_program_references({*library, data, &*library, &sceneNames, luaFunctions,
                                paths.display(paths.library_swap(sceneName)), registry.get(), &guiNames},
                               diagnostics);
  }
  return diagnostics;
}

} // namespace PimpEx::boot
