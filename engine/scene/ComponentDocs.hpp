#pragma once

#include "scene/ComponentRegistry.hpp"

#include <string>

namespace PimpEx::scene {

// Opis wszystkich komponentów jako Markdown (docs/components.md, `pimpex components`).
std::string components_markdown(const ComponentRegistry &registry);

} // namespace PimpEx::scene
