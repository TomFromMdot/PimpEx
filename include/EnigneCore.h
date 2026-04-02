#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "./EngineFileChecker.h"
#include "./EngineStorage.h"

#include "interfaces/IEngineContextAPI.h"

namespace PimpEx {
class EngineCore : public IEngineContextAPI {
  std::unique_ptr<EngineStorage> _engineMemory;

public:
  EngineCore() {
    _engineMemory =
        std::make_unique<EngineStorage>(std::make_unique<EngineFileChecker>());
  }
  IEngineStorageAPI *hook_engine_storage();
};
} // namespace PimpEx
