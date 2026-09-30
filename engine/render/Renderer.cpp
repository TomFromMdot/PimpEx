#include "render/Renderer.hpp"

#include <raylib.h>

#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace PimpEx::render {
namespace {

Color to_color(const scene::Rgba &c) { return Color{c.r, c.g, c.b, c.a}; }

} // namespace

struct Renderer::Impl {
  std::filesystem::path root;
  std::map<std::string, Texture2D> textures; // klucz: ścieżka pliku
  std::set<std::string> failed;

  const Texture2D *texture(const scene::SceneData &data, const std::string &id) {
    const auto res = data.textures.find(id);
    if (res == data.textures.end())
      return nullptr;
    const auto &path = res->second.path;
    if (auto it = textures.find(path); it != textures.end())
      return &it->second;
    if (failed.contains(path))
      return nullptr;
    const auto full = (root / path).string();
    Texture2D tex = LoadTexture(full.c_str());
    if (tex.id == 0) {
      failed.insert(path); // raylib sam wypisze ostrzeżenie; rysujemy zastępczy prostokąt
      return nullptr;
    }
    return &textures.emplace(path, tex).first->second;
  }
};

Renderer::Renderer(std::filesystem::path projectRoot) : _impl(std::make_unique<Impl>()) {
  _impl->root = std::move(projectRoot);
}

Renderer::~Renderer() { unload_textures(); }

void Renderer::unload_textures() {
  for (auto &[path, tex] : _impl->textures)
    UnloadTexture(tex);
  _impl->textures.clear();
  _impl->failed.clear();
}

void Renderer::draw(const scene::World &world, const scene::SceneData &data,
                    const scene::ProjectConfig &config, const vm::FrameContext &ctx) {
  const auto &registry = world.registry();

  // Kolejność rysowania: z rosnąco, a przy równym z – po nazwie (stabilnie).
  struct Item {
    int z;
    const std::string *name;
    entt::entity entity;
  };
  std::vector<Item> items;
  for (auto [e, name, t] : registry.view<scene::Name, scene::Transform>().each())
    if (t.visible)
      items.push_back({t.z, &name.value, e});
  std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) {
    return a.z != b.z ? a.z < b.z : *a.name < *b.name;
  });

  BeginDrawing();
  ClearBackground(to_color(world.background().value_or(config.background)));

  for (const auto &item : items) {
    const auto &t = registry.get<scene::Transform>(item.entity);
    const Rectangle rect{t.x, t.y, t.w, t.h};
    const Vector2 origin{t.w / 2, t.h / 2};

    if (const auto *sprite = registry.try_get<scene::Sprite>(item.entity)) {
      if (const auto *tex = _impl->texture(data, sprite->texture)) {
        const Rectangle source{0, 0, static_cast<float>(tex->width),
                               static_cast<float>(tex->height)};
        DrawTexturePro(*tex, source, rect, origin, t.rotation, to_color(sprite->tint));
      } else {
        DrawRectanglePro(rect, origin, t.rotation, MAGENTA); // brak obrazka
      }
    } else if (const auto *shape = registry.try_get<scene::Shape>(item.entity)) {
      if (shape->kind == scene::Shape::Kind::Circle)
        DrawCircleV({t.x, t.y}, t.w / 2, to_color(shape->color));
      else
        DrawRectanglePro(rect, origin, t.rotation, to_color(shape->color));
    }

    if (const auto *label = registry.try_get<scene::Label>(item.entity)) {
      const auto text = vm::format_text(world, ctx, label->text, *item.name);
      // Napis zaczyna się w lewym górnym rogu aktora (x, y dla aktora bez rozmiaru).
      DrawText(text.c_str(), static_cast<int>(t.x - t.w / 2), static_cast<int>(t.y - t.h / 2),
               label->size, to_color(label->color));
    }
  }
  EndDrawing();
}

} // namespace PimpEx::render
