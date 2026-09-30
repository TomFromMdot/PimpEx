#pragma once

#include "core/Diagnostic.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

namespace PimpEx {

// Dziennik silnika: konsola + plik .pimpex/runtime.log.
// Plik czyta człowiek albo agent AI, żeby zobaczyć, co się stało w grze.
// Bezpieczny dla wielu wątków.
class Log {
  std::mutex _mutex;
  std::ofstream _file;
  bool _quiet = false;

  void write(std::string_view level, const std::string &message, bool toStderr) {
    const auto now = std::chrono::floor<std::chrono::seconds>(
        std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
    const auto line = std::format("[{:%H:%M:%S}] {}{}", now, level, message);
    std::lock_guard lock(_mutex);
    if (!_quiet)
      (toStderr ? std::cerr : std::cout) << line << '\n' << std::flush;
    if (_file.is_open())
      _file << line << '\n' << std::flush;
  }

public:
  // Otwiera plik dziennika od nowa (poprzednia sesja znika).
  void open(const std::filesystem::path &path) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::lock_guard lock(_mutex);
    _file.open(path, std::ios::trunc);
  }

  // Bez wypisywania na konsolę (tylko plik) – dla testów.
  void set_quiet(bool quiet) { _quiet = quiet; }

  void info(const std::string &message) { write("", message, false); }
  void error(const std::string &message) { write("BŁĄD: ", message, true); }
  void diagnostic(const Diagnostic &d) { write("", d.format(), d.severity == Diagnostic::Severity::Error); }
  void diagnostics(const Diagnostics &list) {
    for (const auto &d : list)
      diagnostic(d);
  }
};

} // namespace PimpEx
