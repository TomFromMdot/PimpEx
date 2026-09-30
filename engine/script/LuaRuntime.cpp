#include "script/LuaRuntime.hpp"
#include "script/LuaErrors.hpp"
#include "core/Format.hpp"

#include <sol/sol.hpp>

#include <charconv>
#include <cmath>
#include <format>
#include <tuple>
#include <map>
#include <set>

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

// Metody i pola `self` w zachowaniach. self.x czyta pole aktora, self.x = 5
// je ustawia; inne klucze (self.timer, parametry z XML) to zwykłe dane Lua.
constexpr const char *SelfPrelude = R"(
local fields = { x=true, y=true, z=true, w=true, h=true, d=true, rx=true, ry=true, rz=true,
  rotation=true, scale=true, layer=true, visible=true, vx=true, vy=true, vz=true,
  gravity=true, grounded=true }
local Self = {}
function Self:get(f) return get(self.name, f) end
function Self:set(f, v) return set(self.name, f, v) end
function Self:move(dx, dy, dz) return move(self.name, dx, dy, dz) end
function Self:destroy() return destroy(self.name) end
function Self:exists() return exists(self.name) end
function Self:has(c) return has(self.name, c) end
function Self:has_tag(t) return has_tag(self.name, t) end
function Self:touching(o) return touching(self.name, o) end
function Self:text(t) return text(self.name, t) end
function Self:distance(o) return distance(self.name, o) end
-- Kierunek patrzenia: w 3D z obrotu ry (0 = w stronę -z), w 2D z rotation (0 = w prawo).
function Self:forward()
  if is3d() then
    local yaw = math.rad(self.ry)
    return -math.sin(yaw), 0, -math.cos(yaw)
  end
  local a = math.rad(self.rotation)
  return math.cos(a), math.sin(a), 0
end
-- Co widzę przed sobą (do max)? Zwraca nazwę aktora z solidnym Colliderem i odległość.
function Self:look(max)
  local dx, dy, dz = self:forward()
  return raycast(self.x, self.y, self.z, dx, dy, dz, max or 1000, self.name)
end
-- Czy widzę aktora `target` (nic solidnego pomiędzy)?
function Self:sees(target, max)
  if not exists(target) then return false end
  local dx, dy, dz = get(target, "x") - self.x, get(target, "y") - self.y, get(target, "z") - self.z
  local dist = math.sqrt(dx * dx + dy * dy + dz * dz)
  if max and dist > max then return false end
  local hit = raycast(self.x, self.y, self.z, dx, dy, dz, dist + 1, self.name)
  return hit == target
end
-- Obraca się przodem do aktora `target`.
function Self:face(target)
  if not exists(target) then return end
  local dx, dy, dz = get(target, "x") - self.x, get(target, "y") - self.y, get(target, "z") - self.z
  if is3d() then self.ry = math.deg(math.atan(-dx, -dz)) else self.rotation = math.deg(math.atan(dy, dx)) end
end
local meta = {
  __index = function(t, k)
    if Self[k] then return Self[k] end
    if fields[k] then return get(rawget(t, "name"), k) end
    return nil
  end,
  __newindex = function(t, k, v)
    if fields[k] then set(rawget(t, "name"), k, v) else rawset(t, k, v) end
  end,
  __tostring = function(t) return rawget(t, "name") end,
}
function __pimpex_new_self(name, params)
  local self = {}
  for k, v in pairs(params) do self[k] = v end
  self.name = name
  return setmetatable(self, meta)
end
)";

sol::object param_to_lua(sol::state &lua, const std::string &text) {
  if (text == "true")
    return sol::make_object(lua, true);
  if (text == "false")
    return sol::make_object(lua, false);
  return to_lua_argument(lua, text);
}

struct LuaRuntime::Impl {
  ILuaHost &host;
  sol::state lua;

  // Zachowania: nazwa -> tabela z pliku; aktor -> jego instancje `self`.
  struct Instance {
    std::string behavior;
    sol::table self;
  };
  struct Tracked {
    std::string name;
    std::vector<std::string> behaviorNames;
    std::vector<Instance> list;
  };
  std::map<std::string, sol::table> behaviors;
  std::map<entt::entity, Tracked> instances;

  // Systemy z system(selektor, funkcja) – zapamiętane z plikiem, który je dodał.
  struct System {
    std::string script;
    std::string selector;
    sol::protected_function fn;
  };
  std::vector<System> systems;
  std::string loadingScript;
  std::set<std::string> reported; // ten sam błąd Lua tylko raz

  void report(const std::string &message, const std::string &fallbackFile) {
    if (reported.insert(message).second)
      host.lua_error(lua_error_to_diagnostic(message, fallbackFile));
  }

  // Woła behavior.fn(self, args...), jeśli zachowanie ma taką funkcję.
  template <typename... Args> void invoke(const Instance &instance, const char *fn, Args &&...args) {
    const auto it = behaviors.find(instance.behavior);
    if (it == behaviors.end())
      return;
    sol::object value = it->second[fn];
    if (value.get_type() != sol::type::function)
      return;
    sol::protected_function function = value;
    auto result = function(instance.self, std::forward<Args>(args)...);
    if (!result.valid()) {
      sol::error err = result;
      report(err.what(), "scripts/behaviors/" + instance.behavior + ".lua");
    }
  }

  sol::table new_self(const std::string &name, const std::map<std::string, std::string> &params) {
    sol::table table = lua.create_table();
    for (const auto &[key, value] : params)
      table[key] = param_to_lua(lua, value);
    sol::protected_function make = lua["__pimpex_new_self"];
    return make(name, table).get<sol::table>();
  }

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
      const auto value = vm::read_value(host.lua_world(), frame(), actor, field);
      if (!value)
        return sol::lua_nil;
      if (const auto *text = std::get_if<std::string>(&*value))
        return sol::make_object(lua, *text);
      // Pełne liczby jako całkowite: "Wynik: " .. 3 daje "3", a nie "3.0".
      const double number = std::get<double>(*value);
      if (number == static_cast<double>(static_cast<long long>(number)))
        return sol::make_object(lua, static_cast<long long>(number));
      return sol::make_object(lua, number);
    });
    lua.set_function("set", [this](const std::string &actor, const std::string &field, sol::object value) {
      scene::FieldValue v;
      if (value.get_type() == sol::type::number)
        v = value.as<double>();
      else if (value.get_type() == sol::type::boolean)
        v = value.as<bool>() ? 1.0 : 0.0;
      else if (value.get_type() == sol::type::string)
        v = value.as<std::string>();
      else
        throw sol::error("set: wartość musi być liczbą, tekstem albo true/false");
      if (auto error = host.lua_world().set_field(actor, field, v))
        throw sol::error("set: " + *error);
      return true;
    });
    lua.set_function("move", [this](const std::string &actor, double dx, double dy, sol::optional<double> dz) {
      auto &w = host.lua_world();
      if (!w.exists(actor))
        return false;
      w.set(actor, "x", *w.get(actor, "x") + dx);
      w.set(actor, "y", *w.get(actor, "y") + dy);
      if (dz)
        w.set(actor, "z", *w.get(actor, "z") + *dz);
      return true;
    });
    lua.set_function("exists", [this](const std::string &actor) {
      return host.lua_world().exists(actor);
    });
    lua.set_function("touching", [this](const std::string &a, const std::string &b) {
      return host.lua_world().touching(a, b);
    });
    lua.set_function("spawn", [this](const std::string &archetype, sol::optional<std::string> name,
                                     sol::optional<double> x, sol::optional<double> y,
                                     sol::optional<double> z) -> std::optional<std::string> {
      std::optional<std::pair<float, float>> position;
      if (x && y)
        position = std::pair{static_cast<float>(*x), static_cast<float>(*y)};
      auto &w = host.lua_world();
      auto created = w.spawn(archetype, name.value_or(""), position);
      if (created && z)
        w.set(*created, "z", *z);
      return created;
    });
    lua.set_function("destroy", [this](const std::string &actor) {
      return host.lua_world().destroy(actor);
    });
    lua.set_function("text", [this](const std::string &actor, const std::string &value) {
      return !host.lua_world().set_field(actor, "Text.value", value).has_value();
    });
    // Komponenty i zapytania (ECS).
    lua.set_function("actors", [this](sol::optional<std::string> selector) {
      return sol::as_table(host.lua_world().query(selector.value_or("")));
    });
    lua.set_function("has", [this](const std::string &actor, const std::string &component) {
      auto &w = host.lua_world();
      return w.has_component(w.find(actor), component);
    });
    lua.set_function("add", [this](const std::string &actor, const std::string &component) {
      if (auto error = host.lua_world().add_component(actor, component))
        throw sol::error("add: " + *error);
      return true;
    });
    lua.set_function("remove", [this](const std::string &actor, const std::string &component) {
      return host.lua_world().remove_component(actor, component);
    });
    lua.set_function("has_tag", [this](const std::string &actor, const std::string &tag) {
      auto &w = host.lua_world();
      return w.has_tag(w.find(actor), tag);
    });
    // Promień: pierwszy aktor z solidnym Colliderem. Zwraca nazwę i odległość albo nil.
    lua.set_function("raycast", [this](double x, double y, double z, double dx, double dy, double dz,
                                       double maxDistance, sol::optional<std::string> ignore,
                                       sol::this_state state) -> std::tuple<sol::object, sol::object> {
      auto &w = host.lua_world();
      const auto hit = w.raycast(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z),
                                 static_cast<float>(dx), static_cast<float>(dy), static_cast<float>(dz),
                                 static_cast<float>(maxDistance), ignore ? w.find(*ignore) : entt::null);
      if (!hit)
        return {sol::lua_nil, sol::lua_nil};
      return {sol::make_object(state, w.name_of(hit->entity)), sol::make_object(state, hit->distance)};
    });
    lua.set_function("distance", [this](const std::string &a, const std::string &b) -> std::optional<double> {
      auto &w = host.lua_world();
      if (!w.exists(a) || !w.exists(b))
        return std::nullopt;
      const double dx = *w.get(a, "x") - *w.get(b, "x");
      const double dy = *w.get(a, "y") - *w.get(b, "y");
      const double dz = *w.get(a, "z") - *w.get(b, "z");
      return std::sqrt(dx * dx + dy * dy + dz * dz);
    });
    lua.set_function("is3d", [this]() { return host.lua_world().mode() == scene::SceneMode::Mode3D; });
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

    lua.set_function("system", [this](const std::string &selector, sol::protected_function fn) {
      systems.push_back({loadingScript, selector, std::move(fn)});
    });

    auto swap = lua.create_named_table("swap");
    swap.set_function("add", [this](const std::string &name) { host.request_proc_add(name, 0); });
    swap.set_function("remove",
                      [this](const std::string &name) { host.request_proc_remove(name, 0); });
    swap.set_function("paste", [this](const std::string &source) { host.request_proc_paste(source); });
    swap.set_function("has", [this](const std::string &name) { return host.lua_has_proc(name); });

    lua.safe_script(SelfPrelude, "@pimpex_prelude");
  }
};

LuaRuntime::LuaRuntime(ILuaHost &host) : _impl(std::make_unique<Impl>(host)) {}
LuaRuntime::~LuaRuntime() = default;

std::optional<Diagnostic> LuaRuntime::load_script(const std::string &displayName,
                                                  std::string_view source) {
  // Systemy z tego pliku rejestrują się od nowa.
  std::erase_if(_impl->systems, [&](const Impl::System &s) { return s.script == displayName; });
  _impl->reported.clear();
  _impl->loadingScript = displayName;
  auto result = _impl->lua.safe_script(source, sol::script_pass_on_error, "@" + displayName);
  _impl->loadingScript.clear();
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

std::optional<Diagnostic> LuaRuntime::load_behavior(const std::string &name,
                                                    const std::string &displayName,
                                                    std::string_view source) {
  auto &lua = _impl->lua;
  auto loaded = lua.load(source, "@" + displayName);
  if (!loaded.valid()) {
    sol::error err = loaded;
    return lua_error_to_diagnostic(err.what(), displayName);
  }
  sol::protected_function chunk = loaded;
  auto result = chunk();
  if (!result.valid()) {
    sol::error err = result;
    return lua_error_to_diagnostic(err.what(), displayName);
  }
  sol::object value = result;
  if (value.get_type() != sol::type::table)
    return Diagnostic{Diagnostic::Severity::Error, displayName, 0,
                      "zachowanie musi kończyć się `return { on_start = ..., on_update = ... }`"};
  _impl->behaviors[name] = value.as<sol::table>();
  _impl->reported.clear();
  return std::nullopt;
}

bool LuaRuntime::has_behavior(const std::string &name) const { return _impl->behaviors.contains(name); }

void LuaRuntime::reset_instances() { _impl->instances.clear(); }

void LuaRuntime::update(scene::World &world) {
  auto &impl = *_impl;
  auto &registry = world.registry();

  // 1. Aktorzy zniszczeni albo bez <Behavior>: on_destroy i koniec.
  for (auto it = impl.instances.begin(); it != impl.instances.end();) {
    const auto e = it->first;
    const bool alive = registry.valid(e) && registry.all_of<scene::Behaviors>(e) &&
                       registry.get<scene::Name>(e).value == it->second.name;
    if (alive) {
      ++it;
      continue;
    }
    auto tracked = std::move(it->second);
    it = impl.instances.erase(it);
    for (const auto &instance : tracked.list)
      impl.invoke(instance, "on_destroy");
  }

  // 2. Nowi aktorzy (albo zmieniona lista zachowań po hot-reloadzie): on_start.
  std::vector<entt::entity> withBehaviors;
  for (auto [e, behaviors] : registry.view<scene::Behaviors>().each())
    withBehaviors.push_back(e);
  for (const auto e : withBehaviors) {
    if (!registry.valid(e))
      continue;
    const auto &refs = registry.get<scene::Behaviors>(e).list;
    std::vector<std::string> names;
    for (const auto &ref : refs)
      names.push_back(ref.name);
    auto found = impl.instances.find(e);
    if (found != impl.instances.end() && found->second.behaviorNames == names)
      continue;

    Impl::Tracked tracked{registry.get<scene::Name>(e).value, names, {}};
    for (const auto &ref : refs) {
      if (!impl.behaviors.contains(ref.name)) {
        impl.report(std::format("{}: nie ma zachowania '{}' (plik scripts/behaviors/{}.lua)",
                                tracked.name, ref.name, ref.name),
                    "scene.xml");
        continue;
      }
      tracked.list.push_back({ref.name, impl.new_self(tracked.name, ref.params)});
    }
    auto list = tracked.list;
    impl.instances[e] = std::move(tracked);
    for (const auto &instance : list) {
      if (!registry.valid(e))
        break;
      impl.invoke(instance, "on_start");
    }
  }

  // 3. on_update – na kopii, bo zachowania mogą niszczyć i tworzyć aktorów.
  std::vector<std::pair<entt::entity, Impl::Instance>> snapshot;
  for (const auto &[e, tracked] : impl.instances)
    for (const auto &instance : tracked.list)
      snapshot.emplace_back(e, instance);
  for (const auto &[e, instance] : snapshot)
    if (registry.valid(e))
      impl.invoke(instance, "on_update");

  // 4. Systemy: funkcja dla każdego aktora z komponentem / tagiem.
  const auto systems = impl.systems;
  for (const auto &system : systems)
    for (const auto &name : world.query(system.selector)) {
      if (!world.exists(name))
        continue;
      auto result = system.fn(impl.new_self(name, {}));
      if (!result.valid()) {
        sol::error err = result;
        impl.report(err.what(), system.script);
      }
    }
}

void LuaRuntime::dispatch_contacts(scene::World &world,
                                   const std::vector<std::pair<std::string, std::string>> &contacts) {
  auto &impl = *_impl;
  auto touch = [&](const std::string &who, const std::string &other) {
    const auto e = world.find(who);
    const auto it = impl.instances.find(e);
    if (e == entt::null || it == impl.instances.end())
      return;
    const auto list = it->second.list;
    for (const auto &instance : list)
      if (world.exists(who) && world.exists(other))
        impl.invoke(instance, "on_touch", other);
  };
  for (const auto &[a, b] : contacts) {
    touch(a, b);
    touch(b, a);
  }
}

LuaRuntime::EvalResult LuaRuntime::eval(std::string_view code) {
  auto &lua = _impl->lua;
  // Najpierw jako wyrażenie ("return ..."), żeby `get("Player", "x")` zwróciło wartość.
  auto loaded = lua.load("return " + std::string(code), "@lua.eval");
  if (!loaded.valid())
    loaded = lua.load(code, "@lua.eval");
  if (!loaded.valid()) {
    sol::error err = loaded;
    return {false, err.what()};
  }
  sol::protected_function chunk = loaded;
  auto result = chunk();
  if (!result.valid()) {
    sol::error err = result;
    return {false, err.what()};
  }
  std::string output;
  sol::protected_function tostring = lua["tostring"];
  for (int i = 0; i < result.return_count(); ++i) {
    sol::object value = result.get<sol::object>(i);
    if (i)
      output += '\t';
    if (value.get_type() == sol::type::number)
      output += format_number(value.as<double>());
    else
      output += tostring(value).get<std::string>();
  }
  return {true, output};
}

} // namespace PimpEx::script
