#include "cli/Templates.hpp"
#include "cli/EmbeddedTemplate.hpp"

#include <format>
#include <fstream>

namespace PimpEx::cli {
namespace {

namespace fs = std::filesystem;

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

// Pełna ścieżka do binarki silnika – trafia do .mcp.json, żeby agent
// uruchomił właściwy `pimpex` nawet bez niego w PATH.
std::string engine_executable() {
  std::error_code ec;
  const auto self = fs::read_symlink("/proc/self/exe", ec);
  return ec ? std::string("pimpex") : self.string();
}

struct Variables {
  std::string project;
  std::string scene;
  std::string apply(std::string_view text) const {
    return replace_all(replace_all(replace_all(text, "{{NAME}}", project), "{{SCENE}}", scene), "{{PIMPEX}}",
                       engine_executable());
  }
};

// Kopiuje pliki z prefiksem `prefix` do `target`. Istniejących nie nadpisuje.
void copy_prefix(std::string_view prefix, const fs::path &root, const fs::path &target,
                 const Variables &vars, TemplateResult &result) {
  for (const auto &file : embedded_template()) {
    if (!file.path.starts_with(prefix))
      continue;
    const auto destination = target / file.path.substr(prefix.size());
    const auto shown = destination.lexically_relative(root).generic_string();
    std::error_code ec;
    if (fs::exists(destination, ec)) {
      result.skipped.push_back(shown);
      continue;
    }
    fs::create_directories(destination.parent_path(), ec);
    std::ofstream out(destination, std::ios::binary);
    if (!out) {
      result.error = std::format("nie mogę zapisać {}", destination.string());
      return;
    }
    out << vars.apply(file.content);
    result.created.push_back(shown);
  }
}

} // namespace

bool valid_name(const std::string &name) {
  if (name.empty())
    return false;
  for (const char c : name)
    if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-'))
      return false;
  return true;
}

std::vector<SceneTemplate> scene_templates() {
  std::vector<SceneTemplate> out;
  for (const auto &file : embedded_template()) {
    if (!file.path.starts_with("scenes/") || !file.path.ends_with("/about.txt"))
      continue;
    const auto name = file.path.substr(7, file.path.size() - 7 - std::string_view("/about.txt").size());
    auto about = std::string(file.content);
    while (!about.empty() && (about.back() == '\n' || about.back() == '\r'))
      about.pop_back();
    out.push_back({std::string(name), about});
  }
  return out;
}

bool scene_template_exists(const std::string &name) {
  for (const auto &t : scene_templates())
    if (t.name == name)
      return true;
  return false;
}

TemplateResult apply_project_template(const fs::path &root, const std::string &projectName) {
  TemplateResult result;
  const Variables vars{projectName, ""};
  copy_prefix("project/", root, root, vars, result);
  if (result.error.empty())
    copy_prefix("behaviors/", root, root / "scripts" / "behaviors", vars, result);
  std::error_code ec;
  fs::create_directories(root / "assets", ec);
  return result;
}

TemplateResult apply_scene_template(const fs::path &root, const std::string &templateName,
                                    const std::string &sceneName, const std::string &projectName) {
  TemplateResult result;
  if (!scene_template_exists(templateName)) {
    result.error = std::format("nie ma szablonu sceny '{}' (lista: pimpex scene list)", templateName);
    return result;
  }
  const auto sceneDir = root / "scenes" / sceneName;
  std::error_code ec;
  if (fs::exists(sceneDir, ec) && !fs::is_empty(sceneDir, ec)) {
    result.error = std::format("scena '{}' już istnieje (scenes/{})", sceneName, sceneName);
    return result;
  }
  const Variables vars{projectName, sceneName};
  const auto base = std::format("scenes/{}/", templateName);
  copy_prefix(base + "scene/", root, sceneDir, vars, result);
  if (result.error.empty())
    copy_prefix(base + "project/", root, root, vars, result);
  if (result.error.empty())
    copy_prefix("behaviors/", root, root / "scripts" / "behaviors", vars, result);
  return result;
}

} // namespace PimpEx::cli
