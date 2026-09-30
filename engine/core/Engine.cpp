#include "core/Engine.hpp"
#include "api/LiveApi.hpp"
#include "boot/Boot.hpp"
#include "boot/FileWatcher.hpp"
#include "boot/XmlLoader.hpp"
#include "core/Log.hpp"
#include "core/Mailbox.hpp"
#include "render/RaylibInput.hpp"
#include "render/Renderer.hpp"

#include <raylib.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>
#include <iostream>

namespace PimpEx {
namespace {

namespace fs = std::filesystem;

// Plik przygotowany na wątku zasobów – gotowy do podmiany w klatce.
struct FileUpdate {
  enum class Kind { Swap, Library, SceneXml, Components, Script };
  Kind kind = Kind::Script;
  fs::path path;
  std::string text;
  vm::ParseResult program;
  scene::SceneData scene;
  Diagnostics diagnostics;
};

// Wątek zasobów: zamienia surowy tekst pliku na gotowe dane.
std::optional<FileUpdate> prepare(const boot::ProjectPaths &paths,
                                  const scene::ComponentRegistry &components, const fs::path &path,
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
    update.scene = boot::parse_scene_xml(content, display, sceneName, update.diagnostics, &components);
  } else if (filename == "components.xml") {
    update.kind = FileUpdate::Kind::Components;
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
  if (update.kind != FileUpdate::Kind::Script && update.kind != FileUpdate::Kind::Components &&
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
  case FileUpdate::Kind::Components:
    boot.apply_components_xml(update.text);
    if (renderer)
      renderer->unload_textures();
    break;
  case FileUpdate::Kind::Script:
    boot.apply_script(update.path, update.text);
    break;
  }
}

// Ctrl+C / kill: kończymy pętlę porządnie (zapis state.txt, usunięcie api.json).
struct StopSignal {
  static inline std::atomic<bool> flag{false};
  static void handler(int) { flag = true; }
  static void install() {
    std::signal(SIGINT, handler);
    std::signal(SIGTERM, handler);
  }
  static bool requested() { return flag.load(); }
};

std::string base64(const unsigned char *data, std::size_t size) {
  static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((size + 2) / 3 * 4);
  for (std::size_t i = 0; i < size; i += 3) {
    const unsigned value = data[i] << 16 | (i + 1 < size ? data[i + 1] << 8 : 0) | (i + 2 < size ? data[i + 2] : 0);
    out += table[(value >> 18) & 63];
    out += table[(value >> 12) & 63];
    out += i + 1 < size ? table[(value >> 6) & 63] : '=';
    out += i + 2 < size ? table[value & 63] : '=';
  }
  return out;
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
  boot.set_persist_swap(_options.watch);
  if (!boot.start(_options.scene)) {
    log.error("Nie udało się uruchomić gry. Popraw błędy i spróbuj ponownie (pimpex check).");
    return 1;
  }
  const auto &config = boot.config();

  Mailbox<FileUpdate> mailbox;
  // Wątek zasobów parsuje scene.xml z aktualnym rejestrem komponentów.
  // Rejestr jest niezmienny, a wskaźnik podmieniamy atomowo.
  std::atomic<std::shared_ptr<const scene::ComponentRegistry>> components{boot.components()};
  boot::FileWatcher watcher([&](const fs::path &path, std::string content) {
    const auto registry = components.load();
    if (auto update = prepare(paths, *registry, path, std::move(content)))
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
    SetExitKey(KEY_NULL); // ESC to zwykły klawisz w grze; okno zamyka się krzyżykiem
    SetTargetFPS(config.fps);
    renderer = std::make_unique<render::Renderer>(paths.root);
  }
  // Prawdziwy input + klawisze wciśnięte przez API (input.press).
  vm::OverlayInput input;
  input.set_base(_options.headless ? static_cast<const vm::IInput *>(&scriptedInput)
                                   : static_cast<const vm::IInput *>(&raylibInput));

  // API na żywo dla agentów (pimpex dev): JSON-RPC na 127.0.0.1.
  api::ApiMailbox apiMailbox;
  api::GameControl control;
  control.input = &input;
  control.canScreenshot = !_options.headless;
  api::LiveServer server(apiMailbox, paths.runtime_dir() / "api.json");
  if (_options.api) {
    if (const int port = server.start(paths.root.string(), _options.apiPort))
      log.info(std::format("API na żywo: http://127.0.0.1:{} (token w .pimpex/api.json)", port));
    else
      log.error("Nie udało się uruchomić API na żywo.");
  }

  StopSignal::install();
  long frame = 0;
  int sceneGeneration = boot.scene_generation();
  const auto frameTime = std::chrono::microseconds(1'000'000 / std::max(1, config.fps));
  auto nextTick = std::chrono::steady_clock::now();
  while (!StopSignal::requested() && !control.quit) {
    if (!_options.headless && WindowShouldClose())
      break;
    if (_options.frames > 0 && frame >= _options.frames)
      break;

    for (auto &update : mailbox.drain())
      apply(boot, renderer.get(), std::move(update));
    if (components.load() != boot.components())
      components.store(boot.components());

    // Żądania API wykonujemy między klatkami, na wątku głównym.
    control.frame = frame;
    input.set_frame(frame);
    for (auto &request : apiMailbox.drain()) {
      try {
        auto result = api::dispatch(request, boot, control, log);
        if (!result.is_null() || request->method != "screenshot")
          request->reply.set_value({{"result", result}});
      } catch (const api::ApiError &e) {
        request->reply.set_value({{"error", {{"code", e.code}, {"message", e.what()}}}});
      } catch (const std::exception &e) {
        request->reply.set_value({{"error", {{"code", -32603}, {"message", e.what()}}}});
      }
    }

    if (_options.headless)
      scriptedInput.update(frame);
    else
      raylibInput.poll();

    const int width = _options.headless ? config.width : GetScreenWidth();
    const int height = _options.headless ? config.height : GetScreenHeight();
    const vm::FrameContext ctx{input, width, height, frame};
    const bool runLogic = !control.paused || control.stepFrames > 0;
    if (runLogic) {
      if (control.paused)
        --control.stepFrames;
      boot.run_frame(ctx);
    }

    if (boot.scene_generation() != sceneGeneration) {
      sceneGeneration = boot.scene_generation();
      if (renderer)
        renderer->unload_textures();
      if (_options.watch)
        watcher.watch(boot.watched_files());
    }

    if (renderer) {
      renderer->draw(boot.world(), boot.scene_data(), config, ctx);
      const bool lastFrame = frame + 1 == _options.frames;
      if ((!_options.screenshot.empty() && lastFrame && runLogic) || !control.screenshots.empty()) {
        Image image = LoadImageFromScreen();
        if (!_options.screenshot.empty() && lastFrame) {
          if (ExportImage(image, _options.screenshot.c_str()))
            log.info("Zrzut ekranu: " + _options.screenshot);
          else
            log.error("Nie mogę zapisać zrzutu ekranu " + _options.screenshot);
        }
        if (!control.screenshots.empty()) {
          int size = 0;
          unsigned char *png = ExportImageToMemory(image, ".png", &size);
          const api::json result = {{"width", image.width},
                                    {"height", image.height},
                                    {"mime", "image/png"},
                                    {"png_base64", base64(png, static_cast<std::size_t>(size))}};
          MemFree(png);
          for (auto &request : control.screenshots)
            request->reply.set_value({{"result", result}});
          control.screenshots.clear();
        }
        UnloadImage(image);
      }
    } else if (_options.api) {
      // Bez okna nikt nie ogranicza tempa – przy API gra chodzi w swoim fps.
      nextTick += frameTime;
      std::this_thread::sleep_until(nextTick);
      if (std::chrono::steady_clock::now() - nextTick > std::chrono::seconds(1))
        nextTick = std::chrono::steady_clock::now();
    }

    if (runLogic) {
      if (_options.stateEvery > 0 && frame % _options.stateEvery == 0)
        boot.write_state(frame);
      ++frame;
    }
  }

  server.stop();
  watcher.stop();
  boot.write_state(frame);
  renderer.reset();
  if (!_options.headless)
    CloseWindow();
  log.info("Koniec gry.");
  return 0;
}

} // namespace PimpEx
