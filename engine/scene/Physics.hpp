#pragma once

#include "scene/World.hpp"

#include <vector>

namespace PimpEx::scene {

// Dwóch aktorów z Colliderem, którzy się stykają (albo zderzyli w tej klatce).
struct Contact {
  entt::entity a = entt::null;
  entt::entity b = entt::null;
};

// Jedna klatka prostej fizyki:
//  - grawitacja zmienia prędkość (w 2D w dół = +y, w 3D w dół = -y),
//  - aktor z Body przesuwa się oś po osi,
//  - jeśli ma Collider, nie wchodzi w solidne Collidery innych aktorów
//    (zatrzymuje się na nich; lądowanie ustawia Body.grounded),
//  - zwraca pary stykających się Colliderów (solid i trigger) – dla on_touch.
std::vector<Contact> step_physics(World &world);

} // namespace PimpEx::scene
