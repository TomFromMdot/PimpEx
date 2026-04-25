#pragma once
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <raylib.h>
#include <string>
#include <sys/types.h>
namespace PimpEx::GraphicsTypes {

enum class WindowStyle { Fullscreen, Window };

struct WindowFlags {
  WindowStyle windowStyle = WindowStyle::Window;
  int exitKey = KEY_ESCAPE;
  std::string title;
  bool isRunning;
};

struct WindowResolution {
  int width, height;
  std::string aspect;
  short id;
};

using WindowResolutionBuffer = std::map<std::string, WindowResolution>;

struct RendererScene {
  entt::registry entityRegister;
};

}; // namespace PimpEx::GraphicsTypes
