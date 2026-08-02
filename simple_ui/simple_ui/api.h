#pragma once

#include "simple_ui/types.h"
#include "simple_ui/platform.h"
#include "simple_ui/style_presets.h"

// --- Style presets -----------------------------------------------------------
// IMPORTANT: Call ui_set_style_presets() BEFORE creating any components.
// Constructors copy from the current presets; later changes do not retro-apply.
// Per-component style/size/color fields can still be overridden after creation.
SIMPLE_UI_API void ui_set_style_presets(const UiStylePresets& presets);
// Immutable view of the current global presets (valid until the next set).
SIMPLE_UI_API const UiStylePresets& ui_get_style_presets();

// Creates the UI context (window + D3D + default font atlas).
// iconPath may be nullptr. Returns nullptr on failure.
SIMPLE_UI_API UiContext* ui_create(const wchar_t* title,
                                   const ScreenSettings* screenSettings,
                                   const wchar_t* iconPath);

SIMPLE_UI_API void ui_destroy(UiContext* ctx);

// True when the window and renderer are usable.
SIMPLE_UI_API bool ui_is_valid(const UiContext* ctx);

// Pumps Win32 messages and refreshes MouseEvents / KeyboardEvents.
// Returns false when the window should close (WM_QUIT).
SIMPLE_UI_API bool ui_process_messages(UiContext* ctx);

SIMPLE_UI_API void ui_clear(UiContext* ctx, const Color& color);
SIMPLE_UI_API void ui_present(UiContext* ctx);

// Re-bind the offscreen scene color target (e.g. after Canvas::end_draw).
SIMPLE_UI_API void ui_bind_scene_target(UiContext* ctx);

SIMPLE_UI_API HWND ui_get_hwnd(const UiContext* ctx);
SIMPLE_UI_API ID3D11Device* ui_get_device(const UiContext* ctx);
SIMPLE_UI_API ID3D11DeviceContext* ui_get_device_context(const UiContext* ctx);
SIMPLE_UI_API int ui_get_width(const UiContext* ctx);
SIMPLE_UI_API int ui_get_height(const UiContext* ctx);
SIMPLE_UI_API ScreenMode ui_get_screen_mode(const UiContext* ctx);

// Switches display mode at runtime (recreates swap-chain buffers as needed).
SIMPLE_UI_API bool ui_set_screen_mode(UiContext* ctx, ScreenMode mode);

// Preferred client size for Windowed mode (also stored when leaving Windowed so
// returning to Windowed restores this size). When already Windowed, resizes the
// window and swap-chain immediately.
SIMPLE_UI_API bool ui_set_screen_size(UiContext* ctx, int width, int height);

// MSAA sample count for the offscreen scene texture (1 disables MSAA; the
// scene is still rendered offscreen and blitted). Values are snapped to
// 1/2/4/8 and clamped to what the GPU supports. Recreates targets.
SIMPLE_UI_API bool ui_set_msaa_samples(UiContext* ctx, int samples);
SIMPLE_UI_API int ui_get_msaa_samples(const UiContext* ctx);

// Final blit brightness multiplier (1.0 = unchanged). Clamped to >= 0.
// Applied when copying the offscreen scene to the swap-chain backbuffer.
SIMPLE_UI_API void ui_set_brightness(UiContext* ctx, float brightness);
SIMPLE_UI_API float ui_get_brightness(const UiContext* ctx);

// Valid until the next ui_process_messages call.
SIMPLE_UI_API const MouseEvents* ui_get_mouse_events(const UiContext* ctx);
SIMPLE_UI_API const KeyboardEvents* ui_get_keyboard_events(const UiContext* ctx);

// Loads an image file (PNG/JPEG/BMP/TIFF via WIC) into GPU memory.
// Returns texture id (>= 0) or -1 on failure.
SIMPLE_UI_API int ui_load_texture(UiContext* ctx, const wchar_t* path);
// Creates a texture from tightly packed RGBA8 pixels. Returns id or -1.
SIMPLE_UI_API int ui_create_texture(UiContext* ctx, int width, int height,
                                    const unsigned char* rgba);
SIMPLE_UI_API void ui_unload_texture(UiContext* ctx, int texture_id);
SIMPLE_UI_API bool ui_get_texture_size(UiContext* ctx, int texture_id, int* out_w,
                                       int* out_h);

// --- Fonts -----------------------------------------------------------------
// Default atlas created by ui_create (may be nullptr if no system font found).
SIMPLE_UI_API const FontAtlas* ui_default_font(const UiContext* ctx);
// Build a new atlas from a TTF/OTF file. size_px is glyph pixel height.
// The atlas is owned by ctx until ui_destroy_font / ui_destroy.
SIMPLE_UI_API const FontAtlas* ui_create_font(UiContext* ctx,
                                             const wchar_t* ttf_path,
                                             float size_px);
// Destroys an atlas from ui_create_font. No-op for the default font / nullptr.
SIMPLE_UI_API void ui_destroy_font(UiContext* ctx, const FontAtlas* font);

// Pixel width of text (0 if unavailable). font nullptr = default atlas.
// font_size <= 0 uses the atlas default size.
SIMPLE_UI_API float ui_measure_text(const UiContext* ctx, const wchar_t* text,
                                    float font_size = 0.f,
                                    const FontAtlas* font = nullptr);
SIMPLE_UI_API float ui_measure_text_n(const UiContext* ctx, const wchar_t* text,
                                      size_t length, float font_size = 0.f,
                                      const FontAtlas* font = nullptr);
SIMPLE_UI_API size_t ui_text_index_at_x(const UiContext* ctx, const wchar_t* text,
                                        float x, float font_size = 0.f,
                                        const FontAtlas* font = nullptr);
SIMPLE_UI_API size_t ui_text_index_at_x_n(const UiContext* ctx,
                                         const wchar_t* text, size_t length,
                                         float x, float font_size = 0.f,
                                         const FontAtlas* font = nullptr);
SIMPLE_UI_API float ui_font_atlas_size(const UiContext* ctx,
                                       const FontAtlas* font = nullptr);
SIMPLE_UI_API float ui_font_line_height(const UiContext* ctx,
                                        float font_size = 0.f,
                                        const FontAtlas* font = nullptr);
SIMPLE_UI_API float ui_font_ascent(const UiContext* ctx, float font_size = 0.f,
                                   const FontAtlas* font = nullptr);

