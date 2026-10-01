#include "cli/commands/RunCommand.hpp"

#include <iostream>
#include <print>

namespace PimpEx::cli {

CLI::App *RunCommand::setup(CLI::App &app) {
  auto *sub = _dev ? app.add_subcommand("dev", "Runs the game with hot-reload: saved file changes "
                                               "show up immediately.")
                   : app.add_subcommand("play", "Runs the game (without watching files).");
  sub->add_option("project", _project, "Project directory")->capture_default_str();
  sub->add_option("--scene", _options.scene, "Start scene (instead of <StartScene> from pimpex.xml)");
  sub->add_option("--frames", _options.frames, "Stop after this many frames (0 = no limit)");
  sub->add_flag("--headless", _options.headless,
                "No window – logic only (tests, CI, AI agents); needs --frames");
  sub->add_option("--state-every", _options.stateEvery,
                  "Frames between .pimpex/state.txt writes (0 = only at the end)")
      ->capture_default_str();
  sub->add_option("--input", _options.input,
                  "With --headless: scripted input, e.g. \"RIGHT:0-60 SPACE:10 CLICK@400/300:20\"");
  sub->add_option("--screenshot", _options.screenshot,
                  "Save a screenshot of the last frame as .png (needs --frames)");
  if (_dev)
    sub->add_flag("!--no-api", _options.api, "Without the live API for agents");
  else
    sub->add_flag("--api", _options.api, "Enable the live API (JSON-RPC, .pimpex/api.json)");
  sub->add_option("--api-port", _options.apiPort, "API port (default: any free port)");
  sub->add_flag("--quiet", _options.quiet, "Log only to .pimpex/runtime.log");
  sub->add_option("--runtime-dir", _options.runtimeDir,
                  "Directory for runtime.log and state.txt instead of .pimpex/ (e.g. for tests)");
  return sub;
}

int RunCommand::run() {
  if (_options.headless && _options.frames <= 0 && !_options.api) {
    std::println(std::cerr, "--headless needs --frames N or the API (pimpex dev --headless runs until "
                            "game.quit / Ctrl+C).");
    return 1;
  }
  if (!_options.input.empty() && !_options.headless) {
    std::println(std::cerr, "--input only works with --headless.");
    return 1;
  }
  if (!_options.screenshot.empty() && (_options.headless || _options.frames <= 0)) {
    std::println(std::cerr, "--screenshot needs a window (no --headless) and --frames N.");
    return 1;
  }
  Engine engine(_project, _options);
  return engine.run();
}

} // namespace PimpEx::cli
