#include "script/LuaRuntime.hpp"
#include "script/LuaErrors.hpp"
#include "core/Format.hpp"

#include <sol/sol.hpp>

#include <charconv>

namespace PimpEx::script {
namespace {

sol::object to_lua_argument(sol::state &lua, const std::string &text) {
  double number = 0;
  const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), number);
  if (ec == std::errc{} && ptr == text.data() + text.size())
    return sol::make_object(lua, number);
  return sol::make_object(lua, text);
}

} // namespace

struct LuaRuntime::Impl {
  ILuaHost &host;
  sol::state lua;

  explicit Impl(ILuaHost &h) : host(h) {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table,
                       sol::lib::os);
    register_api();
  }

  static vm::ManualInput &no_input() {
    static vm::ManualInput input;
    return input;
  }

  // Kontekst klatki – poza klatką (np. przy wczytywaniu skryptu) pusty.
  vm::FrameContext frame() {
    if (const auto *ctx = host.lua_frame())
      return *ctx;
    return vm::FrameContext{no_input()};
  }

  void register_api() {
    lua.set_function("get", [this](const std::string &actor, const std::string &field) -> sol::object {
      const auto value = vm::read_field(host.lua_world(), frame(), actor, field);
      if (!value)
        return sol::lua_nil;
      // Pełne liczby jako całkowite: "Wynik: " .. 3 daje "3", a nie "3.0".
      if (*value == static_cast<double>(static_cast<long long>(*value)))
        return sol::make_object(lua, static_cast<long long>(*value));
      return sol::make_object(lua, *value);
    });
    lua.set_function("set", [this](const std::string &actor, const std::string &field,
                                   double value) { return host.lua_world().set(actor, field, value); });
    lua.set_function("move", [this](const std::string &actor, double dx, double dy) {
      auto &w = host.lua_world();
      if (!w.exists(actor))
        return false;
      w.set(actor, "x", *w.get(actor, "x") + dx);
      w.set(actor, "y", *w.get(actor, "y") + dy);
      return true;
    });
    lua.set_function("exists", [this](const std::string &actor) {
      return host.lua_world().exists(actor);
    });
    lua.set_function("touching", [this](const std::string &a, const std::string &b) {
      return host.lua_world().touching(a, b);
    });
    lua.set_function("spawn", [this](const std::string &archetype, sol::optional<std::string> name,
                                     sol::optional<double> x, sol::optional<double> y)
                                  -> std::optional<std::string> {
      std::optional<std::pair<float, float>> position;
      if (x && y)
        position = std::pair{static_cast<float>(*x), static_cast<float>(*y)};
      return host.lua_world().spawn(archetype, name.value_or(""), position);
    });
    lua.set_function("destroy", [this](const std::string &actor) {
      return host.lua_world().destroy(actor);
    });
    lua.set_function("text", [this](const std::string &actor, const std::string &value) {
      auto &w = host.lua_world();
      const auto e = w.find(actor);
      if (e == entt::null)
        return false;
      w.registry().get_or_emplace<scene::Label>(e).text = value;
      return true;
    });
    lua.set_function("actors", [this](sol::optional<std::string> archetype) {
      std::vector<std::string> names;
      auto &w = host.lua_world();
      for (const auto &name : w.names()) {
        if (archetype) {
          const auto &origin = w.registry().get<scene::Origin>(w.find(name));
          if (origin.archetype != *archetype)
            continue;
        }
        names.push_back(name);
      }
      return sol::as_table(names);
    });
    lua.set_function("key", [this](const std::string &name) {
      const auto key = vm::key_from_name(name);
      return key && frame().input.down(*key);
    });
    lua.set_function("pressed", [this](const std::string &name) {
      const auto key = vm::key_from_name(name);
      return key && frame().input.pressed(*key);
    });
    lua.set_function("frame", [this]() { return static_cast<double>(frame().frame); });
    lua.set_function("scene", [this](const std::string &name) { host.request_scene(name, 0); });
    lua.set_function("log", [this](sol::variadic_args args) {
      std::string text;
      for (auto arg : args) {
        if (!text.empty())
          text += ' ';
        const auto value = arg.get<sol::object>();
        if (value.get_type() == sol::type::number)
          text += format_number(value.as<double>()); // 1 zamiast 1.0
        else
          text += lua["tostring"](value).get<std::string>();
      }
      host.log(text);
    });

    auto swap = lua.create_named_table("swap");
    swap.set_function("add", [this](const std::string &name) { host.request_proc_add(name, 0); });
    swap.set_function("remove",
                      [this](const std::string &name) { host.request_proc_remove(name, 0); });
    swap.set_function("paste", [this](const std::string &source) { host.request_proc_paste(source); });
    swap.set_function("has", [this](const std::string &name) { return host.lua_has_proc(name); });
  }
};

LuaRuntime::LuaRuntime(ILuaHost &host) : _impl(std::make_unique<Impl>(host)) {}
LuaRuntime::~LuaRuntime() = default;

std::optional<Diagnostic> LuaRuntime::load_script(const std::string &displayName,
                                                  std::string_view source) {
  auto result = _impl->lua.safe_script(source, sol::script_pass_on_error, "@" + displayName);
  if (result.valid())
    return std::nullopt;
  sol::error err = result;
  return lua_error_to_diagnostic(err.what(), displayName);
}

bool LuaRuntime::has_function(const std::string &name) const {
  return _impl->lua[name].get_type() == sol::type::function;
}

std::optional<Diagnostic> LuaRuntime::call(const std::string &name,
                                           const std::vector<std::string> &args) {
  sol::protected_function fn = _impl->lua[name];
  if (!fn.valid())
    return Diagnostic{Diagnostic::Severity::Error, "scripts", 0,
                      "nie ma funkcji Lua '" + name + "'"};
  std::vector<sol::object> luaArgs;
  for (const auto &a : args)
    luaArgs.push_back(to_lua_argument(_impl->lua, a));
  auto result = fn(sol::as_args(luaArgs));
  if (result.valid())
    return std::nullopt;
  sol::error err = result;
  return lua_error_to_diagnostic(err.what(), "scripts");
}

} // namespace PimpEx::script
