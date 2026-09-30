#pragma once

#include "scene/SceneData.hpp"
#include "scene/World.hpp"
#include "vm/Vm.hpp"

#include <filesystem>
#include <memory>

namespace PimpEx::render {

// Rysuje świat za pomocą raylib:
//  - scena 2D: kształty, obrazki i napisy (z kamerą 2D, jeśli jest),
//  - scena 3D: prymitywy i modele z oświetleniem + kamera 3D,
//  - HUD: aktorzy ze space="screen" w pikselach ekranu.
// Zasoby (tekstury, modele) wczytuje leniwie i trzyma do zmiany sceny.
class Renderer {
  struct Impl; // ukrywa typy raylib przed resztą silnika
  std::unique_ptr<Impl> _impl;

public:
  explicit Renderer(std::filesystem::path projectRoot);
  ~Renderer();

  // Zwalnia tekstury i modele (zmiana sceny albo zmiana <Resources>).
  void unload_textures();

  void draw(const scene::World &world, const scene::SceneData &data,
            const scene::ProjectConfig &config, const vm::FrameContext &ctx);
};

} // namespace PimpEx::render
