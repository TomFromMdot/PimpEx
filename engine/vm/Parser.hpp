#pragma once

#include "core/Diagnostic.hpp"
#include "vm/Program.hpp"

#include <string>
#include <string_view>

namespace PimpEx::vm {

struct ParseResult {
  Program program;
  Diagnostics diagnostics;

  bool ok() const { return !has_errors(diagnostics); }
};

// Zamienia tekst pliku .swap na Program. Nie przerywa się na pierwszym
// błędzie – zbiera wszystkie, żeby pokazać je naraz.
// `file` służy tylko do opisu błędów.
ParseResult parse_program(std::string_view source, const std::string &file);

// Instrukcje bez PROC, oddzielone średnikiem – np. `click` przycisku GUI:
// "SET Game.level 2; GUI_HIDE menu; SCENE poziom". Wszystkie dostają numer
// linii `line` (linia elementu w pliku XML).
struct InstructionsResult {
  std::vector<Instr> instructions;
  Diagnostics diagnostics;

  bool ok() const { return !has_errors(diagnostics); }
};
InstructionsResult parse_instructions(std::string_view source, const std::string &file, int line);

// Lista wszystkich słów kluczowych instrukcji (do podpowiedzi i dokumentacji).
const std::vector<std::string_view> &instruction_names();

} // namespace PimpEx::vm
