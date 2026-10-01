#pragma once

#include "scene/World.hpp"
#include "vm/Input.hpp"
#include "vm/Program.hpp"

#include <random>
#include <set>
#include <string>
#include <vector>

namespace PimpEx::vm {

struct FrameContext {
  const IInput &input;
  int screenWidth = 800;
  int screenHeight = 600;
  long frame = 0;
};

// What the VM delegates to Boot.
class IVmHost {
public:
  virtual ~IVmHost() = default;
  virtual void call_function(const std::string &name, const std::vector<std::string> &args,
                             int line) = 0;
  virtual void request_proc_add(const std::string &name, int line) = 0;
  virtual void request_proc_remove(const std::string &name, int line) = 0;
  virtual void request_scene(const std::string &name, int line) = 0;
  virtual void request_gui(Op op, const std::string &name, int line) = 0;
  virtual void log(const std::string &message) = 0;
  virtual void runtime_error(int line, const std::string &message) = 0;
};

class Vm {
  std::mt19937 _rng;
  std::set<std::pair<int, std::string>> _reported; // report each error once

  const FrameContext *_ctx = nullptr;
  scene::World *_world = nullptr;
  IVmHost *_host = nullptr;
  std::string _self;  // SELF in PROC ... FOR
  std::string _other; // OTHER – last actor found by TOUCH

  void report(int line, const std::string &message);
  // SELF/OTHER -> actor name (empty and reported when unset).
  std::string actor(const std::string &name, int line);
  bool touch(const std::string &a, const std::string &b, int line);
  double read(const Ref &ref, int line);
  double eval(const Value &value, int line);
  bool test(const Condition &cond, int line);
  void execute(const Instr &instr);

public:
  explicit Vm(unsigned seed = std::random_device{}()) : _rng(seed) {}

  void run_frame(const Program &program, scene::World &world, const FrameContext &ctx,
                 IVmHost &host);

  // Instructions outside a PROC, e.g. a GUI button click.
  void run_instructions(const std::vector<Instr> &instructions, scene::World &world,
                        const FrameContext &ctx, IVmHost &host);

  // After a program swap the same errors may be reported again.
  void reset_reports() { _reported.clear(); }

  double random(double min, double max);
};

// Read-only actors that are not in scene.xml: Screen, Mouse, Scene.
bool is_builtin_actor(std::string_view name);

// Includes the built-in actors. An unset variable reads as 0.
std::optional<scene::FieldValue> read_value(const scene::World &world, const FrameContext &ctx,
                                            const std::string &actor, const std::string &field,
                                            std::string *error = nullptr);
std::optional<double> read_field(const scene::World &world, const FrameContext &ctx,
                                 const std::string &actor, const std::string &field);

// Replaces {Actor.field} and {field} (a field of `self`) with values.
std::string format_text(const scene::World &world, const FrameContext &ctx,
                        std::string_view text, const std::string &self);

} // namespace PimpEx::vm
