#include "cli/commands/InitCommand.hpp"
#include "cli/EmbeddedTemplate.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>

namespace PimpEx::cli {
namespace {

namespace fs = std::filesystem;

bool valid_project_name(const std::string &name) {
  if (name.empty())
    return false;
  for (const char c : name)
    if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-'))
      return false;
  return true;
}

std::string replace_all(std::string_view text, std::string_view from, std::string_view to) {
  std::string out;
  std::size_t pos = 0;
  while (true) {
    const auto found = text.find(from, pos);
    out.append(text.substr(pos, found - pos));
    if (found == std::string_view::npos)
      return out;
    out.append(to);
    pos = found + from.size();
  }
}

} // namespace

CLI::App *InitCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("init", "Tworzy nowy projekt gry z szablonu.");
  sub->add_option("name", _name, "Nazwa projektu (litery, cyfry, _ i -)")->required();
  sub->add_option("--dir", _dir, "Katalog, w którym powstanie projekt")->capture_default_str();
  return sub;
}

int InitCommand::run() {
  if (!valid_project_name(_name)) {
    std::println(std::cerr, "Nazwa projektu '{}' może zawierać tylko litery, cyfry, _ i -.", _name);
    return 1;
  }
  const fs::path root = fs::path(_dir) / _name;
  std::error_code ec;
  if (fs::exists(root, ec) && !fs::is_empty(root, ec)) {
    std::println(std::cerr, "Katalog {} już istnieje i nie jest pusty.", root.string());
    return 1;
  }

  for (const auto &file : embedded_template()) {
    const auto target = root / file.path;
    fs::create_directories(target.parent_path(), ec);
    std::ofstream out(target, std::ios::binary);
    if (!out) {
      std::println(std::cerr, "Nie mogę zapisać pliku {}.", target.string());
      return 1;
    }
    out << replace_all(file.content, "{{NAME}}", _name);
  }
  fs::create_directories(root / "assets", ec);

  std::println("Projekt '{}' gotowy w {}", _name, root.string());
  std::println("  pimpex check {}   – sprawdź pliki", root.string());
  std::println("  pimpex dev {}     – uruchom z hot-reloadem", root.string());
  return 0;
}

} // namespace PimpEx::cli
