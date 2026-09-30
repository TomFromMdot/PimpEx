# Generuje plik C++ z zawartością szablonów (templates/**) i dokumentacji.
# Klucze: ścieżka względem templates/ (np. "project/pimpex.xml",
# "scenes/arena3d/scene/scene.xml"), a docs trafiają do "project/docs/...".
# Wywołanie: cmake -DSOURCE_DIR=... -DOUT=... -P EmbedTemplate.cmake

file(GLOB_RECURSE TEMPLATE_FILES RELATIVE "${SOURCE_DIR}/templates"
  "${SOURCE_DIR}/templates/*")
file(GLOB DOC_FILES RELATIVE "${SOURCE_DIR}" "${SOURCE_DIR}/docs/*.md")
list(SORT TEMPLATE_FILES)
list(SORT DOC_FILES)

set(ARRAYS "")
set(ENTRIES "")
set(INDEX 0)

macro(embed_file ABSOLUTE_PATH PROJECT_PATH)
  file(READ "${ABSOLUTE_PATH}" HEX HEX)
  string(LENGTH "${HEX}" HEX_LENGTH)
  math(EXPR BYTE_COUNT "${HEX_LENGTH} / 2")
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," BYTES "${HEX}")
  string(APPEND ARRAYS "const unsigned char file${INDEX}[] = {${BYTES}0x00};\n")
  string(APPEND ENTRIES
    "    {\"${PROJECT_PATH}\", std::string_view(reinterpret_cast<const char *>(file${INDEX}), ${BYTE_COUNT})},\n")
  math(EXPR INDEX "${INDEX} + 1")
endmacro()

foreach(FILE ${TEMPLATE_FILES})
  embed_file("${SOURCE_DIR}/templates/${FILE}" "${FILE}")
endforeach()
foreach(FILE ${DOC_FILES})
  embed_file("${SOURCE_DIR}/${FILE}" "project/${FILE}")
endforeach()

file(WRITE "${OUT}.tmp"
"// Plik wygenerowany przez cmake/EmbedTemplate.cmake – nie edytuj ręcznie.
#include \"cli/EmbeddedTemplate.hpp\"

namespace PimpEx::cli {
namespace {
${ARRAYS}} // namespace

const std::vector<EmbeddedFile> &embedded_template() {
  static const std::vector<EmbeddedFile> files = {
${ENTRIES}  };
  return files;
}
} // namespace PimpEx::cli
")
file(COPY_FILE "${OUT}.tmp" "${OUT}" ONLY_IF_DIFFERENT)
file(REMOVE "${OUT}.tmp")
