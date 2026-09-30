#pragma once

#include <string_view>
#include <vector>

namespace PimpEx::cli {

// Jeden plik szablonu wbudowany w binarkę silnika.
struct EmbeddedFile {
  std::string_view path; // względem templates/: "project/...", "scenes/<szablon>/..."
  std::string_view content;
};

// Wszystkie pliki szablonów (projekt, sceny, dokumentacja).
const std::vector<EmbeddedFile> &embedded_template();

} // namespace PimpEx::cli
