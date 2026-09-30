# Wbudowuje jeden plik binarny (np. czcionkę) jako funkcję C++ zwracającą std::string_view.
# Wywołanie: cmake -DINPUT=... -DOUT=... -DNAMESPACE=... -DFUNCTION=... -DHEADER=... -P EmbedBinary.cmake

file(READ "${INPUT}" HEX HEX)
string(LENGTH "${HEX}" HEX_LENGTH)
math(EXPR BYTE_COUNT "${HEX_LENGTH} / 2")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," BYTES "${HEX}")
file(WRITE "${OUT}.tmp"
"// Plik wygenerowany przez cmake/EmbedBinary.cmake – nie edytuj ręcznie.
#include \"${HEADER}\"

namespace ${NAMESPACE} {
namespace {
const unsigned char data[] = {${BYTES}0x00};
} // namespace

std::string_view ${FUNCTION}() {
  return std::string_view(reinterpret_cast<const char *>(data), ${BYTE_COUNT});
}
} // namespace ${NAMESPACE}
")
file(COPY_FILE "${OUT}.tmp" "${OUT}" ONLY_IF_DIFFERENT)
file(REMOVE "${OUT}.tmp")
