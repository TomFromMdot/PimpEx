#pragma once

#include "core/Diagnostic.hpp"
#include "scene/SceneData.hpp"

#include <string>
#include <string_view>

// Wczytywanie pimpex.xml i scene.xml. Funkcje działają na tekście,
// więc można je wołać z wątku zasobów i łatwo testować.
namespace PimpEx::boot {

scene::ProjectConfig parse_project_xml(std::string_view content, const std::string &file,
                                       Diagnostics &diagnostics);

scene::SceneData parse_scene_xml(std::string_view content, const std::string &file,
                                 const std::string &sceneName, Diagnostics &diagnostics);

} // namespace PimpEx::boot
