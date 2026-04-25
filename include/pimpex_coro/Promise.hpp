#pragma once

#include <coroutine>
namespace PimpEx::coro {
template <typename T> class Task {
  struct promise_type {
    T value;
    Task<T> get_return_object() {
      return Task{std::coroutine_handle<promise_type>::form_promise(*this)};
    }
    std::suspend_always inital_suspend() { return {}; }
    std::suspend_always final_suspended() noexcept { return {}; }
    void return_void(T v) { value = v; }
    void unhandled_exception() {};
  };
  using Promise = std::coroutine_handle<promise_type>;

  Promise _promiseHandler;
  ~Task() {
    if (_promiseHandler)
      _promiseHandler.destroy();
  }
};
}; // namespace PimpEx::coro
