#include "../include/graphics/EnigneWindow.hpp"

#include <algorithm>
#include <iostream>
#include <locale>
#include <raylib.h>
#include <string_view>

void PimpEx::graphics::EngineWindow::set_current_resolution(
    const std::string &resName) {
  auto it = _resolution.find(resName.data());
  if (it == _resolution.end()) {
    std::cerr << "Resolution: " << resName << "\n";
    return;
  }
  _currentResolution = it->second;
}

void PimpEx::graphics::EngineWindow::add_window_resolution(
    GraphicsTypes::WindowResolution res, const std::string &resName) {
  _resolution[resName] = res;
}
void PimpEx::graphics::EngineWindow::run() {
  InitWindow(_currentResolution.width, _currentResolution.height,
             _flags.title.c_str());

  while (!WindowShouldClose()) {
    _renderer.render();
  }
}
