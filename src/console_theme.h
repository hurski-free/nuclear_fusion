#pragma once

#include "simple_ui.h"

// CRT / terminal console palette for Star Clicker.
inline UiStylePresets MakeConsoleStylePresets() {
  UiStylePresets p{};

  const Color phosphor{0.35f, 1.f, 0.45f, 1.f};
  const Color phosphor_dim{0.18f, 0.55f, 0.28f, 1.f};
  const Color phosphor_hot{0.65f, 1.f, 0.7f, 1.f};
  const Color panel_bg{0.02f, 0.05f, 0.03f, 0.92f};
  const Color btn_base{0.02f, 0.14f, 0.06f, 1.f};
  const Color btn_hover{0.06f, 0.32f, 0.14f, 1.f};
  const Color btn_active{0.12f, 0.5f, 0.22f, 1.f};
  const Color btn_disabled{0.04f, 0.08f, 0.05f, 0.45f};
  const Color border_base{0.35f, 0.9f, 0.45f, 1.f};
  const Color border_hover{0.55f, 1.f, 0.6f, 1.f};
  const Color border_active{0.8f, 1.f, 0.85f, 1.f};
  const Color track{0.08f, 0.16f, 0.1f, 1.f};
  const Color thumb{0.3f, 0.85f, 0.4f, 1.f};

  p.component.font_size = 20.f;
  p.component.transition.background_duration = 0.18f;
  p.component.transition.border_duration = 0.14f;

  Border in_border{2.f, BorderMode::In, border_base};
  Border in_border_hover{2.f, BorderMode::In, border_hover};
  Border in_border_active{2.f, BorderMode::In, border_active};
  Border in_border_disabled{2.f, BorderMode::In, phosphor_dim};

  p.button.width = 240.f;
  p.button.height = 48.f;
  p.button.text_color = phosphor;
  p.button.style_base = {btn_base, in_border};
  p.button.style_hovered = {btn_hover, in_border_hover};
  p.button.style_active = {btn_active, in_border_active};
  p.button.style_disabled = {btn_disabled, in_border_disabled};

  p.label.color = phosphor;
  p.label.height = 28.f;

  p.text.color = phosphor;

  p.select.width = 280.f;
  p.select.height = 40.f;
  p.select.dropdown_height = 180.f;
  p.select.item_height = 36.f;
  p.select.text_color = phosphor;
  p.select.dropdown_bg = panel_bg;
  p.select.item_hover_color = btn_hover;
  p.select.style_base = {btn_base, in_border};
  p.select.style_hovered = {btn_hover, in_border_hover};
  p.select.style_active = {btn_active, in_border_active};
  p.select.style_disabled = {btn_disabled, in_border_disabled};

  p.toggle.width = 52.f;
  p.toggle.height = 26.f;
  p.toggle.label_color = phosphor;
  p.toggle.track_off = track;
  p.toggle.track_on = btn_active;
  p.toggle.thumb_color = phosphor_hot;
  p.toggle.transition_duration = 0.16f;

  p.range.width = 280.f;
  p.range.height = 32.f;
  p.range.label_width = 140.f;
  p.range.text_color = phosphor;
  p.range.value_box_background = btn_base;
  p.range.value_box_border = border_base;
  p.range.value_text_color = phosphor;
  p.range.track_color = track;
  p.range.thumb_color = thumb;
  p.range.thumb_active_color = phosphor_hot;

  p.checkbox.label_color = phosphor;
  p.checkbox.box_border_color = border_base;
  p.checkbox.box_background_color = btn_base;
  p.checkbox.box_background_hovered = btn_hover;
  p.checkbox.check_color = phosphor;

  p.radio_group.label_color = phosphor;
  p.radio_group.box_border_color = border_base;
  p.radio_group.box_background_color = btn_base;
  p.radio_group.box_background_hovered = btn_hover;
  p.radio_group.check_color = phosphor;
  p.radio_group.item_width = 220.f;

  p.panel.width = 520.f;
  p.panel.height = 420.f;
  p.panel.title_color = phosphor;
  p.panel.title_bar_color = {0.03f, 0.1f, 0.05f, 1.f};
  p.panel.style_base = {panel_bg, in_border};
  p.panel.style_hovered = {panel_bg, in_border};
  p.panel.style_active = {panel_bg, in_border};
  p.panel.style_disabled = {panel_bg, in_border_disabled};

  p.container.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  p.container.style_hovered = p.container.style_base;
  p.container.style_active = p.container.style_base;
  p.container.style_disabled = p.container.style_base;

  p.image.tint = {1.f, 1.f, 1.f, 1.f};

  return p;
}

inline Color ConsoleClearColor() {
  return Color{0.01f, 0.02f, 0.015f, 1.f};
}

inline void ApplyConsoleButtonStyle(Button& btn, float width = 240.f,
                                    float height = 48.f) {
  const Color phosphor{0.45f, 1.f, 0.55f, 1.f};
  const Color btn_base{0.02f, 0.16f, 0.07f, 1.f};
  const Color btn_hover{0.08f, 0.36f, 0.16f, 1.f};
  const Color btn_active{0.14f, 0.55f, 0.24f, 1.f};
  const Color btn_disabled{0.04f, 0.08f, 0.05f, 0.45f};
  const Border in_border{2.f, BorderMode::In, Color{0.35f, 0.9f, 0.45f, 1.f}};
  const Border in_border_hover{2.f, BorderMode::In,
                               Color{0.55f, 1.f, 0.6f, 1.f}};
  const Border in_border_active{2.f, BorderMode::In,
                                Color{0.85f, 1.f, 0.9f, 1.f}};
  const Border in_border_disabled{2.f, BorderMode::In,
                                  Color{0.18f, 0.55f, 0.28f, 1.f}};

  btn.width = width;
  btn.height = height;
  btn.font_size = 20.f;
  btn.text_color = phosphor;
  btn.style_base = {btn_base, in_border};
  btn.style_hovered = {btn_hover, in_border_hover};
  btn.style_active = {btn_active, in_border_active};
  btn.style_disabled = {btn_disabled, in_border_disabled};
  btn.transition.background_duration = 0.18f;
  btn.transition.border_duration = 0.14f;
}

inline void MakeFullscreenBackground(Image& image, int texture_id) {
  image.texture_id = texture_id;
  image.tint = {1.f, 1.f, 1.f, 1.f};
  image.layer = 0;
  image.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  image.style_hovered = image.style_base;
  image.style_active = image.style_base;
  image.style_disabled = image.style_base;
  image.transition = {};
}
