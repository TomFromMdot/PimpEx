#pragma once

#include <filesystem>
#include <string>

namespace PimpEx {

struct EngineOptions {
  bool watch = true;     // hot-reload
  bool headless = false;
  long frames = 0;       // 0 = until the window closes
  int stateEvery = 30;   // frames between .pimpex/state.txt dumps
  bool quiet = false;    // log only to the file
  std::string input;     // --headless: scripted input, e.g. "RIGHT:0-60 SPACE:10"
  std::string screenshot; // of the last frame (needs a window and --frames)
  std::string scene;      // overrides <StartScene>
  bool api = false;       // live JSON-RPC API on 127.0.0.1 (.pimpex/api.json)
  int apiPort = 0;        // 0 = any free port
  std::filesystem::path runtimeDir; // instead of .pimpex/ (runtime.log, state.txt)
};

// Game loop:
//   asset thread: FileWatcher -> parsing -> Mailbox
//   main thread:  Mailbox -> Boot (data swap) -> VM -> movement -> drawing
class Engine {
  std::filesystem::path _root;
  EngineOptions _options;

public:
  Engine(std::filesystem::path projectRoot, EngineOptions options)
      : _root(std::move(projectRoot)), _options(options) {}

  int run();
};

} // namespace PimpEx
