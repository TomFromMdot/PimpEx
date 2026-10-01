#pragma once

#include "vm/Keys.hpp"

#include <string>
#include <vector>

// A parsed .swap file. Every PROC is a list of instructions the VM runs each frame.
namespace PimpEx::vm {

// `Player.x`, `Game.score`, `Player.Health.hp`, `SELF.x`.
// `field` is everything after the first dot.
struct Ref {
  std::string actor;
  std::string field;
};

// A number, an actor field, RANDOM a b or "text" (SET only).
struct Value {
  enum class Kind { Number, Ref, Random, Text };

  Kind kind = Kind::Number;
  double number = 0.0;
  std::string text;
  Ref ref;
  bool negate = false; // `-Ball.vx`
  double min = 0.0, max = 0.0;
};

enum class CompareOp { Less, LessEqual, Greater, GreaterEqual, Equal, NotEqual };

struct Condition {
  enum class Kind { KeyDown, KeyPressed, Touch, Exists, Chance, Compare };

  Kind kind = Kind::KeyDown;
  bool negate = false;         // NOT
  Key key = Key::Space;        // KEY / PRESSED
  std::string a, b;            // TOUCH a b, EXISTS a; may be @tags
  Value left, right;           // comparison; CHANCE uses `left` as the percentage
  CompareOp op = CompareOp::Equal;
};

enum class Op {
  Move,       // MOVE actor dx dy [dz]
  Set,        // SET Actor.field value
  Add,        // ADD Actor.field value
  Mul,        // MUL Actor.field value
  Spawn,      // SPAWN Archetype [AS name] [AT x y [z]]
  Destroy,    // DESTROY actor
  Follow,     // FOLLOW actor target speed
  Text,       // TEXT actor "text {field}"
  Log,        // LOG "text"
  Call,       // CALL lua_function [args...]
  ProcAdd,    // PROC_ADD name    – Boot pastes the PROC from library.swap
  ProcRemove, // PROC_REMOVE name – Boot cuts the PROC from scene.swap
  Scene,      // SCENE name
  GuiShow,    // GUI_SHOW name    – gui/<name>.xml
  GuiHide,    // GUI_HIDE name
  GuiToggle,  // GUI_TOGGLE name
};

struct Instr {
  Op op = Op::Log;
  int line = 0;
  std::vector<Condition> guard; // IF ... AND ... : – all must hold

  std::string target;            // actor / archetype / PROC / scene / function / GUI
  std::string other;             // FOLLOW target, SPAWN ... AS name
  Ref ref;                       // SET / ADD / MUL
  std::vector<Value> values;
  std::string text;              // LOG / TEXT
  std::vector<std::string> args; // CALL
};

enum class ProcMode {
  Loop,
  Once,  // runs once, then Boot removes it from scene.swap
  Every, // every N frames
};

struct Proc {
  std::string name;
  ProcMode mode = ProcMode::Loop;
  int every = 1;
  // FOR Health / FOR @enemy: the body runs once per matching actor (SELF).
  std::string forEach;
  int firstLine = 0; // PROC line
  int lastLine = 0;  // END line
  std::vector<Instr> body;
};

struct Program {
  std::vector<Proc> procs;

  const Proc *find(const std::string &name) const {
    for (const auto &p : procs)
      if (p.name == name)
        return &p;
    return nullptr;
  }
};

} // namespace PimpEx::vm
