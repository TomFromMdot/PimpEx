#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Templates embedded in the binary: project, scenes and the behavior library.
namespace PimpEx::cli {

struct SceneTemplate {
  std::string name;
  std::string about;
};

std::vector<SceneTemplate> scene_templates();
bool scene_template_exists(const std::string &name);

struct TemplateResult {
  std::vector<std::string> created; // relative to the project root
  std::vector<std::string> skipped; // already existed – left untouched
  std::string error;                // empty = success
};

// Project files (pimpex.xml, AGENTS.md, docs/...) and the behavior library.
TemplateResult apply_project_template(const std::filesystem::path &root, const std::string &projectName);

// scenes/<sceneName>/ from a template plus missing project files it needs
// (behaviors, scripts). Never overwrites.
TemplateResult apply_scene_template(const std::filesystem::path &root, const std::string &templateName,
                                    const std::string &sceneName, const std::string &projectName);

bool valid_name(const std::string &name);

} // namespace PimpEx::cli
