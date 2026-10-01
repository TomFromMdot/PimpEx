#pragma once

#include "boot/Boot.hpp"
#include "core/Log.hpp"
#include "core/Mailbox.hpp"
#include "vm/Input.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace httplib {
class Server;
}

// Live API: `pimpex dev` serves JSON-RPC on 127.0.0.1 so an AI agent (or any
// program) can read and change the running game.
namespace PimpEx::api {

using json = nlohmann::json;

// Becomes the JSON-RPC "error".
struct ApiError : std::runtime_error {
  int code;
  ApiError(int c, const std::string &message) : std::runtime_error(message), code(c) {}
};

// Made on the server thread, executed on the main thread between frames.
struct ApiRequest {
  std::string method;
  json params;
  std::promise<json> reply; // {"result": ...} or {"error": {...}}
};
using ApiMailbox = Mailbox<std::shared_ptr<ApiRequest>>;

struct GameControl {
  bool paused = false;
  long stepFrames = 0; // frames to run while paused
  bool quit = false;
  long frame = 0;
  vm::OverlayInput *input = nullptr;
  std::vector<std::shared_ptr<ApiRequest>> screenshots; // wait for the next drawn frame
  bool canScreenshot = false;                           // false with --headless
};

// Throws ApiError. `screenshot` is queued in GameControl::screenshots and
// answered after the frame is drawn.
json dispatch(const std::shared_ptr<ApiRequest> &request, boot::Boot &boot, GameControl &control, Log &log);

// Methods with descriptions (for docs/api.md and MCP).
json method_list();

// HTTP server on its own thread. Writes .pimpex/api.json (port + token).
class LiveServer {
  ApiMailbox &_mailbox;
  std::filesystem::path _infoFile;
  std::unique_ptr<httplib::Server> _server;
  std::jthread _thread;
  std::string _token;
  int _port = 0;

public:
  LiveServer(ApiMailbox &mailbox, std::filesystem::path infoFile);
  ~LiveServer();

  // port 0 = any free port. Returns the port, or 0 on failure.
  int start(const std::string &projectRoot, int port = 0);
  void stop();
  const std::string &token() const { return _token; }
};

} // namespace PimpEx::api
