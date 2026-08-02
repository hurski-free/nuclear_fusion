#pragma once

#include "simple_ui/types.h"

// Abstract UI element. Prefer concrete types such as Button in client code.
// Typical per-frame order: handle_messages → update → draw → handle_events.
// Call Scene::prepare_scene() after changing the component list or layer values.
class SIMPLE_UI_API Component {
public:
  Component();
  virtual ~Component() = 0;

  // Update interaction state from input (no callbacks are invoked here).
  // Click detection may enqueue on_click for later handle_events().
  virtual void handle_messages(const MouseEvents& mouse,
                               const KeyboardEvents& keyboard);

  // Advance style animations by dt seconds.
  virtual void update(float dt);

  // Append pointers into this component's draw_command_buffer_ (no GPU calls).
  // When force_rebuild is true, always refreshes the buffer (e.g. parent moved us).
  virtual void collect_draw(std::vector<DrawCommand*>& out,
                            bool force_rebuild = false);

  // Optional popups/overlays drawn in a separate pass after the main scene
  // (always above every component, regardless of layer).
  virtual void collect_overlay_draw(std::vector<DrawCommand*>& out);

  // Marks the component static and builds the draw buffer once.
  void build_draw_static();

  // Drain the event queue and invoke handlers (e.g. on_click).
  virtual void handle_events();

  // Axis-aligned size used by parents (e.g. Container) for content bounds.
  virtual void get_layout_size(float& out_w, float& out_h) const;

  // When true, Scene routes input only to capturing components (e.g. open Modal).
  virtual bool captures_input() const;

  // Shift draw commands from local (parent-relative) space into screen space
  // without rebuilding. Parents call this after collect_draw (patches in place).
  // Container/Panel/Modal also propagate the delta to nested children.
  virtual void apply_draw_origin(float origin_x, float origin_y,
                                 int origin_layer);

  // Apply a delta to this component's draw origin (and nested children).
  void nudge_draw_origin(float dx, float dy, int dlayer);

  float x = 0.f;
  float y = 0.f;
  int layer = 0;
  bool disabled = false;  // When true, input is ignored and style_disabled is used.
  ComponentState state = ComponentState::Base;
  // Pixel size for text drawn by this component. 0 = atlas default size.
  // Initialized from UiStylePresets::component in the constructor.
  float font_size = 0.f;
  // Non-owning font atlas for this component's text. nullptr = context default.
  const FontAtlas* font = nullptr;

  // Non-owning; Scene assigns before input/update/draw for text metrics, etc.
  UiContext* ui = nullptr;

  // Target styles per state; transitions interpolate toward the active target.
  // Filled from UiStylePresets by each widget constructor (override after create).
  ComponentStyle style_base{};
  ComponentStyle style_hovered{};
  ComponentStyle style_active{};
  ComponentStyle style_disabled{};
  StyleTransition transition{};

  // Fired on press+release over the component (queued in handle_messages,
  // invoked in handle_events).
  std::function<void()> on_click;

protected:
  // When true, collect_draw skips rewrite of draw_command_buffer_ properties.
  bool static_draw = false;

  // Pre-sized command slots owned by this component; collect_draw emits pointers.
  std::vector<DrawCommand> draw_command_buffer_;

  // Fill/update draw_command_buffer_ slots (called from collect_draw).
  virtual void build_draw_buffer();

  // Write background + up to 4 borders into buffer slots [start, start+5).
  // Buffer must already be large enough. Inactive borders get zero size.
  void write_box_commands(size_t start, float width, float height);

  // Assigns this component's font_size and font onto a Text draw command.
  void apply_font(DrawCommand& cmd) const;

  // Resolved pixel size (font_size, or the active atlas size when font_size <= 0).
  float effective_font_size() const;

  // Push non-empty commands from draw_command_buffer_ into out.
  void emit_draw_buffer(std::vector<DrawCommand*>& out);

  // Call after build_draw_buffer in overridden collect_draw paths.
  void reset_draw_origin_after_build();

  // Hook for parents that nest children (Container/Panel/Modal).
  virtual void on_nudge_draw_origin(float dx, float dy, int dlayer);

  // Current interpolated style used for drawing.
  const ComponentStyle& visual_style() const;

  // Queue a callback to run on the next handle_events().
  void enqueue_event(const std::function<void()>& handler);

private:
  const ComponentStyle& TargetStyle() const;
  void SyncStyleAnimation();
  void AdvanceStyleAnimation(float dt);

  ComponentStyle visual_{};
  bool style_initialized_ = false;
  ComponentState last_state_ = ComponentState::Base;
  bool last_disabled_ = false;

  std::vector<std::function<void()>> event_queue_;

  Color bg_from_{};
  Color bg_to_{};
  float bg_elapsed_ = 0.f;
  float bg_duration_ = 0.f;

  Border borders_from_[5]{};
  Border borders_to_[5]{};
  float border_elapsed_ = 0.f;
  float border_duration_ = 0.f;

  float draw_origin_x_ = 0.f;
  float draw_origin_y_ = 0.f;
  int draw_origin_layer_ = 0;
};

