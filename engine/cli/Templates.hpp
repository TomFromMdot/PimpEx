#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Szablony wbudowane w binarkę: projekt, sceny i biblioteka zachowań.
namespace PimpEx::cli {

struct SceneTemplate {
  std::string name;
  std::string about;
};

std::vector<SceneTemplate> scene_templates();
bool scene_template_exists(const std::string &name);

struct TemplateResult {
  std::vector<std::string> created; // ścieżki względem katalogu projektu
  std::vector<std::string> skipped; // już istniały – zostawione bez zmian
  std::string error;                // pusty = sukces
};

// Pliki projektu (pimpex.xml, AGENTS.md, docs/...) i biblioteka zachowań.
TemplateResult apply_project_template(const std::filesystem::path &root, const std::string &projectName);

// Nowa scena scenes/<sceneName>/ z szablonu + brakujące pliki projektu,
// których szablon potrzebuje (zachowania, skrypty). Istniejących nie nadpisuje.
TemplateResult apply_scene_template(const std::filesystem::path &root, const std::string &templateName,
                                    const std::string &sceneName, const std::string &projectName);

bool valid_name(const std::string &name);

} // namespace PimpEx::cli
