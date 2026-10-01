#pragma once

#include <mutex>
#include <vector>

namespace PimpEx {
template <typename T> class Mailbox {
  std::mutex _mutex;
  std::vector<T> _items;

public:
  void post(T item) {
    std::lock_guard lock(_mutex);
    _items.push_back(std::move(item));
  }

  std::vector<T> drain() {
    std::lock_guard lock(_mutex);
    std::vector<T> out;
    out.swap(_items);
    return out;
  }
};

} // namespace PimpEx
