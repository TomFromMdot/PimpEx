#include "../include/EnigneCore.h"

void PimpEx::EngineCore::run() {
  while (_coreRunning) {
    if (_graphics->is_init())
      _graphics->init_graphics_state();
  }
}
