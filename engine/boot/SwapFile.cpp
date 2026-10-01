#include "boot/SwapFile.hpp"

#include <fstream>
#include <functional>
#include <vector>

namespace PimpEx::boot {
namespace {

// Lines without line terminators.
std::vector<std::string_view> split_lines(std::string_view text) {
  std::vector<std::string_view> lines;
  std::size_t start = 0;
  while (start <= text.size()) {
    auto end = text.find('\n', start);
    if (end == std::string_view::npos)
      end = text.size();
    lines.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  // "a\nb\n" would give a trailing empty line that is not in the file
  if (!lines.empty() && lines.back().empty() && !text.empty() && text.back() == '\n')
    lines.pop_back();
  return lines;
}

bool is_blank(std::string_view line) {
  return line.find_first_not_of(" \t\r") == std::string_view::npos;
}

} // namespace

std::string extract_proc_source(std::string_view text, const vm::Proc &proc) {
  const auto lines = split_lines(text);
  std::string out;
  for (int i = proc.firstLine; i <= proc.lastLine && i <= static_cast<int>(lines.size()); ++i) {
    out.append(lines[i - 1]);
    out += '\n';
  }
  return out;
}

std::string append_proc(std::string_view text, std::string_view procSource) {
  std::string out(text);
  if (!out.empty() && out.back() != '\n')
    out += '\n';
  if (!out.empty() && !out.ends_with("\n\n"))
    out += '\n';
  out.append(procSource);
  if (!out.empty() && out.back() != '\n')
    out += '\n';
  return out;
}

std::string remove_proc(std::string_view text, const vm::Proc &proc) {
  const auto lines = split_lines(text);
  std::string out;
  int skipUntil = proc.lastLine;
  if (skipUntil < static_cast<int>(lines.size()) && is_blank(lines[skipUntil]))
    ++skipUntil; // the empty line after END
  for (int i = 1; i <= static_cast<int>(lines.size()); ++i) {
    if (i >= proc.firstLine && i <= skipUntil)
      continue;
    out.append(lines[i - 1]);
    out += '\n';
  }
  return out;
}

bool write_file_atomic(const std::filesystem::path &path, std::string_view content) {
  auto temp = path;
  temp += ".tmp";
  {
    std::ofstream out(temp, std::ios::binary | std::ios::trunc);
    if (!out)
      return false;
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out)
      return false;
  }
  std::error_code ec;
  std::filesystem::rename(temp, path, ec);
  return !ec;
}

std::size_t content_hash(std::string_view content) {
  return std::hash<std::string_view>{}(content);
}

} // namespace PimpEx::boot
