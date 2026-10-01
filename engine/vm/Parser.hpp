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

// Does not stop at the first error – collects all of them to show at once.
// `file` is only used in diagnostics.
ParseResult parse_program(std::string_view source, const std::string &file);

// Instructions outside a PROC separated by ';' – a GUI button's `click`:
// "SET Game.level 2; GUI_HIDE menu; SCENE level". All get `line` (the XML
// element's line).
struct InstructionsResult {
  std::vector<Instr> instructions;
  Diagnostics diagnostics;

  bool ok() const { return !has_errors(diagnostics); }
};
InstructionsResult parse_instructions(std::string_view source, const std::string &file, int line);

// Instruction keywords (for typo hints).
const std::vector<std::string_view> &instruction_names();

} // namespace PimpEx::vm
