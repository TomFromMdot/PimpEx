#include "vm/Input.hpp"

#include <charconv>
#include <format>

namespace PimpEx::vm {
namespace {

bool parse_long(std::string_view text, long &out) {
  const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
  return ec == std::errc{} && ptr == text.data() + text.size() && out >= 0;
}

} // namespace

std::string ScriptedInput::parse(std::string_view spec) {
  std::size_t pos = 0;
  while (pos < spec.size()) {
    const auto start = spec.find_first_not_of(" ,", pos);
    if (start == std::string_view::npos)
      break;
    auto end = spec.find_first_of(" ,", start);
    if (end == std::string_view::npos)
      end = spec.size();
    const auto item = spec.substr(start, end - start);
    pos = end;

    const auto colon = item.find(':');
    if (colon == std::string_view::npos)
      return std::format("'{}': użyj KLAWISZ:klatka albo KLAWISZ:od-do", item);
    const auto key = key_from_name(item.substr(0, colon));
    if (!key)
      return std::format("'{}': nieznany klawisz", item);
    const auto range = item.substr(colon + 1);
    const auto dash = range.find('-');
    Press press{*key, 0, 0};
    const bool ok = dash == std::string_view::npos
                        ? parse_long(range, press.from) && parse_long(range, press.to)
                        : parse_long(range.substr(0, dash), press.from) &&
                              parse_long(range.substr(dash + 1), press.to);
    if (!ok || press.to < press.from)
      return std::format("'{}': zły zakres klatek", item);
    _presses.push_back(press);
  }
  return {};
}

void ScriptedInput::update(long frame) {
  for (std::size_t i = 0; i < KeyCount; ++i) {
    set_down(static_cast<Key>(i), false);
    set_pressed(static_cast<Key>(i), false);
  }
  for (const auto &p : _presses) {
    if (frame >= p.from && frame <= p.to)
      set_down(p.key, true);
    if (frame == p.from)
      set_pressed(p.key, true);
  }
}

} // namespace PimpEx::vm
