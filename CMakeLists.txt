cmake_minimum_required(VERSION 3.28)

# vcpkg toolchain from $VCPKG_ROOT or ~/vcpkg unless one was given.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
  elseif(EXISTS "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{HOME}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
  endif()
endif()

project(PimpEx VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# No C++20 modules – otherwise clangd trips over GCC flags.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

find_package(raylib CONFIG REQUIRED)
# raylib from vcpkg uses an external GLFW and does not link it itself.
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# --- Project and scene templates embedded in the binary (pimpex init / scene new)
file(GLOB_RECURSE PIMPEX_TEMPLATE_FILES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/templates/*"
  "${CMAKE_SOURCE_DIR}/docs/*.md"
)
set(PIMPEX_EMBEDDED_CPP "${CMAKE_BINARY_DIR}/generated/EmbeddedTemplate.cpp")
add_custom_command(
  OUTPUT "${PIMPEX_EMBEDDED_CPP}"
  COMMAND ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DOUT=${PIMPEX_EMBEDDED_CPP}
    -P "${CMAKE_SOURCE_DIR}/cmake/EmbedTemplate.cmake"
  DEPENDS ${PIMPEX_TEMPLATE_FILES} "${CMAKE_SOURCE_DIR}/cmake/EmbedTemplate.cmake"
  COMMENT "Embedding project templates"
)

# Font with Polish letters (Anonymous Pro, SIL OFL) embedded in the binary.
set(PIMPEX_FONT_TTF "${CMAKE_SOURCE_DIR}/third_party/fonts/AnonymousPro-Bold.ttf")
set(PIMPEX_EMBEDDED_FONT_CPP "${CMAKE_BINARY_DIR}/generated/EmbeddedFont.cpp")
add_custom_command(
  OUTPUT "${PIMPEX_EMBEDDED_FONT_CPP}"
  COMMAND ${CMAKE_COMMAND}
    -DINPUT=${PIMPEX_FONT_TTF}
    -DOUT=${PIMPEX_EMBEDDED_FONT_CPP}
    -DNAMESPACE=PimpEx::render
    -DFUNCTION=embedded_font
    -DHEADER=render/EmbeddedFont.hpp
    -P "${CMAKE_SOURCE_DIR}/cmake/EmbedBinary.cmake"
  DEPENDS "${PIMPEX_FONT_TTF}" "${CMAKE_SOURCE_DIR}/cmake/EmbedBinary.cmake"
  COMMENT "Embedding the font"
)

# --- Engine ------------------------------------------------------------------
file(GLOB_RECURSE PIMPEX_ENGINE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/engine/*.cpp"
  "${CMAKE_SOURCE_DIR}/engine/*.hpp"
)

add_library(pimpex_engine STATIC ${PIMPEX_ENGINE_SOURCES} "${PIMPEX_EMBEDDED_CPP}"
  "${PIMPEX_EMBEDDED_FONT_CPP}")
target_include_directories(pimpex_engine PUBLIC "${CMAKE_SOURCE_DIR}/engine" ${LUA_INCLUDE_DIR})
target_link_libraries(pimpex_engine PUBLIC
  raylib
  glfw
  OpenGL::GL
  Threads::Threads
  pugixml::pugixml
  EnTT::EnTT
  CLI11::CLI11
  sol2::sol2
  ${LUA_LIBRARIES}
  httplib::httplib
  nlohmann_json::nlohmann_json
)
target_compile_options(pimpex_engine PRIVATE -Wall -Wextra)

add_executable(pimpex "${CMAKE_SOURCE_DIR}/apps/pimpex/main.cpp")
target_link_libraries(pimpex PRIVATE pimpex_engine)

# --- Tests -------------------------------------------------------------------
enable_testing()
file(GLOB PIMPEX_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
add_executable(pimpex_tests ${PIMPEX_TEST_SOURCES})
target_link_libraries(pimpex_tests PRIVATE pimpex_engine doctest::doctest)
add_test(NAME pimpex_tests COMMAND pimpex_tests)

# End-to-end: the `pimpex init` template and the examples must pass
# `pimpex check` and run without a window.
add_test(NAME template_init_check
  COMMAND sh -c "rm -rf template_test && $<TARGET_FILE:pimpex> init template_test >/dev/null && $<TARGET_FILE:pimpex> check template_test && $<TARGET_FILE:pimpex> play template_test --headless --frames 300 --input 'LEFT:0-60 SPACE:10' --quiet"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
# docs/components.md must match the component registry.
add_test(NAME components_doc_in_sync
  COMMAND sh -c "$<TARGET_FILE:pimpex> components --markdown | diff -u '${CMAKE_SOURCE_DIR}/docs/components.md' -")
add_test(NAME scene_templates
  COMMAND sh "${CMAKE_SOURCE_DIR}/tests/e2e/templates.sh" $<TARGET_FILE:pimpex> "${CMAKE_BINARY_DIR}")
add_test(NAME wolf3d_gameplay
  COMMAND sh "${CMAKE_SOURCE_DIR}/tests/e2e/wolf3d.sh" $<TARGET_FILE:pimpex>
    "${CMAKE_SOURCE_DIR}/examples/wolf3d" "${CMAKE_BINARY_DIR}")
file(GLOB PIMPEX_EXAMPLES LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/examples/*")
foreach(EXAMPLE ${PIMPEX_EXAMPLES})
  get_filename_component(EXAMPLE_NAME "${EXAMPLE}" NAME)
  add_test(NAME example_${EXAMPLE_NAME}
    COMMAND sh -c "$<TARGET_FILE:pimpex> check '${EXAMPLE}' && $<TARGET_FILE:pimpex> play '${EXAMPLE}' --headless --frames 600 --input 'C:0' --quiet --runtime-dir '${CMAKE_BINARY_DIR}/example_${EXAMPLE_NAME}'")
endforeach()
