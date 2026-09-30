cmake_minimum_required(VERSION 3.28)

# vcpkg: bierzemy toolchain z $VCPKG_ROOT albo z ~/vcpkg, jeśli nikt go nie podał.
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

find_package(raylib CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(doctest CONFIG REQUIRED)

# --- Szablon projektu gry wbudowany w binarkę (pimpex init) -----------------
file(GLOB_RECURSE PIMPEX_TEMPLATE_FILES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/templates/project/*"
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
  COMMENT "Wbudowuję szablon projektu"
)

# --- Silnik ------------------------------------------------------------------
file(GLOB_RECURSE PIMPEX_ENGINE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/engine/*.cpp"
  "${CMAKE_SOURCE_DIR}/engine/*.hpp"
)

add_library(pimpex_engine STATIC ${PIMPEX_ENGINE_SOURCES} "${PIMPEX_EMBEDDED_CPP}")
target_include_directories(pimpex_engine PUBLIC "${CMAKE_SOURCE_DIR}/engine" ${LUA_INCLUDE_DIR})
target_link_libraries(pimpex_engine PUBLIC
  raylib
  pugixml::pugixml
  EnTT::EnTT
  CLI11::CLI11
  sol2::sol2
  ${LUA_LIBRARIES}
)
target_compile_options(pimpex_engine PRIVATE -Wall -Wextra)

add_executable(pimpex "${CMAKE_SOURCE_DIR}/apps/pimpex/main.cpp")
target_link_libraries(pimpex PRIVATE pimpex_engine)

# --- Testy -------------------------------------------------------------------
enable_testing()
file(GLOB PIMPEX_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
add_executable(pimpex_tests ${PIMPEX_TEST_SOURCES})
target_link_libraries(pimpex_tests PRIVATE pimpex_engine doctest::doctest)
target_compile_definitions(pimpex_tests PRIVATE PIMPEX_TEST_DATA="${CMAKE_SOURCE_DIR}/tests/data")
add_test(NAME pimpex_tests COMMAND pimpex_tests)
