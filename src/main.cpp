#include <filesystem>
#include <iostream>
#include <print>
#include <tinyxml2.h>

#include "../include/Utils.h"

int main(int argvgc, char *argv[]) {

  std::cout << "Hello world\n";

  PimpEx::Utils::print_actor_component("./test.xml", "player");
  // PimpEx::Utils::print_actors("./test.xml");
  return 0;
}
