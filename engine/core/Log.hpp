#pragma once

#include "core/Diagnostic.hpp"

#include <chrono>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

namespace PimpEx {
class Log {
  std::mutex _mutex;
  std::ofstream _file;
  bool _quiet = false;
  std::deque<std::string> _recent; // for the API (log.tail)
  static constexpr std::size_t RecentLimit = 500;

  void write(std::string_view level, const std::string &message,
             bool toStderr) {
    const auto now = std::chrono::floor<std::chrono::seconds>(
        std::chrono::current_zone()->to_local(
            std::chrono::system_clock::now()));
    const auto line = std::format("[{:%H:%M:%S}] {}{}", now, level, message);
    std::lock_guard lock(_mutex);
    if (!_quiet)
      (toStderr ? std::cerr : std::cout) << line << '\n' << std::flush;
    if (_file.is_open())
      _file << line << '\n' << std::flush;
    _recent.push_back(line);
    if (_recent.size() > RecentLimit)
      _recent.pop_front();
  }

public:
  void open(const std::filesystem::path &path) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::lock_guard lock(_mutex);
    _file.open(path, std::ios::trunc);
  }

  void set_quiet(bool quiet) { _quiet = quiet; }

  std::vector<std::string> tail(std::size_t count) {
    std::lock_guard lock(_mutex);
    const auto start = _recent.size() > count ? _recent.size() - count : 0;
    return {_recent.begin() + static_cast<std::ptrdiff_t>(start),
            _recent.end()};
  }

  void info(const std::string &message) { write("", message, false); }
  void error(const std::string &message) { write("ERROR: ", message, true); }
  void diagnostic(const Diagnostic &d) {
    write("", d.format(), d.severity == Diagnostic::Severity::Error);
  }
  void diagnostics(const Diagnostics &list) {
    for (const auto &d : list)
      diagnostic(d);
  }
};

} // namespace PimpEx
