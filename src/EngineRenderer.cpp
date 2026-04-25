#include "../include/graphics/EngineRenderer.hpp"
#include <memory>
#include <raylib.h>

void PimpEx::graphics::EngineRenderer::set_target_scene(
    std::shared_ptr<EngineScene> scenePtr) {
  _currentScene = scenePtr;
}

void PimpEx::graphics::EngineRenderer::render() {

  BeginMode3D((*_currentScene->get_camera().get()));

  EndMode3D();
}
