#pragma once
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>

#include <tinyxml2.h>

namespace PimpEx::edm {

namespace package {

enum class XMLFormatEngineData { Mesh, Config, Animation };

struct EngineData {
  virtual ~EngineData() = default;
  XMLFormatEngineData format;
  bool error = false;
  explicit EngineData(XMLFormatEngineData f) : format(f) {}
};

struct MeshXMLData : public EngineData {
  std::string mesh_name;
  int vertex_cout;
  MeshXMLData() : EngineData(XMLFormatEngineData::Mesh) {}
};

struct ConfigXMLData : public EngineData {
  std::string window_title;
  bool is_fullscreen;
  ConfigXMLData() : EngineData(XMLFormatEngineData::Config) {}
};

struct AnimationXMLData : public EngineData {
  std::string animation_name;
  AnimationXMLData() : EngineData(XMLFormatEngineData::Animation) {}
};
}; // namespace package

struct EDMTask {
  struct promise_type {
    std::unique_ptr<package::EngineData> result_data;

    EDMTask get_return_object() {
      return EDMTask{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
  };
  std::coroutine_handle<promise_type> handler;

  EDMTask(std::coroutine_handle<promise_type> h) : handler(h) {}
  ~EDMTask() {
    if (handler)
      handler.destroy();
  }

  bool done() const { return handler && handler.done(); }
};

struct LoadEvent {
  std::string filepath;
  package::XMLFormatEngineData format;
  std::coroutine_handle<EDMTask::promise_type> caller;
};

class Loader {
  std::queue<LoadEvent> _q;
  std::mutex _mtx;
  std::condition_variable _cv;
  std::jthread _worker;

  void worker_loop(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
      std::unique_lock<std::mutex> lock(_mtx);
      _cv.wait(lock, [&] { return stoken.stop_requested() || !_q.empty(); });

      if (stoken.stop_requested() && _q.empty())
        break;

      if (_q.empty())
        continue;

      LoadEvent event = _q.front();
      _q.pop();

      lock.unlock();

      std::unique_ptr<package::EngineData> result_data;

      switch (event.format) {
      case package::XMLFormatEngineData::Config:
        result_data = std::move(read_xml_config(event));
      case package::XMLFormatEngineData::Mesh:
        result_data = std::move(read_xml_mesh());
      case package::XMLFormatEngineData::Animation:
        result_data = std::move(read_xml_animation());
      }

      event.caller.promise().result_data = std::move(result_data);
      event.caller.resume();
    }
  }

  std::unique_ptr<package::ConfigXMLData> read_xml_config(const LoadEvent &e) {
    auto result = std::make_unique<package::ConfigXMLData>();

    using namespace tinyxml2;

    XMLDocument doc;
    auto error = doc.LoadFile(e.filepath.c_str());
    if (error != XML_SUCCESS) {
      std::cerr << "XML Error : " << e.filepath
                << "\nMessage: " << doc.ErrorStr() << "\n";
      return result;
    }

    result->format = package::XMLFormatEngineData::Config;
    result->is_fullscreen = true;
    result->window_title = "Window name";
    return result;
  }

  std::unique_ptr<package::MeshXMLData> read_xml_mesh() {
    auto result = std::make_unique<package::MeshXMLData>();

    // TODO TINYXML

    return result;
  }

  std::unique_ptr<package::AnimationXMLData> read_xml_animation() {
    auto result = std::make_unique<package::AnimationXMLData>();

    return result;
  }

public:
  Loader() : _worker([this](std::stop_token st) { worker_loop(st); }) {}

  void Enqueue(const std::string &filepath, package::XMLFormatEngineData format,
               std::coroutine_handle<EDMTask::promise_type> h) {
    {
      std::lock_guard<std::mutex> lock(_mtx);
      _q.push({filepath, format, h});
    }
    _cv.notify_one();
  }

  struct XMLAwaiter {
    Loader &_loader;
    std::string _filepath;
    package::XMLFormatEngineData _format;
    std::coroutine_handle<EDMTask::promise_type> _handle;

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<EDMTask::promise_type> h) {
      _handle = h;
      _loader.Enqueue(_filepath, _format, h);
    }

    std::unique_ptr<package::EngineData> await_resume() {
      return std::move(_handle.promise().result_data);
    }
  };

  XMLAwaiter LoadXMLAsync(const std::string &path,
                          package::XMLFormatEngineData format) {
    return XMLAwaiter{*this, path, format, nullptr};
  }
};

}; // namespace PimpEx::edm
