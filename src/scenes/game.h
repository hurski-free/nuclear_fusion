#pragma once

#include "../game/star_canvas_renderer.h"

#include "scene_base.h"
#include "../console_theme.h"
#include "../game/game_assets.h"
#include "../game/game_save.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <sstream>
#include <string>
#include <vector>

namespace game_ui {

inline std::wstring FormatNum(double v) {
  wchar_t buf[64] = {};
  if (v >= 1.0e9) {
    std::swprintf(buf, 64, L"%.2fe9", v / 1.0e9);
  } else if (v >= 1.0e6) {
    std::swprintf(buf, 64, L"%.2fe6", v / 1.0e6);
  } else if (v >= 1000.0) {
    std::swprintf(buf, 64, L"%.1f", v);
  } else if (v >= 10.0) {
    std::swprintf(buf, 64, L"%.1f", v);
  } else {
    std::swprintf(buf, 64, L"%.2f", v);
  }
  return buf;
}

inline void ClearImageStyle(Image& img) {
  img.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  img.style_hovered = img.style_base;
  img.style_active = img.style_base;
  img.style_disabled = img.style_base;
  img.transition = {};
  img.tint = {1.f, 1.f, 1.f, 1.f};
}

struct ElementRow {
  Image icon{};
  Image iso_icon{};
  Label name{};
  Label stats{};
  Label cost_nucleus{};
  Label cost_atom{};
  Button craft_nucleus{};
  Button craft_atom{};
  Button annihilate{};
  Button invest{};
  int element_index = -1;
};

struct UpgradeRow {
  Label title{};
  Label desc{};
  Button buy{};
  int upgrade_index = -1;
};

}  // namespace game_ui

class GameScene : public IScene {
public:
  void on_enter(AppState& app) override {
    app_ = &app;
    if (!app.game_initialized) {
      app.game.InitNewGame();
      LoadGame(app.game);
      app.game_initialized = true;
    }
    assets_.Load(app.ctx);
    Rebuild(app);
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
    // Layout alone would place every tab's controls; re-apply visibility
    // so only the active Star/Lab/Tech block stays on screen.
    if (app_) {
      ApplyTabVisibility();
    } else {
      Layout(width, height);
    }
    canvas_.ensure_targets(app_ ? app_->ctx : nullptr);
    star_renderer_.Resize(width, height);
  }

  void update(AppState& app, float dt) override {
    app.game.Tick(dt);
    UpdateStarAnimation(dt);
    star_renderer_.Update(dt);
    RefreshLabels();
    RefreshDynamicStyles();
  }

  Scene& scene() override { return ui_scene_; }

  void handle_messages(UiContext* ctx) override {
    HandleStarClick(ctx);
    ui_scene_.handle_messages(ctx);
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
  static constexpr int kMaxElementRows = 8;
  static constexpr int kMaxUpgradeRows = 12;

  GameState& G() { return app_->game; }
  const GameState& G() const { return app_->game; }

  void StyleHudLabel(Label& label, float size = 20.f) {
    label.font_size = size;
    label.color = {0.55f, 1.f, 0.65f, 1.f};
    label.layer = 0;
    label.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
    label.style_hovered = label.style_base;
    label.style_active = label.style_base;
    label.style_disabled = label.style_base;
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
    const bool show_star = G().tab == GameTab::Star;
    star_renderer_.SetStarType(G().star_type);
    star_renderer_.Draw(d3d, star_center_x_, star_center_y_, size, show_star);
    canvas_.end_draw(ctx);
  }

  void Rebuild(AppState& app) {
    canvas_scene_ = Scene{};
    ui_scene_ = Scene{};
    element_rows_.assign(kMaxElementRows, {});
    upgrade_rows_.assign(kMaxUpgradeRows, {});

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

    hud_panel_.title.clear();
    hud_panel_.draggable = false;
    hud_panel_.closable = false;
    hud_panel_.layer = 0;
    hud_panel_.width = 720.f;
    hud_panel_.height = 148.f;
    hud_panel_.title_height = 0.f;
    hud_panel_.style_base = {
        Color{0.02f, 0.08f, 0.04f, 0.88f},
        Border{2.f, BorderMode::In, Color{0.3f, 0.85f, 0.4f, 1.f}}};
    hud_panel_.style_hovered = hud_panel_.style_base;
    hud_panel_.style_active = hud_panel_.style_base;

    title_.text = L"Nuclear Fusion";
    StyleHudLabel(title_, 32.f);
    title_.width = 420.f;
    title_.height = 40.f;

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

    StyleHudLabel(energy_label_, 30.f);
    energy_label_.width = 520.f;
    energy_label_.height = 36.f;
    StyleHudLabel(eps_label_, 20.f);
    eps_label_.width = 620.f;
    eps_label_.height = 26.f;
    StyleHudLabel(count_p_, 18.f);
    count_p_.width = 160.f;
    count_p_.height = 24.f;
    StyleHudLabel(count_n_, 18.f);
    count_n_.width = 160.f;
    count_n_.height = 24.f;
    StyleHudLabel(count_e_, 18.f);
    count_e_.width = 160.f;
    count_e_.height = 24.f;
    StyleHudLabel(star_label_, 16.f);
    star_label_.width = 420.f;
    star_label_.height = 22.f;

    SetupTabButton(tab_star_, L"Star", assets_.StarIcon(G().star_type),
                   GameTab::Star);
    SetupTabButton(tab_lab_, L"Lab", assets_.lab, GameTab::Lab);
    SetupTabButton(tab_tech_, L"Tech", assets_.tech, GameTab::Tech);

    ApplyConsoleButtonStyle(settings_btn_, 140.f, 40.f);
    settings_btn_.text = L"Settings";
    settings_btn_.layer = 1;
    settings_btn_.on_click = [this]() {
      if (app_) {
        SaveGame(app_->game);
        RequestScene(*app_, SceneId::Settings);
      }
    };

    ApplyConsoleButtonStyle(exit_btn_, 120.f, 40.f);
    exit_btn_.text = L"Exit";
    exit_btn_.layer = 1;
    exit_btn_.on_click = [this]() {
      if (app_) {
        SaveGame(app_->game);
        app_->request_quit = true;
      }
    };

    star_base_size_ = 168.f;
    star_scale_ = 1.f;
    star_anim_t_ = 0.f;

    StyleHudLabel(star_hint_, 16.f);
    star_hint_.text = L"Click the star to gain energy";
    star_hint_.width = 360.f;
    star_hint_.height = 24.f;

    SetupBuyControl(icon_p_, buy_p_, assets_.proton, ResourceKind::Proton);
    SetupBuyControl(icon_n_, buy_n_, assets_.neutron, ResourceKind::Neutron);
    SetupBuyControl(icon_e_, buy_e_, assets_.electron, ResourceKind::Electron);

    game_ui::ClearImageStyle(icon_supernova_);
    icon_supernova_.texture_id = assets_.supernova;
    icon_supernova_.width = icon_supernova_.height = 40.f;
    icon_supernova_.layer = 1;

    ApplyConsoleButtonStyle(supernova_, 220.f, 44.f);
    supernova_.text = L"Supernova";
    supernova_.layer = 1;
    supernova_.on_click = [this]() {
      if (G().TriggerSupernova()) {
        SaveGame(G());
        RefreshElementRows();
        RefreshUpgradeRows();
      }
    };

    lab_scroll_.width = 900.f;
    lab_scroll_.height = 480.f;
    lab_scroll_.scroll_y.mode = ScrollMode::Auto;
    lab_scroll_.style_base = {
        Color{0.02f, 0.06f, 0.03f, 0.75f},
        Border{1.5f, BorderMode::In, Color{0.25f, 0.7f, 0.35f, 1.f}}};
    lab_scroll_.style_hovered = lab_scroll_.style_base;
    lab_scroll_.style_active = lab_scroll_.style_base;

    for (int i = 0; i < kMaxElementRows; ++i) {
      auto& row = element_rows_[i];
      row.element_index = i;
      game_ui::ClearImageStyle(row.icon);
      row.icon.width = row.icon.height = 48.f;
      game_ui::ClearImageStyle(row.iso_icon);
      row.iso_icon.width = row.iso_icon.height = 36.f;

      row.name.font_size = 24.f;
      row.name.color = {0.65f, 1.f, 0.72f, 1.f};
      row.name.width = 280.f;
      row.name.height = 32.f;
      row.stats.font_size = 17.f;
      row.stats.color = {0.55f, 0.95f, 0.65f, 1.f};
      row.stats.width = 820.f;
      row.stats.height = 26.f;
      row.cost_nucleus.font_size = 16.f;
      row.cost_nucleus.color = {0.75f, 1.f, 0.8f, 1.f};
      row.cost_nucleus.width = 820.f;
      row.cost_nucleus.height = 24.f;
      row.cost_atom.font_size = 16.f;
      row.cost_atom.color = {0.7f, 0.95f, 0.78f, 1.f};
      row.cost_atom.width = 820.f;
      row.cost_atom.height = 24.f;

      ApplyConsoleButtonStyle(row.craft_nucleus, 150.f, 36.f);
      row.craft_nucleus.text = L"Nucleus";
      row.craft_nucleus.on_click = [this, i]() {
        if (i < static_cast<int>(G().elements.size())) {
          G().CraftNucleus(G().elements[i], 0);
        }
      };

      ApplyConsoleButtonStyle(row.craft_atom, 150.f, 36.f);
      row.craft_atom.text = L"Atom";
      row.craft_atom.on_click = [this, i]() {
        if (i < static_cast<int>(G().elements.size())) {
          G().CraftAtom(G().elements[i], 0);
        }
      };

      ApplyConsoleButtonStyle(row.annihilate, 150.f, 36.f);
      row.annihilate.text = L"Annihilate";
      row.annihilate.on_click = [this, i]() {
        if (i < static_cast<int>(G().elements.size())) {
          G().AnnihilateIsotopes(G().elements[i], 0);
        }
      };

      ApplyConsoleButtonStyle(row.invest, 150.f, 36.f);
      row.invest.text = L"Invest";
      row.invest.on_click = [this, i]() {
        if (i < static_cast<int>(G().elements.size())) {
          G().InvestIsotopes(G().elements[i], 0);
        }
      };

      lab_scroll_.components.push_back(&row.icon);
      lab_scroll_.components.push_back(&row.iso_icon);
      lab_scroll_.components.push_back(&row.name);
      lab_scroll_.components.push_back(&row.stats);
      lab_scroll_.components.push_back(&row.cost_nucleus);
      lab_scroll_.components.push_back(&row.cost_atom);
      lab_scroll_.components.push_back(&row.craft_nucleus);
      lab_scroll_.components.push_back(&row.craft_atom);
      lab_scroll_.components.push_back(&row.annihilate);
      lab_scroll_.components.push_back(&row.invest);
    }

    tech_scroll_.width = 900.f;
    tech_scroll_.height = 520.f;
    tech_scroll_.scroll_y.mode = ScrollMode::Auto;
    tech_scroll_.layer = 1;
    tech_scroll_.style_base = {
        Color{0.02f, 0.06f, 0.03f, 0.75f},
        Border{1.5f, BorderMode::In, Color{0.25f, 0.7f, 0.35f, 1.f}}};
    tech_scroll_.style_hovered = tech_scroll_.style_base;
    tech_scroll_.style_active = tech_scroll_.style_base;

    for (int i = 0; i < kMaxUpgradeRows; ++i) {
      auto& row = upgrade_rows_[i];
      row.upgrade_index = i;
      row.title.font_size = 24.f;
      row.title.color = {0.65f, 1.f, 0.72f, 1.f};
      row.title.width = 700.f;
      row.title.height = 32.f;
      row.desc.font_size = 17.f;
      row.desc.color = {0.55f, 0.95f, 0.65f, 1.f};
      row.desc.width = 700.f;
      row.desc.height = 44.f;
      ApplyConsoleButtonStyle(row.buy, 170.f, 44.f);
      row.buy.text = L"Buy";
      row.buy.on_click = [this, i]() {
        if (i < static_cast<int>(upgrade_rows_.size())) {
          const int up_i = upgrade_rows_[i].upgrade_index;
          if (up_i >= 0 && up_i < static_cast<int>(G().upgrades.size())) {
            if (G().BuyUpgrade(G().upgrades[up_i])) {
              SaveGame(G());
              RefreshUpgradeRows();
            }
          }
        }
      };
      tech_scroll_.components.push_back(&row.title);
      tech_scroll_.components.push_back(&row.desc);
      tech_scroll_.components.push_back(&row.buy);
    }

    RefreshElementRows();
    RefreshUpgradeRows();
    RefreshLabels();
    Layout(w, h);
    ApplyTabVisibility();

    // Canvas is the bottom-most visual layer (drawn first via its own scene).
    canvas_scene_.components = {&canvas_};
    canvas_scene_.prepare_scene();

    ui_scene_.components = {
        &hud_panel_,     &title_,          &energy_icon_,    &energy_label_,
        &eps_label_,     &hud_icon_p_,     &count_p_,        &hud_icon_n_,
        &count_n_,       &hud_icon_e_,     &count_e_,        &star_label_,
        &tab_star_icon_, &tab_star_,       &tab_lab_icon_,   &tab_lab_,
        &tab_tech_icon_, &tab_tech_,       &settings_btn_,   &exit_btn_,
        &star_hint_,     &icon_p_,         &buy_p_,          &icon_n_,
        &buy_n_,         &icon_e_,         &buy_e_,          &icon_supernova_,
        &supernova_,     &lab_scroll_,     &tech_scroll_,
    };
    ui_scene_.prepare_scene();
  }

  void SetupTabButton(Button& btn, const wchar_t* text, int /*tex*/,
                      GameTab tab) {
    ApplyConsoleButtonStyle(btn, 120.f, 40.f);
    btn.text = text;
    btn.layer = 1;
    btn.on_click = [this, tab]() {
      G().tab = tab;
      ApplyTabVisibility();
    };
  }

  void SetupBuyControl(Image& icon, Button& buy, int tex, ResourceKind kind) {
    game_ui::ClearImageStyle(icon);
    icon.texture_id = tex;
    icon.width = icon.height = 40.f;
    icon.layer = 1;

    ApplyConsoleButtonStyle(buy, 240.f, 44.f);
    buy.font_size = 18.f;
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

  std::wstring ParticleBuyLabel(const wchar_t* name, ResourceKind kind) const {
    return std::wstring(L"Buy ") + name + L" (" +
           game_ui::FormatNum(ParticleUnitPrice(kind)) + L" eV)";
  }

  bool CanBuyParticle(ResourceKind kind) const {
    return G().energy + 1e-9 >= ParticleUnitPrice(kind);
  }

  void BuyParticle(ResourceKind kind) {
    if (kind == ResourceKind::Proton) {
      G().MaterializeProton(1.0);
    } else if (kind == ResourceKind::Neutron) {
      G().MaterializeNeutron(1.0);
    } else {
      G().MaterializeElectron(1.0);
    }
  }

  void HandleStarClick(UiContext* ctx) {
    if (!app_ || G().tab != GameTab::Star || !ctx) {
      return;
    }
    const MouseEvents* mouse = ui_get_mouse_events(ctx);
    if (!mouse || !mouse->left_pressed) {
      return;
    }

    const float size = star_base_size_ * star_scale_;
    const float cx = star_center_x_;
    const float cy = star_center_y_;
    const float radius = size * 0.48f;
    const float dx = mouse->x - cx;
    const float dy = mouse->y - cy;
    if (dx * dx + dy * dy <= radius * radius) {
      G().ClickStar();
      star_anim_t_ = 1.f;
    }
  }

  void UpdateStarAnimation(float dt) {
    if (star_anim_t_ > 0.f) {
      star_anim_t_ = std::max(0.f, star_anim_t_ - dt / 0.18f);
    }
    // Ease: shrink to 85% at peak, then restore.
    const float wave = std::sin(star_anim_t_ * 3.14159265f);
    star_scale_ = 1.f - 0.15f * wave;
  }

  void RefreshElementRows() {
    for (int i = 0; i < kMaxElementRows; ++i) {
      auto& row = element_rows_[i];
      const bool active = i < static_cast<int>(G().elements.size()) &&
                          G().elements[i].unlocked;
      row.icon.disabled = !active;
      row.iso_icon.disabled = !active;
      row.name.disabled = !active;
      row.stats.disabled = !active;
      row.cost_nucleus.disabled = !active;
      row.cost_atom.disabled = !active;
      if (!active) {
        row.craft_nucleus.disabled = true;
        row.craft_atom.disabled = true;
        row.annihilate.disabled = true;
        row.invest.disabled = true;
        row.name.text.clear();
        row.stats.text.clear();
        row.cost_nucleus.text.clear();
        row.cost_atom.text.clear();
        row.icon.texture_id = -1;
        row.iso_icon.texture_id = -1;
        continue;
      }

      const auto& el = G().elements[i];
      row.icon.texture_id = assets_.ElementIcon(el.id);
      if (el.isotope_discovered) {
        row.iso_icon.texture_id = assets_.IsotopeIcon(el.id);
        row.iso_icon.disabled = false;
      } else {
        row.iso_icon.texture_id = -1;
        row.iso_icon.disabled = true;
      }
      row.name.text = el.name;

      std::wstringstream ss;
      ss << L"Nuc " << game_ui::FormatNum(el.nucleus_count) << L" | Atom "
         << game_ui::FormatNum(el.atom_count) << L" | Iso "
         << game_ui::FormatNum(el.isotope_count) << L" | Mut "
         << game_ui::FormatNum(G().EffectiveMutationChance(el) * 100.0) << L"%"
         << L" | Keff " << game_ui::FormatNum(G().EffectiveKeff(el));
      row.stats.text = ss.str();

      const double e_nuc = G().ActivationEnergy(el);
      const double e_atom = e_nuc * 0.35;
      const double refund =
          el.atomic_number <= kIronAtomicNumber ? G().EffectiveKeff(el) : 0.0;

      std::wstringstream cost_n;
      cost_n << L"Nucleus price: " << el.protons_needed << L"p + "
             << el.neutrons_needed << L"n + " << game_ui::FormatNum(e_nuc)
             << L" eV";
      if (refund > 0.0) {
        cost_n << L"  (energy refund x" << game_ui::FormatNum(refund) << L")";
      }
      row.cost_nucleus.text = cost_n.str();

      std::wstringstream cost_a;
      cost_a << L"Atom price: 1 nucleus + " << el.electrons_needed << L"e + "
             << game_ui::FormatNum(e_atom) << L" eV | Annihilate payout: "
             << game_ui::FormatNum(G().CraftEnergyCost(el) * G().annihilation_mult)
             << L" eV / isotope";
      row.cost_atom.text = cost_a.str();

      const int iso_available = static_cast<int>(el.isotope_count);
      row.craft_nucleus.disabled =
          G().ResolveBatchCount(G().MaxNucleusCraft(el)) <= 0;
      row.craft_atom.disabled =
          G().ResolveBatchCount(G().MaxAtomCraft(el)) <= 0;
      row.annihilate.disabled = G().ResolveBatchCount(iso_available) <= 0;
      row.invest.disabled = G().ResolveBatchCount(iso_available) <= 0;
    }
  }

  void RefreshUpgradeRows() {
    // Map visible upgrades into fixed UI rows (hide isotope-gated tech).
    visible_upgrade_indices_.clear();
    for (int i = 0; i < static_cast<int>(G().upgrades.size()); ++i) {
      if (G().IsUpgradeVisible(G().upgrades[i])) {
        visible_upgrade_indices_.push_back(i);
      }
    }

    for (int row_i = 0; row_i < kMaxUpgradeRows; ++row_i) {
      auto& row = upgrade_rows_[row_i];
      const bool active =
          row_i < static_cast<int>(visible_upgrade_indices_.size());
      row.title.disabled = !active;
      row.desc.disabled = !active;
      row.buy.disabled = !active;
      if (!active) {
        row.title.text.clear();
        row.desc.text.clear();
        row.upgrade_index = -1;
        continue;
      }

      const int up_i = visible_upgrade_indices_[row_i];
      row.upgrade_index = up_i;
      const auto& up = G().upgrades[up_i];
      std::wstringstream title;
      title << up.name << L"  [" << up.level << L"/" << up.max_level << L"]";
      row.title.text = title.str();

      std::wstringstream desc;
      desc << up.description << L"  Cost: ";
      for (size_t c = 0; c < up.base_costs.size(); ++c) {
        if (c) {
          desc << L", ";
        }
        const auto& cost = up.base_costs[c];
        const double amt = G().ScaledCostAmount(up, cost);
        const double have = G().ResourceAmount(cost);
        const std::wstring el_id(cost.element_id.begin(), cost.element_id.end());
        desc << game_ui::FormatNum(amt) << L"/";
        desc << game_ui::FormatNum(have) << L" ";
        switch (cost.kind) {
          case ResourceKind::Energy:
            desc << L"E";
            break;
          case ResourceKind::Proton:
            desc << L"p";
            break;
          case ResourceKind::Neutron:
            desc << L"n";
            break;
          case ResourceKind::Electron:
            desc << L"e";
            break;
          case ResourceKind::Nucleus:
            desc << L"nuc(" << el_id << L")";
            break;
          case ResourceKind::Atom:
            desc << L"atom(" << el_id << L")";
            break;
          case ResourceKind::Isotope:
            desc << L"iso(" << el_id << L")";
            break;
          case ResourceKind::StarDust:
            desc << L"dust";
            break;
        }
      }
      row.desc.text = desc.str();
      row.buy.disabled = !G().CanAffordUpgrade(up);
    }
  }

  void RefreshLabels() {
    if (!app_) {
      return;
    }
    G().craft_batch = CraftBatch::x1;

    energy_label_.text =
        L"Energy: " + game_ui::FormatNum(G().energy) + L" eV";
    eps_label_.text = L"EPS: " + game_ui::FormatNum(G().Eps()) + L"/s   Click: " +
                      game_ui::FormatNum(G().click_power) + L" eV   Dust: " +
                      game_ui::FormatNum(G().star_dust);
    count_p_.text = L"p  " + game_ui::FormatNum(G().protons);
    count_n_.text = L"n  " + game_ui::FormatNum(G().neutrons);
    count_e_.text = L"e  " + game_ui::FormatNum(G().electrons);
    star_label_.text = std::wstring(L"Star: ") + StarTypeName(G().star_type);
    tab_star_icon_.texture_id = assets_.StarIcon(G().star_type);

    buy_p_.text = ParticleBuyLabel(L"p", ResourceKind::Proton);
    buy_n_.text = ParticleBuyLabel(L"n", ResourceKind::Neutron);
    buy_e_.text = ParticleBuyLabel(L"e", ResourceKind::Electron);

    RefreshElementRows();
    if (G().tab == GameTab::Tech) {
      RefreshUpgradeRows();
    }
  }

  void RefreshDynamicStyles() {
    if (!app_) {
      return;
    }
    const bool star = G().tab == GameTab::Star;
    buy_p_.disabled = !star || !CanBuyParticle(ResourceKind::Proton);
    buy_n_.disabled = !star || !CanBuyParticle(ResourceKind::Neutron);
    buy_e_.disabled = !star || !CanBuyParticle(ResourceKind::Electron);
    supernova_.disabled = !star || !G().CanTriggerSupernova() ||
                          G().star_type == StarType::NeutronStar;
  }

  void Hide(Component& c) { c.x = -4000.f; }

  void ApplyTabVisibility() {
    const bool star = G().tab == GameTab::Star;
    const bool lab = G().tab == GameTab::Lab;
    const bool tech = G().tab == GameTab::Tech;

    lab_scroll_.disabled = !lab;
    tech_scroll_.disabled = !tech;

    if (app_) {
      Layout(ui_get_width(app_->ctx), ui_get_height(app_->ctx));
      RefreshDynamicStyles();
      if (!star) {
        Hide(star_hint_);
        Hide(icon_p_);
        Hide(buy_p_);
        Hide(icon_n_);
        Hide(buy_n_);
        Hide(icon_e_);
        Hide(buy_e_);
        Hide(icon_supernova_);
        Hide(supernova_);
      }
      if (!lab) {
        Hide(lab_scroll_);
      }
      if (!tech) {
        Hide(tech_scroll_);
      }
      canvas_scene_.prepare_scene();
      ui_scene_.prepare_scene();
    }
  }

  void Layout(int width, int height) {
    const float w = static_cast<float>(width > 0 ? width : 1280);
    const float h = static_cast<float>(height > 0 ? height : 720);

    constexpr float kMargin = 16.f;
    constexpr float kGap = 12.f;
    constexpr float kTabW = 120.f;
    constexpr float kTabIcon = 28.f;
    constexpr float kTabGap = 16.f;

    canvas_.x = 0.f;
    canvas_.y = 0.f;
    canvas_.width = w;
    canvas_.height = h;

    settings_btn_.x = kMargin;
    settings_btn_.y = h - 56.f;
    exit_btn_.x = settings_btn_.x + settings_btn_.width + kGap;
    exit_btn_.y = settings_btn_.y;
    const float bottom_ui = settings_btn_.y - kGap;

    // Tabs anchored top-right without colliding with each other.
    const float tab_y = 16.f;
    tab_tech_.width = tab_lab_.width = tab_star_.width = kTabW;
    tab_tech_.x = w - kMargin - kTabW;
    tab_tech_.y = tab_y;
    tab_lab_.x = tab_tech_.x - kTabGap - kTabIcon - 6.f - kTabW;
    tab_lab_.y = tab_y;
    tab_star_.x = tab_lab_.x - kTabGap - kTabIcon - 6.f - kTabW;
    tab_star_.y = tab_y;

    tab_star_icon_.texture_id = assets_.StarIcon(G().star_type);
    tab_lab_icon_.texture_id = assets_.lab;
    tab_tech_icon_.texture_id = assets_.tech;
    game_ui::ClearImageStyle(tab_star_icon_);
    game_ui::ClearImageStyle(tab_lab_icon_);
    game_ui::ClearImageStyle(tab_tech_icon_);
    tab_star_icon_.width = tab_star_icon_.height = kTabIcon;
    tab_lab_icon_.width = tab_lab_icon_.height = kTabIcon;
    tab_tech_icon_.width = tab_tech_icon_.height = kTabIcon;
    tab_star_icon_.layer = tab_lab_icon_.layer = tab_tech_icon_.layer = 1;
    tab_star_icon_.x = tab_star_.x - kTabIcon - 6.f;
    tab_star_icon_.y = tab_y + 6.f;
    tab_lab_icon_.x = tab_lab_.x - kTabIcon - 6.f;
    tab_lab_icon_.y = tab_y + 6.f;
    tab_tech_icon_.x = tab_tech_.x - kTabIcon - 6.f;
    tab_tech_icon_.y = tab_y + 6.f;

    title_.x = kMargin;
    title_.y = 16.f;
    title_.width = std::max(180.f, tab_star_icon_.x - kMargin - kGap);

    // Energy HUD shares the same left edge as Lab/Tech frames.
    const float content_left = kMargin;
    const float tabs_left = tab_star_icon_.x;
    hud_panel_.x = content_left;
    hud_panel_.y = 56.f;
    hud_panel_.width =
        std::clamp(tabs_left - content_left - kGap, 420.f, 800.f);
    hud_panel_.height = 148.f;
    const float content_top = hud_panel_.y + hud_panel_.height + kGap;
    const float content_h = std::max(160.f, bottom_ui - content_top);
    const float content_w = w - content_left * 2.f;

    energy_icon_.x = content_left + 12.f;
    energy_icon_.y = hud_panel_.y + 12.f;
    energy_label_.x = energy_icon_.x + energy_icon_.width + 8.f;
    energy_label_.y = hud_panel_.y + 10.f;
    energy_label_.width = std::max(160.f, hud_panel_.width * 0.55f);
    star_label_.x = energy_label_.x + energy_label_.width + 8.f;
    star_label_.y = hud_panel_.y + 14.f;
    star_label_.width =
        std::max(120.f, hud_panel_.width - (star_label_.x - content_left) - 12.f);
    eps_label_.x = content_left + 12.f;
    eps_label_.y = hud_panel_.y + 52.f;
    eps_label_.width = hud_panel_.width - 24.f;

    const float count_y = hud_panel_.y + 96.f;
    const float count_slot = std::max(120.f, (hud_panel_.width - 24.f) / 3.f);
    hud_icon_p_.x = content_left + 12.f;
    hud_icon_p_.y = count_y;
    count_p_.x = hud_icon_p_.x + 28.f;
    count_p_.y = count_y;
    count_p_.width = count_slot - 36.f;
    hud_icon_n_.x = content_left + 12.f + count_slot;
    hud_icon_n_.y = count_y;
    count_n_.x = hud_icon_n_.x + 28.f;
    count_n_.y = count_y;
    count_n_.width = count_slot - 36.f;
    hud_icon_e_.x = content_left + 12.f + count_slot * 2.f;
    hud_icon_e_.y = count_y;
    count_e_.x = hud_icon_e_.x + 28.f;
    count_e_.y = count_y;
    count_e_.width = count_slot - 36.f;

    // Center star + buy controls in free area under HUD / above bottom buttons.
    const float buy_row_h = 44.f;
    const float buy_gap = 16.f;
    float buy_btn_w = buy_p_.width;
    float buy_group_w = 40.f + 8.f + buy_btn_w + buy_gap + 40.f + 8.f +
                        buy_btn_w + buy_gap + 40.f + 8.f + buy_btn_w;
    if (buy_group_w > content_w) {
      buy_btn_w = std::max(140.f, (content_w - 2.f * buy_gap - 3.f * 48.f) / 3.f);
      buy_p_.width = buy_n_.width = buy_e_.width = buy_btn_w;
      buy_group_w = 40.f + 8.f + buy_btn_w + buy_gap + 40.f + 8.f + buy_btn_w +
                    buy_gap + 40.f + 8.f + buy_btn_w;
    }

    constexpr float kStarDesired = 168.f;
    float star_size = kStarDesired;
    float block_h = star_size + 8.f + star_hint_.height + 28.f + buy_row_h +
                    20.f + 44.f;
    if (block_h > content_h) {
      star_size = std::max(96.f, kStarDesired - (block_h - content_h));
      block_h = star_size + 8.f + star_hint_.height + 28.f + buy_row_h + 20.f +
                44.f;
    }
    star_base_size_ = star_size;

    float block_top = content_top + (content_h - block_h) * 0.5f;
    if (bottom_ui - block_h < content_top) {
      block_top = content_top;
    } else {
      block_top = std::clamp(block_top, content_top, bottom_ui - block_h);
    }

    star_center_x_ = w * 0.5f;
    star_center_y_ = block_top + star_size * 0.5f;
    star_hint_.x = star_center_x_ - star_hint_.width * 0.5f;
    star_hint_.y = star_center_y_ + star_size * 0.5f + 8.f;

    const float buy_y = star_hint_.y + 28.f;
    float buy_x = star_center_x_ - buy_group_w * 0.5f;
    buy_x = std::clamp(buy_x, content_left, w - content_left - buy_group_w);

    icon_p_.x = buy_x;
    icon_p_.y = buy_y + 2.f;
    buy_p_.x = icon_p_.x + 48.f;
    buy_p_.y = buy_y;
    buy_x = buy_p_.x + buy_p_.width + buy_gap;

    icon_n_.x = buy_x;
    icon_n_.y = buy_y + 2.f;
    buy_n_.x = icon_n_.x + 48.f;
    buy_n_.y = buy_y;
    buy_x = buy_n_.x + buy_n_.width + buy_gap;

    icon_e_.x = buy_x;
    icon_e_.y = buy_y + 2.f;
    buy_e_.x = icon_e_.x + 48.f;
    buy_e_.y = buy_y;

    const float sn_w = 40.f + 8.f + supernova_.width;
    icon_supernova_.x = star_center_x_ - sn_w * 0.5f;
    icon_supernova_.y = buy_y + buy_row_h + 20.f;
    supernova_.x = icon_supernova_.x + 48.f;
    supernova_.y = icon_supernova_.y - 2.f;

    // Lab / Tech frames: same left edge and width band as energy HUD.
    lab_scroll_.x = content_left;
    lab_scroll_.y = content_top;
    lab_scroll_.width = content_w;
    lab_scroll_.height = content_h;

    tech_scroll_.x = content_left;
    tech_scroll_.y = content_top;
    tech_scroll_.width = content_w;
    tech_scroll_.height = content_h;

    const float inner_text_w = std::max(200.f, content_w - 24.f);
    const float btn_w = 150.f;
    const float btn_gap = 12.f;
    const float craft_x0 = 8.f;

    float y = 8.f;
    for (int i = 0; i < kMaxElementRows; ++i) {
      auto& row = element_rows_[i];
      const bool active = i < static_cast<int>(G().elements.size()) &&
                          G().elements[i].unlocked;
      if (!active) {
        Hide(row.icon);
        Hide(row.iso_icon);
        Hide(row.name);
        Hide(row.stats);
        Hide(row.cost_nucleus);
        Hide(row.cost_atom);
        Hide(row.craft_nucleus);
        Hide(row.craft_atom);
        Hide(row.annihilate);
        Hide(row.invest);
        continue;
      }
      row.stats.width = inner_text_w - 110.f;
      row.cost_nucleus.width = inner_text_w - 110.f;
      row.cost_atom.width = inner_text_w - 110.f;
      row.craft_nucleus.width = row.craft_atom.width = row.annihilate.width =
          row.invest.width = btn_w;

      row.icon.x = 8.f;
      row.icon.y = y + 4.f;
      if (G().elements[i].isotope_discovered) {
        row.iso_icon.x = 62.f;
        row.iso_icon.y = y + 10.f;
      } else {
        Hide(row.iso_icon);
      }
      row.name.x = 110.f;
      row.name.y = y;
      row.stats.x = 110.f;
      row.stats.y = y + 34.f;
      row.cost_nucleus.x = 110.f;
      row.cost_nucleus.y = y + 62.f;
      row.cost_atom.x = 110.f;
      row.cost_atom.y = y + 88.f;
      row.craft_nucleus.x = craft_x0;
      row.craft_nucleus.y = y + 120.f;
      row.craft_atom.x = craft_x0 + (btn_w + btn_gap);
      row.craft_atom.y = y + 120.f;
      row.annihilate.x = craft_x0 + (btn_w + btn_gap) * 2.f;
      row.annihilate.y = y + 120.f;
      row.invest.x = craft_x0 + (btn_w + btn_gap) * 3.f;
      row.invest.y = y + 120.f;
      y += 180.f;
    }

    const float tech_buy_w = 170.f;
    const float tech_text_w =
        std::max(180.f, content_w - tech_buy_w - 32.f);
    y = 8.f;
    for (int i = 0; i < kMaxUpgradeRows; ++i) {
      auto& row = upgrade_rows_[i];
      const bool active =
          i < static_cast<int>(visible_upgrade_indices_.size());
      if (!active) {
        Hide(row.title);
        Hide(row.desc);
        Hide(row.buy);
        continue;
      }
      row.title.width = tech_text_w;
      row.desc.width = tech_text_w;
      row.buy.width = tech_buy_w;
      row.title.x = 8.f;
      row.title.y = y;
      row.desc.x = 8.f;
      row.desc.y = y + 34.f;
      row.buy.x = content_w - tech_buy_w - 16.f;
      row.buy.y = y + 12.f;
      y += 110.f;
    }
  }

  AppState* app_ = nullptr;
  GameAssets assets_{};
  StarCanvasRenderer star_renderer_{};
  Scene canvas_scene_{};
  Scene ui_scene_{};

  Canvas canvas_{};
  Panel hud_panel_{};
  Label title_{};
  Image energy_icon_{};
  Label energy_label_{};
  Label eps_label_{};
  Image hud_icon_p_{};
  Image hud_icon_n_{};
  Image hud_icon_e_{};
  Label count_p_{};
  Label count_n_{};
  Label count_e_{};
  Label star_label_{};

  Image tab_star_icon_{};
  Image tab_lab_icon_{};
  Image tab_tech_icon_{};
  Button tab_star_{};
  Button tab_lab_{};
  Button tab_tech_{};
  Button settings_btn_{};
  Button exit_btn_{};

  Label star_hint_{};
  float star_base_size_ = 168.f;
  float star_scale_ = 1.f;
  float star_anim_t_ = 0.f;
  float star_center_x_ = 0.f;
  float star_center_y_ = 0.f;
  Image icon_p_{};
  Image icon_n_{};
  Image icon_e_{};
  Button buy_p_{};
  Button buy_n_{};
  Button buy_e_{};
  Image icon_supernova_{};
  Button supernova_{};

  ScrollView lab_scroll_{};
  std::vector<game_ui::ElementRow> element_rows_;

  ScrollView tech_scroll_{};
  std::vector<game_ui::UpgradeRow> upgrade_rows_;
  std::vector<int> visible_upgrade_indices_;
};

inline ScenePtr CreateGameScene() {
  return std::make_unique<GameScene>();
}
