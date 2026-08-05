#include "game.h"

#include "../game/star_canvas_renderer.h"
#include "../console_theme.h"
#include "../game/game_assets.h"
#include "../game/game_format.h"
#include "../game/game_save.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace {

class GameScene : public IScene {
public:
  void on_enter(AppState& app) override {
    app_ = &app;
    bool show_howto = false;
    if (!app.game_initialized) {
      app.game.InitNewGame();
      // First launch with no save file: show the tutorial modal.
      show_howto = !save_io::SaveExists();
      LoadGame(app.game);
      app.game_initialized = true;
    }
    assets_.Load(app.ctx);
    Rebuild(app);
    if (show_howto) {
      OpenHowToPlay();
    }
  }

  void on_leave() override {
    if (app_) {
      SaveGame(app_->game);
    }
    canvas_scene_ = Scene{};
    ui_scene_ = Scene{};
    app_ = nullptr;
  }

  void on_resize(AppState&, int width, int height) override {
    Layout(width, height);
    if (app_) {
      canvas_scene_.prepare_scene();
      ui_scene_.prepare_scene();
    }
    canvas_.ensure_targets(app_ ? app_->ctx : nullptr);
    star_renderer_.Resize(width, height);
  }

  void update(AppState& app, float dt) override {
    app.game.Tick(dt);
    UpdateStarAnimation(dt);
    UpdateFloatTexts(dt);
    UpdateSupernovaConfirm(dt);
    star_renderer_.Update(dt);

    SyncBatchesFromUi();
    SyncAutoBuyFromUi();
    UpdateAutoBuy(dt);

    hud_refresh_accum_ += dt;
    const float interval = std::max(0.05f, app.settings.hud_refresh_sec);
    if (hud_force_refresh_ || hud_refresh_accum_ >= interval) {
      hud_refresh_accum_ = 0.f;
      hud_force_refresh_ = false;
      RefreshLabels();
    }
    RefreshDynamicStyles();
    AdvanceStockTipTimer(dt);
  }

  Scene& scene() override { return ui_scene_; }

  void handle_messages(UiContext* ctx) override {
    HandleStarClick(ctx);
    ui_scene_.handle_messages(ctx);
    // After input routing so Hovered is current, then resolve tooltips.
    UpdateHoverTooltips();
  }

  void update_scene(float dt) override {
    canvas_scene_.update(dt);
    ui_scene_.update(dt);
  }

  void draw(UiContext* ctx) override {
    RenderCanvas(ctx);
    canvas_scene_.draw(ctx);
    ui_scene_.draw(ctx);
  }

  void handle_events() override { ui_scene_.handle_events(); }

private:
  static constexpr int kMaxElementRows = 12;
  // Room for all Tech upgrades (particles + 12 elements×3 + specials).
  static constexpr int kMaxUpgradeRows = 48;
  static constexpr int kMaxFloatTexts = 10;

  GameState& G() { return app_->game; }
  const GameState& G() const { return app_->game; }

  void StyleHudLabel(Label& label, float size = 20.f) {
    label.font_size = size;
    label.color = ColorPhosphor();
    label.layer = 0;
    label.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
    label.style_hovered = label.style_base;
    label.style_active = label.style_base;
    label.style_disabled = label.style_base;
    ApplyUiFont(label);
  }

  void StylePanelScroll(ScrollView& scroll) {
    scroll.scroll_y.mode = ScrollMode::Auto;
    scroll.layer = 1;
    scroll.style_base = {
        Color{0.012f, 0.04f, 0.022f, 0.92f},
        Border{2.f, BorderMode::In, Color{0.28f, 0.78f, 0.38f, 1.f}}};
    scroll.style_hovered = scroll.style_base;
    scroll.style_active = scroll.style_base;
  }

  void RenderCanvas(UiContext* ctx) {
    if (!ctx || !app_) {
      return;
    }
    ID3D11Device* device = ui_get_device(ctx);
    ID3D11DeviceContext* d3d = ui_get_device_context(ctx);
    if (!device || !d3d) {
      return;
    }

    if (!star_renderer_.EnsureReady(device)) {
      return;
    }

    canvas_.width = static_cast<float>(ui_get_width(ctx));
    canvas_.height = static_cast<float>(ui_get_height(ctx));
    canvas_.x = 0.f;
    canvas_.y = 0.f;
    canvas_.layer = 0;
    canvas_.clear_color = {0.01f, 0.02f, 0.015f, 1.f};
    canvas_.ensure_targets(ctx);
    star_renderer_.Resize(ui_get_width(ctx), ui_get_height(ctx));

    if (!canvas_.begin_draw(ctx, true)) {
      return;
    }

    const float size = star_base_size_ * star_scale_;
    star_renderer_.SetStarType(G().star_type);
    star_renderer_.SetOrbitCounts(G().protons, G().neutrons, G().electrons);
    star_renderer_.Draw(d3d, star_center_x_, star_center_y_, size, true);
    canvas_.end_draw(ctx);
  }

  void StyleBatchRadio(RadioGroup& rg) {
    // Orbitron is wide: keep batch options at 16px and give each slot >=72px.
    rg.options = {L"x1", L"x10", L"x100", L"Max"};
    rg.orientation = RadioOrientation::Horizontal;
    rg.mode = RadioMode::Circle;
    rg.font_size = 16.f;
    rg.layer = 1;
    rg.radio_size = 12.f;
    rg.item_height = 26.f;
    rg.item_width = 80.f;
    rg.gap = 6.f;
    rg.label_color = {0.55f, 1.f, 0.65f, 1.f};
    rg.box_border_color = {0.25f, 0.7f, 0.35f, 1.f};
    rg.box_background_color = {0.04f, 0.12f, 0.06f, 0.9f};
    rg.box_background_hovered = {0.08f, 0.22f, 0.12f, 0.95f};
    rg.check_color = {0.55f, 1.f, 0.65f, 1.f};
    rg.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
    rg.style_hovered = rg.style_base;
    rg.style_active = rg.style_base;
    rg.style_disabled = rg.style_base;
    ApplyUiFont(rg);
  }

  static CraftBatch ClampBatchIndex(int index) {
    if (index < 0 || index > static_cast<int>(CraftBatch::Max)) {
      return CraftBatch::x1;
    }
    return static_cast<CraftBatch>(index);
  }

  void SyncBatchesFromUi() {
    const CraftBatch prev_craft = G().craft_batch;
    const CraftBatch prev_buy = G().buy_batch;
    G().craft_batch = ClampBatchIndex(craft_batch_index_);
    G().buy_batch = ClampBatchIndex(buy_batch_index_);
    if (G().craft_batch != prev_craft || G().buy_batch != prev_buy) {
      RequestHudRefresh();
    }
  }

  void SyncBatchesToUi() {
    craft_batch_index_ = static_cast<int>(G().craft_batch);
    buy_batch_index_ = static_cast<int>(G().buy_batch);
    lab_batch_.selected = craft_batch_index_;
    buy_batch_.selected = buy_batch_index_;
  }

  void StyleAutoBuyToggle(Toggle& tog) {
    tog.label.clear();
    tog.width = 48.f;
    tog.height = 26.f;
    tog.gap = 0.f;
    tog.layer = 1;
    tog.transition_duration = 0.16f;
    tog.label_color = ColorPhosphor();
    tog.track_off = {0.08f, 0.16f, 0.1f, 1.f};
    tog.track_on = {0.14f, 0.55f, 0.24f, 1.f};
    tog.thumb_color = {0.65f, 1.f, 0.7f, 1.f};
    ApplyUiFont(tog);
  }

  void SyncAutoBuyToUi() {
    auto_tog_p_ = G().auto_buy == AutoBuyMode::Proton;
    auto_tog_n_ = G().auto_buy == AutoBuyMode::Neutron;
    auto_tog_e_ = G().auto_buy == AutoBuyMode::Electron;
    auto_tog_pn_ = G().auto_buy == AutoBuyMode::ProtonNeutron;
    auto_tog_all_ = G().auto_buy == AutoBuyMode::All;
    auto_p_.checked = auto_tog_p_;
    auto_n_.checked = auto_tog_n_;
    auto_e_.checked = auto_tog_e_;
    auto_pn_.checked = auto_tog_pn_;
    auto_all_.checked = auto_tog_all_;
  }

  void SyncAutoBuyFromUi() {
    struct Slot {
      bool* flag;
      Toggle* tog;
      AutoBuyMode mode;
    };
    const Slot slots[] = {
        {&auto_tog_p_, &auto_p_, AutoBuyMode::Proton},
        {&auto_tog_n_, &auto_n_, AutoBuyMode::Neutron},
        {&auto_tog_e_, &auto_e_, AutoBuyMode::Electron},
        {&auto_tog_pn_, &auto_pn_, AutoBuyMode::ProtonNeutron},
        {&auto_tog_all_, &auto_all_, AutoBuyMode::All},
    };

    AutoBuyMode selected = AutoBuyMode::None;
    int checked = 0;
    for (const auto& s : slots) {
      // Prefer widget state (source of truth after click).
      *s.flag = s.tog->checked;
      if (*s.flag) {
        selected = s.mode;
        ++checked;
      }
    }

    if (checked > 1) {
      // Keep the newly enabled toggle; turn the previous mode off.
      AutoBuyMode newly = AutoBuyMode::None;
      for (const auto& s : slots) {
        if (*s.flag && s.mode != G().auto_buy) {
          newly = s.mode;
          break;
        }
      }
      if (newly == AutoBuyMode::None) {
        newly = selected;
      }
      selected = newly;
      for (const auto& s : slots) {
        *s.flag = (s.mode == selected);
        s.tog->checked = *s.flag;
      }
    }

    if (G().auto_buy != selected) {
      G().auto_buy = selected;
      auto_buy_accum_ = 0.f;
      SaveGame(G());
    }
  }

  void UpdateAutoBuy(float dt) {
    if (G().auto_buy == AutoBuyMode::None) {
      auto_buy_accum_ = 0.f;
      return;
    }
    auto_buy_accum_ += dt;
    if (auto_buy_accum_ < 1.f) {
      return;
    }
    auto_buy_accum_ = 0.f;
    if (G().TickAutoBuy()) {
      RequestHudRefresh();
    }
  }

  void StyleTechTabButton(Button& btn, const wchar_t* label) {
    ApplyConsoleButtonStyle(btn, 120.f, 32.f);
    btn.text = label;
    btn.font_size = 18.f;
    btn.layer = 1;
  }

  void RefreshTechTabButtonStyles() {
    auto style_selected = [](Button& btn) {
      btn.style_base = {
          Color{0.10f, 0.42f, 0.18f, 1.f},
          Border{2.f, BorderMode::In, Color{0.65f, 1.f, 0.7f, 1.f}}};
      btn.style_hovered = {
          Color{0.14f, 0.52f, 0.24f, 1.f},
          Border{2.f, BorderMode::In, Color{0.8f, 1.f, 0.85f, 1.f}}};
      btn.style_active = btn.style_hovered;
      btn.text_color = {0.75f, 1.f, 0.8f, 1.f};
    };
    auto style_idle = [](Button& btn) {
      btn.style_base = {
          Color{0.02f, 0.12f, 0.05f, 0.85f},
          Border{2.f, BorderMode::In, Color{0.22f, 0.55f, 0.3f, 0.9f}}};
      btn.style_hovered = {
          Color{0.06f, 0.28f, 0.12f, 0.95f},
          Border{2.f, BorderMode::In, Color{0.4f, 0.85f, 0.5f, 1.f}}};
      btn.style_active = {
          Color{0.10f, 0.38f, 0.16f, 1.f},
          Border{2.f, BorderMode::In, Color{0.55f, 1.f, 0.65f, 1.f}}};
      btn.text_color = {0.4f, 0.75f, 0.48f, 1.f};
    };

    if (tech_tab_index_ == 0) {
      style_selected(tech_tab_btn_);
      style_idle(dtech_tab_btn_);
    } else {
      style_idle(tech_tab_btn_);
      style_selected(dtech_tab_btn_);
    }
  }

  void SelectTechTab(int tab) {
    const int next = (tab == 1 && G().DTechUnlocked()) ? 1 : 0;
    if (next == tech_tab_index_) {
      RefreshTechTabButtonStyles();
      return;
    }
    tech_tab_index_ = next;
    RefreshTechTabButtonStyles();
    RefreshUpgradeRows();
    Relayout();
  }

  void SyncTechTabVisibility() {
    if (!app_) {
      return;
    }
    const bool unlocked = G().DTechUnlocked();
    if (unlocked) {
      Hide(tech_header_);
      tech_tab_btn_.disabled = false;
      dtech_tab_btn_.disabled = false;
      RefreshTechTabButtonStyles();
    } else {
      tech_tab_index_ = 0;
      Hide(tech_tab_btn_);
      Hide(dtech_tab_btn_);
      tech_header_.disabled = false;
      tech_header_.text = L"Tech";
    }
  }

  void Rebuild(AppState& app) {
    canvas_scene_ = Scene{};
    ui_scene_ = Scene{};
    element_rows_.assign(kMaxElementRows, {});
    upgrade_rows_.clear();
    upgrade_rows_.resize(kMaxUpgradeRows);
    float_texts_.assign(kMaxFloatTexts, {});

    const int w = ui_get_width(app.ctx);
    const int h = ui_get_height(app.ctx);
    canvas_.width = static_cast<float>(w);
    canvas_.height = static_cast<float>(h);
    canvas_.x = 0.f;
    canvas_.y = 0.f;
    canvas_.layer = 0;
    canvas_.clear_color = {0.01f, 0.02f, 0.015f, 1.f};
    canvas_.tint = {1.f, 1.f, 1.f, 1.f};
    canvas_.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
    canvas_.style_hovered = canvas_.style_base;
    canvas_.style_active = canvas_.style_base;
    canvas_.ensure_targets(app.ctx);
    star_renderer_.EnsureReady(ui_get_device(app.ctx));
    star_renderer_.Resize(w, h);

    title_.text = L"Nuclear Fusion";
    StyleHudLabel(title_, 40.f);
    title_.width = 480.f;
    title_.height = 48.f;

    StyleHudLabel(lab_header_, 32.f);
    lab_header_.text = L"Lab";
    lab_header_.width = 100.f;
    lab_header_.height = 36.f;

    StyleHudLabel(tech_header_, 32.f);
    tech_header_.text = L"Tech";
    tech_header_.width = 120.f;
    tech_header_.height = 36.f;

    StyleTechTabButton(tech_tab_btn_, L"Tech");
    StyleTechTabButton(dtech_tab_btn_, L"D Tech");
    tech_tab_btn_.on_click = [this]() { SelectTechTab(0); };
    dtech_tab_btn_.on_click = [this]() { SelectTechTab(1); };
    tech_tab_index_ = 0;
    RefreshTechTabButtonStyles();

    StyleHudLabel(lab_batch_label_, 16.f);
    lab_batch_label_.text = L"Batch";
    lab_batch_label_.width = 62.f;
    lab_batch_label_.height = 24.f;

    StyleHudLabel(buy_batch_label_, 16.f);
    buy_batch_label_.text = L"Batch";
    buy_batch_label_.width = 62.f;
    buy_batch_label_.height = 24.f;

    StyleBatchRadio(lab_batch_);
    StyleBatchRadio(buy_batch_);
    SyncBatchesToUi();
    lab_batch_.bind_data(&craft_batch_index_);
    buy_batch_.bind_data(&buy_batch_index_);

    StyleCardPanel(hud_panel_);
    hud_panel_.width = 360.f;
    hud_panel_.height = 148.f;
    hud_panel_.style_base = {
        Color{0.018f, 0.06f, 0.035f, 0.94f},
        Border{2.5f, BorderMode::In, Color{0.35f, 0.92f, 0.45f, 1.f}}};
    hud_panel_.style_hovered = hud_panel_.style_base;
    hud_panel_.style_active = hud_panel_.style_base;

    game_ui::ClearImageStyle(energy_icon_);
    energy_icon_.texture_id = assets_.energy;
    energy_icon_.width = energy_icon_.height = 36.f;

    game_ui::ClearImageStyle(hud_icon_p_);
    game_ui::ClearImageStyle(hud_icon_n_);
    game_ui::ClearImageStyle(hud_icon_e_);
    hud_icon_p_.texture_id = assets_.proton;
    hud_icon_n_.texture_id = assets_.neutron;
    hud_icon_e_.texture_id = assets_.electron;
    hud_icon_p_.width = hud_icon_p_.height = 24.f;
    hud_icon_n_.width = hud_icon_n_.height = 24.f;
    hud_icon_e_.width = hud_icon_e_.height = 24.f;
    hud_icon_p_.tint = {1.f, 1.f, 1.f, 1.f};
    hud_icon_n_.tint = {1.f, 1.f, 1.f, 1.f};
    hud_icon_e_.tint = {1.f, 1.f, 1.f, 1.f};

    StyleHudLabel(energy_label_, 32.f);
    energy_label_.width = 320.f;
    energy_label_.height = 38.f;
    StyleHudLabel(eps_label_, 24.f);
    eps_label_.width = 340.f;
    eps_label_.height = 30.f;
    StyleHudLabel(dust_label_, 22.f);
    dust_label_.width = 160.f;
    dust_label_.height = 28.f;
    dust_label_.layer = 1;
    dust_hit_.width = 160.f;
    dust_hit_.height = 28.f;
    dust_hit_.layer = 2;
    dust_hit_.text.clear();
    dust_hit_.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
    dust_hit_.style_hovered = dust_hit_.style_base;
    dust_hit_.style_active = dust_hit_.style_base;
    dust_hit_.style_disabled = dust_hit_.style_base;
    dust_hit_.transition = {};
    StyleHudLabel(crit_label_, 22.f);
    crit_label_.width = 340.f;
    crit_label_.height = 28.f;
    StyleHudLabel(count_p_, 24.f);
    count_p_.color = ColorProton();
    count_p_.width = 120.f;
    count_p_.height = 30.f;
    StyleHudLabel(count_n_, 24.f);
    count_n_.color = ColorNeutron();
    count_n_.width = 120.f;
    count_n_.height = 30.f;
    StyleHudLabel(count_e_, 24.f);
    count_e_.color = ColorElectron();
    count_e_.width = 120.f;
    count_e_.height = 30.f;
    StyleHudLabel(star_label_, 24.f);
    star_label_.width = 300.f;
    star_label_.height = 30.f;

    ApplyConsoleButtonStyle(settings_btn_, 160.f, 48.f);
    settings_btn_.text = L"Settings";
    settings_btn_.font_size = 22.f;
    settings_btn_.layer = 1;
    settings_btn_.on_click = [this]() {
      if (app_) {
        SaveGame(app_->game);
        RequestScene(*app_, SceneId::Settings);
      }
    };

    ApplyConsoleButtonStyle(howto_btn_, 180.f, 48.f);
    howto_btn_.text = L"How to play";
    howto_btn_.font_size = 22.f;
    howto_btn_.layer = 1;
    howto_btn_.on_click = [this]() { OpenHowToPlay(); };

    ApplyConsoleButtonStyle(reset_btn_, 140.f, 48.f);
    reset_btn_.text = L"Reset";
    reset_btn_.font_size = 22.f;
    reset_btn_.layer = 1;
    reset_btn_.text_color = Color{1.f, 0.55f, 0.35f, 1.f};
    reset_btn_.on_click = [this]() { OpenResetConfirm(); };

    ApplyConsoleButtonStyle(exit_btn_, 140.f, 48.f);
    exit_btn_.text = L"Exit";
    exit_btn_.font_size = 22.f;
    exit_btn_.layer = 1;
    exit_btn_.on_click = [this]() {
      if (app_) {
        SaveGame(app_->game);
        app_->request_quit = true;
      }
    };

    SetupHowToModal();
    SetupResetModal();
    SetupPrestigeModal();

    star_base_size_ = 160.f;
    star_scale_ = 1.f;
    star_anim_t_ = 0.f;

    StyleHudLabel(star_hint_, 24.f);
    star_hint_.text = L"Click the star to gain energy";
    star_hint_.width = 380.f;
    star_hint_.height = 30.f;

    SetupBuyControl(icon_p_, buy_p_, assets_.proton, ResourceKind::Proton);
    SetupBuyControl(icon_n_, buy_n_, assets_.neutron, ResourceKind::Neutron);
    SetupBuyControl(icon_e_, buy_e_, assets_.electron, ResourceKind::Electron);

    auto setup_combo_icon = [](Image& img, int tex) {
      game_ui::ClearImageStyle(img);
      img.texture_id = tex;
      img.width = img.height = 28.f;
      img.layer = 1;
      img.tint = {1.f, 1.f, 1.f, 1.f};
    };
    setup_combo_icon(icon_pn_p_, assets_.proton);
    setup_combo_icon(icon_pn_n_, assets_.neutron);
    setup_combo_icon(icon_all_p_, assets_.proton);
    setup_combo_icon(icon_all_n_, assets_.neutron);
    setup_combo_icon(icon_all_e_, assets_.electron);

    ApplyConsoleButtonStyle(buy_pn_, 220.f, 48.f);
    buy_pn_.font_size = 20.f;
    buy_pn_.layer = 1;
    buy_pn_.on_click = [this]() { BuyPair(); };

    ApplyConsoleButtonStyle(buy_all_, 220.f, 48.f);
    buy_all_.font_size = 20.f;
    buy_all_.layer = 1;
    buy_all_.on_click = [this]() { BuyTrio(); };

    StyleAutoBuyToggle(auto_p_);
    StyleAutoBuyToggle(auto_n_);
    StyleAutoBuyToggle(auto_e_);
    StyleAutoBuyToggle(auto_pn_);
    StyleAutoBuyToggle(auto_all_);
    auto_p_.bind_data(&auto_tog_p_);
    auto_n_.bind_data(&auto_tog_n_);
    auto_e_.bind_data(&auto_tog_e_);
    auto_pn_.bind_data(&auto_tog_pn_);
    auto_all_.bind_data(&auto_tog_all_);
    SyncAutoBuyToUi();

    game_ui::ClearImageStyle(icon_supernova_);
    icon_supernova_.texture_id = assets_.supernova;
    icon_supernova_.width = icon_supernova_.height = 48.f;
    icon_supernova_.layer = 1;

    ApplyConsoleButtonStyle(supernova_, 180.f, 48.f);
    supernova_.text = L"Supernova";
    supernova_.font_size = 24.f;
    supernova_.layer = 1;
    supernova_.on_click = [this]() { OnSupernovaClick(); };

    // Auto-click / click-mult indicator: top-right of the central block.
    game_ui::ClearImageStyle(icon_autoclick_);
    icon_autoclick_.texture_id = assets_.autoclick;
    icon_autoclick_.width = icon_autoclick_.height = 48.f;
    icon_autoclick_.layer = 1;
    StyleHudLabel(autoclick_label_, 18.f);
    autoclick_label_.width = 72.f;
    autoclick_label_.height = 24.f;
    autoclick_label_.layer = 1;
    autoclick_label_.color = {0.65f, 1.f, 0.72f, 1.f};
    autoclick_hit_.width = 56.f;
    autoclick_hit_.height = 72.f;
    autoclick_hit_.layer = 2;
    autoclick_hit_.text.clear();
    autoclick_hit_.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
    autoclick_hit_.style_hovered = autoclick_hit_.style_base;
    autoclick_hit_.style_active = autoclick_hit_.style_base;
    autoclick_hit_.style_disabled = autoclick_hit_.style_base;
    autoclick_hit_.transition = {};
    Hide(icon_autoclick_);
    Hide(autoclick_label_);
    Hide(autoclick_hit_);

    ApplyConsoleButtonStyle(sn_help_btn_, 36.f, 36.f);
    sn_help_btn_.text = L"?";
    sn_help_btn_.font_size = 22.f;
    sn_help_btn_.layer = 2;
    sn_help_btn_.on_click = nullptr;

    for (auto& ft : float_texts_) {
      StyleHudLabel(ft.label, 26.f);
      ft.label.layer = 3;
      ft.label.width = 160.f;
      ft.label.height = 28.f;
      Hide(ft.label);
    }

    StylePanelScroll(lab_scroll_);
    StylePanelScroll(tech_scroll_);

    for (int i = 0; i < kMaxElementRows; ++i) {
      auto& row = element_rows_[i];
      row.element_index = i;
      StyleCardPanel(row.card);
      row.card.layer = 0;

      game_ui::ClearImageStyle(row.icon);
      row.icon.width = row.icon.height = 40.f;
      row.icon.layer = 1;

      auto setup_stock_icon = [](Image& img, int tex) {
        game_ui::ClearImageStyle(img);
        img.width = img.height = 64.f;
        img.layer = 1;
        img.texture_id = tex;
      };
      setup_stock_icon(row.nuc_icon, assets_.nucleus);
      setup_stock_icon(row.atom_icon, assets_.atom);
      setup_stock_icon(row.iso_icon, assets_.isotope);

      auto setup_stock_hit = [](Button& hit) {
        hit.width = hit.height = 64.f;
        hit.layer = 2;
        hit.text.clear();
        hit.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
        hit.style_hovered = hit.style_base;
        hit.style_active = hit.style_base;
        hit.style_disabled = hit.style_base;
        hit.transition = {};
      };
      setup_stock_hit(row.nuc_hit);
      setup_stock_hit(row.atom_hit);
      setup_stock_hit(row.iso_hit);

      row.name.font_size = 28.f;
      row.name.color = {0.65f, 1.f, 0.72f, 1.f};
      row.name.width = 220.f;
      row.name.height = 34.f;
      row.name.layer = 1;
      ApplyUiFont(row.name);

      auto setup_stock_count = [this](Label& label) {
        label.font_size = 16.f;
        label.color = {0.55f, 0.95f, 0.65f, 1.f};
        label.width = 72.f;
        label.height = 64.f;
        label.layer = 1;
        ApplyUiFont(label);
      };
      setup_stock_count(row.nuc_count);
      setup_stock_count(row.atom_count);
      setup_stock_count(row.iso_count);

      row.stats.font_size = 20.f;
      row.stats.color = {0.55f, 0.95f, 0.65f, 1.f};
      row.stats.width = 320.f;
      row.stats.height = 28.f;
      row.stats.layer = 1;
      ApplyUiFont(row.stats);

      SetupCostFormula(row.cost_nucleus, assets_.nucleus, assets_.proton,
                       assets_.neutron);
      SetupCostFormula(row.cost_atom, assets_.atom, assets_.nucleus,
                       assets_.electron);

      ApplyConsoleButtonStyle(row.craft_nucleus, 150.f, 44.f);
      row.craft_nucleus.text = L"Nucleus x1";
      row.craft_nucleus.font_size = 18.f;
      row.craft_nucleus.layer = 1;
      row.craft_nucleus.on_click = [this, i]() {
        if (i < static_cast<int>(G().elements.size())) {
          if (G().CraftNucleus(G().elements[i], 0)) {
            RequestHudRefresh();
          }
        }
      };

      ApplyConsoleButtonStyle(row.craft_atom, 150.f, 44.f);
      row.craft_atom.text = L"Atom x1";
      row.craft_atom.font_size = 18.f;
      row.craft_atom.layer = 1;
      row.craft_atom.on_click = [this, i]() {
        if (i < static_cast<int>(G().elements.size())) {
          if (G().CraftAtom(G().elements[i], 0)) {
            RequestHudRefresh();
          }
        }
      };

      lab_scroll_.components.push_back(&row.card);
      lab_scroll_.components.push_back(&row.icon);
      lab_scroll_.components.push_back(&row.nuc_icon);
      lab_scroll_.components.push_back(&row.atom_icon);
      lab_scroll_.components.push_back(&row.iso_icon);
      lab_scroll_.components.push_back(&row.nuc_hit);
      lab_scroll_.components.push_back(&row.atom_hit);
      lab_scroll_.components.push_back(&row.iso_hit);
      lab_scroll_.components.push_back(&row.name);
      lab_scroll_.components.push_back(&row.nuc_count);
      lab_scroll_.components.push_back(&row.atom_count);
      lab_scroll_.components.push_back(&row.iso_count);
      lab_scroll_.components.push_back(&row.stats);
      PushCostFormula(lab_scroll_, row.cost_nucleus);
      PushCostFormula(lab_scroll_, row.cost_atom);
      lab_scroll_.components.push_back(&row.craft_nucleus);
      lab_scroll_.components.push_back(&row.craft_atom);
    }

    for (int i = 0; i < kMaxUpgradeRows; ++i) {
      auto& row = upgrade_rows_[i];
      row.upgrade_index = i;
      StyleCardPanel(row.card);
      row.card.layer = 0;
      game_ui::ClearImageStyle(row.icon);
      row.icon.width = row.icon.height = 48.f;
      row.icon.layer = 1;
      // Transparent hit target so hover works on the upgrade icon.
      row.icon_hit.width = row.icon_hit.height = 48.f;
      row.icon_hit.layer = 2;
      row.icon_hit.text.clear();
      row.icon_hit.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
      row.icon_hit.style_hovered = row.icon_hit.style_base;
      row.icon_hit.style_active = row.icon_hit.style_base;
      row.icon_hit.style_disabled = row.icon_hit.style_base;
      row.icon_hit.transition = {};
      row.title.font_size = 24.f;
      row.title.color = {0.65f, 1.f, 0.72f, 1.f};
      row.title.width = 280.f;
      row.title.height = 34.f;
      row.title.layer = 1;
      ApplyUiFont(row.title);
      row.desc.font_size = 18.f;
      row.desc.color = {0.55f, 0.95f, 0.65f, 1.f};
      row.desc.width = 280.f;
      row.desc.height = 110.f;
      row.desc.layer = 1;
      ApplyUiFont(row.desc);
      row.cost_label.font_size = 18.f;
      row.cost_label.color = {0.55f, 0.95f, 0.65f, 1.f};
      row.cost_label.width = 80.f;
      row.cost_label.height = 24.f;
      row.cost_label.layer = 1;
      row.cost_label.text = L"Cost:";
      ApplyUiFont(row.cost_label);
      for (int c = 0; c < game_ui::UpgradeRow::kMaxCosts; ++c) {
        auto& line = row.costs[c];
        game_ui::ClearImageStyle(line.icon);
        line.icon.width = line.icon.height = 36.f;
        line.icon.layer = 1;
        line.amount.font_size = 18.f;
        line.amount.color = {0.55f, 0.95f, 0.65f, 1.f};
        line.amount.width = 120.f;
        line.amount.height = 36.f;
        line.amount.layer = 1;
        ApplyUiFont(line.amount);
      }
      ApplyConsoleButtonStyle(row.buy, 160.f, 44.f);
      row.buy.text = L"Buy";
      row.buy.font_size = 22.f;
      row.buy.layer = 1;
      row.buy.on_click = [this, i]() {
        if (i < static_cast<int>(upgrade_rows_.size())) {
          const int up_i = upgrade_rows_[i].upgrade_index;
          if (up_i >= 0 && up_i < static_cast<int>(G().upgrades.size())) {
            if (G().BuyUpgrade(G().upgrades[up_i])) {
              SaveGame(G());
              RequestHudRefresh();
              RefreshLabels();
              Relayout();
            }
          }
        }
      };
      game_ui::ClearImageStyle(row.grade_icon);
      row.grade_icon.texture_id = assets_.grade;
      row.grade_icon.width = row.grade_icon.height = 32.f;
      row.grade_icon.layer = 2;
      row.grade_icon.tint = {1.f, 1.f, 1.f, 1.f};
      StyleHudLabel(row.grade_label, 16.f);
      row.grade_label.width = 72.f;
      row.grade_label.height = 22.f;
      row.grade_label.layer = 2;
      row.grade_label.color = {1.f, 0.92f, 0.4f, 1.f};
      tech_scroll_.components.push_back(&row.card);
      tech_scroll_.components.push_back(&row.icon);
      tech_scroll_.components.push_back(&row.icon_hit);
      tech_scroll_.components.push_back(&row.title);
      tech_scroll_.components.push_back(&row.desc);
      tech_scroll_.components.push_back(&row.cost_label);
      for (int c = 0; c < game_ui::UpgradeRow::kMaxCosts; ++c) {
        tech_scroll_.components.push_back(&row.costs[c].icon);
        tech_scroll_.components.push_back(&row.costs[c].amount);
      }
      tech_scroll_.components.push_back(&row.buy);
      tech_scroll_.components.push_back(&row.grade_icon);
      tech_scroll_.components.push_back(&row.grade_label);
    }

    tech_tip_panel_.width = 280.f;
    tech_tip_panel_.height = 56.f;
    tech_tip_panel_.layer = 90;
    tech_tip_panel_.style_base = {
        Color{0.02f, 0.08f, 0.04f, 0.96f},
        Border{1.5f, BorderMode::In, Color{0.35f, 0.9f, 0.45f, 1.f}}};
    tech_tip_panel_.style_hovered = tech_tip_panel_.style_base;
    tech_tip_panel_.style_active = tech_tip_panel_.style_base;
    StyleHowToText(tech_tip_label_, 18.f);
    tech_tip_label_.layer = 91;
    tech_tip_label_.width = 260.f;
    tech_tip_label_.height = 44.f;
    Hide(tech_tip_panel_);
    Hide(tech_tip_label_);

    RefreshElementRows();
    RefreshUpgradeRows();
    RefreshLabels();
    Layout(w, h);
    supernova_visible_ = AscendAvailable();
    unlocked_count_ = CountUnlockedElements();
    isotope_discovered_count_ = CountDiscoveredIsotopes();

    canvas_scene_.components = {&canvas_};
    canvas_scene_.prepare_scene();

    ui_scene_.components = {
        &title_,           &lab_header_,      &tech_header_,
        &tech_tab_btn_,    &dtech_tab_btn_,    &lab_batch_label_, &lab_batch_,
        &buy_batch_label_,
        &buy_batch_,       &hud_panel_,       &energy_icon_,
        &energy_label_,    &star_label_,      &crit_label_,
        &eps_label_,       &dust_label_,      &dust_hit_,
        &hud_icon_p_,      &count_p_,         &hud_icon_n_,
        &count_n_,         &hud_icon_e_,      &count_e_,
        &settings_btn_,    &howto_btn_,       &reset_btn_,      &exit_btn_,
        &star_hint_,       &icon_p_,          &buy_p_,          &auto_p_,
        &icon_n_,          &buy_n_,           &auto_n_,
        &icon_e_,          &buy_e_,           &auto_e_,
        &icon_pn_p_,       &icon_pn_n_,       &buy_pn_,         &auto_pn_,
        &icon_all_p_,      &icon_all_n_,      &icon_all_e_,     &buy_all_,
        &auto_all_,        &icon_supernova_,  &supernova_,
        &icon_autoclick_,  &autoclick_label_, &autoclick_hit_,  &sn_help_btn_,
        &lab_scroll_,      &tech_scroll_,     &howto_modal_,    &reset_modal_,
        &prestige_modal_,  &tech_tip_panel_,  &tech_tip_label_,
    };
    for (auto& ft : float_texts_) {
      ui_scene_.components.push_back(&ft.label);
    }
    ui_scene_.prepare_scene();
  }

  void StyleHowToText(Text& text, float size = 18.f) {
    text.font_size = size;
    text.color = ColorPhosphor();
    text.layer = 1;
    text.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
    text.style_hovered = text.style_base;
    text.style_active = text.style_base;
    text.style_disabled = text.style_base;
    ApplyUiFont(text);
  }

  void OpenHowToPlay() {
    howto_modal_.open = true;
  }

  void OpenResetConfirm() {
    reset_modal_.open = true;
  }

  void ConfirmResetProgress() {
    if (!app_) {
      return;
    }
    G().InitNewGame();
    save_io::DeleteSave();
    SaveGame(G());

    supernova_armed_ = false;
    supernova_arm_t_ = 0.f;
    supernova_.text = AscendButtonLabel();
    star_hint_dismissed_ = false;
    unlocked_count_ = CountUnlockedElements();
    isotope_discovered_count_ = CountDiscoveredIsotopes();
    hud_force_refresh_ = true;
    hud_refresh_accum_ = 0.f;

    reset_modal_.open = false;
    RequestHudRefresh();
    RefreshLabels();
    Relayout();
  }

  // Wrapped multi-line height for How-to Text blocks (explicit \\n + wrap).
  float MeasureHowToTextHeight(const std::wstring& text, float width,
                               float font_size) const {
    if (!app_ || !app_->ctx || width <= 1.f) {
      return font_size * 4.f;
    }
    const float line_h =
        std::max(font_size + 4.f,
                 ui_font_line_height(app_->ctx, font_size, UiFont()));
    int lines = 0;
    size_t i = 0;
    const size_t n = text.size();
    while (i < n) {
      // One paragraph until '\n'.
      size_t para_end = text.find(L'\n', i);
      if (para_end == std::wstring::npos) {
        para_end = n;
      }
      if (para_end == i) {
        ++lines;  // blank line
        i = para_end + 1;
        continue;
      }
      size_t p = i;
      while (p < para_end) {
        // Greedy wrap: take as many chars as fit in width.
        size_t fit = para_end - p;
        while (fit > 1 &&
               ui_measure_text_n(app_->ctx, text.c_str() + p, fit, font_size,
                                 UiFont()) > width) {
          --fit;
        }
        // Prefer breaking on spaces when truncating mid-paragraph.
        if (p + fit < para_end) {
          size_t break_at = fit;
          for (size_t k = fit; k > 0; --k) {
            if (text[p + k - 1] == L' ') {
              break_at = k;
              break;
            }
          }
          if (break_at < fit && break_at > 0) {
            fit = break_at;
          }
        }
        if (fit == 0) {
          fit = 1;
        }
        ++lines;
        p += fit;
        while (p < para_end && text[p] == L' ') {
          ++p;
        }
      }
      i = (para_end < n) ? para_end + 1 : para_end;
    }
    if (lines <= 0) {
      lines = 1;
    }
    return static_cast<float>(lines) * line_h + 4.f;
  }

  void SetupHowToTip(int index, int tex, const wchar_t* text, Color tint) {
    auto& row = howto_tips_[index];
    game_ui::ClearImageStyle(row.icon);
    row.icon.texture_id = tex;
    row.icon.width = row.icon.height = 40.f;
    row.icon.tint = tint;
    row.icon.layer = 1;

    StyleHowToText(row.label, 17.f);
    row.label.text = text;
    row.label.width = 860.f;
    row.label.height =
        MeasureHowToTextHeight(row.label.text, row.label.width, 17.f);
  }

  void SetupHowToModal() {
    howto_modal_.title = L"How to play";
    howto_modal_.open = false;
    howto_modal_.close_on_overlay_click = true;
    howto_modal_.width = 980.f;
    howto_modal_.height = 720.f;
    howto_modal_.title_height = 44.f;
    howto_modal_.layer = 100;
    howto_modal_.overlay_color = {0.f, 0.f, 0.f, 0.72f};
    howto_modal_.title_color = ColorPhosphor();
    howto_modal_.title_bar_color = {0.03f, 0.12f, 0.06f, 1.f};
    howto_modal_.style_base = {
        Color{0.015f, 0.05f, 0.03f, 0.98f},
        Border{2.f, BorderMode::In, Color{0.35f, 0.9f, 0.45f, 1.f}}};
    howto_modal_.style_hovered = howto_modal_.style_base;
    howto_modal_.style_active = howto_modal_.style_base;
    howto_modal_.on_close = [this]() { howto_modal_.open = false; };
    ApplyUiFont(howto_modal_);

    constexpr float kPad = 16.f;
    constexpr float kGap = 18.f;
    constexpr float kIconCol = 56.f;
    constexpr float kLabelW = 860.f;

    howto_body_.width = 940.f;
    howto_body_.height = 590.f;
    howto_body_.x = 20.f;
    howto_body_.y = 12.f;
    howto_body_.style_base = {
        Color{0.02f, 0.07f, 0.04f, 0.7f},
        Border{1.5f, BorderMode::In, Color{0.22f, 0.65f, 0.32f, 0.9f}}};
    howto_body_.style_hovered = howto_body_.style_base;
    howto_body_.style_active = howto_body_.style_base;
    howto_body_.scroll_y.mode = ScrollMode::Auto;

    SetupHowToTip(
        0, assets_.energy,
        L"Energy & the star\n"
        L"Click the central star to gain eV. Crits multiply a click when they "
        L"trigger.\n"
        L"EPS adds energy automatically. Dust (from Supernova / Prestige) "
        L"adds flat EPS and raises atom -> isotope chance.",
        {1.f, 1.f, 1.f, 1.f});
    SetupHowToTip(
        1, assets_.proton,
        L"Particles (p / n / e)\n"
        L"Buttons under the star spend eV. Batch (x1 / x10 / x100 / Max) "
        L"sets how many you buy at once.\n"
        L"Protons and neutrons feed Lab nuclei; electrons complete atoms. "
        L"Spend leftovers in Tech.",
        {1.f, 1.f, 1.f, 1.f});
    SetupHowToTip(
        2, assets_.nucleus,
        L"Lab - synthesize elements\n"
        L"Craft a Nucleus (p + n + eV), then an Atom (nucleus + e + eV). "
        L"Heavier elements unlock on hotter stars.\n"
        L"Isotopes can appear from atoms and grant passive EPS. Lab Batch "
        L"controls craft amount.",
        {1.f, 1.f, 1.f, 1.f});
    SetupHowToTip(
        3, assets_.grade,
        L"Tech - upgrades\n"
        L"Order: Electron Lens (EPS) -> Proton Injector (click) -> Neutron "
        L"Channel (crit %, starts at 0%, caps at 100%).\n"
        L"Then per element: nucleus (click), atom (EPS), isotope coil.\n"
        L"At levels 10, 25, 50, 100, then every +100, Click / EPS gain a "
        L"grade multiplier (x10, x100, ...); isotope coils use x2, x4, "
        L"x8, ...\n"
        L"Dust boosts live in D Tech (unlocks after the first Supernova, when "
        L"Dust appears): Dust Dynamo (+10 EPS for 1 Dust), plus Electron Lens / "
        L"Proton Injector x10 boosts that survive Prestige.\n"
        L"Critical Cascade (+1 crit mult) costs atoms H..Fe; Quantum "
        L"Processor adds click multiplier (one click counts as many).",
        {1.f, 1.f, 1.f, 1.f});
    SetupHowToTip(
        4, assets_.supernova,
        L"Supernova & Prestige\n"
        L"Hover ? beside the star for the goal. Stock 100 atoms of the star's "
        L"cap element (Helium -> Oxygen -> Iron) to Supernova.\n"
        L"On Neutron Star, Prestige converts Gold stock into Dust with a rising "
        L"cost: first Dust starts at 10 Au, each next Dust needs +1 Au more "
        L"(10, 11, 12, ...). That counter survives Prestige.\n"
        L"Both reset resources and Tech; isotope discoveries keep.",
        {1.f, 1.f, 1.f, 1.f});

    // Extra particle icons for tip 1 - stacked in the icon column.
    game_ui::ClearImageStyle(howto_icon_n_);
    game_ui::ClearImageStyle(howto_icon_e_);
    howto_icon_n_.texture_id = assets_.neutron;
    howto_icon_e_.texture_id = assets_.electron;
    howto_icon_n_.width = howto_icon_n_.height = 22.f;
    howto_icon_e_.width = howto_icon_e_.height = 22.f;
    howto_icon_n_.tint = howto_icon_e_.tint = {1.f, 1.f, 1.f, 1.f};
    howto_icon_n_.layer = howto_icon_e_.layer = 1;
    howto_tips_[1].icon.width = howto_tips_[1].icon.height = 22.f;

    StyleHowToText(howto_intro_, 17.f);
    howto_intro_.text =
        L"Nuclear Fusion is an idle clicker about building a star's matter "
        L"economy.\n"
        L"Loop: click for eV -> buy particles -> craft in Lab -> buy Tech -> "
        L"reach the star cap -> Supernova (then Prestige on Neutron Star).\n"
        L"Progress saves automatically. Open this guide anytime from "
        L"How to play.";
    howto_intro_.width = 900.f;
    howto_intro_.height =
        MeasureHowToTextHeight(howto_intro_.text, howto_intro_.width, 17.f);

    ApplyConsoleButtonStyle(howto_close_btn_, 180.f, 44.f);
    howto_close_btn_.text = L"Got it";
    howto_close_btn_.font_size = 22.f;
    howto_close_btn_.on_click = [this]() { howto_modal_.open = false; };

    float y = 12.f;
    howto_intro_.x = kPad;
    howto_intro_.y = y;
    y += howto_intro_.height + kGap;

    for (int i = 0; i < kHowToTips; ++i) {
      auto& row = howto_tips_[i];
      row.label.width = kLabelW;
      row.label.height =
          MeasureHowToTextHeight(row.label.text, row.label.width, 17.f);
      // Tip 1 needs room for three stacked particle icons.
      const float min_h = (i == 1) ? 72.f : 48.f;
      const float block_h = std::max(row.label.height, min_h);

      row.icon.x = kPad;
      row.icon.y = y + 4.f;
      row.label.x = kPad + kIconCol;
      row.label.y = y;
      if (i == 1) {
        row.icon.x = 20.f;
        row.icon.y = y + 4.f;
        howto_icon_n_.x = 20.f;
        howto_icon_n_.y = y + 26.f;
        howto_icon_e_.x = 20.f;
        howto_icon_e_.y = y + 48.f;
      }
      row.label.height = block_h;
      y += block_h + kGap;
    }

    howto_body_.components = {
        &howto_intro_,
        &howto_tips_[0].icon, &howto_tips_[0].label,
        &howto_tips_[1].icon, &howto_icon_n_, &howto_icon_e_,
        &howto_tips_[1].label,
        &howto_tips_[2].icon, &howto_tips_[2].label,
        &howto_tips_[3].icon, &howto_tips_[3].label,
        &howto_tips_[4].icon, &howto_tips_[4].label,
    };

    howto_close_btn_.x = (howto_modal_.width - howto_close_btn_.width) * 0.5f;
    howto_close_btn_.y = howto_modal_.height - howto_modal_.title_height - 56.f;

    howto_modal_.components = {&howto_body_, &howto_close_btn_};
  }

  void SetupResetModal() {
    reset_modal_.title = L"Reset progress?";
    reset_modal_.open = false;
    reset_modal_.close_on_overlay_click = true;
    reset_modal_.width = 520.f;
    reset_modal_.height = 260.f;
    reset_modal_.title_height = 44.f;
    reset_modal_.layer = 110;
    reset_modal_.overlay_color = {0.f, 0.f, 0.f, 0.72f};
    reset_modal_.title_color = Color{1.f, 0.55f, 0.35f, 1.f};
    reset_modal_.title_bar_color = {0.12f, 0.04f, 0.02f, 1.f};
    reset_modal_.style_base = {
        Color{0.05f, 0.02f, 0.015f, 0.98f},
        Border{2.f, BorderMode::In, Color{1.f, 0.45f, 0.3f, 1.f}}};
    reset_modal_.style_hovered = reset_modal_.style_base;
    reset_modal_.style_active = reset_modal_.style_base;
    reset_modal_.on_close = [this]() { reset_modal_.open = false; };
    ApplyUiFont(reset_modal_);

    StyleHowToText(reset_body_, 18.f);
    reset_body_.text =
        L"This will erase your save and start a new game.\n"
        L"Energy, particles, Lab stock, Tech levels, Dust\n"
        L"and star progress will all be lost.\n\n"
        L"This cannot be undone.";
    reset_body_.width = 460.f;
    reset_body_.height = 120.f;
    reset_body_.x = 30.f;
    reset_body_.y = 16.f;

    ApplyConsoleButtonStyle(reset_confirm_btn_, 180.f, 44.f);
    reset_confirm_btn_.text = L"Reset";
    reset_confirm_btn_.font_size = 22.f;
    reset_confirm_btn_.text_color = Color{1.f, 0.55f, 0.35f, 1.f};
    reset_confirm_btn_.on_click = [this]() { ConfirmResetProgress(); };

    ApplyConsoleButtonStyle(reset_cancel_btn_, 180.f, 44.f);
    reset_cancel_btn_.text = L"Cancel";
    reset_cancel_btn_.font_size = 22.f;
    reset_cancel_btn_.on_click = [this]() { reset_modal_.open = false; };

    const float btn_y = reset_modal_.height - reset_modal_.title_height - 56.f;
    reset_confirm_btn_.x = 40.f;
    reset_confirm_btn_.y = btn_y;
    reset_cancel_btn_.x =
        reset_modal_.width - reset_cancel_btn_.width - 40.f;
    reset_cancel_btn_.y = btn_y;

    reset_modal_.components = {&reset_body_, &reset_confirm_btn_,
                               &reset_cancel_btn_};
  }

  void SetupPrestigeModal() {
    prestige_modal_.title = L"Prestige?";
    prestige_modal_.open = false;
    prestige_modal_.close_on_overlay_click = true;
    prestige_modal_.width = 640.f;
    prestige_modal_.height = 340.f;
    prestige_modal_.title_height = 44.f;
    prestige_modal_.layer = 110;
    prestige_modal_.overlay_color = {0.f, 0.f, 0.f, 0.72f};
    prestige_modal_.title_color = ColorPhosphor();
    prestige_modal_.title_bar_color = {0.02f, 0.1f, 0.04f, 1.f};
    prestige_modal_.style_base = {
        Color{0.02f, 0.07f, 0.04f, 0.98f},
        Border{2.f, BorderMode::In, Color{0.3f, 0.85f, 0.4f, 1.f}}};
    prestige_modal_.style_hovered = prestige_modal_.style_base;
    prestige_modal_.style_active = prestige_modal_.style_base;
    prestige_modal_.on_close = [this]() { prestige_modal_.open = false; };
    ApplyUiFont(prestige_modal_);

    StyleHowToText(prestige_body_, 18.f);
    prestige_body_.width = 580.f;
    prestige_body_.height = 190.f;
    prestige_body_.x = 30.f;
    prestige_body_.y = 12.f;
    RefreshPrestigeModalText();

    ApplyConsoleButtonStyle(prestige_confirm_btn_, 180.f, 44.f);
    prestige_confirm_btn_.text = L"Confirm";
    prestige_confirm_btn_.font_size = 22.f;
    prestige_confirm_btn_.on_click = [this]() { ConfirmPrestige(); };

    ApplyConsoleButtonStyle(prestige_cancel_btn_, 180.f, 44.f);
    prestige_cancel_btn_.text = L"Cancel";
    prestige_cancel_btn_.font_size = 22.f;
    prestige_cancel_btn_.on_click = [this]() { prestige_modal_.open = false; };

    const float btn_y =
        prestige_modal_.height - prestige_modal_.title_height - 56.f;
    prestige_confirm_btn_.x = 40.f;
    prestige_confirm_btn_.y = btn_y;
    prestige_cancel_btn_.x =
        prestige_modal_.width - prestige_cancel_btn_.width - 40.f;
    prestige_cancel_btn_.y = btn_y;

    prestige_modal_.components = {&prestige_body_, &prestige_confirm_btn_,
                                  &prestige_cancel_btn_};
  }

  void RefreshPrestigeModalText() {
    std::wstringstream ss;
    ss << L"Prestige will reset energy, particles, Lab stock and all Tech\n"
       << L"upgrades. Dust and D Tech upgrades are kept.\n"
       << L"Isotope discoveries also persist.\n\n"
       << L"Next Dust costs "
       << game_ui::FormatInt(G().prestige_gold_next) << L" Au "
       << L"(+1 Au per Dust thereafter).\n"
       << L"You will receive: +"
       << game_ui::FormatInt(G().PrestigeDustReward()) << L" Dust\n\n"
       << L"Continue?";
    prestige_body_.text = ss.str();
  }

  void SetupBuyControl(Image& icon, Button& buy, int tex, ResourceKind kind) {
    game_ui::ClearImageStyle(icon);
    icon.texture_id = tex;
    icon.width = icon.height = 36.f;
    icon.layer = 1;
    icon.tint = {1.f, 1.f, 1.f, 1.f};

    ApplyConsoleButtonStyle(buy, 220.f, 48.f);
    buy.font_size = 22.f;
    buy.layer = 1;
    buy.on_click = [this, kind]() { BuyParticle(kind); };
  }

  double ParticleUnitPrice(ResourceKind kind) const {
    if (kind == ResourceKind::Neutron) {
      return G().price_neutron;
    }
    if (kind == ResourceKind::Electron) {
      return G().price_electron;
    }
    return G().price_proton;
  }

  std::wstring ParticleBuyLabel(ResourceKind kind) const {
    const double amount = G().ResolveParticleBuyAmount(kind);
    const double unit = ParticleUnitPrice(kind);
    const double show_amt = amount > 0.0 ? amount : 1.0;
    return L"x" + game_ui::FormatInt(show_amt) + L" (" +
           game_ui::FormatInt(unit * show_amt) + L" eV)";
  }

  bool CanBuyParticle(ResourceKind kind) const {
    return G().ResolveParticleBuyAmount(kind) > 0.0;
  }

  void BuyParticle(ResourceKind kind) {
    const double amount = G().ResolveParticleBuyAmount(kind);
    if (amount <= 0.0) {
      return;
    }
    if (kind == ResourceKind::Proton) {
      G().MaterializeProton(amount);
    } else if (kind == ResourceKind::Neutron) {
      G().MaterializeNeutron(amount);
    } else {
      G().MaterializeElectron(amount);
    }
    RequestHudRefresh();
  }

  std::wstring ComboBuyLabel(double amount, double unit_price) const {
    const double show_amt = amount > 0.0 ? amount : 1.0;
    return L"x" + game_ui::FormatInt(show_amt) + L" (" +
           game_ui::FormatInt(unit_price * show_amt) + L" eV)";
  }

  void BuyPair() {
    if (G().MaterializePair(G().ResolvePairBuyAmount())) {
      RequestHudRefresh();
    }
  }

  void BuyTrio() {
    if (G().MaterializeTrio(G().ResolveTrioBuyAmount())) {
      RequestHudRefresh();
    }
  }

  bool SupernovaAvailable() const {
    return G().CanTriggerSupernova() && G().star_type != StarType::NeutronStar;
  }

  bool PrestigeAvailable() const { return G().CanTriggerPrestige(); }

  // Shared star-side ascend button: Supernova (pre-final) or Prestige (Neutron).
  bool AscendAvailable() const {
    return SupernovaAvailable() || PrestigeAvailable();
  }

  bool IsPrestigeMode() const {
    return G().star_type == StarType::NeutronStar;
  }

  const wchar_t* AscendButtonLabel() const {
    return IsPrestigeMode() ? L"Prestige" : L"Supernova";
  }

  void OnSupernovaClick() {
    if (!AscendAvailable()) {
      return;
    }
    // Prestige uses a confirmation dialog; Supernova keeps the arm timer.
    if (IsPrestigeMode()) {
      OpenPrestigeConfirm();
      return;
    }
    if (!supernova_armed_) {
      supernova_armed_ = true;
      supernova_arm_t_ = 3.f;
      supernova_.text = L"Confirm?";
      return;
    }
    const bool ok = G().TriggerSupernova();
    if (ok) {
      supernova_armed_ = false;
      supernova_arm_t_ = 0.f;
      supernova_.text = AscendButtonLabel();
      SaveGame(G());
      SelectTechTab(1);
      RequestHudRefresh();
      RefreshLabels();
      Relayout();
    }
  }

  void OpenPrestigeConfirm() {
    RefreshPrestigeModalText();
    prestige_modal_.open = true;
  }

  void ConfirmPrestige() {
    prestige_modal_.open = false;
    if (!PrestigeAvailable()) {
      return;
    }
    if (!G().TriggerPrestige()) {
      return;
    }
    supernova_armed_ = false;
    supernova_arm_t_ = 0.f;
    supernova_.text = AscendButtonLabel();
    SaveGame(G());
    SelectTechTab(1);
    RequestHudRefresh();
    RefreshLabels();
    Relayout();
  }

  void UpdateSupernovaConfirm(float dt) {
    if (!supernova_armed_) {
      return;
    }
    supernova_arm_t_ -= dt;
    if (supernova_arm_t_ <= 0.f) {
      supernova_armed_ = false;
      supernova_.text = AscendButtonLabel();
    }
  }

  void SpawnFloatText(float x, float y, double gain, bool crit) {
    for (auto& ft : float_texts_) {
      if (ft.active) {
        continue;
      }
      ft.active = true;
      ft.life = 0.9f;
      ft.vy = crit ? -70.f : -45.f;
      ft.label.text = (crit ? L"CRIT +" : L"+") + game_ui::FormatEv(gain) +
                      L" eV";
      ft.label.width = crit ? 200.f : 160.f;
      ft.label.x = x - ft.label.width * 0.5f;
      ft.label.y = y - ft.label.height * 0.5f;
      // Warm fill reads on white/cyan star; dark outline keeps glyph edges sharp.
      ft.label.color =
          crit ? Color{1.f, 0.92f, 0.35f, 1.f} : Color{1.f, 0.86f, 0.28f, 1.f};
      ft.label.outline.thickness = crit ? 2.5f : 2.f;
      ft.label.outline.color = {0.02f, 0.04f, 0.06f, 0.95f};
      return;
    }
  }

  void UpdateFloatTexts(float dt) {
    for (auto& ft : float_texts_) {
      if (!ft.active) {
        continue;
      }
      ft.life -= dt;
      ft.label.y += ft.vy * dt;
      const float a = std::clamp(ft.life / 0.9f, 0.f, 1.f);
      ft.label.color.a = a;
      ft.label.outline.color.a = a * 0.95f;
      if (ft.life <= 0.f) {
        ft.active = false;
        Hide(ft.label);
      }
    }
  }

  void HandleStarClick(UiContext* ctx) {
    if (!app_ || !ctx) {
      return;
    }
    const MouseEvents* mouse = ui_get_mouse_events(ctx);
    if (!mouse || !mouse->left_pressed) {
      return;
    }

    const float size = star_base_size_ * star_scale_;
    const float radius = size * 0.52f;
    const float dx = mouse->x - star_center_x_;
    const float dy = mouse->y - star_center_y_;
    if (dx * dx + dy * dy <= radius * radius) {
      const ClickResult result = G().ClickStar();
      SpawnFloatText(mouse->x, mouse->y, result.gain, result.crit);
      star_anim_t_ = 1.f;
      star_renderer_.TriggerClickFlash(result.crit ? 1.f : 0.7f);
      RequestHudRefresh();
      if (!star_hint_dismissed_) {
        star_hint_dismissed_ = true;
        Hide(star_hint_);
        Relayout();
      }
    }
  }

  void UpdateStarAnimation(float dt) {
    star_idle_t_ += dt;
    if (star_anim_t_ > 0.f) {
      star_anim_t_ = std::max(0.f, star_anim_t_ - dt / 0.18f);
    }
    const float click_wave = std::sin(star_anim_t_ * 3.14159265f);
    const float idle = 0.02f * std::sin(star_idle_t_ * 2.15f);
    star_scale_ = (1.f - 0.05f * click_wave) * (1.f + idle);
  }

  void SetupCostFormula(game_ui::CostFormula& f, int result_tex, int a_tex,
                        int b_tex) {
    auto setup_icon = [](Image& img, int tex) {
      game_ui::ClearImageStyle(img);
      img.width = img.height = 32.f;
      img.layer = 1;
      img.texture_id = tex;
    };
    auto setup_label = [this](Label& label, float w = 36.f) {
      label.font_size = 16.f;
      label.color = {0.75f, 1.f, 0.8f, 1.f};
      label.width = w;
      label.height = 32.f;
      label.layer = 1;
      ApplyUiFont(label);
    };
    setup_icon(f.result, result_tex);
    setup_icon(f.a_icon, a_tex);
    setup_icon(f.b_icon, b_tex);
    setup_label(f.eq, 18.f);
    f.eq.text = L"=";
    setup_label(f.a_amt, 40.f);
    setup_label(f.plus1, 16.f);
    f.plus1.text = L"+";
    setup_label(f.b_amt, 40.f);
    setup_label(f.plus2, 16.f);
    f.plus2.text = L"+";
    setup_label(f.energy, 90.f);
  }

  static void PushCostFormula(ScrollView& scroll, game_ui::CostFormula& f) {
    scroll.components.push_back(&f.result);
    scroll.components.push_back(&f.eq);
    scroll.components.push_back(&f.a_icon);
    scroll.components.push_back(&f.a_amt);
    scroll.components.push_back(&f.plus1);
    scroll.components.push_back(&f.b_icon);
    scroll.components.push_back(&f.b_amt);
    scroll.components.push_back(&f.plus2);
    scroll.components.push_back(&f.energy);
  }

  static void SetCostFormulaDisabled(game_ui::CostFormula& f, bool disabled) {
    f.result.disabled = disabled;
    f.eq.disabled = disabled;
    f.a_icon.disabled = disabled;
    f.a_amt.disabled = disabled;
    f.plus1.disabled = disabled;
    f.b_icon.disabled = disabled;
    f.b_amt.disabled = disabled;
    f.plus2.disabled = disabled;
    f.energy.disabled = disabled;
  }

  static void ClearCostFormulaAmounts(game_ui::CostFormula& f) {
    f.a_amt.text.clear();
    f.b_amt.text.clear();
    f.energy.text.clear();
  }

  static void SetCostFormulaAmounts(game_ui::CostFormula& f, int a_count,
                                    int b_count, double energy_ev) {
    f.a_amt.text = game_ui::FormatInt(a_count);
    f.b_amt.text = game_ui::FormatInt(b_count);
    f.energy.text = game_ui::FormatEv(energy_ev) + L" eV";
  }

  void HideCostFormula(game_ui::CostFormula& f) {
    Hide(f.result);
    Hide(f.eq);
    Hide(f.a_icon);
    Hide(f.a_amt);
    Hide(f.plus1);
    Hide(f.b_icon);
    Hide(f.b_amt);
    Hide(f.plus2);
    Hide(f.energy);
  }

  static void LayoutCostFormula(game_ui::CostFormula& f, float x, float y,
                                float icon_sz) {
    constexpr float kGap = 4.f;
    auto place_icon = [&](Image& img) {
      img.width = img.height = icon_sz;
      img.x = x;
      img.y = y;
      x += icon_sz + kGap;
    };
    auto place_label = [&](Label& label, float w) {
      // Same height as icons so LeftMiddle centers text vertically.
      label.width = w;
      label.height = icon_sz;
      label.x = x;
      label.y = y;
      x += w + kGap;
    };

    place_icon(f.result);
    place_label(f.eq, 16.f);
    place_icon(f.a_icon);
    place_label(f.a_amt, 36.f);
    place_label(f.plus1, 14.f);
    place_icon(f.b_icon);
    place_label(f.b_amt, 36.f);
    place_label(f.plus2, 14.f);
    place_label(f.energy, 88.f);
  }

  void RefreshElementRows() {
    for (int i = 0; i < kMaxElementRows; ++i) {
      auto& row = element_rows_[i];
      const bool active = i < static_cast<int>(G().elements.size()) &&
                          G().elements[i].unlocked;
      row.card.disabled = !active;
      row.icon.disabled = !active;
      row.nuc_icon.disabled = !active;
      row.atom_icon.disabled = !active;
      row.iso_icon.disabled = !active;
      row.nuc_hit.disabled = !active;
      row.atom_hit.disabled = !active;
      row.iso_hit.disabled = !active;
      row.name.disabled = !active;
      row.nuc_count.disabled = !active;
      row.atom_count.disabled = !active;
      row.iso_count.disabled = !active;
      row.stats.disabled = !active;
      SetCostFormulaDisabled(row.cost_nucleus, !active);
      SetCostFormulaDisabled(row.cost_atom, !active);
      if (!active) {
        row.craft_nucleus.disabled = true;
        row.craft_atom.disabled = true;
        row.name.text.clear();
        row.nuc_count.text.clear();
        row.atom_count.text.clear();
        row.iso_count.text.clear();
        row.stats.text.clear();
        ClearCostFormulaAmounts(row.cost_nucleus);
        ClearCostFormulaAmounts(row.cost_atom);
        row.icon.texture_id = -1;
        continue;
      }

      const auto& el = G().elements[i];
      row.icon.texture_id = assets_.ElementIcon(el.id);
      row.nuc_icon.texture_id = assets_.nucleus;
      row.atom_icon.texture_id = assets_.atom;
      row.iso_icon.texture_id = assets_.isotope;
      row.name.text = el.name;
      row.nuc_count.text = game_ui::FormatInt(el.nucleus_count);
      row.atom_count.text = game_ui::FormatInt(el.atom_count);
      row.iso_count.text = game_ui::FormatInt(el.isotope_count);

      std::wstringstream ss;
      ss << L"Iso EPS " << game_ui::FormatEv(G().ElementIsotopeEps(el))
         << L"/s";
      row.stats.text = ss.str();

      const double e_nuc = G().NucleusCraftEnergy(el);
      const double e_atom = G().AtomCraftEnergy(el);

      // Per-element particle/energy costs (icons are shared for all elements).
      SetCostFormulaAmounts(row.cost_nucleus, el.protons_needed,
                            el.neutrons_needed, e_nuc);
      SetCostFormulaAmounts(row.cost_atom, 1, el.electrons_needed, e_atom);

      row.craft_nucleus.disabled =
          G().ResolveCraftBatchAmount(G().MaxNucleusCraft(el)) <= 0.0;
      row.craft_atom.disabled =
          G().ResolveCraftBatchAmount(G().MaxAtomCraft(el)) <= 0.0;

      const double nuc_amt =
          G().ResolveCraftBatchAmount(G().MaxNucleusCraft(el));
      const double atom_amt =
          G().ResolveCraftBatchAmount(G().MaxAtomCraft(el));
      auto craft_label = [](const wchar_t* kind, double amt, CraftBatch batch) {
        if (batch == CraftBatch::Max) {
          return std::wstring(kind) + L" x" + game_ui::FormatInt(amt);
        }
        const int want = BatchMultiplier(batch);
        const double show = amt > 0.0 ? amt : static_cast<double>(want);
        return std::wstring(kind) + L" x" + game_ui::FormatInt(show);
      };
      row.craft_nucleus.text =
          craft_label(L"Nucleus", nuc_amt, G().craft_batch);
      row.craft_atom.text = craft_label(L"Atom", atom_amt, G().craft_batch);
    }
  }

  int CostResourceIcon(ResourceKind kind) const {
    switch (kind) {
      case ResourceKind::Energy:
        return assets_.energy;
      case ResourceKind::Proton:
        return assets_.proton;
      case ResourceKind::Neutron:
        return assets_.neutron;
      case ResourceKind::Electron:
        return assets_.electron;
      case ResourceKind::Nucleus:
        return assets_.nucleus;
      case ResourceKind::Atom:
        return assets_.atom;
      case ResourceKind::Isotope:
        return assets_.isotope;
      case ResourceKind::StarDust:
        return assets_.supernova;
    }
    return -1;
  }

  void RefreshUpgradeRows() {
    visible_upgrade_indices_.clear();
    const bool show_dust = tech_tab_index_ == 1 && G().DTechUnlocked();
    for (int i = 0; i < static_cast<int>(G().upgrades.size()); ++i) {
      const auto& up = G().upgrades[i];
      if (up.persist_on_reset != show_dust) {
        continue;
      }
      if (G().IsUpgradeVisible(up)) {
        visible_upgrade_indices_.push_back(i);
      }
    }

    for (int row_i = 0; row_i < kMaxUpgradeRows; ++row_i) {
      auto& row = upgrade_rows_[row_i];
      const bool active =
          row_i < static_cast<int>(visible_upgrade_indices_.size());
      row.card.disabled = !active;
      row.icon.disabled = !active;
      row.icon_hit.disabled = !active;
      row.title.disabled = !active;
      row.desc.disabled = !active;
      row.cost_label.disabled = !active;
      row.buy.disabled = !active;
      row.grade_icon.disabled = !active;
      row.grade_label.disabled = !active;
      for (int c = 0; c < game_ui::UpgradeRow::kMaxCosts; ++c) {
        row.costs[c].icon.disabled = !active;
        row.costs[c].amount.disabled = !active;
      }
      if (!active) {
        row.icon.texture_id = -1;
        row.title.text.clear();
        row.desc.text.clear();
        row.cost_label.text.clear();
        row.grade_label.text.clear();
        row.grade_icon.texture_id = -1;
        row.upgrade_index = -1;
        row.cost_count = 0;
        for (int c = 0; c < game_ui::UpgradeRow::kMaxCosts; ++c) {
          row.costs[c].icon.texture_id = -1;
          row.costs[c].amount.text.clear();
        }
        continue;
      }

      const int up_i = visible_upgrade_indices_[row_i];
      row.upgrade_index = up_i;
      const auto& up = G().upgrades[up_i];
      row.icon.texture_id = assets_.UpgradeIcon(up.id);
      std::wstringstream title;
      title << up.name << L"  [" << up.level << L"]";
      row.title.text = title.str();

      const int grade =
          UpgradeUsesGrade(up.effect) ? UpgradeGrade(up.level) : 0;
      if (grade > 0) {
        row.grade_icon.texture_id = assets_.grade;
        row.grade_label.text = L"x" + game_ui::FormatInt(grade);
      } else {
        row.grade_icon.texture_id = -1;
        row.grade_label.text.clear();
      }

      row.desc.text = FormatUpgradePerLevelDesc(up);
      row.cost_label.text = L"Cost:";
      row.cost_count = std::min(game_ui::UpgradeRow::kMaxCosts,
                                static_cast<int>(up.base_costs.size()));
      for (int c = 0; c < game_ui::UpgradeRow::kMaxCosts; ++c) {
        auto& line = row.costs[c];
        if (c >= row.cost_count) {
          line.icon.texture_id = -1;
          line.amount.text.clear();
          continue;
        }
        const auto& cost = up.base_costs[c];
        const double amt = G().ScaledCostAmount(up, cost);
        if (cost.kind == ResourceKind::Atom) {
          // Element art from images/elements — no symbol suffix.
          line.icon.texture_id = assets_.ElementIcon(cost.element_id);
          line.amount.text = game_ui::FormatInt(amt);
        } else {
          line.icon.texture_id = CostResourceIcon(cost.kind);
          std::wstringstream amt_ss;
          if (cost.kind == ResourceKind::Nucleus ||
              cost.kind == ResourceKind::Isotope) {
            // [icon] (El) amount
            amt_ss << L"(" << ElementSymbol(cost.element_id) << L") "
                   << game_ui::FormatInt(amt);
          } else {
            // [icon] amount
            amt_ss << game_ui::FormatInt(amt);
          }
          line.amount.text = amt_ss.str();
        }
      }
      row.buy.disabled = !G().CanAffordUpgrade(up);
    }
  }

  int CountUnlockedElements() const {
    int n = 0;
    for (const auto& el : G().elements) {
      if (el.unlocked) {
        ++n;
      }
    }
    return n;
  }

  int CountDiscoveredIsotopes() const {
    int n = 0;
    for (const auto& el : G().elements) {
      if (el.isotope_discovered) {
        ++n;
      }
    }
    return n;
  }

  void Relayout() {
    if (!app_) {
      return;
    }
    Layout(ui_get_width(app_->ctx), ui_get_height(app_->ctx));
    canvas_scene_.prepare_scene();
    ui_scene_.prepare_scene();
  }

  std::wstring FormatUpgradePerLevelDesc(const UpgradeDef& up) const {
    const double v = up.effect_per_level;
    switch (up.effect) {
      case UpgradeEffect::ClickPower:
        return L"+" + game_ui::FormatEv(v) + L" eV click power";
      case UpgradeEffect::AutoEps:
        return L"+" + game_ui::FormatEv(v) + L" EPS";
      case UpgradeEffect::CritChance:
        return L"+" + game_ui::FormatEv(v * 100.0) + L"% crit chance";
      case UpgradeEffect::CritMultiplier:
        return L"+" + game_ui::FormatEv(v) + L" crit multiplier";
      case UpgradeEffect::AutoClickMult:
        return L"+" + game_ui::FormatEv(v) + L" click multiplier";
      case UpgradeEffect::IsotopeEpsMult:
        return L"+" + game_ui::FormatEv(v) +
               L" this element's isotope EPS mult";
      case UpgradeEffect::CraftEnergyDiscount:
        return L"-" + game_ui::FormatEv(v * 100.0) +
               L"% nucleus/atom eV craft cost";
      case UpgradeEffect::DustFlatEps:
        return L"+" + game_ui::FormatEv(v) + L" EPS (permanent)";
      case UpgradeEffect::ElectronLensBoost:
        return L"x" + game_ui::FormatEv(v) +
               L" Electron Lens EPS (permanent)";
      case UpgradeEffect::ProtonInjectorBoost:
        return L"x" + game_ui::FormatEv(v) +
               L" Proton Injector click (permanent)";
    }
    return up.description;
  }

  // Passive for a single upgrade's contribution (D-Tech boosts included where
  // relevant).
  double UpgradeBonusTotal(const UpgradeDef& up) const {
    double total = UpgradeScaledEffect(up);
    if (up.id == "click_e") {
      total *= G().DustBoostMultiplier(UpgradeEffect::ElectronLensBoost);
    } else if (up.id == "click_p") {
      total *= G().DustBoostMultiplier(UpgradeEffect::ProtonInjectorBoost);
    }
    return total;
  }

  std::wstring FormatUpgradeBonusValue(const UpgradeDef& up,
                                       double total) const {
    switch (up.effect) {
      case UpgradeEffect::ClickPower:
        return L"+" + game_ui::FormatEv(total) + L" eV click";
      case UpgradeEffect::AutoEps:
        return L"+" + game_ui::FormatEv(total) + L" EPS";
      case UpgradeEffect::CritChance:
        return L"+" + game_ui::FormatEv(total * 100.0) + L"% crit chance";
      case UpgradeEffect::CritMultiplier:
        return L"+" + game_ui::FormatEv(total) + L" crit mult";
      case UpgradeEffect::AutoClickMult:
        return L"+" + game_ui::FormatEv(total) + L" click mult";
      case UpgradeEffect::IsotopeEpsMult:
        return L"+" + game_ui::FormatEv(total) + L" isotope EPS mult";
      case UpgradeEffect::CraftEnergyDiscount:
      case UpgradeEffect::DustFlatEps:
      case UpgradeEffect::ElectronLensBoost:
      case UpgradeEffect::ProtonInjectorBoost:
        break;
    }
    return L"+" + game_ui::FormatEv(total);
  }

  std::wstring FormatUpgradeTotalBonus(const UpgradeDef& up) const {
    std::wstringstream ss;
    if (up.effect == UpgradeEffect::DustFlatEps) {
      ss << L"Total: +" << game_ui::FormatEv(UpgradeScaledEffect(up))
         << L" EPS";
      return ss.str();
    }
    if (up.effect == UpgradeEffect::CraftEnergyDiscount) {
      const double discount = 1.0 - G().CraftEnergyMult();
      ss << L"Total: -" << game_ui::FormatEv(discount * 100.0)
         << L"% nucleus/atom eV craft cost";
      return ss.str();
    }
    if (up.effect == UpgradeEffect::ElectronLensBoost ||
        up.effect == UpgradeEffect::ProtonInjectorBoost) {
      const double mult = up.level > 0
                              ? std::pow(up.effect_per_level,
                                         static_cast<double>(up.level))
                              : 1.0;
      ss << L"Total: x" << game_ui::FormatInt(mult);
      if (up.effect == UpgradeEffect::ElectronLensBoost) {
        ss << L" Electron Lens EPS";
      } else {
        ss << L" Proton Injector click";
      }
      return ss.str();
    }

    const double total = UpgradeBonusTotal(up);
    const int grade =
        UpgradeUsesGrade(up.effect) ? UpgradeGrade(up.level) : 0;
    if (grade > 0) {
      const double grade_x = UpgradeGradeMultiplier(
          grade, UpgradeGradeBase(up.effect));
      ss << L"grade x" << game_ui::FormatInt(grade_x) << L"\n"
         << L"Total: " << FormatUpgradeBonusValue(up, total);
      return ss.str();
    }

    ss << L"Total: " << FormatUpgradeBonusValue(up, total);
    if (up.effect == UpgradeEffect::AutoClickMult) {
      ss << L"\nClick: " << game_ui::FormatEv(G().EffectiveClickPower())
         << L" eV";
    }
    return ss.str();
  }

  void ShowHoverTip(const std::wstring& text, float tip_w, float tip_h) {
    if (!app_) {
      HideHoverTip();
      return;
    }
    const MouseEvents* mouse = ui_get_mouse_events(app_->ctx);
    const float mx = mouse ? mouse->x : 0.f;
    const float my = mouse ? mouse->y : 0.f;
    const float sw = static_cast<float>(ui_get_width(app_->ctx));
    const float sh = static_cast<float>(ui_get_height(app_->ctx));

    tech_tip_label_.text = text;
    tech_tip_panel_.width = tip_w;
    tech_tip_panel_.height = tip_h;
    tech_tip_label_.width = tip_w - 20.f;
    tech_tip_label_.height = tip_h - 16.f;

    float tip_x = mx + 16.f;
    float tip_y = my + 16.f;
    if (tip_x + tech_tip_panel_.width > sw - 8.f) {
      tip_x = mx - tech_tip_panel_.width - 12.f;
    }
    if (tip_y + tech_tip_panel_.height > sh - 8.f) {
      tip_y = my - tech_tip_panel_.height - 12.f;
    }
    tip_x = std::max(8.f, tip_x);
    tip_y = std::max(8.f, tip_y);

    tech_tip_panel_.x = tip_x;
    tech_tip_panel_.y = tip_y;
    tech_tip_label_.x = tip_x + 10.f;
    tech_tip_label_.y = tip_y + 8.f;
  }

  void HideHoverTip() {
    Hide(tech_tip_panel_);
    Hide(tech_tip_label_);
  }

  static bool PointInRect(float px, float py, float x, float y, float w,
                          float h) {
    return px >= x && py >= y && px < x + w && py < y + h;
  }

  enum class DelayedTipKind {
    None = 0,
    Nucleus,
    Atom,
    Isotope,
    Proton,
    Neutron,
    Electron
  };

  void ClearDelayedTipHover() {
    delayed_tip_kind_ = DelayedTipKind::None;
    delayed_tip_row_ = -1;
    delayed_tip_t_ = 0.f;
    delayed_tip_armed_ = false;
    delayed_tip_text_.clear();
  }

  void ArmDelayedTip(int row, DelayedTipKind kind, const std::wstring& text,
                     float tip_w, float tip_h) {
    delayed_tip_armed_ = true;
    if (delayed_tip_row_ != row || delayed_tip_kind_ != kind) {
      delayed_tip_row_ = row;
      delayed_tip_kind_ = kind;
      delayed_tip_t_ = 0.f;
    }
    delayed_tip_text_ = text;
    delayed_tip_w_ = tip_w;
    delayed_tip_h_ = tip_h;
    if (delayed_tip_t_ >= kDelayedTipSec) {
      ShowHoverTip(delayed_tip_text_, delayed_tip_w_, delayed_tip_h_);
    } else {
      HideHoverTip();
    }
  }

  void AdvanceStockTipTimer(float dt) {
    if (!delayed_tip_armed_ || delayed_tip_kind_ == DelayedTipKind::None) {
      return;
    }
    delayed_tip_t_ += dt;
    if (delayed_tip_t_ >= kDelayedTipSec) {
      ShowHoverTip(delayed_tip_text_, delayed_tip_w_, delayed_tip_h_);
    }
  }

  std::wstring DelayedTipText(int row_index, DelayedTipKind kind) const {
    switch (kind) {
      case DelayedTipKind::Nucleus:
        // Explicit breaks so Orbitron wrap does not split mid-word.
        return L"Nucleus\nCrafted from protons,\nneutrons and energy.";
      case DelayedTipKind::Atom:
        return L"Atom\nNucleus + electrons\n+ energy.";
      case DelayedTipKind::Isotope: {
        std::wstringstream ss;
        ss << L"Isotope\nRare atom mutation.";
        if (row_index >= 0 &&
            row_index < static_cast<int>(G().elements.size())) {
          const auto& el = G().elements[row_index];
          const double base = G().IsotopeEpsPer(el);
          const double with_coil = base * G().ElementIsotopeMult(el);
          ss << L"\nBase +" << game_ui::FormatEv(base) << L" eV/s each"
             << L"\nWith coil +" << game_ui::FormatEv(with_coil)
             << L" eV/s each";
        }
        return ss.str();
      }
      case DelayedTipKind::Proton:
        return L"Proton\nUsed with neutrons\nto craft nuclei.";
      case DelayedTipKind::Neutron:
        return L"Neutron\nUsed with protons\nto craft nuclei.";
      case DelayedTipKind::Electron:
        return L"Electron\nCompletes a nucleus\ninto an atom.";
      default:
        return L"";
    }
  }

  void UpdateHoverTooltips() {
    delayed_tip_armed_ = false;
    if (!app_) {
      ClearDelayedTipHover();
      HideHoverTip();
      return;
    }
    // Modal open or any mouse button held: never show a sticky tip.
    if (howto_modal_.open || reset_modal_.open || prestige_modal_.open) {
      ClearDelayedTipHover();
      HideHoverTip();
      return;
    }
    const MouseEvents* mouse = ui_get_mouse_events(app_->ctx);
    if (!mouse || mouse->left_down || mouse->right_down || mouse->middle_down) {
      ClearDelayedTipHover();
      HideHoverTip();
      return;
    }

    // Energy HUD particle icons: tip after 1s hover.
    auto particle_hit = [&](const Image& icon) {
      return PointInRect(mouse->x, mouse->y, icon.x, icon.y, icon.width,
                         icon.height);
    };
    if (particle_hit(hud_icon_p_)) {
      ArmDelayedTip(-1, DelayedTipKind::Proton,
                    DelayedTipText(-1, DelayedTipKind::Proton), 260.f,
                    78.f);
      return;
    }
    if (particle_hit(hud_icon_n_)) {
      ArmDelayedTip(-1, DelayedTipKind::Neutron,
                    DelayedTipText(-1, DelayedTipKind::Neutron), 260.f,
                    78.f);
      return;
    }
    if (particle_hit(hud_icon_e_)) {
      ArmDelayedTip(-1, DelayedTipKind::Electron,
                    DelayedTipText(-1, DelayedTipKind::Electron), 260.f,
                    78.f);
      return;
    }

    // Lab stock icons: tip appears only after hovering for 1 second.
    if (PointInRect(mouse->x, mouse->y, lab_scroll_.x, lab_scroll_.y,
                    lab_scroll_.width, lab_scroll_.height)) {
      for (int i = 0; i < kMaxElementRows; ++i) {
        auto& row = element_rows_[i];
        if (row.element_index < 0 || row.nuc_hit.disabled) {
          continue;
        }
        DelayedTipKind kind = DelayedTipKind::None;
        if (row.nuc_hit.state == ComponentState::Hovered) {
          kind = DelayedTipKind::Nucleus;
        } else if (row.atom_hit.state == ComponentState::Hovered) {
          kind = DelayedTipKind::Atom;
        } else if (row.iso_hit.state == ComponentState::Hovered) {
          kind = DelayedTipKind::Isotope;
        }
        if (kind == DelayedTipKind::None) {
          continue;
        }
        const float tip_w =
            (kind == DelayedTipKind::Isotope) ? 360.f : 300.f;
        const float tip_h = (kind == DelayedTipKind::Isotope) ? 110.f
                            : (kind == DelayedTipKind::Nucleus ||
                               kind == DelayedTipKind::Atom)
                                ? 84.f
                                : 64.f;
        ArmDelayedTip(i, kind, DelayedTipText(i, kind), tip_w, tip_h);
        return;
      }
    }

    ClearDelayedTipHover();

    // Click-mult icon tip (center panel).
    if (!autoclick_hit_.disabled &&
        autoclick_hit_.state == ComponentState::Hovered &&
        PointInRect(mouse->x, mouse->y, autoclick_hit_.x, autoclick_hit_.y,
                    autoclick_hit_.width, autoclick_hit_.height)) {
      std::wstringstream ss;
      ss << L"Click multiplier x"
         << game_ui::FormatInt(G().auto_click_mult) << L"\n"
         << L"Click: " << game_ui::FormatEv(G().EffectiveClickPower())
         << L" eV";
      ShowHoverTip(ss.str(), 280.f, 56.f);
      return;
    }

    // Tech icon tips only while the cursor is inside the Tech scroll viewport.
    // ScrollView can leave child Hovered stuck after the mouse leaves the panel.
    if (PointInRect(mouse->x, mouse->y, tech_scroll_.x, tech_scroll_.y,
                    tech_scroll_.width, tech_scroll_.height)) {
      for (int i = 0; i < kMaxUpgradeRows; ++i) {
        auto& row = upgrade_rows_[i];
        if (row.upgrade_index < 0 || row.icon_hit.disabled) {
          continue;
        }
        if (row.icon_hit.state != ComponentState::Hovered) {
          continue;
        }
        if (row.upgrade_index >= static_cast<int>(G().upgrades.size())) {
          continue;
        }
        {
          const auto& tip_up = G().upgrades[row.upgrade_index];
          const int tip_grade = UpgradeUsesGrade(tip_up.effect)
                                    ? UpgradeGrade(tip_up.level)
                                    : 0;
          const float tip_h =
              tip_grade > 0
                  ? 56.f
                  : (tip_up.effect == UpgradeEffect::AutoClickMult ? 64.f
                                                                  : 52.f);
          ShowHoverTip(FormatUpgradeTotalBonus(tip_up), 400.f, tip_h);
        }
        return;
      }
    }

    // Supernova help: require both Hovered and cursor still over the button.
    if (sn_help_btn_.state == ComponentState::Hovered &&
        PointInRect(mouse->x, mouse->y, sn_help_btn_.x, sn_help_btn_.y,
                    sn_help_btn_.width, sn_help_btn_.height)) {
      ShowHoverTip(FormatSupernovaHelpText(), 400.f,
                   IsPrestigeMode() ? 140.f : 110.f);
      return;
    }

    if (dust_hit_.state == ComponentState::Hovered &&
        PointInRect(mouse->x, mouse->y, dust_hit_.x, dust_hit_.y,
                    dust_hit_.width, dust_hit_.height)) {
      ShowHoverTip(FormatDustBonusText(), 340.f, 90.f);
      return;
    }

    HideHoverTip();
  }

  std::wstring FormatDustBonusText() const {
    const double dust = G().star_dust;
    std::wstringstream ss;
    ss << L"Dust bonuses:\n"
       << L"+" << game_ui::FormatEv(dust) << L" flat EPS\n";
    if (G().dust_flat_eps > 0.0) {
      ss << L"+" << game_ui::FormatEv(G().dust_flat_eps)
         << L" Dust Dynamo EPS\n";
    }
    ss << L"Mut: " << game_ui::FormatInt(std::clamp(dust * 0.1, 0.0, 50.0))
       << L"% atom -> isotope";
    return ss.str();
  }

  std::wstring FormatSupernovaHelpText() const {
    if (G().star_type == StarType::NeutronStar) {
      const Element* gold = G().FindElement("Gold");
      const double have = gold ? gold->atom_count : 0.0;
      const double reward = G().PrestigeDustReward();
      std::wstringstream ss;
      ss << L"Prestige (Neutron Star):\n"
         << L"Next Dust: " << game_ui::FormatInt(G().prestige_gold_next)
         << L" Au, then +1 Au each\n"
         << L"Have: " << game_ui::FormatInt(have) << L" Au\n"
         << L"Reward: +" << game_ui::FormatInt(reward) << L" Dust\n"
         << L"Resets resources and Tech;\n"
         << L"keeps Dust, D Tech and isotopes.";
      return ss.str();
    }

    const int max_z = StarMaxAtomicNumber(G().star_type);
    const Element* cap = nullptr;
    for (const auto& el : G().elements) {
      if (el.atomic_number == max_z) {
        cap = &el;
        break;
      }
    }
    if (!cap) {
      return L"Supernova requirements unknown.";
    }

    const double have = cap->atom_count;
    std::wstringstream ss;
    ss << L"Supernova requires:\n"
       << game_ui::FormatInt(kSupernovaAtomGoal) << L" " << cap->name
       << L" atoms\nHave: " << game_ui::FormatInt(have) << L"/"
       << game_ui::FormatInt(kSupernovaAtomGoal) << L"\nReward: +"
       << game_ui::FormatInt(StarDustReward(G().star_type)) << L" Dust";
    return ss.str();
  }

  void RequestHudRefresh() { hud_force_refresh_ = true; }

  void RefreshLabels() {
    if (!app_) {
      return;
    }

    energy_label_.text =
        L"Energy(eV): " + game_ui::FormatEv(G().energy);
    {
      wchar_t mult_buf[32] = {};
      std::swprintf(mult_buf, 32, L"%.1f", G().crit_multiplier);
      crit_label_.text =
          L"Crit: " + game_ui::FormatInt(G().crit_chance * 100.0) + L"%   x" +
          mult_buf;
    }
    eps_label_.text = L"EPS: " + game_ui::FormatEv(G().Eps()) + L"/s   Click: " +
                      game_ui::FormatEv(G().EffectiveClickPower());
    dust_label_.text = L"Dust: " + game_ui::FormatInt(G().star_dust);
    count_p_.text = game_ui::FormatInt(G().protons);
    count_n_.text = game_ui::FormatInt(G().neutrons);
    count_e_.text = game_ui::FormatInt(G().electrons);
    star_label_.text = std::wstring(L"Star: ") + StarTypeName(G().star_type);

    buy_p_.text = ParticleBuyLabel(ResourceKind::Proton);
    buy_n_.text = ParticleBuyLabel(ResourceKind::Neutron);
    buy_e_.text = ParticleBuyLabel(ResourceKind::Electron);
    buy_pn_.text =
        ComboBuyLabel(G().ResolvePairBuyAmount(), G().PairUnitPrice());
    buy_all_.text =
        ComboBuyLabel(G().ResolveTrioBuyAmount(), G().TrioUnitPrice());

    RefreshElementRows();
    RefreshUpgradeRows();

    const int unlocked = CountUnlockedElements();
    const int isotopes = CountDiscoveredIsotopes();
    if (unlocked != unlocked_count_ || isotopes != isotope_discovered_count_) {
      unlocked_count_ = unlocked;
      isotope_discovered_count_ = isotopes;
      Relayout();
    }
  }

  void SetButtonAffordable(Button& btn, bool can, Color active_color) {
    btn.disabled = !can;
    btn.text_color =
        can ? active_color : Color{0.5f, 0.5f, 0.5f, 0.45f};
  }

  void RefreshDynamicStyles() {
    if (!app_) {
      return;
    }
    SetButtonAffordable(buy_p_, CanBuyParticle(ResourceKind::Proton),
                        ColorProton());
    SetButtonAffordable(buy_n_, CanBuyParticle(ResourceKind::Neutron),
                        ColorNeutron());
    SetButtonAffordable(buy_e_, CanBuyParticle(ResourceKind::Electron),
                        ColorElectron());
    SetButtonAffordable(buy_pn_, G().ResolvePairBuyAmount() > 0.0,
                        ColorPhosphor());
    SetButtonAffordable(buy_all_, G().ResolveTrioBuyAmount() > 0.0,
                        ColorPhosphor());

    for (auto& row : element_rows_) {
      if (row.element_index < 0 ||
          row.element_index >= static_cast<int>(G().elements.size()) ||
          !G().elements[row.element_index].unlocked) {
        continue;
      }
      const auto& el = G().elements[row.element_index];
      SetButtonAffordable(
          row.craft_nucleus,
          G().ResolveCraftBatchAmount(G().MaxNucleusCraft(el)) > 0.0,
          ColorPhosphor());
      SetButtonAffordable(
          row.craft_atom,
          G().ResolveCraftBatchAmount(G().MaxAtomCraft(el)) > 0.0,
          ColorPhosphor());
    }
    for (auto& row : upgrade_rows_) {
      if (row.upgrade_index < 0 ||
          row.upgrade_index >= static_cast<int>(G().upgrades.size())) {
        continue;
      }
      SetButtonAffordable(
          row.buy, G().CanAffordUpgrade(G().upgrades[row.upgrade_index]),
          ColorPhosphor());
    }

    const bool sn = AscendAvailable();
    SetButtonAffordable(supernova_, sn, ColorPhosphor());
    if (!sn) {
      supernova_armed_ = false;
      supernova_.text = AscendButtonLabel();
    } else if (!supernova_armed_) {
      supernova_.text = AscendButtonLabel();
    }
    if (sn != supernova_visible_) {
      supernova_visible_ = sn;
      Relayout();
    } else if (!sn) {
      Hide(icon_supernova_);
      Hide(supernova_);
    }
  }

  void Hide(Component& c) { c.x = -4000.f; }

  void Layout(int width, int height) {
    const float w = static_cast<float>(width > 0 ? width : 1280);
    const float h = static_cast<float>(height > 0 ? height : 720);

    constexpr float kMargin = 16.f;
    constexpr float kGap = 12.f;
    constexpr float kHeaderH = 48.f;

    canvas_.x = 0.f;
    canvas_.y = 0.f;
    canvas_.width = w;
    canvas_.height = h;

    settings_btn_.x = kMargin;
    settings_btn_.y = h - 64.f;
    howto_btn_.x = settings_btn_.x + settings_btn_.width + kGap;
    howto_btn_.y = settings_btn_.y;
    reset_btn_.x = howto_btn_.x + howto_btn_.width + kGap;
    reset_btn_.y = settings_btn_.y;
    exit_btn_.x = reset_btn_.x + reset_btn_.width + kGap;
    exit_btn_.y = settings_btn_.y;
    howto_modal_.screen_width = w;
    howto_modal_.screen_height = h;
    reset_modal_.screen_width = w;
    reset_modal_.screen_height = h;
    prestige_modal_.screen_width = w;
    prestige_modal_.screen_height = h;
    const float bottom_ui = settings_btn_.y - kGap;

    title_.x = kMargin;
    title_.y = 10.f;
    title_.width = std::max(200.f, w - kMargin * 2.f);

    const float content_top = title_.y + kHeaderH + 4.f;
    const float content_w = w - kMargin * 2.f;

    // Lab and Tech share the same width; +20% vs the former Tech base.
    float side_w = std::clamp(content_w * 0.30f, 300.f, 460.f) * 1.2f;
    float center_w = content_w - side_w * 2.f - kGap * 2.f;
    if (center_w < 300.f) {
      side_w = std::max(260.f, (content_w - 300.f - kGap * 2.f) * 0.5f);
      center_w = content_w - side_w * 2.f - kGap * 2.f;
    }
    center_w = std::max(260.f, center_w);
    const float lab_w = side_w;
    const float tech_w = side_w;

    const float left_x = kMargin;
    const float center_x = left_x + lab_w + kGap;
    const float right_x = center_x + center_w + kGap;

    // Lab / Tech titles on their own row; Lab batch radios on the next row so
    // Orbitron glyphs never collide (Lab↔Batch, x100↔Max).
    lab_header_.x = left_x + 4.f;
    lab_header_.y = content_top;
    if (app_) {
      lab_header_.width =
          std::max(56.f, ui_measure_text(app_->ctx, L"Lab", lab_header_.font_size,
                                         UiFont()) +
                             8.f);
    }
    SyncTechTabVisibility();
    if (app_ && G().DTechUnlocked()) {
      const float tab_gap = 8.f;
      const float tab_w =
          std::clamp((tech_w - 8.f - tab_gap) * 0.5f, 96.f, 140.f);
      tech_tab_btn_.width = tab_w;
      dtech_tab_btn_.width = tab_w;
      tech_tab_btn_.height = 32.f;
      dtech_tab_btn_.height = 32.f;
      tech_tab_btn_.x = right_x + 4.f;
      tech_tab_btn_.y = content_top;
      dtech_tab_btn_.x = tech_tab_btn_.x + tab_w + tab_gap;
      dtech_tab_btn_.y = content_top;
    } else {
      tech_header_.x = right_x + 4.f;
      tech_header_.y = content_top;
    }

    const float batch_row_y = content_top + 30.f;
    lab_batch_label_.x = left_x + 4.f;
    lab_batch_label_.y = batch_row_y + 2.f;
    lab_batch_.x = lab_batch_label_.x + lab_batch_label_.width + 8.f;
    lab_batch_.y = batch_row_y;
    const float lab_batch_max_w =
        std::max(200.f, lab_w - lab_batch_label_.width - 20.f);
    lab_batch_.item_width =
        std::clamp(lab_batch_max_w / 4.f - lab_batch_.gap, 72.f, 96.f);

    const float panels_top = content_top + 62.f;
    const float panels_h = std::max(160.f, bottom_ui - panels_top);

    lab_scroll_.x = left_x;
    lab_scroll_.y = panels_top;
    lab_scroll_.width = lab_w;
    lab_scroll_.height = panels_h;
    lab_scroll_.disabled = false;

    tech_scroll_.x = right_x;
    tech_scroll_.y = panels_top;
    tech_scroll_.width = tech_w;
    tech_scroll_.height = panels_h;
    tech_scroll_.disabled = false;

    hud_panel_.x = center_x;
    hud_panel_.y = panels_top;
    hud_panel_.width = center_w;
    hud_panel_.height = 196.f;

    energy_icon_.x = hud_panel_.x + 12.f;
    energy_icon_.y = hud_panel_.y + 12.f;
    energy_label_.x = energy_icon_.x + energy_icon_.width + 8.f;
    energy_label_.y = hud_panel_.y + 12.f;
    energy_label_.width = std::max(120.f, center_w - 60.f);
    star_label_.x = energy_label_.x;
    star_label_.y = hud_panel_.y + 48.f;
    star_label_.width = energy_label_.width;
    crit_label_.x = hud_panel_.x + 12.f;
    crit_label_.y = hud_panel_.y + 80.f;
    crit_label_.width = center_w - 24.f;
    eps_label_.x = hud_panel_.x + 12.f;
    eps_label_.y = hud_panel_.y + 110.f;
    eps_label_.width = std::max(160.f, center_w - 160.f);
    dust_label_.width = 140.f;
    dust_label_.height = 28.f;
    dust_label_.x = hud_panel_.x + hud_panel_.width - dust_label_.width - 12.f;
    dust_label_.y = hud_panel_.y + 110.f;
    dust_hit_.width = dust_label_.width;
    dust_hit_.height = dust_label_.height;
    dust_hit_.x = dust_label_.x;
    dust_hit_.y = dust_label_.y;

    const float count_y = hud_panel_.y + 148.f;
    const float count_slot = std::max(100.f, (center_w - 24.f) / 3.f);
    hud_icon_p_.x = hud_panel_.x + 12.f;
    hud_icon_p_.y = count_y;
    count_p_.x = hud_icon_p_.x + 28.f;
    count_p_.y = count_y;
    count_p_.width = count_slot - 36.f;
    hud_icon_n_.x = hud_panel_.x + 12.f + count_slot;
    hud_icon_n_.y = count_y;
    count_n_.x = hud_icon_n_.x + 28.f;
    count_n_.y = count_y;
    count_n_.width = count_slot - 36.f;
    hud_icon_e_.x = hud_panel_.x + 12.f + count_slot * 2.f;
    hud_icon_e_.y = count_y;
    count_e_.x = hud_icon_e_.x + 28.f;
    count_e_.y = count_y;
    count_e_.width = count_slot - 36.f;

    constexpr float kBuyH = 44.f;
    constexpr float kBuyIcon = 36.f;
    constexpr float kBuyGap = 8.f;
    constexpr float kBuyBatchH = 28.f;
    constexpr float kIconCol = 92.f;
    constexpr float kToggleW = 48.f;
    constexpr float kToggleGap = 10.f;
    constexpr int kBuyRows = 5;
    // Button short enough to leave room for the auto-buy toggle on the right.
    float buy_btn_w =
        center_w - kIconCol - 8.f - kToggleW - kToggleGap - 16.f;
    buy_btn_w = std::clamp(buy_btn_w, 140.f, 330.f);
    buy_p_.width = buy_n_.width = buy_e_.width = buy_pn_.width =
        buy_all_.width = buy_btn_w;
    buy_p_.height = buy_n_.height = buy_e_.height = buy_pn_.height =
        buy_all_.height = kBuyH;
    const float buy_font = buy_btn_w >= 220.f ? 18.f : 15.f;
    buy_p_.font_size = buy_n_.font_size = buy_e_.font_size = buy_font;
    buy_pn_.font_size = buy_all_.font_size = buy_font;
    icon_p_.width = icon_p_.height = kBuyIcon;
    icon_n_.width = icon_n_.height = kBuyIcon;
    icon_e_.width = icon_e_.height = kBuyIcon;
    constexpr float kComboIcon = 28.f;
    icon_pn_p_.width = icon_pn_p_.height = kComboIcon;
    icon_pn_n_.width = icon_pn_n_.height = kComboIcon;
    icon_all_p_.width = icon_all_p_.height = kComboIcon;
    icon_all_n_.width = icon_all_n_.height = kComboIcon;
    icon_all_e_.width = icon_all_e_.height = kComboIcon;

    const float buy_block_h =
        kBuyBatchH + 8.f + kBuyH * kBuyRows + kBuyGap * (kBuyRows - 1);
    const float buy_y0 = panels_top + panels_h - buy_block_h;
    const float row_w = kIconCol + 8.f + buy_btn_w + kToggleGap + kToggleW;
    float buy_x = center_x + (center_w - row_w) * 0.5f;
    buy_x = std::clamp(buy_x, center_x, center_x + center_w - row_w);

    // Batch controls centered above the buy button column.
    buy_batch_.item_width = 80.f;
    const float batch_cluster_w =
        buy_batch_label_.width + 8.f + buy_batch_.item_width * 4.f +
        buy_batch_.gap * 3.f;
    buy_batch_label_.x = buy_x + (row_w - batch_cluster_w) * 0.5f;
    buy_batch_label_.y = buy_y0 + 2.f;
    buy_batch_.x = buy_batch_label_.x + buy_batch_label_.width + 8.f;
    buy_batch_.y = buy_y0;

    const float buy_btns_y0 = buy_y0 + kBuyBatchH + 8.f;
    const float btn_x = buy_x + kIconCol + 8.f;
    const float tog_x = btn_x + buy_btn_w + kToggleGap;

    auto place_row = [&](float row_y, Image* icons[], int icon_count,
                         Button& buy, Toggle& tog) {
      const float icon_gap = 4.f;
      // Single large icon uses kBuyIcon; combo rows use kComboIcon.
      const float use_icon = (icon_count == 1) ? kBuyIcon : kComboIcon;
      const float pack_w =
          icon_count * use_icon + (icon_count - 1) * icon_gap;
      float ix = buy_x + (kIconCol - pack_w) * 0.5f;
      for (int i = 0; i < icon_count; ++i) {
        icons[i]->width = icons[i]->height = use_icon;
        icons[i]->x = ix;
        icons[i]->y = row_y + (kBuyH - use_icon) * 0.5f;
        ix += use_icon + icon_gap;
      }
      buy.x = btn_x;
      buy.y = row_y;
      tog.x = tog_x;
      tog.y = row_y + (kBuyH - tog.height) * 0.5f;
    };

    Image* row_p[] = {&icon_p_};
    Image* row_n[] = {&icon_n_};
    Image* row_e[] = {&icon_e_};
    Image* row_pn[] = {&icon_pn_p_, &icon_pn_n_};
    Image* row_all[] = {&icon_all_p_, &icon_all_n_, &icon_all_e_};
    place_row(buy_btns_y0, row_p, 1, buy_p_, auto_p_);
    place_row(buy_btns_y0 + (kBuyH + kBuyGap), row_n, 1, buy_n_, auto_n_);
    place_row(buy_btns_y0 + (kBuyH + kBuyGap) * 2.f, row_e, 1, buy_e_,
              auto_e_);
    place_row(buy_btns_y0 + (kBuyH + kBuyGap) * 3.f, row_pn, 2, buy_pn_,
              auto_pn_);
    place_row(buy_btns_y0 + (kBuyH + kBuyGap) * 4.f, row_all, 3, buy_all_,
              auto_all_);

    const float star_area_top = hud_panel_.y + hud_panel_.height + 8.f;
    const float star_area_bottom = buy_y0 - 8.f;
    const float star_area_h = std::max(80.f, star_area_bottom - star_area_top);
    const float hint_reserve =
        star_hint_dismissed_ ? 0.f : (star_hint_.height + 12.f);

    const bool sn = AscendAvailable();
    float star_size =
        std::min({360.f, center_w - 16.f, star_area_h - hint_reserve - 8.f});
    star_size = std::max(140.f, star_size);
    star_base_size_ = star_size;

    star_center_x_ = w * 0.5f;
    star_center_y_ = h * 0.5f;

    const float half = star_size * 0.5f;
    const float min_y = star_area_top + half;
    const float max_y = star_area_bottom - hint_reserve - half;
    if (min_y <= max_y) {
      star_center_y_ = std::clamp(star_center_y_, min_y, max_y);
    } else {
      star_center_y_ = star_area_top + star_area_h * 0.5f;
    }

    const float min_x = center_x + half + 4.f;
    const float max_x = center_x + center_w - half - 4.f;
    if (min_x <= max_x) {
      star_center_x_ = std::clamp(star_center_x_, min_x, max_x);
    } else {
      star_center_x_ = center_x + center_w * 0.5f;
    }

    if (star_hint_dismissed_) {
      Hide(star_hint_);
    } else {
      star_hint_.width = std::min(420.f, center_w - 8.f);
      star_hint_.x = star_center_x_ - star_hint_.width * 0.5f;
      star_hint_.layer = 2;
      // Keep hint clear of Buy batch / particle buttons.
      const float hint_below = star_center_y_ + half + 8.f;
      const float hint_limit = buy_y0 - star_hint_.height - 6.f;
      if (hint_below <= hint_limit) {
        star_hint_.y = hint_below;
      } else {
        star_hint_.y = star_center_y_ - half - star_hint_.height - 8.f;
        if (star_hint_.y < star_area_top) {
          star_hint_.y = std::max(star_area_top, hint_limit);
        }
      }
    }

    if (sn) {
      // Top-left of the central star block.
      icon_supernova_.width = icon_supernova_.height = 48.f;
      icon_supernova_.x = center_x + 8.f;
      icon_supernova_.y = star_area_top + 4.f;
      supernova_.x = icon_supernova_.x + icon_supernova_.width + 8.f;
      supernova_.y = icon_supernova_.y +
                     (icon_supernova_.height - supernova_.height) * 0.5f;
    } else {
      Hide(icon_supernova_);
      Hide(supernova_);
    }

    // Click-mult indicator: top-right of the central block.
    if (G().auto_click_mult > 1.0 + 1e-9) {
      constexpr float kAutoIcon = 48.f;
      icon_autoclick_.width = icon_autoclick_.height = kAutoIcon;
      icon_autoclick_.x = center_x + center_w - kAutoIcon - 8.f;
      icon_autoclick_.y = star_area_top + 4.f;
      autoclick_label_.text =
          L"x" + game_ui::FormatInt(G().auto_click_mult);
      autoclick_label_.width = std::max(48.f, kAutoIcon + 8.f);
      autoclick_label_.height = 22.f;
      autoclick_label_.x =
          icon_autoclick_.x +
          (kAutoIcon - autoclick_label_.width) * 0.5f;
      autoclick_label_.y = icon_autoclick_.y + kAutoIcon + 2.f;
      autoclick_hit_.width = kAutoIcon + 8.f;
      autoclick_hit_.height = kAutoIcon + autoclick_label_.height + 6.f;
      autoclick_hit_.x = icon_autoclick_.x - 4.f;
      autoclick_hit_.y = icon_autoclick_.y;
      autoclick_hit_.disabled = false;
    } else {
      Hide(icon_autoclick_);
      Hide(autoclick_label_);
      Hide(autoclick_hit_);
      autoclick_hit_.disabled = true;
    }

    // Help "?" sits to the left of the star.
    sn_help_btn_.width = sn_help_btn_.height = 36.f;
    sn_help_btn_.x = star_center_x_ - half - sn_help_btn_.width - 10.f;
    sn_help_btn_.y = star_center_y_ - sn_help_btn_.height * 0.5f;
    if (sn_help_btn_.x < center_x + 4.f) {
      sn_help_btn_.x = center_x + 4.f;
      sn_help_btn_.y = star_center_y_ - half - sn_help_btn_.height - 8.f;
    }

    const float lab_inner_w = std::max(180.f, lab_w - 24.f);
    const float btn_w = std::max(100.f, (lab_inner_w - 8.f) * 0.5f);
    const float btn_gap = 8.f;
    const float craft_x0 = 8.f;
    float y = 8.f;
    constexpr float kCardPad = 10.f;
    constexpr float kCardH = 320.f;
    constexpr float kStockIcon = 64.f;
    constexpr float kCostIcon = 32.f;
    for (int i = 0; i < kMaxElementRows; ++i) {
      auto& row = element_rows_[i];
      const bool active = i < static_cast<int>(G().elements.size()) &&
                          G().elements[i].unlocked;
      if (!active) {
        Hide(row.card);
        Hide(row.icon);
        Hide(row.nuc_icon);
        Hide(row.atom_icon);
        Hide(row.iso_icon);
        Hide(row.nuc_hit);
        Hide(row.atom_hit);
        Hide(row.iso_hit);
        Hide(row.name);
        Hide(row.nuc_count);
        Hide(row.atom_count);
        Hide(row.iso_count);
        Hide(row.stats);
        HideCostFormula(row.cost_nucleus);
        HideCostFormula(row.cost_atom);
        Hide(row.craft_nucleus);
        Hide(row.craft_atom);
        continue;
      }

      row.card.x = 6.f;
      row.card.y = y;
      row.card.width = lab_inner_w + 4.f;
      row.card.height = kCardH;

      row.name.width = lab_inner_w - 90.f;
      row.stats.width = lab_inner_w - 8.f;
      row.craft_nucleus.width = row.craft_atom.width = btn_w;

      row.icon.x = kCardPad;
      row.icon.y = y + kCardPad;
      row.name.x = 90.f;
      row.name.y = y + kCardPad + 4.f;

      // 1) craft costs, 2) stock counts in one row, 3) Iso EPS total
      LayoutCostFormula(row.cost_nucleus, kCardPad, y + 56.f, kCostIcon);
      LayoutCostFormula(row.cost_atom, kCardPad, y + 96.f, kCostIcon);

      const float stock_y = y + 140.f;
      const float stock_slot =
          std::max(100.f, (lab_inner_w - kCardPad * 2.f) / 3.f);
      const float count_w = std::max(36.f, stock_slot - kStockIcon - 6.f);
      row.nuc_icon.width = row.nuc_icon.height = kStockIcon;
      row.atom_icon.width = row.atom_icon.height = kStockIcon;
      row.iso_icon.width = row.iso_icon.height = kStockIcon;
      row.nuc_hit.width = row.nuc_hit.height = kStockIcon;
      row.atom_hit.width = row.atom_hit.height = kStockIcon;
      row.iso_hit.width = row.iso_hit.height = kStockIcon;
      row.nuc_count.width = row.atom_count.width = row.iso_count.width =
          count_w;
      row.nuc_count.height = row.atom_count.height = row.iso_count.height =
          kStockIcon;

      auto place_stock = [&](Image& icon, Button& hit, Label& count,
                             float slot_x) {
        icon.x = slot_x;
        icon.y = stock_y;
        hit.x = slot_x;
        hit.y = stock_y;
        count.x = slot_x + kStockIcon + 4.f;
        count.y = stock_y;
      };
      place_stock(row.nuc_icon, row.nuc_hit, row.nuc_count, kCardPad);
      place_stock(row.atom_icon, row.atom_hit, row.atom_count,
                  kCardPad + stock_slot);
      place_stock(row.iso_icon, row.iso_hit, row.iso_count,
                  kCardPad + stock_slot * 2.f);

      row.stats.x = kCardPad;
      row.stats.y = y + 216.f;

      row.craft_nucleus.x = craft_x0 + 2.f;
      row.craft_nucleus.y = y + 252.f;
      row.craft_atom.x = craft_x0 + btn_w + btn_gap + 2.f;
      row.craft_atom.y = y + 252.f;
      y += kCardH + 12.f;
    }

    const float tech_inner_w = std::max(160.f, tech_w - 24.f);
    const float tech_buy_w = std::min(180.f, tech_inner_w);
    constexpr float kUpIcon = 48.f;
    constexpr float kUpIconGap = 12.f;
    constexpr float kUpBuyH = 44.f;
    constexpr float kUpTitleH = 32.f;
    constexpr float kUpLineH = 20.f;
    constexpr float kUpPad = 12.f;
    y = 8.f;
    for (int i = 0; i < kMaxUpgradeRows; ++i) {
      auto& row = upgrade_rows_[i];
      const bool active =
          i < static_cast<int>(visible_upgrade_indices_.size());
      if (!active) {
        Hide(row.card);
        Hide(row.icon);
        Hide(row.icon_hit);
        Hide(row.title);
        Hide(row.desc);
        Hide(row.cost_label);
        for (int c = 0; c < game_ui::UpgradeRow::kMaxCosts; ++c) {
          Hide(row.costs[c].icon);
          Hide(row.costs[c].amount);
        }
        Hide(row.buy);
        Hide(row.grade_icon);
        Hide(row.grade_label);
        continue;
      }
      const int cost_n = std::max(1, row.cost_count);
      constexpr float kCostIcon = 36.f;
      constexpr float kCostLineH = 36.f;
      constexpr float kGradeIcon = 32.f;
      const float desc_h = kUpLineH;
      const float cost_block_h = kUpLineH + cost_n * kCostLineH;
      const float card_h =
          kUpPad + kUpTitleH + desc_h + cost_block_h + 8.f + kUpBuyH + kUpPad;

      const float text_x = 14.f + kUpIcon + kUpIconGap;
      const float text_w =
          std::max(120.f, tech_inner_w - kUpIcon - kUpIconGap - 8.f);
      row.card.x = 6.f;
      row.card.y = y;
      row.card.width = tech_inner_w + 4.f;
      row.card.height = card_h;
      row.icon.width = row.icon.height = kUpIcon;
      row.icon.x = 14.f;
      row.icon.y = y + kUpPad;
      row.icon_hit.width = row.icon_hit.height = kUpIcon;
      row.icon_hit.x = row.icon.x;
      row.icon_hit.y = row.icon.y;
      row.title.width = text_w - 72.f;
      row.title.height = kUpTitleH;
      row.desc.width = text_w;
      row.desc.height = desc_h;
      row.buy.width = tech_buy_w;
      row.buy.height = kUpBuyH;
      row.title.x = text_x;
      row.title.y = y + kUpPad - 2.f;
      row.desc.x = text_x;
      row.desc.y = y + kUpPad + kUpTitleH;

      row.cost_label.width = 64.f;
      row.cost_label.height = kUpLineH;
      row.cost_label.x = text_x;
      row.cost_label.y = row.desc.y + desc_h;
      for (int c = 0; c < game_ui::UpgradeRow::kMaxCosts; ++c) {
        auto& line = row.costs[c];
        if (c >= row.cost_count) {
          Hide(line.icon);
          Hide(line.amount);
          continue;
        }
        // Same rect height as icon so LeftMiddle centers amount text.
        const float cy = row.cost_label.y + kUpLineH + c * kCostLineH;
        line.icon.width = line.icon.height = kCostIcon;
        line.icon.x = text_x;
        line.icon.y = cy;
        line.amount.width = text_w - kCostIcon - 6.f;
        line.amount.height = kCostLineH;
        line.amount.x = text_x + kCostIcon + 6.f;
        line.amount.y = cy;
      }

      row.buy.x = text_x;
      row.buy.y = row.cost_label.y + cost_block_h + 8.f;

      const int grade =
          (row.upgrade_index >= 0 &&
           row.upgrade_index < static_cast<int>(G().upgrades.size()) &&
           UpgradeUsesGrade(G().upgrades[row.upgrade_index].effect))
              ? UpgradeGrade(G().upgrades[row.upgrade_index].level)
              : 0;
      if (grade > 0) {
        row.grade_icon.width = row.grade_icon.height = kGradeIcon;
        row.grade_icon.x = row.card.x + row.card.width - kGradeIcon - 8.f;
        row.grade_icon.y = y + 8.f;
        row.grade_label.width = 40.f;
        row.grade_label.height = 20.f;
        row.grade_label.x = row.grade_icon.x - 2.f;
        row.grade_label.y = row.grade_icon.y + row.grade_icon.height - 2.f;
      } else {
        Hide(row.grade_icon);
        Hide(row.grade_label);
      }
      y += card_h + 12.f;
    }
  }

  AppState* app_ = nullptr;
  GameAssets assets_{};
  StarCanvasRenderer star_renderer_{};
  Scene canvas_scene_{};
  Scene ui_scene_{};

  Canvas canvas_{};
  Label title_{};
  Label lab_header_{};
  Label tech_header_{};
  Button tech_tab_btn_{};
  Button dtech_tab_btn_{};
  int tech_tab_index_ = 0;
  Label lab_batch_label_{};
  RadioGroup lab_batch_{};
  Label buy_batch_label_{};
  RadioGroup buy_batch_{};
  int craft_batch_index_ = 0;
  int buy_batch_index_ = 0;
  Panel hud_panel_{};
  Image energy_icon_{};
  Label energy_label_{};
  Label star_label_{};
  Label crit_label_{};
  Label eps_label_{};
  Label dust_label_{};
  Button dust_hit_{};
  Image hud_icon_p_{};
  Image hud_icon_n_{};
  Image hud_icon_e_{};
  Label count_p_{};
  Label count_n_{};
  Label count_e_{};

  Button settings_btn_{};
  Button howto_btn_{};
  Button reset_btn_{};
  Button exit_btn_{};

  static constexpr int kHowToTips = 5;
  struct HowToTip {
    Image icon{};
    Text label{};
  };
  Modal howto_modal_{};
  Container howto_body_{};
  Text howto_intro_{};
  HowToTip howto_tips_[kHowToTips]{};
  Image howto_icon_n_{};
  Image howto_icon_e_{};
  Button howto_close_btn_{};

  Modal reset_modal_{};
  Text reset_body_{};
  Button reset_confirm_btn_{};
  Button reset_cancel_btn_{};

  Modal prestige_modal_{};
  Text prestige_body_{};
  Button prestige_confirm_btn_{};
  Button prestige_cancel_btn_{};

  Label star_hint_{};
  bool star_hint_dismissed_ = false;
  float star_base_size_ = 220.f;
  float star_scale_ = 1.f;
  float star_anim_t_ = 0.f;
  float star_idle_t_ = 0.f;
  float star_center_x_ = 0.f;
  float star_center_y_ = 0.f;
  Image icon_p_{};
  Image icon_n_{};
  Image icon_e_{};
  Button buy_p_{};
  Button buy_n_{};
  Button buy_e_{};
  Toggle auto_p_{};
  Toggle auto_n_{};
  Toggle auto_e_{};
  Image icon_pn_p_{};
  Image icon_pn_n_{};
  Button buy_pn_{};
  Toggle auto_pn_{};
  Image icon_all_p_{};
  Image icon_all_n_{};
  Image icon_all_e_{};
  Button buy_all_{};
  Toggle auto_all_{};
  bool auto_tog_p_ = false;
  bool auto_tog_n_ = false;
  bool auto_tog_e_ = false;
  bool auto_tog_pn_ = false;
  bool auto_tog_all_ = false;
  float auto_buy_accum_ = 0.f;
  Image icon_supernova_{};
  Button supernova_{};
  Image icon_autoclick_{};
  Label autoclick_label_{};
  Button autoclick_hit_{};
  Button sn_help_btn_{};
  bool supernova_visible_ = false;
  bool supernova_armed_ = false;
  float supernova_arm_t_ = 0.f;
  int unlocked_count_ = 0;
  int isotope_discovered_count_ = 0;
  float hud_refresh_accum_ = 0.f;
  bool hud_force_refresh_ = true;

  ScrollView lab_scroll_{};
  std::vector<game_ui::ElementRow> element_rows_;

  ScrollView tech_scroll_{};
  std::vector<game_ui::UpgradeRow> upgrade_rows_;
  std::vector<int> visible_upgrade_indices_;
  std::vector<game_ui::FloatText> float_texts_;
  Panel tech_tip_panel_{};
  Text tech_tip_label_{};

  static constexpr float kDelayedTipSec = 1.f;
  DelayedTipKind delayed_tip_kind_ = DelayedTipKind::None;
  int delayed_tip_row_ = -1;
  float delayed_tip_t_ = 0.f;
  bool delayed_tip_armed_ = false;
  std::wstring delayed_tip_text_{};
  float delayed_tip_w_ = 280.f;
  float delayed_tip_h_ = 64.f;
};

}  // namespace

ScenePtr CreateGameScene() {
  return std::make_unique<GameScene>();
}
