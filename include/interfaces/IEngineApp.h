#pragma once

#include <filesystem>
#include <map>
#include <vector>
namespace PimpEx::interfaces {
class IEngineApp {
  std::map<std::string, std::filesystem::path> projects;

public:
  void load_pimpex_apps(const std::filesystem::path &);
};
} // namespace PimpEx::interfaces
