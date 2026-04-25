#pragma once
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

namespace PimpEx::interfaces {
class IEngineGraphicsAPI {
public:
  virtual ~IEngineGraphicsAPI() = default;

  virtual void init_graphics_state() = 0;
  virtual void draw_render() = 0;
  virtual bool is_init() = 0;
};
} // namespace PimpEx::interfaces
