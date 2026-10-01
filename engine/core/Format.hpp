#pragma once

#include <format>
#include <string>

namespace PimpEx {

// Whole numbers without a fraction ("3"), others with two decimals ("2.50").
inline std::string format_number(double value) {
  if (value == static_cast<double>(static_cast<long long>(value)))
    return std::format("{}", static_cast<long long>(value));
  return std::format("{:.2f}", value);
}

} // namespace PimpEx
