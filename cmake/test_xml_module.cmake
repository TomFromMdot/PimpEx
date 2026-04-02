
add_library(
  XML_MODULE 
  "../include/EngineXML.h"
  "../src/EngineXMLReader.cpp"
)

target_link_libraries(XML_MODULE tinyxml glfw glad)

add_executable(test "../test/test_e.cpp")
target_link_libraries(test XML_MODULE)
