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

// API na żywo: `pimpex dev` wystawia JSON-RPC na 127.0.0.1, żeby agent AI
// (albo inny program) mógł czytać i zmieniać działającą grę.
namespace PimpEx::api {

using json = nlohmann::json;

// Błąd metody API – trafia do odpowiedzi JSON-RPC jako "error".
struct ApiError : std::runtime_error {
  int code;
  ApiError(int c, const std::string &message) : std::runtime_error(message), code(c) {}
};

// Żądanie z wątku serwera, wykonywane na wątku głównym między klatkami.
struct ApiRequest {
  std::string method;
  json params;
  std::promise<json> reply; // {"result": ...} albo {"error": {...}}
};
using ApiMailbox = Mailbox<std::shared_ptr<ApiRequest>>;

// Sterowanie pętlą gry z API.
struct GameControl {
  bool paused = false;
  long stepFrames = 0; // ile klatek wykonać mimo pauzy
  bool quit = false;
  long frame = 0;
  vm::OverlayInput *input = nullptr;
  std::vector<std::shared_ptr<ApiRequest>> screenshots; // czekają na narysowanie klatki
  bool canScreenshot = false;                           // false w trybie --headless
};

// Wykonuje metodę API. Rzuca ApiError. `screenshot` odkłada żądanie
// do GameControl::screenshots (odpowiedź po narysowaniu klatki).
json dispatch(const std::shared_ptr<ApiRequest> &request, boot::Boot &boot, GameControl &control, Log &log);

// Lista metod z opisami (dla docs/api.md i MCP).
json method_list();

// Serwer HTTP na osobnym wątku. Zapisuje .pimpex/api.json (port + token).
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

  // Uruchamia serwer (port 0 = losowy wolny). Zwraca port albo 0 przy błędzie.
  int start(const std::string &projectRoot, int port = 0);
  void stop();
  const std::string &token() const { return _token; }
};

} // namespace PimpEx::api
