#pragma once
#include "../GraphicsTypes.hpp"
#include "./EngineRenderer.hpp"

namespace PimpEx::graphics {
class EngineWindow {
  GraphicsTypes::WindowFlags _flags;
  GraphicsTypes::WindowResolutionBuffer _resolution;
  GraphicsTypes::WindowResolution _currentResolution;
  EngineRenderer _renderer;

public:
  EngineWindow(GraphicsTypes::WindowResolutionBuffer resBuffer,
               GraphicsTypes::WindowFlags flags)
      : _flags(flags), _resolution(std::move(resBuffer)) {}

  void set_current_resolution(const std::string &resName);

  void add_window_resolution(GraphicsTypes::WindowResolution res,
                             const std::string &resName);
  void run();
};
} // namespace PimpEx::graphics
