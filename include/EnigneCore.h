#pragma once

#include "./EngineFileChecker.h"
#include "./EngineStorage.h"
#include "EngineGraphics.h"
#include "interfaces/IEngineContextAPI.h"
#include <memory>

namespace PimpEx {
class EngineCore : public IEngineContextAPI {
  std::unique_ptr<EngineStorage> _storage;
  std::unique_ptr<EngineGraphics> _graphics;

public:
  EngineCore() {
    _storage =
        std::make_unique<EngineStorage>(std::make_unique<EngineFileChecker>());
  }

  void run();

  IEngineStorageAPI *hook_engine_storage();
};
} // namespace PimpEx
