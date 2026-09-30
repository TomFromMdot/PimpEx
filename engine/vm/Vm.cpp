#include "vm/Vm.hpp"
#include "core/Format.hpp"

#include <cmath>
#include <format>

namespace PimpEx::vm {

std::optional<double> read_field(const scene::World &world, const FrameContext &ctx,
                                 const std::string &actor, const std::string &field) {
  if (actor == "Screen") {
    if (field == "w") return ctx.screenWidth;
    if (field == "h") return ctx.screenHeight;
    return std::nullopt;
  }
  if (actor == "Mouse") {
    if (field == "x") return ctx.input.mouse_x();
    if (field == "y") return ctx.input.mouse_y();
    return std::nullopt;
  }
  if (actor == "Scene") {
    if (field == "frame") return static_cast<double>(ctx.frame);
    return std::nullopt;
  }
  if (!world.exists(actor))
    return std::nullopt;
  // Zmienna, której jeszcze nie ustawiono, ma wartość 0.
  return world.get(actor, field).value_or(0.0);
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
    const auto dot = inside.rfind('.');
    const auto actor = dot == std::string::npos ? self : inside.substr(0, dot);
    const auto field = dot == std::string::npos ? inside : inside.substr(dot + 1);
    const auto value = read_field(world, ctx, actor, field);
    out += value ? format_number(*value) : "?";
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

double Vm::read(const Ref &ref, int line) {
  const auto value = read_field(*_world, *_ctx, ref.actor, ref.field);
  if (!value) {
    if (ref.actor == "Screen" || ref.actor == "Mouse" || ref.actor == "Scene")
      report(line, std::format("{} nie ma pola '{}'", ref.actor, ref.field));
    else
      report(line, std::format("aktor '{}' nie istnieje", ref.actor));
    return 0.0;
  }
  return *value;
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
  }
  return 0.0;
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
    result = _world->touching(cond.a, cond.b);
    break;
  case Condition::Kind::Exists:
    result = _world->exists(cond.a);
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
  auto requireActor = [&](const std::string &name) {
    if (world.exists(name))
      return true;
    report(line, std::format("aktor '{}' nie istnieje", name));
    return false;
  };

  switch (instr.op) {
  case Op::Move: {
    if (!requireActor(instr.target))
      return;
    const double dx = eval(instr.values[0], line);
    const double dy = eval(instr.values[1], line);
    world.set(instr.target, "x", *world.get(instr.target, "x") + dx);
    world.set(instr.target, "y", *world.get(instr.target, "y") + dy);
    return;
  }
  case Op::Set:
  case Op::Add:
  case Op::Mul: {
    const auto &ref = instr.ref;
    if (ref.actor == "Screen" || ref.actor == "Mouse" || ref.actor == "Scene") {
      report(line, std::format("pola {} są tylko do odczytu", ref.actor));
      return;
    }
    if (!requireActor(ref.actor))
      return;
    const double value = eval(instr.values[0], line);
    const double current = world.get(ref.actor, ref.field).value_or(0.0);
    const double next = instr.op == Op::Set   ? value
                        : instr.op == Op::Add ? current + value
                                              : current * value;
    world.set(ref.actor, ref.field, next);
    return;
  }
  case Op::Spawn: {
    std::optional<std::pair<float, float>> position;
    if (instr.values.size() == 2)
      position = std::pair{static_cast<float>(eval(instr.values[0], line)),
                           static_cast<float>(eval(instr.values[1], line))};
    if (!world.spawn(instr.target, instr.other, position))
      report(line, std::format("nie ma archetypu '{}'", instr.target));
    return;
  }
  case Op::Destroy:
    world.destroy(instr.target); // brak aktora to nie błąd – mógł już zniknąć
    return;
  case Op::Follow: {
    if (!requireActor(instr.target) || !requireActor(instr.other))
      return;
    const double speed = eval(instr.values[0], line);
    const double x = *world.get(instr.target, "x"), y = *world.get(instr.target, "y");
    const double dx = *world.get(instr.other, "x") - x;
    const double dy = *world.get(instr.other, "y") - y;
    const double distance = std::hypot(dx, dy);
    if (distance < 1e-6)
      return;
    const double step = std::min(speed, distance);
    world.set(instr.target, "x", x + dx / distance * step);
    world.set(instr.target, "y", y + dy / distance * step);
    return;
  }
  case Op::Text: {
    if (!requireActor(instr.target))
      return;
    auto &registry = world.registry();
    const auto e = world.find(instr.target);
    if (auto *label = registry.try_get<scene::Label>(e))
      label->text = instr.text;
    else
      registry.emplace<scene::Label>(e).text = instr.text;
    return;
  }
  case Op::Log:
    _host->log(format_text(world, *_ctx, instr.text, ""));
    return;
  case Op::Call:
    _host->call_function(instr.target, instr.args, line);
    return;
  case Op::ProcAdd:
    _host->request_proc_add(instr.target, line);
    return;
  case Op::ProcRemove:
    _host->request_proc_remove(instr.target, line);
    return;
  case Op::Scene:
    _host->request_scene(instr.target, line);
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
    for (const auto &instr : proc.body)
      execute(instr);
    if (proc.mode == ProcMode::Once)
      host.request_proc_remove(proc.name, proc.firstLine);
  }
  _ctx = nullptr;
  _world = nullptr;
  _host = nullptr;
}

} // namespace PimpEx::vm
