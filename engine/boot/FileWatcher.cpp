#include "boot/FileWatcher.hpp"
#include "boot/ProjectPaths.hpp"
#include "boot/SwapFile.hpp"

namespace PimpEx::boot {

void FileWatcher::watch(const std::vector<fs::path> &files) {
  std::map<fs::path, Entry> entries;
  for (const auto &path : files) {
    Entry entry;
    std::error_code ec;
    entry.time = fs::last_write_time(path, ec);
    entry.exists = !ec;
    if (entry.exists)
      if (const auto content = read_file(path))
        entry.hash = content_hash(*content);
    entries[path] = entry;
  }
  std::lock_guard lock(_mutex);
  _files = std::move(entries);
}

void FileWatcher::expect(const fs::path &path, std::size_t hash) {
  std::lock_guard lock(_mutex);
  auto it = _files.find(path);
  if (it != _files.end())
    it->second.hash = hash;
}

void FileWatcher::poll_once() {
  std::vector<std::pair<fs::path, std::string>> changed;
  {
    std::lock_guard lock(_mutex);
    for (auto &[path, entry] : _files) {
      std::error_code ec;
      const auto time = fs::last_write_time(path, ec);
      if (ec) {
        entry.exists = false; // plik zniknął – czekamy, aż wróci
        continue;
      }
      if (entry.exists && time == entry.time)
        continue;
      auto content = read_file(path);
      if (!content)
        continue;
      entry.time = time;
      entry.exists = true;
      const auto hash = content_hash(*content);
      if (hash == entry.hash)
        continue; // ta sama treść (np. zapis silnika albo „zapisz” bez zmian)
      entry.hash = hash;
      changed.emplace_back(path, std::move(*content));
    }
  }
  for (auto &[path, content] : changed)
    _onChange(path, std::move(content));
}

void FileWatcher::start() {
  _thread = std::jthread([this](std::stop_token stop) {
    while (!stop.stop_requested()) {
      poll_once();
      std::this_thread::sleep_for(_interval);
    }
  });
}

void FileWatcher::stop() {
  if (_thread.joinable()) {
    _thread.request_stop();
    _thread.join();
  }
}

} // namespace PimpEx::boot
