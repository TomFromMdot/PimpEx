#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace PimpEx {

inline std::size_t edit_distance(std::string_view a, std::string_view b) {
  std::vector<std::size_t> row(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j)
    row[j] = j;
  for (std::size_t i = 1; i <= a.size(); ++i) {
    std::size_t prev = row[0];
    row[0] = i;
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const std::size_t cur = row[j];
      row[j] = std::min(
          {row[j] + 1, row[j - 1] + 1, prev + (a[i - 1] == b[j - 1] ? 0 : 1)});
      prev = cur;
    }
  }
  return row[b.size()];
}

inline std::string to_upper_ascii(std::string_view s) {
  std::string out(s);
  for (auto &c : out)
    if (c >= 'a' && c <= 'z')
      c = static_cast<char>(c - 'a' + 'A');
  return out;
}

inline std::string closest_match(std::string_view word,
                                 const std::vector<std::string> &candidates) {
  const auto upper = to_upper_ascii(word);
  std::string best;
  std::size_t bestDistance = 3;
  for (const auto &candidate : candidates) {
    const auto d = edit_distance(upper, to_upper_ascii(candidate));
    if (d < bestDistance) {
      bestDistance = d;
      best = candidate;
    }
  }
  return best;
}

inline std::vector<std::string> split_words(std::string_view text) {
  std::vector<std::string> words;
  std::size_t pos = 0;
  while (pos < text.size()) {
    const auto start = text.find_first_not_of(" ,\t", pos);
    if (start == std::string_view::npos)
      break;
    auto end = text.find_first_of(" ,\t", start);
    if (end == std::string_view::npos)
      end = text.size();
    words.emplace_back(text.substr(start, end - start));
    pos = end;
  }
  return words;
}

} // namespace PimpEx
