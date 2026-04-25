#pragma once
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>

namespace PimpEx::Components {
struct Transform {
  glm::vec3 position;
  glm::vec3 scale;
  glm::vec3 rotation;
};
}; // namespace PimpEx::Components
