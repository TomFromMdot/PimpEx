#include "core/Engine.hpp"
#include "boot/Boot.hpp"
#include "boot/FileWatcher.hpp"
#include "boot/XmlLoader.hpp"
#include "core/Log.hpp"
#include "core/Mailbox.hpp"
#include "render/RaylibInput.hpp"
#include "render/Renderer.hpp"

#include <raylib.h>

#include <iostream>

namespace PimpEx {
namespace {

namespace fs = std::filesystem;

// Plik przygotowany na wątku zasobów – gotowy do podmiany w klatce.
struct FileUpdate {
  enum class Kind { Swap, Library, SceneXml, Script };
  Kind kind = Kind::Script;
  fs::path path;
  std::string text;
  vm::ParseResult program;
  scene::SceneData scene;
  Diagnostics diagnostics;
};

// Wątek zasobów: zamienia surowy tekst pliku na gotowe dane.
std::optional<FileUpdate> prepare(const boot::ProjectPaths &paths, const fs::path &path,
                                  std::string content) {
  FileUpdate update;
  update.path = path;
  const auto filename = path.filename().string();
  const auto display = paths.display(path);
  if (filename == "scene.swap" || filename == "library.swap") {
    update.kind = filename == "scene.swap" ? FileUpdate::Kind::Swap : FileUpdate::Kind::Library;
    update.program = vm::parse_program(content, display);
  } else if (filename == "scene.xml") {
    update.kind = FileUpdate::Kind::SceneXml;
    const auto sceneName = path.parent_path().filename().string();
    update.scene = boot::parse_scene_xml(content, display, sceneName, update.diagnostics);
  } else if (path.extension() == ".lua") {
    update.kind = FileUpdate::Kind::Script;
  } else {
    return std::nullopt;
  }
  update.text = std::move(content);
  return update;
}

void apply(boot::Boot &boot, render::Renderer *renderer, FileUpdate update) {
  const auto &paths = boot.paths();
  // Zmiany z innej sceny (np. tuż po SCENE) pomijamy.
  if (update.kind != FileUpdate::Kind::Script &&
      update.path.parent_path() != paths.scene_dir(boot.scene_name()))
    return;
  switch (update.kind) {
  case FileUpdate::Kind::Swap:
    boot.apply_swap(std::move(update.text), std::move(update.program));
    break;
  case FileUpdate::Kind::Library:
    boot.apply_library(std::move(update.text), std::move(update.program));
    break;
  case FileUpdate::Kind::SceneXml:
    boot.apply_scene_xml(std::move(update.scene), std::move(update.diagnostics));
    if (renderer)
      renderer->unload_textures(); // <Resources> mogły się zmienić
    break;
  case FileUpdate::Kind::Script:
    boot.apply_script(update.path, update.text);
    break;
  }
}

} // namespace

int Engine::run() {
  const boot::ProjectPaths paths{fs::absolute(_root).lexically_normal()};
  vm::ScriptedInput scriptedInput;
  if (const auto error = scriptedInput.parse(_options.input); !error.empty()) {
    std::cerr << "--input: " << error << '\n';
    return 1;
  }
  Log log;
  log.set_quiet(_options.quiet);
  log.open(paths.runtime_log());

  boot::Boot boot(paths, log);
  if (!boot.start()) {
    log.error("Nie udało się uruchomić gry. Popraw błędy i spróbuj ponownie (pimpex check).");
    return 1;
  }
  const auto &config = boot.config();

  Mailbox<FileUpdate> mailbox;
  boot::FileWatcher watcher([&](const fs::path &path, std::string content) {
    if (auto update = prepare(paths, path, std::move(content)))
      mailbox.post(std::move(*update));
  });
  if (_options.watch) {
    boot.set_self_write_callback(
        [&](const fs::path &path, std::size_t hash) { watcher.expect(path, hash); });
    watcher.watch(boot.watched_files());
    watcher.start();
    log.info("Hot-reload włączony – zapisz plik, a gra podmieni dane w locie.");
  }

  std::unique_ptr<render::Renderer> renderer;
  render::RaylibInput raylibInput;
  if (!_options.headless) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(config.width, config.height, config.title.c_str());
    SetTargetFPS(config.fps);
    renderer = std::make_unique<render::Renderer>(paths.root);
  }
  const vm::IInput &input = _options.headless ? static_cast<const vm::IInput &>(scriptedInput)
                                              : static_cast<const vm::IInput &>(raylibInput);

  long frame = 0;
  int sceneGeneration = boot.scene_generation();
  while (true) {
    if (!_options.headless && WindowShouldClose())
      break;
    if (_options.frames > 0 && frame >= _options.frames)
      break;

    for (auto &update : mailbox.drain())
      apply(boot, renderer.get(), std::move(update));

    if (_options.headless)
      scriptedInput.update(frame);
    else
      raylibInput.poll();

    const int width = _options.headless ? config.width : GetScreenWidth();
    const int height = _options.headless ? config.height : GetScreenHeight();
    const vm::FrameContext ctx{input, width, height, frame};
    boot.run_frame(ctx);

    if (boot.scene_generation() != sceneGeneration) {
      sceneGeneration = boot.scene_generation();
      if (renderer)
        renderer->unload_textures();
      if (_options.watch)
        watcher.watch(boot.watched_files());
    }

    if (renderer) {
      renderer->draw(boot.world(), boot.scene_data(), config, ctx);
      if (!_options.screenshot.empty() && frame + 1 == _options.frames) {
        Image image = LoadImageFromScreen();
        if (ExportImage(image, _options.screenshot.c_str()))
          log.info("Zrzut ekranu: " + _options.screenshot);
        else
          log.error("Nie mogę zapisać zrzutu ekranu " + _options.screenshot);
        UnloadImage(image);
      }
    }

    if (_options.stateEvery > 0 && frame % _options.stateEvery == 0)
      boot.write_state(frame);
    ++frame;
  }

  watcher.stop();
  boot.write_state(frame);
  renderer.reset();
  if (!_options.headless)
    CloseWindow();
  log.info("Koniec gry.");
  return 0;
}

} // namespace PimpEx
