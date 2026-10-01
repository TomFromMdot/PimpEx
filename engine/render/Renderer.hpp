#pragma once

#include "gui/Gui.hpp"
#include "scene/SceneData.hpp"
#include "scene/World.hpp"
#include "vm/Vm.hpp"

#include <filesystem>
#include <memory>

namespace PimpEx::render {

// Draws the world, the HUD (space="screen") and the visible GUI on top.
// Textures and models are loaded lazily and kept until the scene changes.
class Renderer {
  struct Impl; // keeps raylib types out of the rest of the engine
  std::unique_ptr<Impl> _impl;

public:
  explicit Renderer(std::filesystem::path projectRoot);
  ~Renderer();

  void unload_textures();

  void draw(const scene::World &world, const scene::SceneData &data, const gui::GuiSystem &gui,
            const scene::ProjectConfig &config, const vm::FrameContext &ctx);

  // The fps camera is active but the cursor is free – mouse motion must not turn the player.
  bool mouse_look_suspended() const;
};

} // namespace PimpEx::render
