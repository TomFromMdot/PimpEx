#pragma once

#include "core/Diagnostic.hpp"
#include "gui/Gui.hpp"
#include "scene/ComponentRegistry.hpp"
#include "scene/SceneData.hpp"

#include <string>
#include <string_view>

// pimpex.xml, scene.xml and gui/*.xml. Text in, data out – safe on the asset
// thread and easy to test.
namespace PimpEx::boot {

scene::ProjectConfig parse_project_xml(std::string_view content, const std::string &file,
                                       Diagnostics &diagnostics);

// `components` includes user components (null = built-ins only).
scene::SceneData parse_scene_xml(std::string_view content, const std::string &file,
                                 const std::string &sceneName, Diagnostics &diagnostics,
                                 const scene::ComponentRegistry *components = nullptr);

gui::GuiScene parse_gui_xml(std::string_view content, const std::string &file, const std::string &name,
                            Diagnostics &diagnostics);

} // namespace PimpEx::boot
