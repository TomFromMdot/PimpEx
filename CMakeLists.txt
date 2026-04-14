cmake_minimum_required(VERSION 3.28)

project(Pimpex)

enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)




file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
  "./include/*.h"
  "./include/*.hpp"
)

message(STATUS "Print all headers files")
foreach(FILE ${HEADER_FILES})
  message(STATUS "[HEADER]: ${FILE}")
endforeach()


file(
  GLOB_RECURSE
  SOURCE_FILES
  CONFIGURE_DEPENDS
  "src/*.cpp"
)
message(STATUS "Print all source files")
foreach(FILE ${SOURCE_FILES})
  message(STATUS "[SRC]: ${FILE}")
endforeach()


add_library(
  PimpexLib
  ${HEADER_FILES}
  ${SOURCE_FILES}
)

target_link_directories(PimpexLib PUBLIC "./include")
target_link_libraries(PimpexLib PUBLIC CLI11::CLI11 pugixml::pugixml tinyxml2::tinyxml2)
add_executable(${PROJECT_NAME} "./main.cpp")


target_link_libraries(${PROJECT_NAME} PRIVATE PimpexLib)
enable_testing()

add_executable(cpp_test "./test/test_e.cpp")
target_link_libraries(cpp_test PRIVATE PimpexLib)

add_test(NAME TEST_XMLReader COMMAND $<TARGET_FILE:cpp_test>)

add_dependencies(${PROJECT_NAME} cpp_test)

