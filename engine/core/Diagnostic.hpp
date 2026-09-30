#pragma once

#include <format>
#include <string>
#include <vector>

namespace PimpEx {

// Komunikat o problemie w pliku projektu, w formacie „plik:linia: opis”.
// Taki format rozumieją edytory, terminale i agenci AI.
struct Diagnostic {
  enum class Severity { Error, Warning };

  Severity severity = Severity::Error;
  std::string file;
  int line = 0;
  std::string message;

  std::string format() const {
    const char *kind = severity == Severity::Error ? "błąd" : "ostrzeżenie";
    if (line > 0)
      return std::format("{}:{}: {}: {}", file, line, kind, message);
    return std::format("{}: {}: {}", file, kind, message);
  }
};

using Diagnostics = std::vector<Diagnostic>;

inline bool has_errors(const Diagnostics &diagnostics) {
  for (const auto &d : diagnostics)
    if (d.severity == Diagnostic::Severity::Error)
      return true;
  return false;
}

} // namespace PimpEx
