#include "scene/Physics.hpp"

#include <algorithm>
#include <cmath>

namespace PimpEx::scene {
namespace {

// Axis by axis: 0 = x, 1 = y, 2 = z.
float &position(Transform &t, int axis) { return axis == 0 ? t.x : axis == 1 ? t.y : t.z; }
float size(const Transform &t, int axis) { return axis == 0 ? t.w : axis == 1 ? t.h : t.d; }
float &velocity(Body &b, int axis) { return axis == 0 ? b.vx : axis == 1 ? b.vy : b.vz; }

bool overlap(const Transform &a, const Transform &b) {
  return std::abs(a.x - b.x) * 2 < (a.w + b.w) && std::abs(a.y - b.y) * 2 < (a.h + b.h) &&
         std::abs(a.z - b.z) * 2 < (a.d + b.d);
}

} // namespace

std::vector<Contact> step_physics(World &world) {
  auto &registry = world.registry();
  const bool is3d = world.mode() == SceneMode::Mode3D;
  // "Down" is +y in 2D (screen) and -y in 3D (world).
  const float down = is3d ? -1.0f : 1.0f;

  std::vector<entt::entity> solids;
  for (auto [e, collider] : registry.view<Collider>().each())
    if (collider.kind == "solid")
      solids.push_back(e);

  for (auto [e, t, body] : registry.view<Transform, Body>().each()) {
    body.grounded = false;
    body.vy += body.gravity * down;
    const auto *collider = registry.try_get<Collider>(e);
    const bool collides = collider && collider->kind == "solid";

    for (int axis = 0; axis < (is3d ? 3 : 2); ++axis) {
      float &v = velocity(body, axis);
      if (v == 0)
        continue;
      position(t, axis) += v;
      if (!collides)
        continue;
      for (const auto other : solids) {
        if (other == e)
          continue;
        const auto &ot = registry.get<Transform>(other);
        if (!overlap(t, ot))
          continue;
        // Step back exactly to the obstacle's edge.
        const float gap = (size(t, axis) + size(ot, axis)) / 2;
        const float otherPos = axis == 0 ? ot.x : axis == 1 ? ot.y : ot.z;
        position(t, axis) = v > 0 ? otherPos - gap : otherPos + gap;
        if (axis == 1 && v * down > 0)
          body.grounded = true;
        v = 0;
      }
    }
    if (!is3d)
      t.z += body.vz;
  }

  std::vector<Contact> contacts;
  std::vector<entt::entity> colliders;
  for (auto [e, collider] : registry.view<Collider>().each())
    colliders.push_back(e);
  // Two static actors (e.g. neighbouring map walls) do not matter – only pairs
  // with at least one actor that can move (Body or a Lua behavior). Movers are
  // first in the list, so a mover–mover pair is counted once.
  auto dynamic = [&](entt::entity e) { return registry.any_of<Body, Behaviors>(e); };
  std::stable_partition(colliders.begin(), colliders.end(), dynamic);
  for (std::size_t i = 0; i < colliders.size() && dynamic(colliders[i]); ++i) {
    // 0.01 margin – an actor standing exactly on the floor also "touches" it.
    Transform grown = registry.get<Transform>(colliders[i]);
    grown.w += 0.02f;
    grown.h += 0.02f;
    grown.d += 0.02f;
    for (std::size_t j = i + 1; j < colliders.size(); ++j)
      if (overlap(grown, registry.get<Transform>(colliders[j])))
        contacts.push_back({colliders[i], colliders[j]});
  }
  return contacts;
}

} // namespace PimpEx::scene
