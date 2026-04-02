#pragma once

namespace PimpEx {
class IEngineConfigurationAPI {
public:
  virtual ~IEngineConfigurationAPI() = default;
  virtual void set_hot_reload() = 0;
};
} // namespace PimpEx
