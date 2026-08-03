#pragma once

#include "app_state.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cmath>
#include <cstdio>
#include <string>

inline std::wstring SettingsFilePath() {
  wchar_t module_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, module_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return L"settings.cfg";
  }

  std::wstring path(module_path, len);
  const size_t slash = path.find_last_of(L"\\/");
  if (slash == std::wstring::npos) {
    return L"settings.cfg";
  }
  return path.substr(0, slash + 1) + L"settings.cfg";
}

inline const char* ScreenModeToToken(ScreenMode mode) {
  switch (mode) {
    case ScreenMode::Windowed:
      return "windowed";
    case ScreenMode::Borderless:
      return "borderless";
    case ScreenMode::Fullscreen:
    default:
      return "fullscreen";
  }
}

inline bool TokenToScreenMode(const std::string& token, ScreenMode& out) {
  if (token == "windowed" || token == "0") {
    out = ScreenMode::Windowed;
    return true;
  }
  if (token == "borderless" || token == "1") {
    out = ScreenMode::Borderless;
    return true;
  }
  if (token == "fullscreen" || token == "2") {
    out = ScreenMode::Fullscreen;
    return true;
  }
  return false;
}

inline int SnapMsaaSamples(int samples) {
  if (samples <= 1) {
    return 1;
  }
  if (samples <= 2) {
    return 2;
  }
  if (samples <= 4) {
    return 4;
  }
  return 8;
}

inline void ClampSettings(AppSettings& settings) {
  if (settings.width < 640) {
    settings.width = 640;
  }
  if (settings.height < 480) {
    settings.height = 480;
  }
  if (settings.brightness < 0.2f) {
    settings.brightness = 0.2f;
  }
  if (settings.brightness > 1.5f) {
    settings.brightness = 1.5f;
  }
  settings.msaa_samples = SnapMsaaSamples(settings.msaa_samples);

  // Snap HUD refresh to one of the supported intervals.
  const float opts[] = {0.25f, 0.5f, 1.0f};
  float best = 0.5f;
  float best_d = 1.0e9f;
  for (float o : opts) {
    const float d = std::fabs(settings.hud_refresh_sec - o);
    if (d < best_d) {
      best_d = d;
      best = o;
    }
  }
  settings.hud_refresh_sec = best;
}

inline bool SaveSettings(const AppSettings& settings) {
  const std::wstring path = SettingsFilePath();
  FILE* file = _wfopen(path.c_str(), L"w");
  if (!file) {
    return false;
  }

  std::fprintf(file, "screen_mode=%s\n", ScreenModeToToken(settings.screen_mode));
  std::fprintf(file, "width=%d\n", settings.width);
  std::fprintf(file, "height=%d\n", settings.height);
  std::fprintf(file, "vsync=%d\n", settings.vsync ? 1 : 0);
  std::fprintf(file, "brightness=%g\n", settings.brightness);
  std::fprintf(file, "msaa=%d\n", settings.msaa_samples);
  std::fprintf(file, "hud_refresh=%g\n", settings.hud_refresh_sec);
  std::fclose(file);
  return true;
}

inline bool LoadSettings(AppSettings& settings) {
  const std::wstring path = SettingsFilePath();
  FILE* file = _wfopen(path.c_str(), L"r");
  if (!file) {
    return false;
  }

  AppSettings loaded = settings;
  char line[256] = {};
  while (std::fgets(line, static_cast<int>(sizeof(line)), file)) {
    std::string text(line);
    while (!text.empty() &&
           (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
      text.pop_back();
    }
    if (text.empty() || text[0] == '#' || text[0] == ';') {
      continue;
    }

    const size_t eq = text.find('=');
    if (eq == std::string::npos) {
      continue;
    }

    const std::string key = text.substr(0, eq);
    const std::string value = text.substr(eq + 1);

    try {
      if (key == "screen_mode") {
        ScreenMode mode = loaded.screen_mode;
        if (TokenToScreenMode(value, mode)) {
          loaded.screen_mode = mode;
        }
      } else if (key == "width") {
        loaded.width = std::stoi(value);
      } else if (key == "height") {
        loaded.height = std::stoi(value);
      } else if (key == "vsync") {
        loaded.vsync = (value == "1" || value == "true" || value == "yes");
      } else if (key == "brightness") {
        loaded.brightness = std::stof(value);
      } else if (key == "msaa" || key == "msaa_samples") {
        loaded.msaa_samples = std::stoi(value);
      } else if (key == "hud_refresh" || key == "hud_refresh_sec") {
        loaded.hud_refresh_sec = std::stof(value);
      }
    } catch (...) {
      // Ignore malformed values and keep defaults/previous fields.
    }
  }

  std::fclose(file);
  ClampSettings(loaded);
  settings = loaded;
  return true;
}
