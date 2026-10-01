#pragma once

#include <string_view>
#include <vector>

namespace PimpEx::cli {

struct EmbeddedFile {
  std::string_view path; // relative to templates/: "project/...", "scenes/<template>/..."
  std::string_view content;
};

// Template files embedded in the binary (project, scenes, docs).
const std::vector<EmbeddedFile> &embedded_template();

} // namespace PimpEx::cli
