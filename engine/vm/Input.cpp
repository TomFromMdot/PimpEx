#include "vm/Input.hpp"

#include <charconv>
#include <format>

namespace PimpEx::vm {
namespace {

bool parse_long(std::string_view text, long &out) {
  const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
  return ec == std::errc{} && ptr == text.data() + text.size() && out >= 0;
}

bool parse_float(std::string_view text, float &out) {
  const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
  return ec == std::errc{} && ptr == text.data() + text.size();
}

// "10" albo "10-20".
bool parse_range(std::string_view range, long &from, long &to) {
  const auto dash = range.find('-');
  const bool ok = dash == std::string_view::npos
                      ? parse_long(range, from) && parse_long(range, to)
                      : parse_long(range.substr(0, dash), from) && parse_long(range.substr(dash + 1), to);
  return ok && to >= from;
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
      return std::format("'{}': użyj KLAWISZ:klatka, KLAWISZ:od-do albo CLICK@x/y:klatka", item);
    const auto head = item.substr(0, colon);
    const auto range = item.substr(colon + 1);
    if (head.starts_with("CLICK@") || head.starts_with("click@")) {
      const auto position = head.substr(6);
      const auto slash = position.find('/');
      Click click{0, 0, 0, 0};
      if (slash == std::string_view::npos || !parse_float(position.substr(0, slash), click.x) ||
          !parse_float(position.substr(slash + 1), click.y))
        return std::format("'{}': pozycję kliknięcia pisz jako CLICK@x/y, np. CLICK@400/300:10", item);
      if (!parse_range(range, click.from, click.to))
        return std::format("'{}': zły zakres klatek", item);
      _clicks.push_back(click);
      continue;
    }
    const auto key = key_from_name(head);
    if (!key)
      return std::format("'{}': nieznany klawisz", item);
    Press press{*key, 0, 0};
    if (!parse_range(range, press.from, press.to))
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
  set_mouse_down(MouseButton::Left, false);
  set_mouse_pressed(MouseButton::Left, false);
  for (const auto &c : _clicks) {
    if (frame < c.from)
      continue;
    set_mouse(c.x, c.y); // kursor zostaje tam, gdzie było ostatnie kliknięcie
    if (frame <= c.to)
      set_mouse_down(MouseButton::Left, true);
    if (frame == c.from)
      set_mouse_pressed(MouseButton::Left, true);
  }
}

} // namespace PimpEx::vm
