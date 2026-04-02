#pragma once

#include "IEngineStorageAPI.h"
namespace PimpEx {
class IEngineContextAPI {
public:
  virtual ~IEngineContextAPI() = default;
  virtual IEngineStorageAPI *hook_engine_storage() = 0;
};
} // namespace PimpEx
