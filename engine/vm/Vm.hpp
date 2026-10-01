#pragma once

#include "scene/World.hpp"
#include "vm/Input.hpp"
#include "vm/Program.hpp"

#include <random>
#include <set>
#include <string>
#include <vector>

namespace PimpEx::vm {

// Co VM widzi w danej klatce poza światem.
struct FrameContext {
  const IInput &input;
  int screenWidth = 800;
  int screenHeight = 600;
  long frame = 0;
};

// Rzeczy, których VM sama nie robi – deleguje je do Boota.
class IVmHost {
public:
  virtual ~IVmHost() = default;
  virtual void call_function(const std::string &name, const std::vector<std::string> &args,
                             int line) = 0;
  virtual void request_proc_add(const std::string &name, int line) = 0;
  virtual void request_proc_remove(const std::string &name, int line) = 0;
  virtual void request_scene(const std::string &name, int line) = 0;
  // GUI_SHOW / GUI_HIDE / GUI_TOGGLE nazwa.
  virtual void request_gui(Op op, const std::string &name, int line) = 0;
  virtual void log(const std::string &message) = 0;
  virtual void runtime_error(int line, const std::string &message) = 0;
};

// Wykonuje program sceny: każdą PROC w każdej klatce.
class Vm {
  std::mt19937 _rng;
  std::set<std::pair<int, std::string>> _reported; // żeby nie spamować tym samym błędem

  const FrameContext *_ctx = nullptr;
  scene::World *_world = nullptr;
  IVmHost *_host = nullptr;
  std::string _self;  // SELF w PROC ... FOR
  std::string _other; // OTHER – ostatni aktor znaleziony przez TOUCH

  void report(int line, const std::string &message);
  // SELF/OTHER -> prawdziwa nazwa aktora (pusty tekst + błąd, gdy nie ma).
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

  // Wykonuje instrukcje spoza PROC (np. kliknięcie przycisku GUI).
  void run_instructions(const std::vector<Instr> &instructions, scene::World &world,
                        const FrameContext &ctx, IVmHost &host);

  // Po podmianie programu te same błędy mogą znów zostać zgłoszone.
  void reset_reports() { _reported.clear(); }

  double random(double min, double max);
};

// Aktorzy specjalni, których nie ma w scene.xml, ale da się z nich czytać.
bool is_builtin_actor(std::string_view name);

// Odczyt pola z uwzględnieniem aktorów specjalnych (Screen, Mouse, Scene).
// Nieustawiona zmienna ma wartość 0.
std::optional<scene::FieldValue> read_value(const scene::World &world, const FrameContext &ctx,
                                            const std::string &actor, const std::string &field,
                                            std::string *error = nullptr);
std::optional<double> read_field(const scene::World &world, const FrameContext &ctx,
                                 const std::string &actor, const std::string &field);

// Wstawia wartości w miejsce {Aktor.pole} albo {pole} (pole aktora `self`).
std::string format_text(const scene::World &world, const FrameContext &ctx,
                        std::string_view text, const std::string &self);

} // namespace PimpEx::vm
