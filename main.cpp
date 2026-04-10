#include <iostream>
#include <print>
#include <unistd.h>

#include "./include/EngineXML.h"

int main(int argvgc, char *argv[]) {
  std::cout << "Hello world\n";

  PimpEx::Utils::EngineXMLReader reader("./test.xml");

  auto test = reader.get_scene_actors("Cave");
  if (test.has_value()) {
    for (const auto &[key, value] : *test) {
      std::print("Actor: {} id: {}", value->name, value->id);
    }
  } else {
    std::cerr << "Bład\n";
  }
  /*
    PimpEx::Utils::EngineXMLReader reader("./test.xml");

    auto test = reader.get_scene_actor_components("Cave", "Player");
    if (test.has_value()) {
      for (const auto &c : *test) {
        std::cout << "Component type: " << c.name << "\n";
      }
    } else {
      std::cout << "Somethi wrong\n";
    }
    */
  return 0;
}
