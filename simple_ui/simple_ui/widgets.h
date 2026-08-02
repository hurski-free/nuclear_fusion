#pragma once

#include "simple_ui/component.h"

// Clickable rectangle with optional centered label.
// Visual defaults come from UiStylePresets::button (copied in the constructor).
class SIMPLE_UI_API Button : public Component {
public:
  Button();
  ~Button() override;

  // Hit-tests the button, updates Hovered/Active, and may enqueue on_click.
  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;

  void get_layout_size(float& out_w, float& out_h) const override;

  float width = 0.f;
  float height = 0.f;
  std::wstring text;
  Color text_color{};

protected:
  void build_draw_buffer() override;

private:
  // True while LMB is held after a press that started inside the button.
  bool press_started_here_ = false;
};

// Multi-line text block. Wraps within `width`; honors explicit '\\n' breaks.
class SIMPLE_UI_API Text : public Component {
public:
  Text();
  ~Text() override;

  void get_layout_size(float& out_w, float& out_h) const override;

  std::wstring text;
  Color color{};
  // Maximum line width used for wrapping (pixels).
  float width = 0.f;
  // Layout height for parents (0 = treat as one line ~ font-sized block).
  float height = 0.f;

protected:
  void build_draw_buffer() override;
};



// Checkbox with an optional text label. bind_data links a bool.
class SIMPLE_UI_API Checkbox : public Component {
public:
  Checkbox();
  ~Checkbox() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void get_layout_size(float& out_w, float& out_h) const override;
  void bind_data(bool* data);

  // Built-in shape generators (normalized 0..1 parts).
  static CheckMarkShape generate_square_check();
  static CheckMarkShape generate_checkmark();

  // Sets check_kind to Custom and stores the shape.
  void set_custom_check_shape(const CheckMarkShape& shape);

  bool checked = false;
  std::wstring label;
  Color label_color{};
  float box_size = 0.f;
  float gap = 0.f;
  float height = 0.f;

  Color box_border_color{};
  Color box_background_color{};
  Color box_background_hovered{};
  Color check_color{};
  CheckMarkKind check_kind = CheckMarkKind::Square;
  CheckMarkShape custom_check{};

protected:
  void build_draw_buffer() override;

private:
  void WriteBound();
  void EnsureBuffer();
  const CheckMarkShape& ActiveShape() const;
  bool* bound_ = nullptr;
};

// Optional bottom label aligned to a value on the track.
struct RangeTickLabel {
  float value = 0.f;
  std::wstring text;
};

// Horizontal slider. bind_data links a float value in [min_value, max_value].
class SIMPLE_UI_API Range : public Component {
public:
  Range();
  ~Range() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void collect_draw(std::vector<DrawCommand*>& out,
                    bool force_rebuild = false) override;
  void get_layout_size(float& out_w, float& out_h) const override;
  void bind_data(float* data);

  // Track width (interactive area). Overall layout may be wider due to labels.
  float width = 0.f;
  float height = 0.f;
  float min_value = 0.f;
  float max_value = 100.f;
  float step = 1.f;
  float value = 0.f;

  // Optional caption rendered to the left of the track.
  std::wstring text;
  Color text_color{};
  float label_width = 0.f;
  float label_gap = 0.f;

  // Optional numeric value box to the right of the track (hidden when false).
  bool show_value = false;
  float value_box_width = 0.f;
  float value_box_gap = 0.f;
  Color value_box_background{};
  Color value_box_border{};
  Color value_text_color{};

  // Optional tick marks + labels under the track (empty = none).
  std::vector<RangeTickLabel> tick_labels;
  Color tick_color{};
  Color tick_label_color{};
  float tick_height = 0.f;
  float tick_label_gap = 0.f;
  float tick_label_height = 0.f;

  Color track_color{};
  Color thumb_color{};
  Color thumb_active_color{};
  float track_thickness = 0.f;
  float thumb_size = 0.f;

protected:
  void build_draw_buffer() override;

private:
  void SetValue(float v);
  void WriteBound();
  void EnsureBuffer();
  float TrackX() const;
  float TrackY() const;
  float ValueToTrackX(float v) const;
  float* bound_ = nullptr;
  bool dragging_ = false;
};

enum class RadioOrientation {
  Horizontal,
  Vertical,
};

enum class RadioMode {
  Square,  // Square box with square check (current look).
  Circle,  // Circular ring with circular check inside.
};

// Mutually exclusive options. bind_data links the selected index (int).
class SIMPLE_UI_API RadioGroup : public Component {
public:
  RadioGroup();
  ~RadioGroup() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void get_layout_size(float& out_w, float& out_h) const override;
  void bind_data(int* data);

  std::vector<std::wstring> options;
  int selected = 0;
  RadioOrientation orientation = RadioOrientation::Vertical;
  RadioMode mode = RadioMode::Circle;
  Color label_color{};
  float radio_size = 0.f;
  float item_height = 0.f;
  float item_width = 0.f;
  float gap = 0.f;

  Color box_border_color{};
  Color box_background_color{};
  Color box_background_hovered{};
  Color check_color{};

protected:
  void build_draw_buffer() override;

private:
  void WriteBound();
  void EnsureBuffer();
  int* bound_ = nullptr;
  // Index of the option under the cursor (-1 = none).
  int hovered_index_ = -1;
};

// Single-line text field with caret. bind_data links a std::wstring.
class SIMPLE_UI_API Input : public Component {
public:
  Input();
  ~Input() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void update(float dt) override;
  void get_layout_size(float& out_w, float& out_h) const override;
  void bind_data(std::wstring* data);

  std::wstring text;
  std::wstring placeholder;
  Color text_color{};
  Color placeholder_color{};
  Color caret_color{};
  float width = 0.f;
  float height = 0.f;
  float padding = 0.f;
  // Seconds between caret visibility toggles while focused.
  float caret_blink_period = 0.f;
  bool focused = false;

protected:
  void build_draw_buffer() override;

private:
  void WriteBound();
  float MeasureTextWidth(const std::wstring& s) const;
  size_t CaretIndexAtX(float local_x) const;
  std::wstring* bound_ = nullptr;
  size_t caret_ = 0;
  float caret_blink_ = 0.f;
  bool caret_visible_ = true;
};

// Dropdown select. bind_data links the selected option index (int).
class SIMPLE_UI_API Select : public Component {
public:
  Select();
  ~Select() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void collect_draw(std::vector<DrawCommand*>& out,
                    bool force_rebuild = false) override;
  void collect_overlay_draw(std::vector<DrawCommand*>& out) override;
  void get_layout_size(float& out_w, float& out_h) const override;
  void bind_data(int* data);
  bool captures_input() const override;

  std::vector<std::wstring> options;
  int selected = 0;
  float width = 0.f;
  float height = 0.f;
  // Maximum height of the open dropdown list (px).
  float dropdown_height = 0.f;
  float item_height = 0.f;
  Color text_color{};
  Color dropdown_bg{};
  Color item_hover_color{};
  ScrollBar scroll_y{};

protected:
  void build_draw_buffer() override;

private:
  void WriteBound();
  void EnsureBuffer();
  void ClampScroll();
  float ContentHeight() const;
  bool ShowScroll() const;
  float ViewportWidth() const;
  int MaxVisibleSlots() const;
  int* bound_ = nullptr;
  bool open_ = false;
  float scroll_offset_ = 0.f;
  int hovered_index_ = -1;
  int visible_first_ = 0;
  int visible_count_ = 0;
  bool dragging_scroll_ = false;
  float drag_mouse_anchor_ = 0.f;
  float drag_scroll_anchor_ = 0.f;
  ComponentState scroll_state_ = ComponentState::Base;
};

// Non-interactive text label (vertically centered by default).
class SIMPLE_UI_API Label : public Component {
public:
  Label();
  ~Label() override;

  void get_layout_size(float& out_w, float& out_h) const override;

  std::wstring text;
  Color color{};
  float width = 0.f;
  float height = 0.f;
  TextAlign text_align = TextAlign::LeftMiddle;

protected:
  void build_draw_buffer() override;
};

// Textured rectangle. Load textures via ui_load_texture().
class SIMPLE_UI_API Image : public Component {
public:
  Image();
  ~Image() override;

  void get_layout_size(float& out_w, float& out_h) const override;

  int texture_id = -1;
  float width = 0.f;
  float height = 0.f;
  Color tint{};
  // UV rect in [0..1]. Default = full texture.
  float u0 = 0.f;
  float v0 = 0.f;
  float u1 = 1.f;
  float v1 = 1.f;

protected:
  void build_draw_buffer() override;
};

// Offscreen color + depth target for custom D3D11 drawing outside the library.
// Typical frame:
//   ui_clear(ctx, ...);
//   canvas.begin_draw(ctx);          // bind RTV+DSV, optional clear
//   /* your DrawIndexed / etc. */
//   canvas.end_draw(ctx);            // restore scene target
//   scene.draw(ctx);
//   ui_present(ctx);
// The color buffer is shown as an Image quad using DrawCommand (UI pass).
class SIMPLE_UI_API Canvas : public Component {
public:
  Canvas();
  ~Canvas() override;

  void get_layout_size(float& out_w, float& out_h) const override;
  void collect_draw(std::vector<DrawCommand*>& out,
                    bool force_rebuild = false) override;

  // Create or recreate GPU targets to match width/height (pixel size = ceil).
  bool ensure_targets(UiContext* ctx);
  void release_targets();

  // Bind color RTV + depth DSV for external drawing. Clears if requested.
  bool begin_draw(UiContext* ctx, bool clear = true);
  // Unbind canvas targets and restore the window scene color target.
  void end_draw(UiContext* ctx);

  ID3D11RenderTargetView* color_rtv() const { return color_rtv_; }
  ID3D11DepthStencilView* depth_dsv() const { return depth_dsv_; }
  ID3D11ShaderResourceView* color_srv() const { return color_srv_; }
  ID3D11Texture2D* color_texture() const { return color_tex_; }
  ID3D11Texture2D* depth_texture() const { return depth_tex_; }
  int texture_id() const { return texture_id_; }
  int texture_width() const { return tex_w_; }
  int texture_height() const { return tex_h_; }

  float width = 0.f;
  float height = 0.f;
  Color tint{};
  Color clear_color{};
  float clear_depth = 1.f;

protected:
  void build_draw_buffer() override;

private:
  int PixelWidth() const;
  int PixelHeight() const;

  ID3D11Texture2D* color_tex_ = nullptr;
  ID3D11RenderTargetView* color_rtv_ = nullptr;
  ID3D11ShaderResourceView* color_srv_ = nullptr;
  ID3D11Texture2D* depth_tex_ = nullptr;
  ID3D11DepthStencilView* depth_dsv_ = nullptr;
  int texture_id_ = -1;
  int tex_w_ = 0;
  int tex_h_ = 0;
  bool drawing_ = false;
};

// Horizontal progress bar. bind_data links a float in [min_value, max_value].
class SIMPLE_UI_API ProgressBar : public Component {
public:
  ProgressBar();
  ~ProgressBar() override;

  void get_layout_size(float& out_w, float& out_h) const override;
  void bind_data(float* data);

  float width = 0.f;
  float height = 0.f;
  float min_value = 0.f;
  float max_value = 1.f;
  float value = 0.f;
  Color track_color{};
  Color fill_color{};
  bool show_percent = false;
  Color text_color{};

protected:
  void build_draw_buffer() override;

private:
  float* bound_ = nullptr;
};

// On/off switch. bind_data links a bool.
class SIMPLE_UI_API Toggle : public Component {
public:
  Toggle();
  ~Toggle() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void update(float dt) override;
  void get_layout_size(float& out_w, float& out_h) const override;
  void bind_data(bool* data);

  bool checked = false;
  std::wstring label;
  Color label_color{};
  float width = 0.f;
  float height = 0.f;
  float gap = 0.f;
  Color track_off{};
  Color track_on{};
  Color thumb_color{};
  // Seconds to animate thumb/track between off and on (0 = instant).
  float transition_duration = 0.f;

protected:
  void build_draw_buffer() override;

private:
  void WriteBound();
  bool* bound_ = nullptr;
  // 0 = off, 1 = on; animated toward checked.
  float anim_t_ = 0.f;
};

// Multi-line text editor with caret, selection, and vertical scroll.
class SIMPLE_UI_API TextArea : public Component {
public:
  TextArea();
  ~TextArea() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void update(float dt) override;
  void collect_draw(std::vector<DrawCommand*>& out,
                    bool force_rebuild = false) override;
  void get_layout_size(float& out_w, float& out_h) const override;
  void bind_data(std::wstring* data);
  bool captures_input() const override;

  std::wstring text;
  std::wstring placeholder;
  Color text_color{};
  Color placeholder_color{};
  Color caret_color{};
  Color selection_color{};
  float width = 0.f;
  float height = 0.f;
  float padding = 0.f;
  // Optional row spacing hint when metrics are unavailable. With a valid
  // UiContext, layout uses the font atlas line height (matches Renderer).
  float line_height = 0.f;
  float caret_blink_period = 0.f;
  bool focused = false;
  ScrollBar scroll_y{};

protected:
  void build_draw_buffer() override;

private:
  void WriteBound();
  void EnsureBuffer();
  void ClampScroll();
  int LineCount() const;
  float MeasureTextWidth(const std::wstring& s) const;
  float MeasureTextWidth(const wchar_t* text, size_t length) const;
  size_t IndexAtX(const wchar_t* text, size_t length, float local_x) const;
  size_t IndexAtLocal(float local_x, float local_y) const;
  void PlaceCaretAt(float local_x, float local_y, bool extend_selection);
  void LineBounds(int line, size_t& out_start, size_t& out_end) const;
  void IndexToLineCol(size_t index, int& out_line, size_t& out_col) const;
  size_t LineColToIndex(int line, size_t col) const;
  void MoveVertical(int delta_lines, bool extend_selection);
  void MoveCaretTo(size_t index, bool extend_selection);
  void ClearSelection();
  bool HasSelection() const;
  size_t SelMin() const;
  size_t SelMax() const;
  bool DeleteSelection();
  bool CopySelection() const;
  void EnsureCaretVisible();
  float RowHeight() const;

  std::wstring* bound_ = nullptr;
  size_t caret_ = 0;
  size_t sel_anchor_ = 0;
  float caret_blink_ = 0.f;
  bool caret_visible_ = true;
  float scroll_offset_ = 0.f;
  float preferred_x_ = 0.f;
  bool prefer_x_valid_ = false;
  bool dragging_select_ = false;
};

// Framed panel with title bar and child components (relative positions).
class SIMPLE_UI_API Panel : public Component {
public:
  Panel();
  ~Panel() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void update(float dt) override;
  void collect_draw(std::vector<DrawCommand*>& out,
                    bool force_rebuild = false) override;
  void collect_overlay_draw(std::vector<DrawCommand*>& out) override;
  void handle_events() override;
  void get_layout_size(float& out_w, float& out_h) const override;

  std::wstring title;
  Color title_color{};
  Color title_bar_color{};
  float width = 0.f;
  float height = 0.f;
  float title_height = 0.f;
  bool draggable = true;
  bool closable = false;
  std::function<void()> on_close;
  std::vector<Component*> components;

protected:
  void build_draw_buffer() override;
  void on_nudge_draw_origin(float dx, float dy, int dlayer) override;

private:
  bool dragging_ = false;
  float drag_dx_ = 0.f;
  float drag_dy_ = 0.f;
};

// Modal dialog: dimmed overlay + content panel. Blocks other input while open.
class SIMPLE_UI_API Modal : public Component {
public:
  Modal();
  ~Modal() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void update(float dt) override;
  void collect_draw(std::vector<DrawCommand*>& out,
                    bool force_rebuild = false) override;
  void collect_overlay_draw(std::vector<DrawCommand*>& out) override;
  void handle_events() override;
  void get_layout_size(float& out_w, float& out_h) const override;
  bool captures_input() const override;

  bool open = false;
  bool close_on_overlay_click = true;
  Color overlay_color{};
  std::wstring title;
  Color title_color{};
  Color title_bar_color{};
  float width = 0.f;
  float height = 0.f;
  float title_height = 0.f;
  // Screen size used to center and size the overlay (set each frame or once).
  float screen_width = 0.f;
  float screen_height = 0.f;
  std::vector<Component*> components;
  std::function<void()> on_close;

protected:
  void build_draw_buffer() override;
  void on_nudge_draw_origin(float dx, float dy, int dlayer) override;

private:
  void LayoutDialog();
};

// Panel that hosts child components with optional scrolling.
// Child positions are relative to the container origin.
class SIMPLE_UI_API Container : public Component {
public:
  Container();
  ~Container() override;

  void handle_messages(const MouseEvents& mouse,
                       const KeyboardEvents& keyboard) override;
  void update(float dt) override;
  void collect_draw(std::vector<DrawCommand*>& out,
                    bool force_rebuild = false) override;
  void collect_overlay_draw(std::vector<DrawCommand*>& out) override;
  void handle_events() override;
  void get_layout_size(float& out_w, float& out_h) const override;
  bool captures_input() const override;

  // Non-owning child pointers; positions are relative to this container.
  std::vector<Component*> components;

  float width = 0.f;
  float height = 0.f;

  ScrollBar scroll_x{};
  ScrollBar scroll_y{};
  ScrollPlaceMode place_mode = ScrollPlaceMode::In;

protected:
  void build_draw_buffer() override;
  void on_nudge_draw_origin(float dx, float dy, int dlayer) override;

private:
  struct Metrics {
    float content_w = 0.f;
    float content_h = 0.f;
    float viewport_w = 0.f;
    float viewport_h = 0.f;
    bool show_x = false;
    bool show_y = false;
    float bar_thickness_x = 0.f;
    float bar_thickness_y = 0.f;
  };

  Metrics ComputeMetrics() const;
  const Metrics& MetricsCached() const;
  void InvalidateMetrics() const;
  void ClampScroll(const Metrics& m);
  const ScrollbarStateStyle& StyleForState(const ScrollBar& bar,
                                          ComponentState s) const;
  void WriteScrollbar(size_t track_index, size_t thumb_index, bool vertical,
                      const Metrics& m);
  void ClearCommand(DrawCommand& cmd);

  float scroll_offset_x_ = 0.f;
  float scroll_offset_y_ = 0.f;
  ComponentState scroll_x_state_ = ComponentState::Base;
  ComponentState scroll_y_state_ = ComponentState::Base;
  bool dragging_x_ = false;
  bool dragging_y_ = false;
  float drag_thumb_anchor_ = 0.f;
  float drag_mouse_anchor_ = 0.f;

  mutable Metrics metrics_cache_{};
  mutable bool metrics_valid_ = false;
};

// Scrollable viewport (Container with scroll_y = Auto by default).
class SIMPLE_UI_API ScrollView : public Container {
public:
  ScrollView();
  ~ScrollView() override;
};

// Opaque runtime handle for the window, renderer, and font resources.
// Create/destroy only through ui_create / ui_destroy.
// (Forward-declared at top of this header.)
