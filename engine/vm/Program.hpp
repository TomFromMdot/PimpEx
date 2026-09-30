#pragma once

#include "vm/Keys.hpp"

#include <string>
#include <vector>

// Program sceny: wynik parsowania pliku .swap.
// Każda PROC to lista instrukcji wykonywana przez VM co klatkę.
namespace PimpEx::vm {

// Odwołanie do pola aktora: `Player.x`, `Game.score`, `Player.Health.hp`,
// `SELF.x` (w PROC ... FOR). `field` to wszystko po pierwszej kropce.
struct Ref {
  std::string actor;
  std::string field;
};

// Wartość w instrukcji: liczba, pole aktora, RANDOM a b albo "tekst" (tylko SET).
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
  bool negate = false;         // NOT ...
  Key key = Key::Space;        // KEY / PRESSED
  std::string a, b;            // TOUCH a b, EXISTS a; b/a może być @tagiem
  Value left, right;           // porównanie; CHANCE używa `left` jako procentu
  CompareOp op = CompareOp::Equal;
};

enum class Op {
  Move,       // MOVE aktor dx dy [dz]
  Set,        // SET Aktor.pole wartość
  Add,        // ADD Aktor.pole wartość
  Mul,        // MUL Aktor.pole wartość
  Spawn,      // SPAWN Archetyp [AS nazwa] [AT x y [z]]
  Destroy,    // DESTROY aktor
  Follow,     // FOLLOW aktor cel prędkość
  Text,       // TEXT aktor "tekst {pole}"
  Log,        // LOG "tekst"
  Call,       // CALL funkcja_lua [argumenty...]
  ProcAdd,    // PROC_ADD nazwa   – boot wkleja PROC z biblioteki do .swap
  ProcRemove, // PROC_REMOVE nazwa – boot usuwa PROC z .swap
  Scene,      // SCENE nazwa – zmiana sceny
};

struct Instr {
  Op op = Op::Log;
  int line = 0;
  std::vector<Condition> guard; // warunki z IF ... AND ... : (wszystkie muszą być prawdziwe)

  std::string target;            // aktor / archetyp / nazwa PROC / sceny / funkcji
  std::string other;             // FOLLOW: cel, SPAWN: nazwa po AS
  Ref ref;                       // SET / ADD / MUL
  std::vector<Value> values;     // argumenty liczbowe
  std::string text;              // LOG / TEXT
  std::vector<std::string> args; // CALL
};

enum class ProcMode {
  Loop,  // co klatkę
  Once,  // raz – potem boot usuwa ją z pliku .swap
  Every, // co N klatek
};

struct Proc {
  std::string name;
  ProcMode mode = ProcMode::Loop;
  int every = 1;
  // PROC nazwa FOR Health / FOR @enemy: ciało wykonuje się dla każdego
  // pasującego aktora, który jest wtedy dostępny jako SELF.
  std::string forEach;
  int firstLine = 0; // linia z PROC
  int lastLine = 0;  // linia z END
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
