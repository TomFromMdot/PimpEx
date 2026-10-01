#pragma once

#include "scene/ComponentRegistry.hpp"

#include <string>

namespace PimpEx::scene {

// All components as Markdown (docs/components.md, `pimpex components`).
std::string components_markdown(const ComponentRegistry &registry);

} // namespace PimpEx::scene
