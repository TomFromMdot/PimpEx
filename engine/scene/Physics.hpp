#pragma once

#include "scene/World.hpp"

#include <vector>

namespace PimpEx::scene {

// Two actors with Colliders touching (or colliding this frame).
struct Contact {
  entt::entity a = entt::null;
  entt::entity b = entt::null;
};

// One frame of simple physics: gravity changes velocity, Body actors move axis
// by axis and stop at solid Colliders (landing sets Body.grounded). Returns the
// touching Collider pairs (solid and trigger) for on_touch.
std::vector<Contact> step_physics(World &world);

} // namespace PimpEx::scene
