#pragma once

#include "core/Diagnostic.hpp"

#include <charconv>
#include <string>
#include <string_view>

namespace PimpEx::script {

// Lua reports "file:line: message"; turned into a Diagnostic like .swap and XML errors.
inline Diagnostic lua_error_to_diagnostic(std::string_view message, const std::string &fallbackFile) {
  Diagnostic d{Diagnostic::Severity::Error, fallbackFile, 0, std::string(message)};
  const auto first = message.find(':');
  if (first == std::string_view::npos)
    return d;
  const auto second = message.find(':', first + 1);
  if (second == std::string_view::npos)
    return d;
  int line = 0;
  const auto lineText = message.substr(first + 1, second - first - 1);
  const auto [ptr, ec] = std::from_chars(lineText.data(), lineText.data() + lineText.size(), line);
  if (ec != std::errc{} || ptr != lineText.data() + lineText.size())
    return d;
  d.file = std::string(message.substr(0, first));
  d.line = line;
  auto rest = message.substr(second + 1);
  while (!rest.empty() && rest.front() == ' ')
    rest.remove_prefix(1);
  d.message = "Lua: " + std::string(rest);
  return d;
}

} // namespace PimpEx::script
