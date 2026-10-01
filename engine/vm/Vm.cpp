#include "vm/Vm.hpp"
#include "core/Format.hpp"

#include <cmath>
#include <format>

namespace PimpEx::vm {

bool is_builtin_actor(std::string_view name) {
  return name == "Screen" || name == "Mouse" || name == "Scene";
}

std::optional<scene::FieldValue> read_value(const scene::World &world, const FrameContext &ctx,
                                            const std::string &actor, const std::string &field,
                                            std::string *error) {
  if (is_builtin_actor(actor)) {
    std::optional<double> value;
    if (actor == "Screen" && field == "w") value = ctx.screenWidth;
    if (actor == "Screen" && field == "h") value = ctx.screenHeight;
    if (actor == "Mouse" && field == "x") value = ctx.input.mouse_x();
    if (actor == "Mouse" && field == "y") value = ctx.input.mouse_y();
    if (actor == "Mouse" && field == "dx") value = ctx.input.mouse_dx();
    if (actor == "Mouse" && field == "dy") value = ctx.input.mouse_dy();
    if (actor == "Mouse" && field == "left") value = ctx.input.mouse_down(MouseButton::Left) ? 1 : 0;
    if (actor == "Mouse" && field == "right") value = ctx.input.mouse_down(MouseButton::Right) ? 1 : 0;
    if (actor == "Scene" && field == "frame") value = static_cast<double>(ctx.frame);
    if (!value && error)
      *error = std::format("{} has no field '{}'", actor, field);
    if (!value)
      return std::nullopt;
    return *value;
  }
  std::string message;
  auto value = world.get_field(actor, field, &message);
  if (value)
    return value;
  if (message.empty() && world.exists(actor))
    return 0.0; // a variable that was never set
  if (error)
    *error = message;
  return std::nullopt;
}

std::optional<double> read_field(const scene::World &world, const FrameContext &ctx,
                                 const std::string &actor, const std::string &field) {
  const auto value = read_value(world, ctx, actor, field);
  if (!value)
    return std::nullopt;
  if (const auto *number = std::get_if<double>(&*value))
    return *number;
  return std::nullopt;
}

std::string format_text(const scene::World &world, const FrameContext &ctx,
                        std::string_view text, const std::string &self) {
  std::string out;
  std::size_t pos = 0;
  while (pos < text.size()) {
    const auto open = text.find('{', pos);
    if (open == std::string_view::npos) {
      out.append(text.substr(pos));
      break;
    }
    const auto close = text.find('}', open);
    if (close == std::string_view::npos) {
      out.append(text.substr(pos));
      break;
    }
    out.append(text.substr(pos, open - pos));
    const std::string inside(text.substr(open + 1, close - open - 1));
    // {Actor.field}, {Actor.Component.field}, or a field of self: {field}, {Component.field}.
    const auto dot = inside.find('.');
    std::string actor = self, field = inside;
    if (dot != std::string::npos) {
      const auto head = inside.substr(0, dot);
      if (world.exists(head) || is_builtin_actor(head)) {
        actor = head;
        field = inside.substr(dot + 1);
      }
    }
    const auto value = read_value(world, ctx, actor, field);
    out += value ? scene::field_value_to_string(*value) : "?";
    pos = close + 1;
  }
  return out;
}

void Vm::report(int line, const std::string &message) {
  if (_reported.insert({line, message}).second)
    _host->runtime_error(line, message);
}

double Vm::random(double min, double max) {
  std::uniform_real_distribution<double> dist(min, max);
  return dist(_rng);
}

std::string Vm::actor(const std::string &name, int line) {
  if (name == "SELF") {
    if (_self.empty())
      report(line, "SELF only works in PROC ... FOR");
    return _self;
  }
  if (name == "OTHER") {
    if (_other.empty())
      report(line, "OTHER is empty – use TOUCH with a @tag or a component first");
    return _other;
  }
  return name;
}

double Vm::read(const Ref &ref, int line) {
  const auto name = actor(ref.actor, line);
  if (name.empty())
    return 0.0;
  std::string error;
  const auto value = read_value(*_world, *_ctx, name, ref.field, &error);
  if (!value) {
    report(line, error);
    return 0.0;
  }
  if (const auto *number = std::get_if<double>(&*value))
    return *number;
  report(line, std::format("{}.{} is text, not a number", name, ref.field));
  return 0.0;
}

double Vm::eval(const Value &value, int line) {
  switch (value.kind) {
  case Value::Kind::Number:
    return value.number;
  case Value::Kind::Ref: {
    const double v = read(value.ref, line);
    return value.negate ? -v : v;
  }
  case Value::Kind::Random:
    return random(value.min, value.max);
  case Value::Kind::Text:
    report(line, "quoted text can only be used in SET");
    return 0.0;
  }
  return 0.0;
}

// a and b are actors (also SELF/OTHER) or groups (@tag, Component).
// When b is a group, the first touched member becomes OTHER.
bool Vm::touch(const std::string &a, const std::string &b, int line) {
  auto members = [&](const std::string &selector) -> std::vector<entt::entity> {
    if (selector.starts_with('@') || _world->components().find(selector)) {
      std::vector<entt::entity> out;
      for (const auto &n : _world->query(selector))
        out.push_back(_world->find(n));
      return out;
    }
    const auto name = actor(selector, line);
    const auto e = name.empty() ? entt::null : _world->find(name);
    return e == entt::null ? std::vector<entt::entity>{} : std::vector{e};
  };
  const auto left = members(a);
  const auto right = members(b);
  for (const auto ea : left)
    for (const auto eb : right)
      if (_world->touching(ea, eb)) {
        _other = _world->name_of(eb);
        return true;
      }
  return false;
}

bool Vm::test(const Condition &cond, int line) {
  bool result = false;
  switch (cond.kind) {
  case Condition::Kind::KeyDown:
    result = _ctx->input.down(cond.key);
    break;
  case Condition::Kind::KeyPressed:
    result = _ctx->input.pressed(cond.key);
    break;
  case Condition::Kind::Touch:
    result = touch(cond.a, cond.b, line);
    break;
  case Condition::Kind::Exists:
    if (cond.a.starts_with('@'))
      result = !_world->query(cond.a).empty();
    else
      result = _world->exists(cond.a == "SELF" ? _self : cond.a == "OTHER" ? _other : cond.a);
    break;
  case Condition::Kind::Chance:
    result = random(0.0, 100.0) < eval(cond.left, line);
    break;
  case Condition::Kind::Compare: {
    const double l = eval(cond.left, line);
    const double r = eval(cond.right, line);
    switch (cond.op) {
    case CompareOp::Less: result = l < r; break;
    case CompareOp::LessEqual: result = l <= r; break;
    case CompareOp::Greater: result = l > r; break;
    case CompareOp::GreaterEqual: result = l >= r; break;
    case CompareOp::Equal: result = std::abs(l - r) < 1e-9; break;
    case CompareOp::NotEqual: result = std::abs(l - r) >= 1e-9; break;
    }
    break;
  }
  }
  return cond.negate ? !result : result;
}

void Vm::execute(const Instr &instr) {
  for (const auto &cond : instr.guard)
    if (!test(cond, instr.line))
      return;

  auto &world = *_world;
  const int line = instr.line;
  auto target = [&](const std::string &name) -> std::string {
    const auto resolved = actor(name, line);
    if (resolved.empty())
      return {};
    if (!world.exists(resolved)) {
      report(line, std::format("actor '{}' does not exist", resolved));
      return {};
    }
    return resolved;
  };
  auto setField = [&](const std::string &who, const std::string &field, const scene::FieldValue &v) {
    if (auto error = world.set_field(who, field, v))
      report(line, *error);
  };

  switch (instr.op) {
  case Op::Move: {
    const auto who = target(instr.target);
    if (who.empty())
      return;
    static constexpr const char *axes[] = {"x", "y", "z"};
    for (std::size_t i = 0; i < instr.values.size(); ++i)
      world.set(who, axes[i], *world.get(who, axes[i]) + eval(instr.values[i], line));
    return;
  }
  case Op::Set:
  case Op::Add:
  case Op::Mul: {
    if (is_builtin_actor(instr.ref.actor)) {
      report(line, std::format("fields of {} are read-only", instr.ref.actor));
      return;
    }
    const auto who = target(instr.ref.actor);
    if (who.empty())
      return;
    const auto &value = instr.values[0];
    if (instr.op == Op::Set && value.kind == Value::Kind::Text) {
      setField(who, instr.ref.field, value.text);
      return;
    }
    const double v = eval(value, line);
    if (instr.op == Op::Set) {
      setField(who, instr.ref.field, v);
      return;
    }
    const double current = read(Ref{who, instr.ref.field}, line);
    setField(who, instr.ref.field, instr.op == Op::Add ? current + v : current * v);
    return;
  }
  case Op::Spawn: {
    std::optional<std::pair<float, float>> position;
    if (instr.values.size() >= 2)
      position = std::pair{static_cast<float>(eval(instr.values[0], line)),
                           static_cast<float>(eval(instr.values[1], line))};
    const auto name = world.spawn(instr.target, instr.other, position);
    if (!name) {
      report(line, std::format("no archetype '{}'", instr.target));
      return;
    }
    if (instr.values.size() == 3)
      world.set(*name, "z", eval(instr.values[2], line));
    return;
  }
  case Op::Destroy: {
    const auto who = actor(instr.target, line);
    if (!who.empty())
      world.destroy(who); // a missing actor is fine – it may be gone already
    return;
  }
  case Op::Follow: {
    const auto who = target(instr.target);
    const auto goal = target(instr.other);
    if (who.empty() || goal.empty())
      return;
    const double speed = eval(instr.values[0], line);
    const bool is3d = world.mode() == scene::SceneMode::Mode3D;
    const double dx = *world.get(goal, "x") - *world.get(who, "x");
    const double dy = *world.get(goal, "y") - *world.get(who, "y");
    const double dz = is3d ? *world.get(goal, "z") - *world.get(who, "z") : 0.0;
    const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (distance < 1e-6)
      return;
    const double step = std::min(speed, distance) / distance;
    world.set(who, "x", *world.get(who, "x") + dx * step);
    world.set(who, "y", *world.get(who, "y") + dy * step);
    if (is3d)
      world.set(who, "z", *world.get(who, "z") + dz * step);
    return;
  }
  case Op::Text: {
    const auto who = target(instr.target);
    if (!who.empty())
      setField(who, "Text.value", instr.text);
    return;
  }
  case Op::Log:
    _host->log(format_text(world, *_ctx, instr.text, _self));
    return;
  case Op::Call: {
    auto args = instr.args;
    for (auto &arg : args)
      if (arg == "SELF" || arg == "OTHER")
        arg = actor(arg, line);
    _host->call_function(instr.target, args, line);
    return;
  }
  case Op::ProcAdd:
    _host->request_proc_add(instr.target, line);
    return;
  case Op::ProcRemove:
    _host->request_proc_remove(instr.target, line);
    return;
  case Op::Scene:
    _host->request_scene(instr.target, line);
    return;
  case Op::GuiShow:
  case Op::GuiHide:
  case Op::GuiToggle:
    _host->request_gui(instr.op, instr.target, line);
    return;
  }
}

void Vm::run_frame(const Program &program, scene::World &world, const FrameContext &ctx,
                   IVmHost &host) {
  _ctx = &ctx;
  _world = &world;
  _host = &host;
  for (const auto &proc : program.procs) {
    if (proc.mode == ProcMode::Every && ctx.frame % proc.every != 0)
      continue;
    if (proc.forEach.empty()) {
      _self.clear();
      _other.clear();
      for (const auto &instr : proc.body)
        execute(instr);
    } else {
      for (const auto &name : world.query(proc.forEach)) {
        if (!world.exists(name))
          continue; // destroyed by an earlier iteration
        _self = name;
        _other.clear();
        for (const auto &instr : proc.body) {
          if (!world.exists(_self))
            break;
          execute(instr);
        }
      }
      _self.clear();
    }
    if (proc.mode == ProcMode::Once)
      host.request_proc_remove(proc.name, proc.firstLine);
  }
  _ctx = nullptr;
  _world = nullptr;
  _host = nullptr;
}

void Vm::run_instructions(const std::vector<Instr> &instructions, scene::World &world,
                          const FrameContext &ctx, IVmHost &host) {
  _ctx = &ctx;
  _world = &world;
  _host = &host;
  _self.clear();
  _other.clear();
  for (const auto &instr : instructions)
    execute(instr);
  _ctx = nullptr;
  _world = nullptr;
  _host = nullptr;
}

} // namespace PimpEx::vm
