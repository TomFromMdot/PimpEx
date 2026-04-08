#include <cassert>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <string_view>

#include "../include/EngineXML.h"

constexpr std::string sceneName = "Cave";
constexpr std::string actorName = "Player";

int main(int argc, char *argv[]) {
  std::cout << "Start testing\n";
  PimpEx::Utils::EngineXMLReader reader("test.xml");
  const auto &tmp = reader.get_scene_actor_components(sceneName, actorName);
  if (tmp.has_value()) {
    std::print("Scene: {}\nActor: {}", sceneName, actorName);
    for (auto e : *tmp) {
      std::cout << e.name << "\n";
    }
  }
  return 0;
}
