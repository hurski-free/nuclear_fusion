#pragma once

#include "scene_base.h"
#include "../console_theme.h"
#include "../settings_io.h"

class SettingsScene : public IScene {
public:
  void on_enter(AppState& app) override {
    app_ = &app;
    app.draft = app.settings;
    SyncWidgetsFromDraft(app);
    Rebuild(app);
  }

  void on_leave() override {
    bg_scene_ = Scene{};
    ui_scene_ = Scene{};
    app_ = nullptr;
  }

  void on_resize(AppState&, int width, int height) override {
    Layout(width, height);
    bg_scene_.prepare_scene();
    ui_scene_.prepare_scene();
  }

  void update(AppState&, float) override {}

  Scene& scene() override { return ui_scene_; }

  void handle_messages(UiContext* ctx) override {
    ui_scene_.handle_messages(ctx);
  }

  void update_scene(float dt) override {
    bg_scene_.update(dt);
    ui_scene_.update(dt);
  }

  void draw(UiContext* ctx) override {
    bg_scene_.draw(ctx);
    ui_scene_.draw(ctx);
  }

  void handle_events() override { ui_scene_.handle_events(); }

private:
  void SyncWidgetsFromDraft(AppState& app) {
    mode_index_ = ScreenModeToIndex(app.draft.screen_mode);
    res_index_ = FindResolutionIndex(app, app.draft.width, app.draft.height);
    msaa_index_ = FindMsaaIndex(app, app.draft.msaa_samples);
    hud_refresh_index_ = FindHudRefreshIndex(app, app.draft.hud_refresh_sec);
    vsync_value_ = app.draft.vsync;
    brightness_value_ = app.draft.brightness * 100.f;
  }

  static int ScreenModeToIndex(ScreenMode mode) {
    switch (mode) {
      case ScreenMode::Windowed:
        return 0;
      case ScreenMode::Borderless:
        return 1;
      case ScreenMode::Fullscreen:
      default:
        return 2;
    }
  }

  static ScreenMode IndexToScreenMode(int index) {
    switch (index) {
      case 0:
        return ScreenMode::Windowed;
      case 1:
        return ScreenMode::Borderless;
      case 2:
      default:
        return ScreenMode::Fullscreen;
    }
  }

  void PullDraftFromWidgets(AppState& app) {
    app.draft.screen_mode = IndexToScreenMode(mode_index_);
    if (res_index_ >= 0 &&
        res_index_ < static_cast<int>(app.resolutions.size())) {
      app.draft.width = app.resolutions[res_index_].width;
      app.draft.height = app.resolutions[res_index_].height;
    }
    if (msaa_index_ >= 0 &&
        msaa_index_ < static_cast<int>(app.msaa_options.size())) {
      app.draft.msaa_samples = app.msaa_options[msaa_index_].samples;
    }
    if (hud_refresh_index_ >= 0 &&
        hud_refresh_index_ <
            static_cast<int>(app.hud_refresh_options.size())) {
      app.draft.hud_refresh_sec =
          app.hud_refresh_options[hud_refresh_index_].seconds;
    }
    app.draft.vsync = vsync_value_;
    app.draft.brightness = brightness_value_ / 100.f;
  }

  void ApplySettings(AppState& app) {
    PullDraftFromWidgets(app);
    app.settings = app.draft;

    ui_set_brightness(app.ctx, app.settings.brightness);
    ui_set_screen_size(app.ctx, app.settings.width, app.settings.height);
    ui_set_screen_mode(app.ctx, app.settings.screen_mode);
    ui_set_msaa_samples(app.ctx, app.settings.msaa_samples);
    SaveSettings(app.settings);

    RequestScene(app, SceneId::Game);
  }

  void Cancel(AppState& app) {
    app.draft = app.settings;
    RequestScene(app, SceneId::Game);
  }

  void Rebuild(AppState& app) {
    bg_scene_ = Scene{};
    ui_scene_ = Scene{};

    const int w = ui_get_width(app.ctx);
    const int h = ui_get_height(app.ctx);

    if (tex_id_ < 0) {
      tex_id_ = ui_load_texture(app.ctx, L"assets\\bg_settings.png");
    }

    MakeFullscreenBackground(bg_, tex_id_);

    panel_.title = L"Settings";
    panel_.draggable = false;
    panel_.closable = false;
    panel_.layer = 0;
    panel_.width = 560.f;
    panel_.height = 620.f;

    mode_label_.text = L"Window mode";
    mode_label_.font_size = 24.f;
    mode_label_.width = 240.f;
    mode_label_.height = 28.f;
    ApplyUiFont(mode_label_);

    mode_.options = {L"Windowed", L"Borderless", L"Fullscreen"};
    mode_.orientation = RadioOrientation::Vertical;
    mode_.mode = RadioMode::Circle;
    mode_.bind_data(&mode_index_);
    mode_.selected = mode_index_;
    ApplyUiFont(mode_);

    res_label_.text = L"Resolution";
    res_label_.font_size = 24.f;
    res_label_.width = 240.f;
    res_label_.height = 28.f;
    ApplyUiFont(res_label_);

    res_.options.clear();
    for (const auto& opt : app.resolutions) {
      res_.options.push_back(opt.label);
    }
    res_.bind_data(&res_index_);
    res_.selected = res_index_;
    ApplyUiFont(res_);

    msaa_label_.text = L"Antialiasing";
    msaa_label_.font_size = 24.f;
    msaa_label_.width = 240.f;
    msaa_label_.height = 28.f;
    ApplyUiFont(msaa_label_);

    msaa_.options.clear();
    for (const auto& opt : app.msaa_options) {
      msaa_.options.push_back(opt.label);
    }
    msaa_.bind_data(&msaa_index_);
    msaa_.selected = msaa_index_;
    ApplyUiFont(msaa_);

    hud_refresh_label_.text = L"HUD refresh";
    hud_refresh_label_.font_size = 24.f;
    hud_refresh_label_.width = 240.f;
    hud_refresh_label_.height = 28.f;
    ApplyUiFont(hud_refresh_label_);

    hud_refresh_.options.clear();
    for (const auto& opt : app.hud_refresh_options) {
      hud_refresh_.options.push_back(opt.label);
    }
    hud_refresh_.bind_data(&hud_refresh_index_);
    hud_refresh_.selected = hud_refresh_index_;
    ApplyUiFont(hud_refresh_);

    vsync_.label = L"Vertical sync";
    vsync_.bind_data(&vsync_value_);
    vsync_.checked = vsync_value_;
    ApplyUiFont(vsync_);

    brightness_.text = L"Brightness";
    brightness_.min_value = 20.f;
    brightness_.max_value = 150.f;
    brightness_.step = 1.f;
    brightness_.show_value = true;
    brightness_.bind_data(&brightness_value_);
    brightness_.value = brightness_value_;
    ApplyUiFont(brightness_);
    ApplyUiFont(panel_);

    ApplyConsoleButtonStyle(apply_, 160.f, 44.f);
    apply_.text = L"Apply";
    apply_.on_click = [this]() {
      if (app_) {
        ApplySettings(*app_);
      }
    };

    ApplyConsoleButtonStyle(cancel_, 160.f, 44.f);
    cancel_.text = L"Cancel";
    cancel_.on_click = [this]() {
      if (app_) {
        Cancel(*app_);
      }
    };

    panel_.components = {&mode_label_,       &mode_,
                         &res_label_,        &res_,
                         &msaa_label_,       &msaa_,
                         &hud_refresh_label_, &hud_refresh_,
                         &vsync_,            &brightness_,
                         &apply_,            &cancel_};

    Layout(w, h);

    bg_scene_.components = {&bg_};
    bg_scene_.prepare_scene();

    ui_scene_.components = {&panel_};
    ui_scene_.prepare_scene();
  }

  void Layout(int width, int height) {
    const float w = static_cast<float>(width > 0 ? width : 1280);
    const float h = static_cast<float>(height > 0 ? height : 720);

    bg_.x = 0.f;
    bg_.y = 0.f;
    bg_.width = w;
    bg_.height = h;

    panel_.x = (w - panel_.width) * 0.5f;
    panel_.y = (h - panel_.height) * 0.5f;

    float y = 16.f;
    const float left = 24.f;

    mode_label_.x = left;
    mode_label_.y = y;
    y += 34.f;

    mode_.x = left;
    mode_.y = y;
    y += 110.f;

    res_label_.x = left;
    res_label_.y = y;
    y += 34.f;

    res_.x = left;
    res_.y = y;
    y += 56.f;

    msaa_label_.x = left;
    msaa_label_.y = y;
    y += 34.f;

    msaa_.x = left;
    msaa_.y = y;
    y += 56.f;

    hud_refresh_label_.x = left;
    hud_refresh_label_.y = y;
    y += 34.f;

    hud_refresh_.x = left;
    hud_refresh_.y = y;
    y += 56.f;

    vsync_.x = left;
    vsync_.y = y;
    y += 48.f;

    brightness_.x = left;
    brightness_.y = y;

    const float btn_gap = 16.f;
    apply_.width = 160.f;
    cancel_.width = 160.f;
    apply_.x = left;
    apply_.y = panel_.height - panel_.title_height - 64.f;
    cancel_.x = left + apply_.width + btn_gap;
    cancel_.y = apply_.y;
  }

  AppState* app_ = nullptr;
  Scene bg_scene_{};
  Scene ui_scene_{};
  Image bg_{};
  Panel panel_{};
  Label mode_label_{};
  RadioGroup mode_{};
  Label res_label_{};
  Select res_{};
  Label msaa_label_{};
  Select msaa_{};
  Label hud_refresh_label_{};
  Select hud_refresh_{};
  Toggle vsync_{};
  Range brightness_{};
  Button apply_{};
  Button cancel_{};
  int tex_id_ = -1;

  int mode_index_ = 2;
  int res_index_ = 2;
  int msaa_index_ = 2;
  int hud_refresh_index_ = 1;
  bool vsync_value_ = true;
  float brightness_value_ = 100.f;
};

inline ScenePtr CreateSettingsScene() {
  return std::make_unique<SettingsScene>();
}
