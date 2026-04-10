#include "../include/EngineXML.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <string>
#include <string_view>

constexpr std::string sceneName = "Cave";
constexpr std::string actorName = "Player";

int main(int argc, char *argv[]) {
  std::unique_ptr<PimpEx::IEngineXMLReader> reader =
      std::make_unique<PimpEx::Utils::EngineXMLReader>("./test.xml");

  auto resourceVector = reader->get_scene_resources(sceneName);
  if (!resourceVector.has_value()) {
    std::cerr << "Return nullptr\n";
    return 1;
  }
  std::cout << "Elements: " << (*resourceVector).size() << "\n";
  std::string resType;

  for (const auto &[rName, rPtr] : *resourceVector) {
    resType = rPtr->get_string_type(rPtr->type);

    std::println("Resource name: {}\nPath: {}\nType: {}", rPtr->name,
                 rPtr->path.c_str(), resType);
  }
  std::cout << "Start testing\n";
  return 0;
}
