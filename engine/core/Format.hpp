#pragma once

#include <format>
#include <string>

namespace PimpEx {

// Liczby całkowite bez ułamka ("3"), pozostałe z dwoma miejscami ("2.50").
inline std::string format_number(double value) {
  if (value == static_cast<double>(static_cast<long long>(value)))
    return std::format("{}", static_cast<long long>(value));
  return std::format("{:.2f}", value);
}

} // namespace PimpEx
