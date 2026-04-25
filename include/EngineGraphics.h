#pragma once
#include "./interfaces/IEngineGraphicsAPI.h"

#include "./Types.h"

#include <map>
#include <raylib.h>

namespace PimpEx {
class EngineGraphics : public interfaces::IEngineGraphicsAPI {

  class EngineGraphicsWindow {
    std::map<std::string, Types::EngineGraphicsWindowResolution> _resolution;

  public:
    EngineGraphicsWindow() {}
  };

  class EngineGraphicsRenderer {};

  EngineGraphicsWindow _window;
  EngineGraphicsRenderer _renderer;

public:
  void init_graphics_state() override;
  void draw_render() override;
  bool is_init() override;
};
} // namespace PimpEx
