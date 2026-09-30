#pragma once

#include "scene/SceneData.hpp"
#include "scene/World.hpp"
#include "vm/Vm.hpp"

#include <filesystem>
#include <memory>

namespace PimpEx::render {

// Rysuje świat za pomocą raylib. Tekstury wczytuje leniwie przy pierwszym
// użyciu i trzyma w pamięci do zmiany sceny.
class Renderer {
  struct Impl; // ukrywa typy raylib przed resztą silnika
  std::unique_ptr<Impl> _impl;

public:
  explicit Renderer(std::filesystem::path projectRoot);
  ~Renderer();

  // Zwalnia tekstury (zmiana sceny albo zmiana <Resources>).
  void unload_textures();

  void draw(const scene::World &world, const scene::SceneData &data,
            const scene::ProjectConfig &config, const vm::FrameContext &ctx);
};

} // namespace PimpEx::render
