#pragma once

#include "simple_ui.h"
#include "game/game_state.h"

#include <cmath>
#include <limits>
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
  // How often HUD / Lab / Tech numeric labels refresh (seconds).
  float hud_refresh_sec = 0.5f;
};

struct MsaaOption {
  int samples = 1;
  std::wstring label;
};

struct HudRefreshOption {
  float seconds = 0.5f;
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

  // Logical render resolutions (independent of OS window size).
  // Covers common 16:9 / 16:10 / ultrawide / 4:3 panels up to 8K.
  std::vector<ResolutionOption> resolutions = {
      {1024, 768, L"1024 x 768 (4:3)"},
      {1280, 720, L"1280 x 720 (16:9)"},
      {1280, 800, L"1280 x 800 (16:10)"},
      {1280, 1024, L"1280 x 1024 (5:4)"},
      {1360, 768, L"1360 x 768 (16:9)"},
      {1366, 768, L"1366 x 768 (16:9)"},
      {1440, 900, L"1440 x 900 (16:10)"},
      {1536, 864, L"1536 x 864 (16:9)"},
      {1600, 900, L"1600 x 900 (16:9)"},
      {1600, 1200, L"1600 x 1200 (4:3)"},
      {1680, 1050, L"1680 x 1050 (16:10)"},
      {1920, 1080, L"1920 x 1080 (16:9)"},
      {1920, 1200, L"1920 x 1200 (16:10)"},
      {2048, 1152, L"2048 x 1152 (16:9)"},
      {2560, 1080, L"2560 x 1080 (21:9)"},
      {2560, 1440, L"2560 x 1440 (16:9)"},
      {2560, 1600, L"2560 x 1600 (16:10)"},
      {2880, 1800, L"2880 x 1800 (16:10)"},
      {3440, 1440, L"3440 x 1440 (21:9)"},
      {3840, 1600, L"3840 x 1600 (21:9)"},
      {3840, 2160, L"3840 x 2160 (16:9)"},
      {5120, 1440, L"5120 x 1440 (32:9)"},
      {5120, 2160, L"5120 x 2160 (21:9)"},
      {7680, 4320, L"7680 x 4320 (16:9)"},
  };

  std::vector<MsaaOption> msaa_options = {
      {1, L"Off"},
      {2, L"MSAA x2"},
      {4, L"MSAA x4"},
      {8, L"MSAA x8"},
  };

  std::vector<HudRefreshOption> hud_refresh_options = {
      {0.25f, L"0.25 s"},
      {0.5f, L"0.5 s"},
      {1.0f, L"1 s"},
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

// Pick the listed resolution closest to the OS window, preferring options that
// fit inside the window (typical first-launch / "match display" behavior).
inline int FindResolutionIndexForWindow(const AppState& app, int window_w,
                                        int window_h) {
  if (app.resolutions.empty() || window_w <= 0 || window_h <= 0) {
    return 0;
  }

  int best = 0;
  long long best_score = (std::numeric_limits<long long>::max)();
  for (int i = 0; i < static_cast<int>(app.resolutions.size()); ++i) {
    const auto& r = app.resolutions[i];
    const long long dw = static_cast<long long>(r.width) - window_w;
    const long long dh = static_cast<long long>(r.height) - window_h;
    long long score = dw * dw + dh * dh;
    if (r.width <= window_w && r.height <= window_h) {
      score -= (1LL << 40);
    }
    if (score < best_score) {
      best_score = score;
      best = i;
    }
  }
  return best;
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

inline int FindHudRefreshIndex(const AppState& app, float seconds) {
  int best = 1;  // default 0.5 s
  float best_d = 1.0e9f;
  for (int i = 0; i < static_cast<int>(app.hud_refresh_options.size()); ++i) {
    const float d = std::fabs(app.hud_refresh_options[i].seconds - seconds);
    if (d < best_d) {
      best_d = d;
      best = i;
    }
  }
  return best;
}

inline void RequestScene(AppState& app, SceneId id) {
  app.pending_scene = id;
  app.scene_dirty = true;
}
