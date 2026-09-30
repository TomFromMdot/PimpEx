#pragma once

#include <string_view>
#include <vector>

namespace PimpEx::cli {

// Jeden plik szablonu projektu wbudowany w binarkę silnika.
struct EmbeddedFile {
  std::string_view path; // ścieżka względem katalogu nowego projektu
  std::string_view content;
};

// Wszystkie pliki, które `pimpex init` kopiuje do nowego projektu.
const std::vector<EmbeddedFile> &embedded_template();

} // namespace PimpEx::cli
