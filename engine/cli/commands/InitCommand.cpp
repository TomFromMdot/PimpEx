#include "cli/commands/InitCommand.hpp"
#include "cli/Templates.hpp"

#include <filesystem>
#include <iostream>
#include <print>

namespace PimpEx::cli {

CLI::App *InitCommand::setup(CLI::App &app) {
  auto *sub = app.add_subcommand("init", "Tworzy nowy projekt gry z szablonu.");
  sub->add_option("name", _name, "Nazwa projektu (litery, cyfry, _ i -)")->required();
  sub->add_option("--dir", _dir, "Katalog, w którym powstanie projekt")->capture_default_str();
  sub->add_option("--template", _template, "Szablon pierwszej sceny (lista: pimpex scene list)")
      ->capture_default_str();
  return sub;
}

int InitCommand::run() {
  namespace fs = std::filesystem;
  if (!valid_name(_name)) {
    std::println(std::cerr, "Nazwa projektu '{}' może zawierać tylko litery, cyfry, _ i -.", _name);
    return 1;
  }
  if (!scene_template_exists(_template)) {
    std::println(std::cerr, "Nie ma szablonu '{}'. Dostępne: pimpex scene list", _template);
    return 1;
  }
  const fs::path root = fs::path(_dir) / _name;
  std::error_code ec;
  if (fs::exists(root, ec) && !fs::is_empty(root, ec)) {
    std::println(std::cerr, "Katalog {} już istnieje i nie jest pusty.", root.string());
    return 1;
  }

  for (const auto &result : {apply_project_template(root, _name),
                             apply_scene_template(root, _template, "main", _name)})
    if (!result.error.empty()) {
      std::println(std::cerr, "Błąd: {}", result.error);
      return 1;
    }

  std::println("Projekt '{}' gotowy w {} (scena main z szablonu {})", _name, root.string(), _template);
  std::println("  pimpex check {}   – sprawdź pliki", root.string());
  std::println("  pimpex dev {}     – uruchom z hot-reloadem", root.string());
  return 0;
}

} // namespace PimpEx::cli
