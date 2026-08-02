#pragma once

#include "simple_ui.h"
#include "game/game_state.h"

#include <string>
#include <vector>

enum class SceneId {
  Game,
  Settings,
};

struct ResolutionOption {
  int width = 1280;
  int height = 720;
  std::wstring label;
};

struct AppSettings {
  ScreenMode screen_mode = ScreenMode::Borderless;
  int width = 1920;
  int height = 1080;
  bool vsync = true;
  float brightness = 1.f;
  // 1 = off, otherwise 2 / 4 / 8 (snapped by simple_ui).
  int msaa_samples = 4;
};

struct MsaaOption {
  int samples = 1;
  std::wstring label;
};

struct AppState {
  UiContext* ctx = nullptr;
  SceneId scene = SceneId::Game;
  SceneId pending_scene = SceneId::Game;
  bool request_quit = false;
  bool scene_dirty = true;

  AppSettings settings{};
  AppSettings draft{};
  GameState game{};
  bool game_initialized = false;

  std::vector<ResolutionOption> resolutions = {
      {1280, 720, L"1280 x 720"},
      {1600, 900, L"1600 x 900"},
      {1920, 1080, L"1920 x 1080"},
      {2560, 1440, L"2560 x 1440"},
  };

  std::vector<MsaaOption> msaa_options = {
      {1, L"Off"},
      {2, L"MSAA x2"},
      {4, L"MSAA x4"},
      {8, L"MSAA x8"},
  };
};

inline int FindResolutionIndex(const AppState& app, int width, int height) {
  for (int i = 0; i < static_cast<int>(app.resolutions.size()); ++i) {
    if (app.resolutions[i].width == width &&
        app.resolutions[i].height == height) {
      return i;
    }
  }
  return 0;
}

inline int FindMsaaIndex(const AppState& app, int samples) {
  for (int i = 0; i < static_cast<int>(app.msaa_options.size()); ++i) {
    if (app.msaa_options[i].samples == samples) {
      return i;
    }
  }
  // Default to x4 when the stored value is unsupported.
  for (int i = 0; i < static_cast<int>(app.msaa_options.size()); ++i) {
    if (app.msaa_options[i].samples == 4) {
      return i;
    }
  }
  return 0;
}

inline void RequestScene(AppState& app, SceneId id) {
  app.pending_scene = id;
  app.scene_dirty = true;
}
