#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace PimpEx::boot {

namespace fs = std::filesystem;

// Polls project files on its own thread (modification time). Calls `onChange`
// on that thread when the content really changed; the engine's own writes
// (expect) are skipped.
class FileWatcher {
public:
  using ChangeCallback = std::function<void(const fs::path &, std::string content)>;

private:
  struct Entry {
    fs::file_time_type time{};
    std::size_t hash = 0;
    bool exists = false;
  };

  ChangeCallback _onChange;
  std::chrono::milliseconds _interval;
  std::mutex _mutex;
  std::map<fs::path, Entry> _files;
  std::jthread _thread;

public:
  explicit FileWatcher(ChangeCallback onChange,
                       std::chrono::milliseconds interval = std::chrono::milliseconds(200))
      : _onChange(std::move(onChange)), _interval(interval) {}
  ~FileWatcher() { stop(); }

  // Replaces the watched files; their current content is the baseline.
  void watch(const std::vector<fs::path> &files);

  // The engine wrote the file itself – a change with this hash is not new.
  void expect(const fs::path &path, std::size_t hash);

  void start();
  void stop();

  // One pass over all files (the thread and tests).
  void poll_once();
};

} // namespace PimpEx::boot
