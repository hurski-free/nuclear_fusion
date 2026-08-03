#include "star_canvas_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

StarCanvasRenderer::~StarCanvasRenderer() { Release(); }

bool StarCanvasRenderer::EnsureReady(ID3D11Device* device) {
    if (!device) {
      return false;
    }
    if (ready_) {
      return true;
    }
    if (!CreatePipeline(device)) {
      return false;
    }
    BuildIcosphere(3);
    active_star_ = 0;
    InitDriftStars(1280, 720);
    InitFlares();
    ready_ = true;
    return true;
  }

void StarCanvasRenderer::SetStarType(StarType type) {
    const int i = static_cast<int>(type);
    active_star_ = (i >= 0 && i < 4) ? i : 0;
  }

void StarCanvasRenderer::Release() {
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
    ico_pos_.clear();
    ico_idx_.clear();
    ready_ = false;
  }

void StarCanvasRenderer::Resize(int width, int height) {
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

void StarCanvasRenderer::Update(float dt) {
    anim_t_ += dt;
    if (click_flash_ > 0.f) {
      click_flash_ = std::max(0.f, click_flash_ - dt * 2.8f);
    }
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
    UpdateFlares(dt);
  }

void StarCanvasRenderer::SetOrbitCounts(double protons, double neutrons,
                                        double electrons) {
    orbit_p_ = std::max(0.0, protons);
    orbit_n_ = std::max(0.0, neutrons);
    orbit_e_ = std::max(0.0, electrons);
  }

void StarCanvasRenderer::TriggerClickFlash(float strength) {
    click_flash_ = std::clamp(strength, 0.f, 1.f);
  }

void StarCanvasRenderer::Draw(ID3D11DeviceContext* ctx, float star_cx, float star_cy,
            float star_size, bool draw_main_star) {
    if (!ready_ || !ctx || !vb_) {
      return;
    }

    BindPipeline(ctx);
    verts_.clear();

    for (const auto& s : drift_) {
      EnsureVertCapacity(ctx, 6);
      PushQuad(s.x - s.size * 0.5f, s.y - s.size * 0.5f, s.size, s.size,
               s.brightness, s.brightness, s.brightness * 0.9f, s.brightness);
    }
    FlushVerts(ctx);

    if (draw_main_star && star_size > 1.f) {
      const float radius = star_size * 0.5f;
      PushStarGlow(ctx, star_cx, star_cy, star_size);
      FlushVerts(ctx);
      // Back-facing swirl sheets first for real 3D wrapping.
      PushFlares(ctx, star_cx, star_cy, radius, /*front=*/false);
      FlushVerts(ctx);
      PushIcosphere(ctx, star_cx, star_cy, radius);
      FlushVerts(ctx);
      PushFlares(ctx, star_cx, star_cy, radius, /*front=*/true);
      FlushVerts(ctx);
      PushOrbitParticles(ctx, star_cx, star_cy, radius);
      FlushVerts(ctx);
    }

    ID3D11ShaderResourceView* null_srv = nullptr;
    ctx->PSSetShaderResources(0, 1, &null_srv);
  }

void StarCanvasRenderer::BindPipeline(ID3D11DeviceContext* ctx) {
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
    ctx->PSSetShaderResources(0, 1, &white_srv_);
    ctx->OMSetBlendState(blend_, nullptr, 0xffffffff);
    ctx->OMSetDepthStencilState(dss_, 0);
    ctx->RSSetState(rs_);
  }

void StarCanvasRenderer::FlushVerts(ID3D11DeviceContext* ctx) {
    if (!ctx || verts_.empty()) {
      return;
    }
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(vb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
      verts_.clear();
      return;
    }
    const size_t count = std::min(verts_.size(), kMaxVertices);
    std::memcpy(mapped.pData, verts_.data(), count * sizeof(Vertex));
    ctx->Unmap(vb_, 0);
    ctx->Draw(static_cast<UINT>(count), 0);
    verts_.clear();
  }

void StarCanvasRenderer::EnsureVertCapacity(ID3D11DeviceContext* ctx, size_t need) {
    if (verts_.size() + need > kMaxVertices) {
      FlushVerts(ctx);
    }
  }

StarCanvasRenderer::Vec3 StarCanvasRenderer::Add(Vec3 a, Vec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
  }

StarCanvasRenderer::Vec3 StarCanvasRenderer::Sub(Vec3 a, Vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
  }

StarCanvasRenderer::Vec3 StarCanvasRenderer::Mul(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }

float StarCanvasRenderer::Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

StarCanvasRenderer::Vec3 StarCanvasRenderer::Cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
  }

StarCanvasRenderer::Vec3 StarCanvasRenderer::Normalize(Vec3 v) {
    const float len = std::sqrt(Dot(v, v));
    if (len < 1e-8f) {
      return {0.f, 0.f, 1.f};
    }
    return Mul(v, 1.f / len);
  }

void StarCanvasRenderer::BuildIcosphere(int subdivisions) {
    ico_pos_.clear();
    ico_idx_.clear();

    const float t = (1.f + std::sqrt(5.f)) * 0.5f;
    const Vec3 base[] = {
        {-1, t, 0},  {1, t, 0},  {-1, -t, 0}, {1, -t, 0},
        {0, -1, t},  {0, 1, t},  {0, -1, -t}, {0, 1, -t},
        {t, 0, -1},  {t, 0, 1},  {-t, 0, -1}, {-t, 0, 1},
    };
    ico_pos_.reserve(12);
    for (const auto& p : base) {
      ico_pos_.push_back(Normalize(p));
    }

    const uint32_t faces[][3] = {
        {0, 11, 5},  {0, 5, 1},  {0, 1, 7},  {0, 7, 10}, {0, 10, 11},
        {1, 5, 9},   {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4},   {3, 4, 2},  {3, 2, 6},  {3, 6, 8},  {3, 8, 9},
        {4, 9, 5},   {2, 4, 11}, {6, 2, 10}, {8, 6, 7},  {9, 8, 1},
    };
    ico_idx_.reserve(20 * 3);
    for (const auto& f : faces) {
      ico_idx_.push_back(f[0]);
      ico_idx_.push_back(f[1]);
      ico_idx_.push_back(f[2]);
    }

    for (int s = 0; s < subdivisions; ++s) {
      std::map<uint64_t, uint32_t> midpoint;
      std::vector<uint32_t> next;
      next.reserve(ico_idx_.size() * 4);

      auto mid = [&](uint32_t a, uint32_t b) -> uint32_t {
        const uint32_t lo = a < b ? a : b;
        const uint32_t hi = a < b ? b : a;
        const uint64_t key = (static_cast<uint64_t>(lo) << 32) | hi;
        const auto it = midpoint.find(key);
        if (it != midpoint.end()) {
          return it->second;
        }
        const Vec3 m = Normalize(Add(ico_pos_[a], ico_pos_[b]));
        const uint32_t idx = static_cast<uint32_t>(ico_pos_.size());
        ico_pos_.push_back(m);
        midpoint.emplace(key, idx);
        return idx;
      };

      for (size_t i = 0; i + 2 < ico_idx_.size(); i += 3) {
        const uint32_t v0 = ico_idx_[i];
        const uint32_t v1 = ico_idx_[i + 1];
        const uint32_t v2 = ico_idx_[i + 2];
        const uint32_t a = mid(v0, v1);
        const uint32_t b = mid(v1, v2);
        const uint32_t c = mid(v2, v0);
        next.insert(next.end(), {v0, a, c, v1, b, a, v2, c, b, a, b, c});
      }
      ico_idx_.swap(next);
    }
  }

void StarCanvasRenderer::StarPalette(float& cr, float& cg, float& cb, float& glow_r,
                   float& glow_g, float& glow_b) const {
    switch (static_cast<StarType>(active_star_)) {
      case StarType::BrownDwarf:
        cr = 1.0f;
        cg = 0.62f;
        cb = 0.28f;
        glow_r = 1.0f;
        glow_g = 0.40f;
        glow_b = 0.12f;
        break;
      case StarType::YellowDwarf:
        cr = 1.0f;
        cg = 0.96f;
        cb = 0.62f;
        glow_r = 1.0f;
        glow_g = 0.78f;
        glow_b = 0.28f;
        break;
      case StarType::BlueGiant:
        cr = 0.72f;
        cg = 0.88f;
        cb = 1.0f;
        glow_r = 0.35f;
        glow_g = 0.55f;
        glow_b = 1.0f;
        break;
      case StarType::NeutronStar:
      default:
        cr = 0.92f;
        cg = 0.96f;
        cb = 1.0f;
        glow_r = 0.65f;
        glow_g = 0.55f;
        glow_b = 1.0f;
        break;
    }
  }

StarCanvasRenderer::Vec3 StarCanvasRenderer::RotateY(Vec3 v, float angle) const {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return {v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
  }

StarCanvasRenderer::Vec3 StarCanvasRenderer::RotateX(Vec3 v, float angle) const {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return {v.x, v.y * c - v.z * s, v.y * s + v.z * c};
  }

void StarCanvasRenderer::SecondaryPalette(float& hr, float& hg, float& hb, float& cr2,
                        float& cg2, float& cb2) const {
    switch (static_cast<StarType>(active_star_)) {
      case StarType::BrownDwarf:
        hr = 1.f;
        hg = 0.88f;
        hb = 0.45f;
        cr2 = 1.f;
        cg2 = 0.35f;
        cb2 = 0.12f;
        break;
      case StarType::YellowDwarf:
        hr = 1.f;
        hg = 1.f;
        hb = 0.92f;
        cr2 = 1.f;
        cg2 = 0.55f;
        cb2 = 0.12f;
        break;
      case StarType::BlueGiant:
        hr = 0.95f;
        hg = 1.f;
        hb = 1.f;
        cr2 = 0.45f;
        cg2 = 0.55f;
        cb2 = 1.f;
        break;
      case StarType::NeutronStar:
      default:
        hr = 1.f;
        hg = 1.f;
        hb = 1.f;
        cr2 = 0.75f;
        cg2 = 0.45f;
        cb2 = 1.f;
        break;
    }
  }

float StarCanvasRenderer::Hash11(float n) const {
    const float s = std::sin(n * 127.1f) * 43758.5453f;
    return s - std::floor(s);
  }

float StarCanvasRenderer::Noise2(float x, float y) const {
    const float ix = std::floor(x);
    const float iy = std::floor(y);
    const float fx = x - ix;
    const float fy = y - iy;
    const float ux = fx * fx * (3.f - 2.f * fx);
    const float uy = fy * fy * (3.f - 2.f * fy);
    const float a = Hash11(ix + iy * 57.f);
    const float b = Hash11(ix + 1.f + iy * 57.f);
    const float c = Hash11(ix + (iy + 1.f) * 57.f);
    const float d = Hash11(ix + 1.f + (iy + 1.f) * 57.f);
    return a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy;
  }

float StarCanvasRenderer::PlasmaField(StarCanvasRenderer::Vec3 p, float t) const {
    const float lon = std::atan2(p.x, p.z);
    const float lat = std::asin(std::clamp(p.y, -1.f, 1.f));
    const float bands =
        std::sin(lat * 5.5f + t * 1.6f + std::sin(lon * 2.5f - t * 0.9f));
    const float cells = std::sin(p.x * 7.f + t * 2.4f) *
                        std::cos(p.y * 6.5f - t * 1.9f) *
                        std::sin(p.z * 6.f + t * 1.3f);
    const float swirl =
        std::sin(lon * 3.5f - lat * 2.2f + t * 2.1f + cells * 1.2f);
    return std::clamp(0.45f * bands + 0.40f * cells + 0.50f * swirl, -1.f,
                      1.f);
  }

void StarCanvasRenderer::ShadeVertex(Vec3 p, float& out_r, float& out_g, float& out_b) const {
    float cr, cg, cb, gr, gg, gb;
    StarPalette(cr, cg, cb, gr, gg, gb);
    float hr, hg, hb, cr2, cg2, cb2;
    SecondaryPalette(hr, hg, hb, cr2, cg2, cb2);

    const Vec3 light = Normalize({-0.35f, 0.55f, 0.75f});
    const float ndl = std::max(0.f, Dot(p, light));
    const float rim = std::pow(1.f - std::max(0.f, p.z), 1.8f);
    // Keep the disk emissive — stars are bright, not matte planets.
    const float shade = 0.62f + 0.28f * ndl + 0.35f * rim;

    const float field = PlasmaField(p, anim_t_);
    const float hot = std::clamp(0.55f + 0.55f * field, 0.f, 1.f);
    const float cool = std::clamp(0.35f - 0.35f * field, 0.f, 1.f);
    const float shimmer =
        0.92f + 0.10f * std::sin(anim_t_ * 4.2f + p.x * 10.f + p.y * 8.f);

    float r = cr * (1.f - 0.45f * hot) + hr * hot + cr2 * cool * 0.35f;
    float g = cg * (1.f - 0.45f * hot) + hg * hot + cg2 * cool * 0.30f;
    float b = cb * (1.f - 0.45f * hot) + hb * hot + cb2 * cool * 0.40f;

    r += rim * (hr * 0.45f + cr2 * 0.25f);
    g += rim * (hg * 0.40f + cg2 * 0.20f);
    b += rim * (hb * 0.50f + cb2 * 0.30f);

    // Core boost toward camera.
    const float core = std::pow(std::max(0.f, p.z), 2.5f);
    r += core * 0.35f;
    g += core * 0.32f;
    b += core * 0.28f;

    r = std::min(1.f, r * shade * shimmer + 0.08f);
    g = std::min(1.f, g * shade * shimmer + 0.06f);
    b = std::min(1.f, b * shade * shimmer + 0.05f);
    out_r = r;
    out_g = g;
    out_b = b;
    (void)gr;
    (void)gg;
    (void)gb;
  }

void StarCanvasRenderer::PushStarGlow(ID3D11DeviceContext* ctx, float cx, float cy, float size) {
    float cr, cg, cb, gr, gg, gb;
    StarPalette(cr, cg, cb, gr, gg, gb);
    const float breath = 0.90f + 0.12f * std::sin(anim_t_ * 2.1f);
    const float beat = 0.96f + 0.04f * std::sin(anim_t_ * 5.3f);
    const float flash = 1.f + click_flash_ * 0.55f;
    const float pulse = breath * beat * flash;
    const float scales[] = {2.25f, 1.75f, 1.38f, 1.16f, 1.04f};
    const float alphas[] = {0.12f + click_flash_ * 0.10f, 0.18f, 0.28f, 0.40f,
                            0.55f + click_flash_ * 0.2f};
    for (int i = 0; i < 5; ++i) {
      EnsureVertCapacity(ctx, 6);
      const float d = size * scales[i] * pulse;
      const float mix = static_cast<float>(i) / 4.f;
      PushQuad(cx - d * 0.5f, cy - d * 0.5f, d, d,
               gr * (1.f - mix) + cr * mix, gg * (1.f - mix) + cg * mix,
               gb * (1.f - mix) + cb * mix, alphas[i]);
    }
  }

void StarCanvasRenderer::PushOrbitParticles(ID3D11DeviceContext* ctx, float cx,
                                            float cy, float radius) {
    struct Ring {
      double count;
      float r, g, b;
      float orbit_scale;
      float speed;
      float size;
      float phase;
    };
    const Ring rings[] = {
        {orbit_p_, 1.f, 0.32f, 0.28f, 1.18f, 0.85f, 5.5f, 0.0f},
        {orbit_n_, 0.35f, 0.55f, 1.f, 1.34f, -0.62f, 5.0f, 1.7f},
        {orbit_e_, 1.f, 0.85f, 0.25f, 1.52f, 1.25f, 4.2f, 3.3f},
    };

    for (const auto& ring : rings) {
      if (ring.count < 0.5) {
        continue;
      }
      // Visual count saturates so orbits stay readable.
      const int n = std::clamp(
          static_cast<int>(std::ceil(std::log2(ring.count + 1.0) * 2.2)), 1,
          10);
      const float orbit_r = radius * ring.orbit_scale;
      for (int i = 0; i < n; ++i) {
        const float a =
            ring.phase + anim_t_ * ring.speed +
            (6.2831853f * static_cast<float>(i) / static_cast<float>(n));
        const float wobble =
            1.f + 0.04f * std::sin(anim_t_ * 1.7f + ring.phase + i);
        const float px = cx + std::cos(a) * orbit_r * wobble;
        const float py = cy + std::sin(a) * orbit_r * wobble * 0.72f;
        EnsureVertCapacity(ctx, 6);
        PushQuad(px - ring.size * 0.5f, py - ring.size * 0.5f, ring.size,
                 ring.size, ring.r, ring.g, ring.b, 0.85f);
      }
    }
  }

float StarCanvasRenderer::StarYaw() const { return anim_t_ * 0.42f; }

float StarCanvasRenderer::StarPitch() const {
    return 0.32f + std::sin(anim_t_ * 0.19f) * 0.07f;
  }

StarCanvasRenderer::Vec3 StarCanvasRenderer::Spherical(float lon, float lat) const {
    const float cl = std::cos(lat);
    return {cl * std::sin(lon), std::sin(lat), cl * std::cos(lon)};
  }

void StarCanvasRenderer::OrthonormalTangent(Vec3 n, Vec3& t, Vec3& b) const {
    const Vec3 up =
        (std::abs(n.y) < 0.9f) ? Vec3{0.f, 1.f, 0.f} : Vec3{1.f, 0.f, 0.f};
    t = Normalize(Cross(up, n));
    b = Normalize(Cross(n, t));
  }

StarCanvasRenderer::Vec3 StarCanvasRenderer::WorldFromLocal(Vec3 p) const {
    return RotateY(RotateX(p, StarPitch()), StarYaw());
  }

void StarCanvasRenderer::ProjectPoint(float cx, float cy, float radius, Vec3 p, float& sx,
                    float& sy, float& depth) const {
    depth = 1.f / (1.15f - p.z * 0.35f);
    sx = cx + p.x * radius * depth;
    sy = cy - p.y * radius * depth;
  }

void StarCanvasRenderer::ResetFlare(Flare& f, bool random_age) {
    // Prefer the limb so swirls read in silhouette / 3D depth.
    f.lon = RandFloat(0.f, 6.2831853f);
    f.lat = RandFloat(-0.55f, 0.55f);
    f.lifetime = RandFloat(2.8f, 5.5f);
    f.age = random_age ? RandFloat(0.f, f.lifetime * 0.85f) : 0.f;
    f.seed = RandFloat(0.f, 1000.f);
    f.lift = RandFloat(0.14f, 0.32f);
    f.width = RandFloat(0.045f, 0.09f);
    f.coils = RandFloat(1.0f, 1.8f);
    f.twist = (RandFloat(0.f, 1.f) > 0.5f) ? 1.f : -1.f;
    f.twist *= RandFloat(0.8f, 1.4f);
    f.wave = RandFloat(0.08f, 0.28f);
  }

void StarCanvasRenderer::InitFlares() {
    flares_.clear();
    flares_.resize(6);
    for (auto& f : flares_) {
      ResetFlare(f, true);
    }
    spawn_cooldown_ = 0.f;
  }

void StarCanvasRenderer::UpdateFlares(float dt) {
    spawn_cooldown_ -= dt;
    for (auto& f : flares_) {
      f.age += dt;
      // Slow drift of the foot across the surface.
      f.lon += dt * 0.08f * f.twist;
      f.lat += std::sin(anim_t_ * 0.35f + f.seed) * dt * 0.03f;
      if (f.age >= f.lifetime) {
        ResetFlare(f, false);
      }
    }
    if (spawn_cooldown_ <= 0.f && !flares_.empty()) {
      ResetFlare(flares_[static_cast<size_t>(RandFloat(
                     0.f, static_cast<float>(flares_.size() - 0.01f)))],
                 false);
      spawn_cooldown_ = RandFloat(0.35f, 0.9f);
    }
  }

StarCanvasRenderer::Vec3 StarCanvasRenderer::EvalFlarePoint(const Flare& f, float s, float layer) const {
    const float arch = std::sin(3.14159265f * s);
    const float grow_s = s;

    // Foot drifts + gentle latitude wave.
    const float lon =
        f.lon + f.twist * 0.55f * grow_s +
        0.15f * std::sin(anim_t_ * 0.7f + f.seed + grow_s * 2.f);
    const float lat =
        f.lat + f.wave * std::sin(grow_s * 3.14159265f + anim_t_ * 0.5f) *
                    arch;

    Vec3 radial = Spherical(lon, lat);
    Vec3 tang, bit;
    OrthonormalTangent(radial, tang, bit);

    // Elevation leaves the surface as a smooth arch.
    const float breath =
        0.85f + 0.15f * std::sin(anim_t_ * 1.1f + f.seed * 0.01f);
    const float elev = f.lift * arch * breath;

    // Helical swirl around the local radial axis (true 3D vortex).
    const float helix = f.twist * (f.coils * 6.2831853f * grow_s +
                                   anim_t_ * 0.55f + layer * 0.7f);
    const float cyl = elev * (0.55f + 0.35f * grow_s) *
                      (0.75f + 0.25f * layer);

    // Low-frequency organic wobble — keeps motion smooth, not jittery.
    const float n1 =
        Noise2(f.seed * 0.01f + grow_s * 1.4f + anim_t_ * 0.25f,
               f.seed * 0.02f - grow_s * 0.9f) *
            2.f -
        1.f;
    const float n2 =
        Noise2(f.seed * 0.03f - grow_s * 1.1f,
               anim_t_ * 0.22f + f.seed * 0.01f) *
            2.f -
        1.f;

    Vec3 p = Mul(radial, 1.f + elev);
    p = Add(p, Mul(tang, std::cos(helix) * cyl + n1 * elev * 0.12f));
    p = Add(p, Mul(bit, std::sin(helix) * cyl + n2 * elev * 0.12f));
    // Sheet offset across layers for volume.
    p = Add(p, Mul(Normalize(Cross(radial, tang)), layer * f.width * 0.35f * arch));
    return WorldFromLocal(p);
  }

void StarCanvasRenderer::PushRibbonSegment(ID3D11DeviceContext* ctx, float x0, float y0,
                         float x1, float y1, float sx0, float sy0, float sx1,
                         float sy1, float r, float g, float b, float a0,
                         float a1) {
    // UV across width only (v=0.5) => soft circular falloff becomes edge fade.
    EnsureVertCapacity(ctx, 6);
    const Vertex v[6] = {
        {x0 + sx0, y0 + sy0, 0.f, 0.5f, r, g, b, a0},
        {x0 - sx0, y0 - sy0, 1.f, 0.5f, r, g, b, a0},
        {x1 - sx1, y1 - sy1, 1.f, 0.5f, r, g, b, a1},
        {x0 + sx0, y0 + sy0, 0.f, 0.5f, r, g, b, a0},
        {x1 - sx1, y1 - sy1, 1.f, 0.5f, r, g, b, a1},
        {x1 + sx1, y1 + sy1, 0.f, 0.5f, r, g, b, a1},
    };
    verts_.insert(verts_.end(), v, v + 6);
  }

void StarCanvasRenderer::PushFlareSheet(ID3D11DeviceContext* ctx, float cx, float cy,
                      float radius, const Flare& f, float grow, float fade,
                      bool front) {
    float cr, cg, cb, gr, gg, gb;
    StarPalette(cr, cg, cb, gr, gg, gb);
    float hr, hg, hb, cr2, cg2, cb2;
    SecondaryPalette(hr, hg, hb, cr2, cg2, cb2);
    (void)cr2;
    (void)cg2;
    (void)cb2;

    constexpr int kSegs = 22;
    const int live = std::max(4, static_cast<int>(std::ceil(kSegs * grow)));
    // Two layered ribbons => volumetric swirl sheet without excess CPU.
    const float layers[] = {-0.85f, 0.85f};
    const float layer_alpha[] = {0.4f, 0.55f};

    for (int li = 0; li < 2; ++li) {
      float prev_x = 0.f, prev_y = 0.f, prev_d = 1.f;
      Vec3 prev_p{};
      bool has_prev = false;

      for (int i = 0; i <= live; ++i) {
        const float s = static_cast<float>(i) / static_cast<float>(kSegs);
        const Vec3 p = EvalFlarePoint(f, s, layers[li]);

        const bool is_front = p.z >= 0.12f;
        if (is_front != front || p.z < -0.45f) {
          has_prev = false;
          continue;
        }

        float sx, sy, depth;
        ProjectPoint(cx, cy, radius, p, sx, sy, depth);

        if (has_prev) {
          Vec3 tang = Sub(p, prev_p);
          Vec3 side3 = Cross(tang, Vec3{0.f, 0.f, 1.f});
          if (Dot(side3, side3) < 1e-8f) {
            side3 = Cross(tang, Vec3{0.f, 1.f, 0.f});
          }
          side3 = Normalize(side3);

          const float arch = std::sin(3.14159265f * s);
          const float arch0 =
              std::sin(3.14159265f * static_cast<float>(i - 1) / kSegs);
          const float w =
              radius * f.width * (0.4f + 0.7f * arch) * depth * fade;
          const float w0 =
              radius * f.width * (0.4f + 0.7f * arch0) * prev_d * fade;

          float side_x = side3.x;
          float side_y = -side3.y;
          const float sl = std::sqrt(side_x * side_x + side_y * side_y);
          if (sl > 1e-5f) {
            side_x /= sl;
            side_y /= sl;
          }

          const float heat = 0.35f + 0.65f * arch;
          const float r =
              std::min(1.f, hr * heat + gr * (1.f - heat) + cr * 0.2f);
          const float g =
              std::min(1.f, hg * heat + gg * (1.f - heat) + cg * 0.2f);
          const float b =
              std::min(1.f, hb * heat + gb * (1.f - heat) + cb * 0.2f);
          const float depth_fade =
              front ? (0.65f + 0.35f * p.z) : (0.35f + 0.4f * (p.z + 0.45f));
          const float a0 = fade * layer_alpha[li] *
                           (0.22f + 0.55f * arch0) * depth_fade;
          const float a1 = fade * layer_alpha[li] *
                           (0.22f + 0.55f * arch) * depth_fade;

          PushRibbonSegment(ctx, prev_x, prev_y, sx, sy, side_x * w0,
                            side_y * w0, side_x * w, side_y * w, r, g, b, a0,
                            a1);

          if (li == 0) {
            PushRibbonSegment(ctx, prev_x, prev_y, sx, sy, side_x * w0 * 0.4f,
                              side_y * w0 * 0.4f, side_x * w * 0.4f,
                              side_y * w * 0.4f, hr, hg, hb, a0 * 0.85f,
                              a1 * 0.85f);
          }
        }

        prev_x = sx;
        prev_y = sy;
        prev_d = depth;
        prev_p = p;
        has_prev = true;
      }
    }
  }

void StarCanvasRenderer::PushFlares(ID3D11DeviceContext* ctx, float cx, float cy, float radius,
                  bool front) {
    for (const auto& f : flares_) {
      const float u = f.age / std::max(0.001f, f.lifetime);
      float grow = 1.f;
      float fade = 1.f;
      if (u < 0.28f) {
        grow = u / 0.28f;
        grow = grow * grow * (3.f - 2.f * grow);
      } else if (u > 0.7f) {
        fade = std::max(0.f, 1.f - (u - 0.7f) / 0.3f);
        fade = fade * fade * (3.f - 2.f * fade);
        grow = 0.85f + 0.15f * fade;
      }
      if (fade < 0.02f) {
        continue;
      }
      PushFlareSheet(ctx, cx, cy, radius, f, grow, fade, front);
    }
  }

void StarCanvasRenderer::PushIcosphere(ID3D11DeviceContext* ctx, float cx, float cy,
                     float radius) {
    if (ico_idx_.empty()) {
      return;
    }

    const float yaw = StarYaw();
    const float pitch = StarPitch();

    for (size_t i = 0; i + 2 < ico_idx_.size(); i += 3) {
      Vec3 p0 = RotateY(RotateX(ico_pos_[ico_idx_[i]], pitch), yaw);
      Vec3 p1 = RotateY(RotateX(ico_pos_[ico_idx_[i + 1]], pitch), yaw);
      Vec3 p2 = RotateY(RotateX(ico_pos_[ico_idx_[i + 2]], pitch), yaw);

      Vec3 n = Normalize(Cross(Sub(p1, p0), Sub(p2, p0)));
      if (n.z <= 0.02f) {
        continue;
      }

      EnsureVertCapacity(ctx, 3);

      auto project = [&](Vec3 p) -> Vertex {
        float r, g, b;
        ShadeVertex(p, r, g, b);
        const float depth = 1.f / (1.15f - p.z * 0.35f);
        const float sx = cx + p.x * radius * depth;
        const float sy = cy - p.y * radius * depth;
        return {sx, sy, 0.5f, 0.5f, r, g, b, 0.99f};
      };

      verts_.push_back(project(p0));
      verts_.push_back(project(p1));
      verts_.push_back(project(p2));
    }
  }

bool StarCanvasRenderer::CreatePipeline(ID3D11Device* device) {
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
  float2 d = i.uv - float2(0.5, 0.5);
  float soft = saturate(1.0 - length(d) * 2.0);
  soft *= soft;
  float4 c = t * i.col;
  c.a *= soft;
  return c;
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

void StarCanvasRenderer::InitDriftStars(int width, int height) {
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

float StarCanvasRenderer::RandFloat(float a, float b) {
    std::uniform_real_distribution<float> dist(a, b);
    return dist(rng_);
  }

void StarCanvasRenderer::PushQuad(float x, float y, float w, float h, float r, float g, float b,
                float a) {
    // Caller must EnsureVertCapacity(ctx, 6) before pushing.
    // Full 0..1 UVs so the pixel shader can soft-mask quads into circles.
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

