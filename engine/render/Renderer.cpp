#include "render/Renderer.hpp"
#include "render/EmbeddedFont.hpp"
#include "render/LightingShader.hpp"

#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <map>
#include <set>
#include <vector>

namespace PimpEx::render {
namespace {

using namespace scene;

Color to_color(const Rgba &c) { return Color{c.r, c.g, c.b, c.a}; }

constexpr int MaxLights = 4;

} // namespace

struct Renderer::Impl {
  std::filesystem::path root;
  std::map<std::string, Texture2D> textures; // klucz: ścieżka pliku
  std::map<std::string, ::Model> models;     // klucz: ścieżka pliku
  std::set<std::string> failed;

  // 3D: jednostkowe bryły (rozmiar 1) skalowane do w × h × d, z normalnymi.
  bool ready3d = false;
  ::Model cube{}, sphere{}, cylinder{}, plane{};
  Shader lighting{};
  int ambientLoc = -1, viewPosLoc = -1;
  struct LightLocs {
    int enabled, type, position, target, color;
  };
  LightLocs lightLocs[MaxLights]{};
  bool cursorLocked = false;
  bool cursorFreed = true; // kursor wolny (start, ESC, inne okno); klik w okno go łapie
  bool fpsCamera = false;
  Font font{};

  // Czcionka z polskimi znakami. Rozmiar bazowy duży, żeby napisy były ostre.
  void load_font() {
    std::vector<int> codepoints;
    for (int c = 32; c < 127; ++c)
      codepoints.push_back(c);
    for (int c = 0xA0; c <= 0x17F; ++c) // Latin-1 + Latin Extended-A (ą, ć, ę, ł, ń, ś, ź, ż...)
      codepoints.push_back(c);
    for (int c : {0x2013, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x201E, 0x2022, 0x2026, 0x20AC})
      codepoints.push_back(c);
    const auto data = embedded_font();
    font = LoadFontFromMemory(".ttf", reinterpret_cast<const unsigned char *>(data.data()),
                              static_cast<int>(data.size()), 48, codepoints.data(),
                              static_cast<int>(codepoints.size()));
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
  }

  const Texture2D *texture(const SceneData &data, const std::string &id) {
    const auto res = data.textures.find(id);
    if (res == data.textures.end())
      return nullptr;
    return texture_file(res->second.path);
  }

  // Obrazek po ścieżce względem projektu (np. <Image src> w GUI).
  const Texture2D *texture_file(const std::string &path) {
    if (auto it = textures.find(path); it != textures.end())
      return &it->second;
    if (failed.contains(path))
      return nullptr;
    Texture2D tex = LoadTexture((root / path).string().c_str());
    if (tex.id == 0) {
      failed.insert(path); // raylib sam wypisze ostrzeżenie; rysujemy zastępczy kształt
      return nullptr;
    }
    return &textures.emplace(path, tex).first->second;
  }

  ::Model *model(const SceneData &data, const std::string &id) {
    const auto res = data.models.find(id);
    if (res == data.models.end())
      return nullptr;
    const auto &path = res->second.path;
    if (auto it = models.find(path); it != models.end())
      return &it->second;
    if (failed.contains(path))
      return nullptr;
    ::Model loaded = LoadModel((root / path).string().c_str());
    if (loaded.meshCount == 0) {
      failed.insert(path);
      return nullptr;
    }
    for (int i = 0; i < loaded.materialCount; ++i)
      loaded.materials[i].shader = lighting;
    return &models.emplace(path, loaded).first->second;
  }

  void init3d() {
    if (ready3d)
      return;
    lighting = LoadShaderFromMemory(LightingVertexShader, LightingFragmentShader);
    lighting.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(lighting, "viewPos");
    ambientLoc = GetShaderLocation(lighting, "ambient");
    viewPosLoc = lighting.locs[SHADER_LOC_VECTOR_VIEW];
    for (int i = 0; i < MaxLights; ++i) {
      auto loc = [&](const char *field) {
        return GetShaderLocation(lighting, std::format("lights[{}].{}", i, field).c_str());
      };
      lightLocs[i] = {loc("enabled"), loc("type"), loc("position"), loc("target"), loc("color")};
    }
    auto make = [&](Mesh mesh) {
      ::Model m = LoadModelFromMesh(mesh);
      m.materials[0].shader = lighting;
      return m;
    };
    cube = make(GenMeshCube(1, 1, 1));
    sphere = make(GenMeshSphere(0.5f, 24, 24));
    cylinder = make(GenMeshCylinder(0.5f, 1, 24));
    plane = make(GenMeshPlane(1, 1, 1, 1));
    ready3d = true;
  }

  ~Impl() {
    if (font.texture.id != 0)
      UnloadFont(font);
    if (ready3d) {
      UnloadModel(cube);
      UnloadModel(sphere);
      UnloadModel(cylinder);
      UnloadModel(plane);
      UnloadShader(lighting);
    }
  }

  // --- kamera -------------------------------------------------------------------------

  static entt::entity active_camera(const World &world) {
    for (auto [e, camera] : world.registry().view<Camera>().each())
      if (camera.active)
        return e;
    return entt::null;
  }

  static const Transform *target_of(const World &world, const Camera &camera) {
    const auto e = world.find(camera.target);
    return e == entt::null ? nullptr : &world.registry().get<Transform>(e);
  }

  Camera2D camera2d(const World &world, const vm::FrameContext &ctx) {
    Camera2D cam{};
    cam.zoom = 1;
    const auto e = active_camera(world);
    if (e == entt::null)
      return cam; // bez kamery: współrzędne świata = piksele ekranu
    const auto &camera = world.registry().get<Camera>(e);
    const auto &own = world.registry().get<Transform>(e);
    const auto *target = camera.mode == "follow" ? target_of(world, camera) : nullptr;
    cam.target = target ? Vector2{target->x, target->y} : Vector2{own.x, own.y};
    cam.offset = {ctx.screenWidth / 2.0f, ctx.screenHeight / 2.0f};
    cam.zoom = camera.zoom > 0 ? camera.zoom : 1;
    cam.rotation = own.rz;
    return cam;
  }

  Camera3D camera3d(const World &world, const vm::FrameContext &ctx, bool &fps) {
    Camera3D cam{};
    cam.up = {0, 1, 0};
    cam.fovy = 60;
    cam.projection = CAMERA_PERSPECTIVE;
    fps = false;
    const auto e = active_camera(world);
    if (e == entt::null) {
      // Bez kamery: powolny obrót wokół środka sceny.
      const float angle = static_cast<float>(ctx.frame) * 0.004f;
      cam.position = {std::sin(angle) * 12, 8, std::cos(angle) * 12};
      cam.target = {0, 0, 0};
      return cam;
    }
    const auto &camera = world.registry().get<Camera>(e);
    const auto &own = world.registry().get<Transform>(e);
    cam.fovy = camera.fov;
    const auto *target = target_of(world, camera);
    const Vector3 ownPos{own.x, own.y, own.z};
    const Vector3 targetPos = target ? Vector3{target->x, target->y, target->z} : Vector3{0, 0, 0};

    if (camera.mode == "follow" && target) {
      // Za plecami celu, obróconego o ry.
      const float yaw = target->ry * DEG2RAD;
      cam.position = {targetPos.x + std::sin(yaw) * camera.distance, targetPos.y + camera.height,
                      targetPos.z + std::cos(yaw) * camera.distance};
      cam.target = targetPos;
    } else if (camera.mode == "orbit") {
      const Vector3 center = target ? targetPos : ownPos;
      const float yaw = own.ry * DEG2RAD + static_cast<float>(ctx.frame) * 0.004f;
      cam.position = {center.x + std::sin(yaw) * camera.distance, center.y + camera.height,
                      center.z + std::cos(yaw) * camera.distance};
      cam.target = center;
    } else if (camera.mode == "fps") {
      // Oczy aktora-celu (albo samej kamery); kierunek z ry (obrót) i rx (patrzenie w górę/dół).
      const Transform &body = target ? *target : own;
      const float yaw = body.ry * DEG2RAD, pitch = own.rx * DEG2RAD;
      cam.position = {body.x, body.y + body.h * 0.4f, body.z};
      cam.target = {cam.position.x - std::sin(yaw) * std::cos(pitch), cam.position.y + std::sin(pitch),
                    cam.position.z - std::cos(yaw) * std::cos(pitch)};
      fps = true;
    } else {
      cam.position = ownPos;
      cam.target = target ? targetPos
                          : Vector3{own.x - std::sin(own.ry * DEG2RAD), own.y + std::sin(own.rx * DEG2RAD),
                                    own.z - std::cos(own.ry * DEG2RAD)};
    }
    return cam;
  }

  // --- światło -------------------------------------------------------------------------

  void update_lights(const World &world, const Camera3D &cam) {
    int index = 0;
    bool customAmbient = false;
    float ambient[4] = {0, 0, 0, 1};
    for (auto [e, light, t] : world.registry().view<Light, Transform>().each()) {
      if (light.kind == "ambient") {
        // Światła otoczenia się sumują i nie zajmują miejsca wśród 4 świateł.
        customAmbient = true;
        ambient[0] += light.color.r / 255.f * light.intensity;
        ambient[1] += light.color.g / 255.f * light.intensity;
        ambient[2] += light.color.b / 255.f * light.intensity;
        continue;
      }
      if (index >= MaxLights)
        continue;
      const int enabled = 1;
      const int type = light.kind == "directional" ? 0 : 1;
      const float position[3] = {t.x, t.y, t.z};
      const float target[3] = {0, 0, 0};
      const float color[4] = {light.color.r / 255.f * light.intensity, light.color.g / 255.f * light.intensity,
                              light.color.b / 255.f * light.intensity, 1};
      SetShaderValue(lighting, lightLocs[index].enabled, &enabled, SHADER_UNIFORM_INT);
      SetShaderValue(lighting, lightLocs[index].type, &type, SHADER_UNIFORM_INT);
      SetShaderValue(lighting, lightLocs[index].position, position, SHADER_UNIFORM_VEC3);
      SetShaderValue(lighting, lightLocs[index].target, target, SHADER_UNIFORM_VEC3);
      SetShaderValue(lighting, lightLocs[index].color, color, SHADER_UNIFORM_VEC4);
      ++index;
    }
    if (index == 0) {
      // Brak świateł w scenie: domyślne „słońce”, żeby nic nie było czarne.
      const int enabled = 1, type = 0;
      const float position[3] = {4, 10, 6}, target[3] = {0, 0, 0}, color[4] = {0.8f, 0.8f, 0.8f, 1};
      SetShaderValue(lighting, lightLocs[0].enabled, &enabled, SHADER_UNIFORM_INT);
      SetShaderValue(lighting, lightLocs[0].type, &type, SHADER_UNIFORM_INT);
      SetShaderValue(lighting, lightLocs[0].position, position, SHADER_UNIFORM_VEC3);
      SetShaderValue(lighting, lightLocs[0].target, target, SHADER_UNIFORM_VEC3);
      SetShaderValue(lighting, lightLocs[0].color, color, SHADER_UNIFORM_VEC4);
      index = 1;
    }
    for (int i = index; i < MaxLights; ++i) {
      const int disabled = 0;
      SetShaderValue(lighting, lightLocs[i].enabled, &disabled, SHADER_UNIFORM_INT);
    }
    if (!customAmbient) {
      ambient[0] = ambient[1] = 0.35f;
      ambient[2] = 0.38f;
    }
    SetShaderValue(lighting, ambientLoc, ambient, SHADER_UNIFORM_VEC4);
    const float view[3] = {cam.position.x, cam.position.y, cam.position.z};
    SetShaderValue(lighting, viewPosLoc, view, SHADER_UNIFORM_VEC3);
  }

  // --- kursor ----------------------------------------------------------------------------

  // Kursor złapany przez okno (DisableCursor) to pod X11/WSLg też brak dostępu
  // do paska tytułu: krzyżyka i przesuwania okna. Dlatego gra startuje z wolnym
  // kursorem, a łapie go dopiero kliknięcie w okno przy kamerze fps (jak
  // „kliknij, aby grać” w przeglądarce). ESC, inne okno i GUI z przyciskami
  // go uwalniają.
  void update_cursor(bool fps, const gui::GuiSystem &gui) {
    fpsCamera = fps;
    const bool focused = IsWindowFocused();
    if (!focused || (cursorLocked && IsKeyPressed(KEY_ESCAPE)))
      cursorFreed = true;
    else if (cursorFreed && fps && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
      cursorFreed = false;
    const bool lock = fps && focused && !cursorFreed && !gui.wants_mouse();
    if (lock != cursorLocked) {
      lock ? DisableCursor() : EnableCursor();
      cursorLocked = lock;
    }
  }

  // --- GUI ---------------------------------------------------------------------------------

  void draw_gui_text(const gui::Element &e, const std::string &value, const gui::Rect &r, Color color) {
    const auto size = static_cast<float>(e.size);
    const auto measured = MeasureTextEx(font, value.c_str(), size, size / 16);
    float x = r.x + (r.w - measured.x) / 2;
    if (e.align == gui::Align::Left)
      x = r.x;
    else if (e.align == gui::Align::Right)
      x = r.x + r.w - measured.x;
    const float y = r.y + (r.h - measured.y) / 2;
    DrawTextEx(font, value.c_str(), {std::round(x), std::round(y)}, size, size / 16, color);
  }

  void draw_gui(const gui::GuiSystem &gui, const World &world, const vm::FrameContext &ctx) {
    for (const auto &name : gui.shown()) {
      const auto *scene = gui.find(name);
      if (!scene)
        continue;
      if (scene->dim)
        DrawRectangle(0, 0, ctx.screenWidth, ctx.screenHeight, to_color(*scene->dim));
      for (std::size_t i = 0; i < scene->elements.size(); ++i) {
        const auto &e = scene->elements[i];
        auto r = gui::layout(e, ctx.screenWidth, ctx.screenHeight);
        const Rectangle rect{r.x, r.y, r.w, r.h};
        switch (e.kind) {
        case gui::ElementKind::Panel:
          DrawRectangleRec(rect, to_color(e.color));
          if (e.border)
            DrawRectangleLinesEx(rect, 2, to_color(*e.border));
          break;
        case gui::ElementKind::Label:
          draw_gui_text(e, vm::format_text(world, ctx, e.text, ""), r, to_color(e.color));
          break;
        case gui::ElementKind::Button: {
          Color background = to_color(gui.is_hovered(name, i) ? e.hoverColor : e.color);
          if (gui.is_pressed(name, i))
            background = ColorBrightness(background, -0.25f);
          DrawRectangleRounded(rect, 0.25f, 8, background);
          if (e.border)
            DrawRectangleRoundedLinesEx(rect, 0.25f, 8, 2, to_color(*e.border));
          draw_gui_text(e, vm::format_text(world, ctx, e.text, ""), r, to_color(e.textColor));
          break;
        }
        case gui::ElementKind::Image:
          if (const auto *tex = texture_file(e.image)) {
            const Rectangle source{0, 0, static_cast<float>(tex->width), static_cast<float>(tex->height)};
            if (e.w <= 0 || e.h <= 0) { // bez w/h: rozmiar obrazka
              r.w = source.width;
              r.h = source.height;
              r.x -= r.w / 2;
              r.y -= r.h / 2;
            }
            DrawTexturePro(*tex, source, {r.x, r.y, r.w, r.h}, {0, 0}, 0, to_color(e.color));
          } else {
            DrawRectangleRec(rect, MAGENTA); // brak obrazka
          }
          break;
        }
      }
    }
  }

  // --- rysowanie --------------------------------------------------------------------------

  void draw_text(const World &world, const vm::FrameContext &ctx, entt::entity e, const std::string &name,
                 Vector2 topLeft) {
    const auto &text = world.registry().get<Text>(e);
    const auto value = vm::format_text(world, ctx, text.value, name);
    const auto size = static_cast<float>(text.size);
    DrawTextEx(font, value.c_str(), {std::round(topLeft.x), std::round(topLeft.y)}, size, size / 16,
               to_color(text.color));
  }

  void draw_2d(const World &world, const SceneData &data, entt::entity e, const Transform &t) {
    const auto &registry = world.registry();
    const Rectangle rect{t.x, t.y, t.w, t.h};
    const Vector2 origin{t.w / 2, t.h / 2};
    if (const auto *sprite = registry.try_get<Sprite>(e)) {
      if (const auto *tex = texture(data, sprite->texture)) {
        const Rectangle source{0, 0, static_cast<float>(tex->width), static_cast<float>(tex->height)};
        DrawTexturePro(*tex, source, rect, origin, t.rz, to_color(sprite->tint));
      } else {
        DrawRectanglePro(rect, origin, t.rz, MAGENTA); // brak obrazka
      }
    }
    if (const auto *r = registry.try_get<Rect>(e))
      DrawRectanglePro(rect, origin, t.rz, to_color(r->color));
    if (const auto *c = registry.try_get<Circle>(e))
      DrawCircleV({t.x, t.y}, t.w / 2, to_color(c->color));
  }

  void draw_model(::Model &m, Vector3 position, Vector3 scale, const Transform &t, Color tint, bool wire) {
    m.transform = MatrixRotateXYZ({t.rx * DEG2RAD, t.ry * DEG2RAD, t.rz * DEG2RAD});
    if (wire)
      DrawModelWiresEx(m, position, {0, 1, 0}, 0, scale, tint);
    else
      DrawModelEx(m, position, {0, 1, 0}, 0, scale, tint);
  }

  void draw_3d(const World &world, const SceneData &data, entt::entity e, const Transform &t) {
    const auto &registry = world.registry();
    const Vector3 pos{t.x, t.y, t.z};
    if (const auto *c = registry.try_get<Cube>(e))
      draw_model(cube, pos, {t.w, t.h, t.d}, t, to_color(c->color), c->wire);
    if (const auto *s = registry.try_get<Sphere>(e))
      draw_model(sphere, pos, {t.w, t.w, t.w}, t, to_color(s->color), s->wire);
    if (const auto *c = registry.try_get<Cylinder>(e))
      // GenMeshCylinder zaczyna się na y=0 – przesuwamy, żeby środek był w (x, y, z).
      draw_model(cylinder, {t.x, t.y - t.h / 2, t.z}, {t.w, t.h, t.w}, t, to_color(c->color), c->wire);
    if (const auto *p = registry.try_get<Plane>(e))
      draw_model(plane, pos, {t.w, 1, t.d}, t, to_color(p->color), false);
    if (const auto *m = registry.try_get<scene::Model>(e)) {
      if (auto *loaded = model(data, m->model))
        draw_model(*loaded, pos, {t.scale, t.scale, t.scale}, t, to_color(m->tint), false);
      else
        DrawCubeWires(pos, t.w, t.h, t.d, MAGENTA); // brak modelu
    }
  }
};

Renderer::Renderer(std::filesystem::path projectRoot) : _impl(std::make_unique<Impl>()) {
  _impl->root = std::move(projectRoot);
  _impl->load_font();
}

Renderer::~Renderer() { unload_textures(); }

void Renderer::unload_textures() {
  for (auto &[path, tex] : _impl->textures)
    UnloadTexture(tex);
  for (auto &[path, model] : _impl->models)
    UnloadModel(model);
  _impl->textures.clear();
  _impl->models.clear();
  _impl->failed.clear();
}

bool Renderer::mouse_look_suspended() const { return _impl->fpsCamera && !_impl->cursorLocked; }

void Renderer::draw(const World &world, const SceneData &data, const gui::GuiSystem &gui,
                    const ProjectConfig &config, const vm::FrameContext &ctx) {
  const auto &registry = world.registry();
  const bool is3d = world.mode() == SceneMode::Mode3D;

  // Kolejność: layer rosnąco, potem nazwa (stabilnie).
  struct Item {
    int layer;
    const std::string *name;
    entt::entity entity;
  };
  std::vector<Item> worldItems, screenItems;
  for (auto [e, name, t] : registry.view<Name, Transform>().each())
    if (t.visible)
      (t.space == "screen" ? screenItems : worldItems).push_back({t.layer, &name.value, e});
  auto byLayer = [](const Item &a, const Item &b) {
    return a.layer != b.layer ? a.layer < b.layer : *a.name < *b.name;
  };
  std::sort(worldItems.begin(), worldItems.end(), byLayer);
  std::sort(screenItems.begin(), screenItems.end(), byLayer);

  BeginDrawing();
  const auto background = world.background().value_or(config.background);
  ClearBackground(to_color(background));

  bool fps = false;
  if (is3d) {
    _impl->init3d();
    const auto cam = _impl->camera3d(world, ctx, fps);
    _impl->update_lights(world, cam);
    BeginMode3D(cam);
    for (const auto &item : worldItems)
      _impl->draw_3d(world, data, item.entity, registry.get<Transform>(item.entity));
    EndMode3D();
    // Napisy przypięte do aktorów 3D: rysowane w miejscu, gdzie aktor jest na ekranie.
    for (const auto &item : worldItems)
      if (registry.all_of<Text>(item.entity)) {
        const auto &t = registry.get<Transform>(item.entity);
        const auto screen = GetWorldToScreen({t.x, t.y + t.h / 2, t.z}, cam);
        _impl->draw_text(world, ctx, item.entity, *item.name, screen);
      }
  } else {
    BeginMode2D(_impl->camera2d(world, ctx));
    for (const auto &item : worldItems) {
      const auto &t = registry.get<Transform>(item.entity);
      _impl->draw_2d(world, data, item.entity, t);
      if (registry.all_of<Text>(item.entity))
        _impl->draw_text(world, ctx, item.entity, *item.name, {t.x - t.w / 2, t.y - t.h / 2});
    }
    EndMode2D();
  }

  // HUD: space="screen" – pozycja w pikselach ekranu, niezależnie od kamery.
  for (const auto &item : screenItems) {
    const auto &t = registry.get<Transform>(item.entity);
    _impl->draw_2d(world, data, item.entity, t);
    if (registry.all_of<Text>(item.entity))
      _impl->draw_text(world, ctx, item.entity, *item.name, {t.x - t.w / 2, t.y - t.h / 2});
  }
  _impl->draw_gui(gui, world, ctx);
  if (fps && !_impl->cursorLocked && !gui.wants_mouse()) {
    const char *hint = "Kliknij w okno, aby sterować myszą (ESC uwalnia kursor)";
    const auto size = MeasureTextEx(_impl->font, hint, 22, 22.f / 16);
    const Vector2 at{std::round((ctx.screenWidth - size.x) / 2), 16};
    DrawRectangle(static_cast<int>(at.x) - 10, 10, static_cast<int>(size.x) + 20, static_cast<int>(size.y) + 12,
                  Color{0, 0, 0, 170});
    DrawTextEx(_impl->font, hint, at, 22, 22.f / 16, RAYWHITE);
  }
  EndDrawing();
  // Także w 2D: po przejściu ze sceny fps (np. ekran „przegrana”) kursor wraca.
  _impl->update_cursor(fps, gui);
}

} // namespace PimpEx::render
