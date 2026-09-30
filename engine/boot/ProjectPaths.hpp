#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace PimpEx::boot {

namespace fs = std::filesystem;

// Układ plików projektu gry – jedno miejsce, które zna wszystkie ścieżki.
struct ProjectPaths {
  fs::path root;
  fs::path runtimeOverride; // inny katalog na runtime.log/state.txt (np. przebiegi z MCP)

  ProjectPaths() = default;
  explicit ProjectPaths(fs::path projectRoot, fs::path runtime = {})
      : root(std::move(projectRoot)), runtimeOverride(std::move(runtime)) {}

  fs::path project_xml() const { return root / "pimpex.xml"; }
  fs::path scenes_dir() const { return root / "scenes"; }
  fs::path scene_dir(const std::string &scene) const { return scenes_dir() / scene; }
  fs::path scene_xml(const std::string &scene) const { return scene_dir(scene) / "scene.xml"; }
  fs::path scene_swap(const std::string &scene) const { return scene_dir(scene) / "scene.swap"; }
  fs::path library_swap(const std::string &scene) const {
    return scene_dir(scene) / "library.swap";
  }
  fs::path components_xml() const { return root / "components.xml"; }
  fs::path scripts_dir() const { return root / "scripts"; }
  fs::path behaviors_dir() const { return scripts_dir() / "behaviors"; }
  fs::path behavior_file(const std::string &name) const { return behaviors_dir() / (name + ".lua"); }
  fs::path runtime_dir() const { return runtimeOverride.empty() ? root / ".pimpex" : runtimeOverride; }
  fs::path runtime_log() const { return runtime_dir() / "runtime.log"; }
  fs::path state_file() const { return runtime_dir() / "state.txt"; }

  // Ścieżka do wyświetlania w komunikatach – względem katalogu projektu.
  std::string display(const fs::path &path) const {
    auto rel = path.lexically_relative(root);
    return rel.empty() ? path.generic_string() : rel.generic_string();
  }

  std::vector<std::string> scene_names() const {
    std::vector<std::string> names;
    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(scenes_dir(), ec))
      if (entry.is_directory())
        names.push_back(entry.path().filename().string());
    std::sort(names.begin(), names.end());
    return names;
  }

  std::vector<fs::path> behavior_files() const {
    std::vector<fs::path> files;
    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(behaviors_dir(), ec))
      if (entry.is_regular_file() && entry.path().extension() == ".lua")
        files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    return files;
  }

  // Skrypty globalne (scripts/*.lua, bez podkatalogu behaviors/).
  std::vector<fs::path> script_files() const {
    std::vector<fs::path> files;
    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(scripts_dir(), ec))
      if (entry.is_regular_file() && entry.path().extension() == ".lua")
        files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    return files;
  }
};

inline std::optional<std::string> read_file(const fs::path &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return std::nullopt;
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

} // namespace PimpEx::boot
