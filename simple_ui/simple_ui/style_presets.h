#pragma once

#include "simple_ui/export.h"
#include "simple_ui/types.h"

// =============================================================================
// IMPORTANT: Call ui_set_style_presets() BEFORE creating any components.
// Component constructors copy values from the current global presets. Changing
// presets after construction does NOT update existing instances.
//
// After a component is created, any of its style / size / color fields may still
// be overridden explicitly on that instance.
// =============================================================================

// Shared scrollbar look (Container / Select / TextArea / ScrollView).
struct ScrollBarStylePreset {
  ScrollMode mode = ScrollMode::Hidden;
  ScrollbarStateStyle style_base{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                 Color{0.55f, 0.55f, 0.55f, 1.f}, 12.f};
  ScrollbarStateStyle style_hovered{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                    Color{0.7f, 0.7f, 0.7f, 1.f}, 12.f};
  ScrollbarStateStyle style_active{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                   Color{0.85f, 0.85f, 0.85f, 1.f}, 12.f};
};

struct ComponentStylePreset {
  float font_size = 18.f;
  StyleTransition transition{};
};

struct ButtonStylePreset {
  float width = 120.f;
  float height = 36.f;
  Color text_color{1.f, 1.f, 1.f, 1.f};
  ComponentStyle style_base{Color{0.25f, 0.45f, 0.75f, 1.f}, Border{}};
  ComponentStyle style_hovered{Color{0.25f, 0.45f, 0.75f, 1.f}, Border{}};
  ComponentStyle style_active{Color{0.25f, 0.45f, 0.75f, 1.f}, Border{}};
  ComponentStyle style_disabled{Color{0.25f, 0.45f, 0.75f, 0.4f}, Border{}};
};

struct TextStylePreset {
  float width = 200.f;
  float height = 0.f;
  Color color{1.f, 1.f, 1.f, 1.f};
  TextOutline outline{};
  ComponentStyle style_base{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_hovered{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_active{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_disabled{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
};

struct LabelStylePreset {
  float width = 120.f;
  float height = 24.f;
  Color color{1.f, 1.f, 1.f, 1.f};
  TextOutline outline{};
  TextAlign text_align = TextAlign::LeftMiddle;
  ComponentStyle style_base{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_hovered{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_active{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_disabled{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
};

struct InputStylePreset {
  float width = 220.f;
  float height = 36.f;
  float padding = 8.f;
  Color text_color{1.f, 1.f, 1.f, 1.f};
  Color placeholder_color{0.55f, 0.55f, 0.6f, 1.f};
  Color caret_color{1.f, 1.f, 1.f, 1.f};
  // Seconds between caret visibility toggles while focused.
  float caret_blink_period = 0.5f;
  ComponentStyle style_base{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.55f, 0.6f, 0.75f, 1.f}}};
  ComponentStyle style_hovered{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.75f, 0.8f, 0.95f, 1.f}}};
  ComponentStyle style_active{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.9f, 0.95f, 1.f, 1.f}}};
  ComponentStyle style_disabled{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.55f, 0.6f, 0.75f, 1.f}}};
};

struct TextAreaStylePreset {
  float width = 280.f;
  float height = 120.f;
  float padding = 8.f;
  float line_height = 0.f;
  Color text_color{1.f, 1.f, 1.f, 1.f};
  Color placeholder_color{0.55f, 0.55f, 0.6f, 1.f};
  Color caret_color{1.f, 1.f, 1.f, 1.f};
  Color selection_color{0.25f, 0.45f, 0.75f, 0.85f};
  float caret_blink_period = 0.5f;
  ScrollBarStylePreset scroll_y{ScrollMode::Auto,
                                ScrollbarStateStyle{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                                    Color{0.55f, 0.55f, 0.55f, 1.f},
                                                    12.f},
                                ScrollbarStateStyle{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                                    Color{0.7f, 0.7f, 0.7f, 1.f},
                                                    12.f},
                                ScrollbarStateStyle{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                                    Color{0.85f, 0.85f, 0.85f, 1.f},
                                                    12.f}};
  ComponentStyle style_base{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.55f, 0.6f, 0.75f, 1.f}}};
  ComponentStyle style_hovered{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.55f, 0.6f, 0.75f, 1.f}}};
  ComponentStyle style_active{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.9f, 0.95f, 1.f, 1.f}}};
  ComponentStyle style_disabled{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.55f, 0.6f, 0.75f, 1.f}}};
};

struct SelectStylePreset {
  float width = 220.f;
  float height = 36.f;
  float dropdown_height = 160.f;
  float item_height = 32.f;
  Color text_color{1.f, 1.f, 1.f, 1.f};
  Color dropdown_bg{0.12f, 0.14f, 0.22f, 1.f};
  Color item_hover_color{0.25f, 0.4f, 0.65f, 1.f};
  ScrollBarStylePreset scroll_y{ScrollMode::Auto,
                                ScrollbarStateStyle{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                                    Color{0.55f, 0.55f, 0.55f, 1.f},
                                                    12.f},
                                ScrollbarStateStyle{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                                    Color{0.7f, 0.7f, 0.7f, 1.f},
                                                    12.f},
                                ScrollbarStateStyle{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                                    Color{0.85f, 0.85f, 0.85f, 1.f},
                                                    12.f}};
  ComponentStyle style_base{
      Color{0.14f, 0.18f, 0.28f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.7f, 0.75f, 0.9f, 1.f}}};
  ComponentStyle style_hovered{
      Color{0.2f, 0.28f, 0.42f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.7f, 0.75f, 0.9f, 1.f}}};
  ComponentStyle style_active{
      Color{0.14f, 0.18f, 0.28f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.7f, 0.75f, 0.9f, 1.f}}};
  ComponentStyle style_disabled{
      Color{0.14f, 0.18f, 0.28f, 1.f},
      Border{1.5f, BorderMode::In, Color{0.7f, 0.75f, 0.9f, 1.f}}};
};

struct CheckboxStylePreset {
  float box_size = 20.f;
  float gap = 10.f;
  float height = 28.f;
  Color label_color{1.f, 1.f, 1.f, 1.f};
  Color box_border_color{0.75f, 0.8f, 0.95f, 1.f};
  Color box_background_color{0.15f, 0.18f, 0.25f, 1.f};
  Color box_background_hovered{0.22f, 0.28f, 0.4f, 1.f};
  Color check_color{0.85f, 0.9f, 1.f, 1.f};
  CheckMarkKind check_kind = CheckMarkKind::Square;
  ComponentStyle style_base{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_hovered{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_active{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_disabled{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
};

struct RadioGroupStylePreset {
  float radio_size = 18.f;
  float item_height = 28.f;
  float item_width = 140.f;
  float gap = 8.f;
  Color label_color{1.f, 1.f, 1.f, 1.f};
  Color box_border_color{0.75f, 0.8f, 0.95f, 1.f};
  Color box_background_color{0.15f, 0.18f, 0.25f, 1.f};
  Color box_background_hovered{0.22f, 0.28f, 0.4f, 1.f};
  Color check_color{0.85f, 0.9f, 1.f, 1.f};
  ComponentStyle style_base{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_hovered{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_active{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_disabled{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
};

struct ToggleStylePreset {
  float width = 48.f;
  float height = 24.f;
  float gap = 10.f;
  Color label_color{1.f, 1.f, 1.f, 1.f};
  Color track_off{0.3f, 0.32f, 0.4f, 1.f};
  Color track_on{0.25f, 0.55f, 0.9f, 1.f};
  Color thumb_color{1.f, 1.f, 1.f, 1.f};
  // Seconds to animate thumb/track between off and on (0 = instant).
  float transition_duration = 0.2f;
  ComponentStyle style_base{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_hovered{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_active{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_disabled{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
};

struct RangeStylePreset {
  float width = 220.f;
  float height = 28.f;
  Color text_color{1.f, 1.f, 1.f, 1.f};
  float label_width = 90.f;
  float label_gap = 12.f;
  float value_box_width = 48.f;
  float value_box_gap = 10.f;
  Color value_box_background{0.1f, 0.12f, 0.18f, 1.f};
  Color value_box_border{0.55f, 0.6f, 0.75f, 1.f};
  Color value_text_color{1.f, 1.f, 1.f, 1.f};
  Color tick_color{0.65f, 0.7f, 0.85f, 1.f};
  Color tick_label_color{0.85f, 0.88f, 0.95f, 1.f};
  float tick_height = 6.f;
  float tick_label_gap = 2.f;
  float tick_label_height = 18.f;
  Color track_color{0.25f, 0.25f, 0.3f, 1.f};
  Color thumb_color{0.75f, 0.8f, 0.95f, 1.f};
  Color thumb_active_color{1.f, 1.f, 1.f, 1.f};
  float track_thickness = 6.f;
  float thumb_size = 16.f;
  ComponentStyle style_base{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_hovered{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_active{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_disabled{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
};

struct ProgressBarStylePreset {
  float width = 220.f;
  float height = 18.f;
  Color track_color{0.18f, 0.2f, 0.28f, 1.f};
  Color fill_color{0.3f, 0.55f, 0.95f, 1.f};
  Color text_color{1.f, 1.f, 1.f, 1.f};
  ComponentStyle style_base{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_hovered{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_active{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_disabled{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
};

struct ImageStylePreset {
  float width = 64.f;
  float height = 64.f;
  Color tint{1.f, 1.f, 1.f, 1.f};
  ComponentStyle style_base{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_hovered{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_active{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_disabled{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
};

struct CanvasStylePreset {
  float width = 320.f;
  float height = 180.f;
  Color tint{1.f, 1.f, 1.f, 1.f};
  Color clear_color{0.05f, 0.06f, 0.08f, 1.f};
  float clear_depth = 1.f;
  ComponentStyle style_base{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_hovered{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_active{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  ComponentStyle style_disabled{Color{0.f, 0.f, 0.f, 0.f}, Border{}};
};

struct ContainerStylePreset {
  float width = 200.f;
  float height = 200.f;
  ScrollPlaceMode place_mode = ScrollPlaceMode::In;
  ScrollBarStylePreset scroll_x{};
  ScrollBarStylePreset scroll_y{};
  ComponentStyle style_base{Color{0.08f, 0.08f, 0.1f, 1.f}, Border{}};
  ComponentStyle style_hovered{Color{0.08f, 0.08f, 0.1f, 1.f}, Border{}};
  ComponentStyle style_active{Color{0.08f, 0.08f, 0.1f, 1.f}, Border{}};
  ComponentStyle style_disabled{Color{0.08f, 0.08f, 0.1f, 1.f}, Border{}};
};

struct ScrollViewStylePreset {
  ScrollPlaceMode place_mode = ScrollPlaceMode::In;
  ScrollBarStylePreset scroll_x{};  // Hidden by default
  ScrollBarStylePreset scroll_y{ScrollMode::Auto,
                                ScrollbarStateStyle{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                                    Color{0.55f, 0.55f, 0.55f, 1.f},
                                                    12.f},
                                ScrollbarStateStyle{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                                    Color{0.7f, 0.7f, 0.7f, 1.f},
                                                    12.f},
                                ScrollbarStateStyle{Color{0.16f, 0.16f, 0.16f, 0.95f},
                                                    Color{0.85f, 0.85f, 0.85f, 1.f},
                                                    12.f}};
};

struct PanelStylePreset {
  float width = 360.f;
  float height = 240.f;
  float title_height = 32.f;
  Color title_color{1.f, 1.f, 1.f, 1.f};
  Color title_bar_color{0.12f, 0.16f, 0.28f, 1.f};
  ComponentStyle style_base{
      Color{0.1f, 0.12f, 0.18f, 0.98f},
      Border{1.5f, BorderMode::Out, Color{0.55f, 0.62f, 0.8f, 1.f}}};
  ComponentStyle style_hovered{
      Color{0.1f, 0.12f, 0.18f, 0.98f},
      Border{1.5f, BorderMode::Out, Color{0.55f, 0.62f, 0.8f, 1.f}}};
  ComponentStyle style_active{
      Color{0.1f, 0.12f, 0.18f, 0.98f},
      Border{1.5f, BorderMode::Out, Color{0.55f, 0.62f, 0.8f, 1.f}}};
  ComponentStyle style_disabled{
      Color{0.1f, 0.12f, 0.18f, 0.98f},
      Border{1.5f, BorderMode::Out, Color{0.55f, 0.62f, 0.8f, 1.f}}};
};

struct ModalStylePreset {
  float width = 420.f;
  float height = 280.f;
  float title_height = 32.f;
  float screen_width = 1280.f;
  float screen_height = 720.f;
  int layer = 100;
  Color overlay_color{0.f, 0.f, 0.f, 0.55f};
  Color title_color{1.f, 1.f, 1.f, 1.f};
  Color title_bar_color{0.12f, 0.16f, 0.28f, 1.f};
  ComponentStyle style_base{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::Out, Color{0.65f, 0.72f, 0.9f, 1.f}}};
  ComponentStyle style_hovered{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::Out, Color{0.65f, 0.72f, 0.9f, 1.f}}};
  ComponentStyle style_active{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::Out, Color{0.65f, 0.72f, 0.9f, 1.f}}};
  ComponentStyle style_disabled{
      Color{0.1f, 0.12f, 0.18f, 1.f},
      Border{1.5f, BorderMode::Out, Color{0.65f, 0.72f, 0.9f, 1.f}}};
};

// Global style presets for all built-in components.
// Default member values are the library theme; customize via ui_set_style_presets.
struct UiStylePresets {
  ComponentStylePreset component{};
  ScrollBarStylePreset scrollbar{};
  ButtonStylePreset button{};
  TextStylePreset text{};
  LabelStylePreset label{};
  InputStylePreset input{};
  TextAreaStylePreset text_area{};
  SelectStylePreset select{};
  CheckboxStylePreset checkbox{};
  RadioGroupStylePreset radio_group{};
  ToggleStylePreset toggle{};
  RangeStylePreset range{};
  ProgressBarStylePreset progress_bar{};
  ImageStylePreset image{};
  CanvasStylePreset canvas{};
  ContainerStylePreset container{};
  ScrollViewStylePreset scroll_view{};
  PanelStylePreset panel{};
  ModalStylePreset modal{};
};

inline void ui_apply_scrollbar_preset(ScrollBar& bar,
                                      const ScrollBarStylePreset& preset) {
  bar.mode = preset.mode;
  bar.style_base = preset.style_base;
  bar.style_hovered = preset.style_hovered;
  bar.style_active = preset.style_active;
}
