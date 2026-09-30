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

// Obserwuje pliki projektu na osobnym wątku (sprawdza czas modyfikacji).
// Gdy treść pliku naprawdę się zmieni, woła `onChange` – także na tym wątku.
// Zmiany zapisane przez sam silnik (expect) są pomijane.
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

  // Zastępuje listę obserwowanych plików. Obecna treść to punkt odniesienia.
  void watch(const std::vector<fs::path> &files);

  // Silnik sam zapisał plik – zmiana o tym hashu nie jest „nowa”.
  void expect(const fs::path &path, std::size_t hash);

  void start();
  void stop();

  // Jedno sprawdzenie wszystkich plików (używane przez wątek i testy).
  void poll_once();
};

} // namespace PimpEx::boot
