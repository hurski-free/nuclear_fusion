#pragma once

#include "simple_ui/export.h"
#include "simple_ui/platform.h"

#include <functional>
#include <string>
#include <vector>

// Opaque runtime handle; create/destroy via ui_create / ui_destroy.
struct UiContext;

// Bitmap font atlas (CPU glyph metrics + pixels). Owned by UiContext when
// created via ui_create_font / the default atlas from ui_create.
class FontAtlas;

// Display presentation mode for the application window.
enum class ScreenMode {
  Windowed,    // Decorated window with fixed client size (not user-resizable).
  Borderless,  // Borderless window covering the entire primary monitor.
  Fullscreen,  // Exclusive fullscreen via DXGI.
};

// Initial window/display configuration passed to ui_create.
struct ScreenSettings {
  ScreenMode screen_mode;
  // OS window / swap-chain size for Windowed; preferred buffer hint for
  // Fullscreen. Ignored for Borderless (monitor size is used).
  // Independent of resolution_* (the offscreen scene / UI coordinate space).
  int screen_width;
  int screen_height;
  // Logical render resolution (scene RT + UI/mouse coordinates).
  // 0 = match the window/swap-chain size at create time.
  int resolution_width = 0;
  int resolution_height = 0;
  // Multisample count for the offscreen scene target (1 = off). Typical: 2, 4, 8.
  // Clamped to device-supported values. Changeable later via ui_set_msaa_samples.
  int msaa_samples = 4;
};

// RGBA color in linear 0..1 range (alpha 0 = fully transparent).
struct Color {
  float r = 1.f;
  float g = 1.f;
  float b = 1.f;
  float a = 1.f;
};

// UI coordinate system: (0,0) is the top-left of the logical resolution
// (scene texture), X increases right, Y increases down. Mouse positions are
// scaled from the OS client area into this space.

// Per-frame mouse snapshot. Edge flags (*_pressed / *_released) are valid
// only until the next ui_process_messages call.
struct MouseEvents {
  float x = 0.f;
  float y = 0.f;
  bool left_down = false;
  bool right_down = false;
  bool middle_down = false;
  bool left_pressed = false;    // true on the frame the button went down
  bool right_pressed = false;
  bool middle_pressed = false;
  bool left_released = false;   // true on the frame the button went up
  bool right_released = false;
  bool middle_released = false;
  float wheel_delta = 0.f;      // wheel notches this frame (can accumulate)
  // Set by a handler so parents / lower siblings ignore this frame's wheel.
  // Cleared each frame with wheel_delta. Mutable so const MouseEvents& handlers
  // can mark consumption without changing the handle_messages signature.
  mutable bool wheel_consumed = false;
  // Set when a handler consumes the left-button press (e.g. Select dropdown)
  // so widgets underneath do not also activate on the same click.
  mutable bool click_consumed = false;
};

// Per-frame keyboard snapshot. Indexed by virtual-key codes (0..255).
// Edge arrays and text chars are cleared at the start of each ui_process_messages.
struct KeyboardEvents {
  static constexpr int kKeyCount = 256;
  static constexpr int kMaxChars = 32;
  bool down[kKeyCount] = {};
  bool pressed[kKeyCount] = {};
  bool released[kKeyCount] = {};
  // UTF-16 code units from WM_CHAR this frame (not including control keys).
  wchar_t chars[kMaxChars] = {};
  int char_count = 0;
};

// GPU-agnostic draw primitive emitted by components.
enum class DrawCommandType {
  Rect,         // Solid colored rectangle.
  Circle,       // Filled ellipse inscribed in x/y/width/height (SDF + AA).
  RoundedRect,  // Filled rect with corner_radius (SDF + AA); capsule when radius >= h/2.
  Triangle,     // Filled isosceles triangle in x/y/width/height. Tip down by default;
                // tip up when corner_radius > 0.
  Image,        // Textured quad; texture_id must be a valid loaded texture.
  Text,         // Text. Alignment via text_align; wrap enables word wrap.
  PushClip,     // Enable scissor to x/y/width/height (nested capable).
  PopClip,      // Restore previous scissor (or disable).
};

// Horizontal/vertical placement of Text commands inside their rect.
enum class TextAlign {
  Center,      // Centered on both axes (default for buttons).
  LeftTop,     // Left-aligned, top of the rect (multi-line starts at top).
  LeftMiddle,  // Left-aligned, vertically centered in the rect.
};

// Texture sampling for Image (and any DrawCommandType::Image quad).
// Controls how texels are interpolated when the quad is scaled.
enum class ImageFilter {
  // Nearest-neighbor (point) sampling. Sharp pixels; good for pixel art / icons
  // that should not blur when upscaled or downscaled.
  Nearest,
  // Bilinear filtering (default). Smooth when scaled; softens edges.
  Linear,
};

// Outline drawn around glyph silhouettes (Text / Label). thickness 0 = off.
struct TextOutline {
  float thickness = 0.f;
  Color color{};
};

struct DrawCommand {
  DrawCommandType type = DrawCommandType::Rect;
  float x = 0.f;
  float y = 0.f;
  float width = 0.f;
  float height = 0.f;
  Color color{};
  int texture_id = -1;  // Image: loaded texture id; otherwise unused (-1)
  int layer = 0;        // Lower layers are drawn first.
  std::wstring text;
  bool wrap = false;    // Text only: enable multi-line wrapping
  TextAlign text_align = TextAlign::Center;
  // Image UV rect in [0..1].
  float u0 = 0.f;
  float v0 = 0.f;
  float u1 = 1.f;
  float v1 = 1.f;
  // Image: texture filter (Nearest or Linear).
  ImageFilter image_filter = ImageFilter::Linear;
  // RoundedRect: corner radius in pixels (clamped to half the shorter side).
  // Triangle: tip up when > 0; tip down when 0.
  float corner_radius = 0.f;
  // Text: pixel size. 0 = use the font atlas size.
  float font_size = 0.f;
  // Text: non-owning atlas. nullptr = context default font.
  const FontAtlas* font_atlas = nullptr;
  // Text: outline around glyphs (thickness 0 = none).
  TextOutline outline{};
};

// How a border is placed relative to the component box.
enum class BorderMode {
  None,  // No border (default).
  Out,   // Drawn outside the box; does not consume width/height.
  In,    // Drawn inside the box; overlays the edges of width/height.
};

struct Border {
  float thickness = 0.f;
  BorderMode mode = BorderMode::None;
  Color color{};
};

// Visual look for one interaction state. Side borders override `border`
// when their mode is not None.
struct ComponentStyle {
  Color background_color{0.f, 0.f, 0.f, 0.f};
  Border border{};
  Border border_left{};
  Border border_right{};
  Border border_top{};
  Border border_bottom{};
};

// Independent property transition durations in seconds.
// 0 means the property snaps immediately on state change.
struct StyleTransition {
  float background_duration = 0.f;
  float border_duration = 0.f;
};

// Interaction / visual state used to pick style_* and drive animations.
enum class ComponentState {
  Base,
  Hovered,
  Active,
};

// Scrollbar visibility policy for one axis.
enum class ScrollMode {
  Hidden,  // Never show a scrollbar; content is clipped without scrolling UI.
  Scroll,  // Always show a scrollbar.
  Auto,    // Show only when content overflows the viewport.
};

// Where scrollbars are placed relative to the container box.
enum class ScrollPlaceMode {
  In,   // Overlay/consume inner space; outer layout size stays width x height.
  Out,  // Extend occupied layout size by scrollbar thickness.
};

// Visual metrics/colors for one scrollbar interaction state.
// Theme defaults live in UiStylePresets (see style_presets.h).
struct ScrollbarStateStyle {
  Color track_color{0.f, 0.f, 0.f, 0.f};
  Color thumb_color{0.f, 0.f, 0.f, 0.f};
  float thickness = 0.f;  // Cross-axis size of the bar (px).
};

// Reusable scroll-axis configuration (mode + per-state visuals).
struct ScrollBar {
  ScrollMode mode = ScrollMode::Hidden;
  ScrollbarStateStyle style_base{};
  ScrollbarStateStyle style_hovered{};
  ScrollbarStateStyle style_active{};
};

// Relative rectangle inside a checkbox (coordinates in 0..1 of the box).
struct CheckMarkPart {
  float x = 0.f;
  float y = 0.f;
  float w = 1.f;
  float h = 1.f;
};

// Custom check glyph built from axis-aligned parts (or from generators).
struct CheckMarkShape {
  std::vector<CheckMarkPart> parts;
};

enum class CheckMarkKind {
  Square,     // Filled inset square.
  Checkmark,  // Built-in checkmark shape (✓ via rects).
  Custom,     // Uses custom_check parts.
};

