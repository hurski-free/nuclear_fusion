#pragma once

#include "simple_ui.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <chrono>
#include <thread>

// Frame pacing helper. When enabled, waits for the primary output vblank
// (with a sleep fallback) so the main loop respects vertical sync even
// though simple_ui::ui_present does not expose a sync-interval API.
class VSyncPacer {
public:
  void set_enabled(bool enabled) { enabled_ = enabled; }
  bool enabled() const { return enabled_; }

  void begin_frame() {
    frame_start_ = std::chrono::steady_clock::now();
  }

  void end_frame(UiContext* ctx) {
    if (!enabled_) {
      return;
    }

    if (WaitForVBlank(ctx)) {
      return;
    }

    using clock = std::chrono::steady_clock;
    constexpr auto kTarget = std::chrono::nanoseconds(16666667);
    const auto elapsed = clock::now() - frame_start_;
    if (elapsed < kTarget) {
      std::this_thread::sleep_for(kTarget - elapsed);
    }
  }

private:
  bool WaitForVBlank(UiContext* ctx) {
    if (!ctx) {
      return false;
    }

    ID3D11Device* device = ui_get_device(ctx);
    if (!device) {
      return false;
    }

    IDXGIDevice* dxgi_device = nullptr;
    if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice),
                                      reinterpret_cast<void**>(&dxgi_device))) ||
        !dxgi_device) {
      return false;
    }

    IDXGIAdapter* adapter = nullptr;
    HRESULT hr = dxgi_device->GetAdapter(&adapter);
    dxgi_device->Release();
    if (FAILED(hr) || !adapter) {
      return false;
    }

    IDXGIOutput* output = nullptr;
    hr = adapter->EnumOutputs(0, &output);
    adapter->Release();
    if (FAILED(hr) || !output) {
      return false;
    }

    hr = output->WaitForVBlank();
    output->Release();
    return SUCCEEDED(hr);
  }

  bool enabled_ = true;
  std::chrono::steady_clock::time_point frame_start_{};
};
