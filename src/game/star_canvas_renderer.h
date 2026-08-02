#pragma once

#include "game_types.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wincodec.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowscodecs.lib")

// Draws drifting background stars + the main star PNG (with alpha) into a
// simple_ui Canvas via custom D3D11 commands.
class StarCanvasRenderer {
public:
  ~StarCanvasRenderer() { Release(); }

  bool EnsureReady(ID3D11Device* device) {
    if (!device) {
      return false;
    }
    if (ready_) {
      return true;
    }
    if (!CreatePipeline(device)) {
      return false;
    }
    const StarType types[4] = {StarType::BrownDwarf, StarType::YellowDwarf,
                               StarType::BlueGiant, StarType::NeutronStar};
    for (int i = 0; i < 4; ++i) {
      if (!LoadStarTexture(device, StarTypeImagePath(types[i]), i)) {
        return false;
      }
    }
    active_star_ = 0;
    InitDriftStars(1280, 720);
    ready_ = true;
    return true;
  }

  void SetStarType(StarType type) {
    const int i = static_cast<int>(type);
    active_star_ = (i >= 0 && i < 4) ? i : 0;
  }

  void Release() {
    for (int i = 0; i < 4; ++i) {
      if (star_srv_[i]) {
        star_srv_[i]->Release();
        star_srv_[i] = nullptr;
      }
    }
    if (white_srv_) {
      white_srv_->Release();
      white_srv_ = nullptr;
    }
    if (vb_) {
      vb_->Release();
      vb_ = nullptr;
    }
    if (vs_) {
      vs_->Release();
      vs_ = nullptr;
    }
    if (ps_) {
      ps_->Release();
      ps_ = nullptr;
    }
    if (layout_) {
      layout_->Release();
      layout_ = nullptr;
    }
    if (cb_) {
      cb_->Release();
      cb_ = nullptr;
    }
    if (blend_) {
      blend_->Release();
      blend_ = nullptr;
    }
    if (rs_) {
      rs_->Release();
      rs_ = nullptr;
    }
    if (dss_) {
      dss_->Release();
      dss_ = nullptr;
    }
    if (samp_) {
      samp_->Release();
      samp_ = nullptr;
    }
    ready_ = false;
  }

  void Resize(int width, int height) {
    if (width <= 0 || height <= 0) {
      return;
    }
    if (width == view_w_ && height == view_h_) {
      return;
    }
    view_w_ = width;
    view_h_ = height;
    InitDriftStars(width, height);
  }

  void Update(float dt) {
    if (view_w_ <= 0 || view_h_ <= 0) {
      return;
    }
    for (auto& s : drift_) {
      s.x += s.speed * dt;
      if (s.x > static_cast<float>(view_w_) + 8.f) {
        s.x = -8.f;
        s.y = RandFloat(0.f, static_cast<float>(view_h_));
      }
    }
  }

  void Draw(ID3D11DeviceContext* ctx, float star_cx, float star_cy,
            float star_size, bool draw_main_star) {
    if (!ready_ || !ctx || !vb_) {
      return;
    }

    verts_.clear();
    for (const auto& s : drift_) {
      PushQuad(s.x - s.size * 0.5f, s.y - s.size * 0.5f, s.size, s.size,
               s.brightness, s.brightness, s.brightness * 0.9f, s.brightness,
               false);
    }

    if (draw_main_star && star_size > 1.f) {
      PushQuad(star_cx - star_size * 0.5f, star_cy - star_size * 0.5f,
               star_size, star_size, 1.f, 1.f, 1.f, 1.f, true);
    }

    if (verts_.empty()) {
      return;
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(vb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
      return;
    }
    const size_t bytes = verts_.size() * sizeof(Vertex);
    const size_t copy =
        bytes < kMaxVertices * sizeof(Vertex) ? bytes : kMaxVertices * sizeof(Vertex);
    std::memcpy(mapped.pData, verts_.data(), copy);
    ctx->Unmap(vb_, 0);

    struct alignas(16) ScreenCB {
      float screen_w;
      float screen_h;
      float pad0;
      float pad1;
    } cb{static_cast<float>(view_w_), static_cast<float>(view_h_), 0.f, 0.f};
    ctx->UpdateSubresource(cb_, 0, nullptr, &cb, 0, 0);

    const UINT stride = sizeof(Vertex);
    const UINT offset = 0;
    ctx->IASetInputLayout(layout_);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &vb_, &stride, &offset);
    ctx->VSSetShader(vs_, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &cb_);
    ctx->PSSetShader(ps_, nullptr, 0);
    ctx->PSSetSamplers(0, 1, &samp_);
    ctx->OMSetBlendState(blend_, nullptr, 0xffffffff);
    ctx->OMSetDepthStencilState(dss_, 0);
    ctx->RSSetState(rs_);

    // Drift stars (white texel).
    const UINT drift_count =
        static_cast<UINT>(draw_main_star && star_size > 1.f
                              ? (verts_.size() - 6)
                              : verts_.size());
    if (drift_count > 0) {
      ctx->PSSetShaderResources(0, 1, &white_srv_);
      ctx->Draw(drift_count, 0);
    }

    // Main star PNG with alpha (per current star type).
    ID3D11ShaderResourceView* active = star_srv_[active_star_];
    if (draw_main_star && star_size > 1.f && active) {
      ctx->PSSetShaderResources(0, 1, &active);
      ctx->Draw(6, drift_count);
    }

    ID3D11ShaderResourceView* null_srv = nullptr;
    ctx->PSSetShaderResources(0, 1, &null_srv);
  }

private:
  struct Vertex {
    float x, y, u, v;
    float r, g, b, a;
  };

  struct DriftStar {
    float x = 0.f;
    float y = 0.f;
    float size = 2.f;
    float speed = 20.f;
    float brightness = 1.f;
  };

  static constexpr size_t kMaxVertices = 4096;

  bool CreatePipeline(ID3D11Device* device) {
    static const char* kShader = R"(
cbuffer ScreenCB : register(b0) {
  float2 screen;
  float2 _pad;
};
struct VSIn {
  float2 pos : POSITION;
  float2 uv : TEXCOORD0;
  float4 col : COLOR0;
};
struct VSOut {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float4 col : COLOR0;
};
Texture2D tex0 : register(t0);
SamplerState samp0 : register(s0);

VSOut VSMain(VSIn i) {
  VSOut o;
  float2 ndc;
  ndc.x = (i.pos.x / screen.x) * 2.0 - 1.0;
  ndc.y = 1.0 - (i.pos.y / screen.y) * 2.0;
  o.pos = float4(ndc, 0.0, 1.0);
  o.uv = i.uv;
  o.col = i.col;
  return o;
}
float4 PSMain(VSOut i) : SV_Target {
  float4 t = tex0.Sample(samp0, i.uv);
  return t * i.col;
}
)";

    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* err = nullptr;
    HRESULT hr = D3DCompile(kShader, std::strlen(kShader), nullptr, nullptr,
                            nullptr, "VSMain", "vs_4_0", 0, 0, &vs_blob, &err);
    if (FAILED(hr)) {
      if (err) {
        err->Release();
      }
      return false;
    }
    hr = device->CreateVertexShader(vs_blob->GetBufferPointer(),
                                    vs_blob->GetBufferSize(), nullptr, &vs_);
    if (FAILED(hr)) {
      vs_blob->Release();
      return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = device->CreateInputLayout(layout, 3, vs_blob->GetBufferPointer(),
                                   vs_blob->GetBufferSize(), &layout_);
    vs_blob->Release();
    if (FAILED(hr)) {
      return false;
    }

    ID3DBlob* ps_blob = nullptr;
    hr = D3DCompile(kShader, std::strlen(kShader), nullptr, nullptr, nullptr,
                    "PSMain", "ps_4_0", 0, 0, &ps_blob, &err);
    if (FAILED(hr)) {
      if (err) {
        err->Release();
      }
      return false;
    }
    hr = device->CreatePixelShader(ps_blob->GetBufferPointer(),
                                   ps_blob->GetBufferSize(), nullptr, &ps_);
    ps_blob->Release();
    if (FAILED(hr)) {
      return false;
    }

    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = static_cast<UINT>(kMaxVertices * sizeof(Vertex));
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&vbd, nullptr, &vb_))) {
      return false;
    }

    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = 16;
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device->CreateBuffer(&cbd, nullptr, &cb_))) {
      return false;
    }

    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(device->CreateBlendState(&bd, &blend_))) {
      return false;
    }

    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    if (FAILED(device->CreateRasterizerState(&rd, &rs_))) {
      return false;
    }

    D3D11_DEPTH_STENCIL_DESC dd{};
    dd.DepthEnable = FALSE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dd.DepthFunc = D3D11_COMPARISON_ALWAYS;
    if (FAILED(device->CreateDepthStencilState(&dd, &dss_))) {
      return false;
    }

    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(device->CreateSamplerState(&sd, &samp_))) {
      return false;
    }

    // 1x1 white texture for drift stars.
    const uint32_t white = 0xFFFFFFFFu;
    D3D11_TEXTURE2D_DESC td{};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA srd{};
    srd.pSysMem = &white;
    srd.SysMemPitch = 4;
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(device->CreateTexture2D(&td, &srd, &tex))) {
      return false;
    }
    hr = device->CreateShaderResourceView(tex, nullptr, &white_srv_);
    tex->Release();
    return SUCCEEDED(hr);
  }

  bool LoadStarTexture(ID3D11Device* device, const wchar_t* path, int slot) {
    if (slot < 0 || slot >= 4) {
      return false;
    }
    IWICImagingFactory* factory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                  CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
      // Try COM init once.
      CoInitializeEx(nullptr, COINIT_MULTITHREADED);
      hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
      if (FAILED(hr)) {
        return false;
      }
    }

    IWICBitmapDecoder* decoder = nullptr;
    hr = factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
                                            WICDecodeMetadataCacheOnLoad,
                                            &decoder);
    if (FAILED(hr)) {
      factory->Release();
      return false;
    }

    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
      decoder->Release();
      factory->Release();
      return false;
    }

    IWICFormatConverter* conv = nullptr;
    hr = factory->CreateFormatConverter(&conv);
    if (FAILED(hr)) {
      frame->Release();
      decoder->Release();
      factory->Release();
      return false;
    }

    hr = conv->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                          WICBitmapDitherTypeNone, nullptr, 0.0,
                          WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
      conv->Release();
      frame->Release();
      decoder->Release();
      factory->Release();
      return false;
    }

    UINT w = 0, h = 0;
    conv->GetSize(&w, &h);
    std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 4);
    hr = conv->CopyPixels(nullptr, w * 4, static_cast<UINT>(pixels.size()),
                          pixels.data());

    conv->Release();
    frame->Release();
    decoder->Release();
    factory->Release();
    if (FAILED(hr)) {
      return false;
    }

    D3D11_TEXTURE2D_DESC td{};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA srd{};
    srd.pSysMem = pixels.data();
    srd.SysMemPitch = w * 4;
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(device->CreateTexture2D(&td, &srd, &tex))) {
      return false;
    }
    if (star_srv_[slot]) {
      star_srv_[slot]->Release();
      star_srv_[slot] = nullptr;
    }
    hr = device->CreateShaderResourceView(tex, nullptr, &star_srv_[slot]);
    tex->Release();
    return SUCCEEDED(hr);
  }

  void InitDriftStars(int width, int height) {
    drift_.clear();
    drift_.reserve(80);
    for (int i = 0; i < 80; ++i) {
      DriftStar s;
      s.x = RandFloat(0.f, static_cast<float>(width));
      s.y = RandFloat(0.f, static_cast<float>(height));
      s.size = RandFloat(1.2f, 4.5f);
      s.speed = RandFloat(8.f, 36.f);
      s.brightness = RandFloat(0.25f, 1.f);
      drift_.push_back(s);
    }
  }

  float RandFloat(float a, float b) {
    std::uniform_real_distribution<float> dist(a, b);
    return dist(rng_);
  }

  void PushQuad(float x, float y, float w, float h, float r, float g, float b,
                float a, bool /*textured*/) {
    if (verts_.size() + 6 > kMaxVertices) {
      return;
    }
    const Vertex v[6] = {
        {x, y, 0.f, 0.f, r, g, b, a},
        {x + w, y, 1.f, 0.f, r, g, b, a},
        {x + w, y + h, 1.f, 1.f, r, g, b, a},
        {x, y, 0.f, 0.f, r, g, b, a},
        {x + w, y + h, 1.f, 1.f, r, g, b, a},
        {x, y + h, 0.f, 1.f, r, g, b, a},
    };
    verts_.insert(verts_.end(), v, v + 6);
  }

  bool ready_ = false;
  int view_w_ = 0;
  int view_h_ = 0;
  std::mt19937 rng_{std::random_device{}()};
  std::vector<DriftStar> drift_;
  std::vector<Vertex> verts_;

  ID3D11VertexShader* vs_ = nullptr;
  ID3D11PixelShader* ps_ = nullptr;
  ID3D11InputLayout* layout_ = nullptr;
  ID3D11Buffer* vb_ = nullptr;
  ID3D11Buffer* cb_ = nullptr;
  ID3D11BlendState* blend_ = nullptr;
  ID3D11RasterizerState* rs_ = nullptr;
  ID3D11DepthStencilState* dss_ = nullptr;
  ID3D11SamplerState* samp_ = nullptr;
  ID3D11ShaderResourceView* star_srv_[4] = {};
  ID3D11ShaderResourceView* white_srv_ = nullptr;
  int active_star_ = 0;
};
