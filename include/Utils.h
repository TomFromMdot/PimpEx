#pragma once
#include <algorithm>
#include <cctype>
#include <string>
namespace PimpEx::Utils {
inline std::string string_to_lower(const std::string &s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}
} // namespace PimpEx::Utils
