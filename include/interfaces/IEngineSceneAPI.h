#pragma once
#include <string>
namespace PimpEx {
class IEngineSceneAPI {
public:
  virtual ~IEngineSceneAPI() = default;
  virtual void change_current_scene(const std::string &sceneName) = 0;
  virtual void load_scene(const std::string &sceneName) = 0;
  virtual void unload_scene(const std::string &sceneName) = 0;
};
} // namespace PimpEx
