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

// Lista wszystkich słów kluczowych instrukcji (do podpowiedzi i dokumentacji).
const std::vector<std::string_view> &instruction_names();

} // namespace PimpEx::vm
