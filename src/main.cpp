#include "simple_ui.h"

#include "app_state.h"
#include "console_theme.h"
#include "settings_io.h"
#include "vsync.h"

#include "scenes/game.h"
#include "scenes/settings.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <chrono>
#include <memory>

namespace {

IScene* SceneById(std::array<ScenePtr, 2>& scenes, SceneId id) {
  switch (id) {
    case SceneId::Settings:
      return scenes[1].get();
    case SceneId::Game:
    default:
      return scenes[0].get();
  }
}

void SwitchScene(AppState& app, std::array<ScenePtr, 2>& scenes,
                 IScene*& current) {
  if (!app.scene_dirty) {
    return;
  }

  if (current) {
    current->on_leave();
    current = nullptr;
  }

  app.scene = app.pending_scene;
  current = SceneById(scenes, app.scene);
  if (current) {
    current->on_enter(app);
  }
  app.scene_dirty = false;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  ui_set_style_presets(MakeConsoleStylePresets());

  AppState app{};
  app.settings.screen_mode = ScreenMode::Borderless;
  app.settings.width = 1920;
  app.settings.height = 1080;
  app.settings.vsync = true;
  app.settings.brightness = 1.f;
  app.settings.msaa_samples = 4;
  LoadSettings(app.settings);
  app.draft = app.settings;

  ScreenSettings screen{};
  screen.screen_mode = app.settings.screen_mode;
  screen.screen_width = app.settings.width;
  screen.screen_height = app.settings.height;
  screen.msaa_samples = app.settings.msaa_samples;

  app.ctx = ui_create(L"Nuclear Fusion", &screen, L"icon.ico");
  if (!app.ctx || !ui_is_valid(app.ctx)) {
    MessageBoxW(nullptr, L"Failed to create the Nuclear Fusion window.",
                L"Nuclear Fusion", MB_OK | MB_ICONERROR);
    return 1;
  }

  // Sci-fi UI font (Orbitron, SIL OFL). Falls back to default if missing.
  if (const FontAtlas* font =
          ui_create_font(app.ctx, L"assets\\fonts\\Orbitron-Medium.ttf", 32.f)) {
    SetUiFont(font);
  }

  ui_set_brightness(app.ctx, app.settings.brightness);

  VSyncPacer vsync;
  vsync.set_enabled(app.settings.vsync);

  std::array<ScenePtr, 2> scenes = {
      CreateGameScene(),
      CreateSettingsScene(),
  };
  IScene* current = nullptr;
  SwitchScene(app, scenes, current);

  int last_w = ui_get_width(app.ctx);
  int last_h = ui_get_height(app.ctx);

  using clock = std::chrono::steady_clock;
  auto prev = clock::now();

  while (!app.request_quit && ui_process_messages(app.ctx)) {
    vsync.set_enabled(app.settings.vsync);
    vsync.begin_frame();

    SwitchScene(app, scenes, current);
    if (!current) {
      break;
    }

    const int w = ui_get_width(app.ctx);
    const int h = ui_get_height(app.ctx);
    if (w != last_w || h != last_h) {
      current->on_resize(app, w, h);
      last_w = w;
      last_h = h;
    }

    const auto now = clock::now();
    const float dt = std::chrono::duration<float>(now - prev).count();
    prev = now;

    current->handle_messages(app.ctx);
    current->update(app, dt);
    current->update_scene(dt);

    ui_clear(app.ctx, ConsoleClearColor());
    current->draw(app.ctx);
    current->handle_events();
    ui_present(app.ctx);

    vsync.end_frame(app.ctx);
  }

  if (current) {
    current->on_leave();
    current = nullptr;
  }

  for (auto& scene : scenes) {
    scene.reset();
  }

  ui_destroy(app.ctx);
  app.ctx = nullptr;
  return 0;
}
