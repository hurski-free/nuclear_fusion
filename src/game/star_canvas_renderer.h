#pragma once

#include "game_types.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <cstdint>
#include <random>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

// Draws drifting background stars + a procedural lit icosphere star into a
// simple_ui Canvas via custom D3D11 commands.
class StarCanvasRenderer {
public:
  ~StarCanvasRenderer();

  bool EnsureReady(ID3D11Device* device);
  void SetStarType(StarType type);
  void Release();
  void Resize(int width, int height);
  void Update(float dt);
  void SetOrbitCounts(double protons, double neutrons, double electrons);
  // Brief brightness flash (e.g. on star click), 0..1.
  void TriggerClickFlash(float strength = 1.f);
  void Draw(ID3D11DeviceContext* ctx, float star_cx, float star_cy,
            float star_size, bool draw_main_star);

private:
  struct Vertex {
    float x, y, u, v;
    float r, g, b, a;
  };

  struct Vec3 {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
  };

  struct DriftStar {
    float x = 0.f;
    float y = 0.f;
    float size = 2.f;
    float speed = 20.f;
    float brightness = 1.f;
  };

  // Transient 3D plasma swirl sheet growing off the photosphere.
  struct Flare {
    float lon = 0.f;
    float lat = 0.f;
    float age = 0.f;
    float lifetime = 3.5f;
    float seed = 0.f;
    float lift = 0.22f;
    float width = 0.07f;
    float coils = 1.6f;
    float twist = 1.f;
    float wave = 0.2f;
  };

  // Subdiv 3 => 20*4^3 = 1280 tris; batched FlushVerts keeps draws in budget.
  static constexpr size_t kMaxVertices = 16384;

  void BindPipeline(ID3D11DeviceContext* ctx);
  void FlushVerts(ID3D11DeviceContext* ctx);
  void EnsureVertCapacity(ID3D11DeviceContext* ctx, size_t need);

  static Vec3 Add(Vec3 a, Vec3 b);
  static Vec3 Sub(Vec3 a, Vec3 b);
  static Vec3 Mul(Vec3 a, float s);
  static float Dot(Vec3 a, Vec3 b);
  static Vec3 Cross(Vec3 a, Vec3 b);
  static Vec3 Normalize(Vec3 v);

  void BuildIcosphere(int subdivisions);
  void StarPalette(float& cr, float& cg, float& cb, float& glow_r,
                   float& glow_g, float& glow_b) const;
  Vec3 RotateY(Vec3 v, float angle) const;
  Vec3 RotateX(Vec3 v, float angle) const;
  void SecondaryPalette(float& hr, float& hg, float& hb, float& cr2,
                        float& cg2, float& cb2) const;
  float Hash11(float n) const;
  float Noise2(float x, float y) const;
  float PlasmaField(Vec3 p, float t) const;
  void ShadeVertex(Vec3 p, float& out_r, float& out_g, float& out_b) const;
  void PushStarGlow(ID3D11DeviceContext* ctx, float cx, float cy, float size);
  float StarYaw() const;
  float StarPitch() const;
  Vec3 Spherical(float lon, float lat) const;
  void OrthonormalTangent(Vec3 n, Vec3& t, Vec3& b) const;
  Vec3 WorldFromLocal(Vec3 p) const;
  void ProjectPoint(float cx, float cy, float radius, Vec3 p, float& sx,
                    float& sy, float& depth) const;
  void ResetFlare(Flare& f, bool random_age);
  void InitFlares();
  void UpdateFlares(float dt);
  Vec3 EvalFlarePoint(const Flare& f, float s, float layer) const;
  void PushRibbonSegment(ID3D11DeviceContext* ctx, float x0, float y0,
                         float x1, float y1, float sx0, float sy0, float sx1,
                         float sy1, float r, float g, float b, float a0,
                         float a1);
  void PushFlareSheet(ID3D11DeviceContext* ctx, float cx, float cy,
                      float radius, const Flare& f, float grow, float fade,
                      bool front);
  void PushFlares(ID3D11DeviceContext* ctx, float cx, float cy, float radius,
                  bool front);
  void PushIcosphere(ID3D11DeviceContext* ctx, float cx, float cy,
                     float radius);
  bool CreatePipeline(ID3D11Device* device);
  void InitDriftStars(int width, int height);
  float RandFloat(float a, float b);
  void PushQuad(float x, float y, float w, float h, float r, float g, float b,
                float a);
  void PushOrbitParticles(ID3D11DeviceContext* ctx, float cx, float cy,
                          float radius);

  bool ready_ = false;
  int view_w_ = 0;
  int view_h_ = 0;
  float anim_t_ = 0.f;
  float click_flash_ = 0.f;
  double orbit_p_ = 0.0;
  double orbit_n_ = 0.0;
  double orbit_e_ = 0.0;
  std::mt19937 rng_{std::random_device{}()};
  std::vector<DriftStar> drift_;
  std::vector<Flare> flares_;
  float spawn_cooldown_ = 0.f;
  std::vector<Vertex> verts_;
  std::vector<Vec3> ico_pos_;
  std::vector<uint32_t> ico_idx_;

  ID3D11VertexShader* vs_ = nullptr;
  ID3D11PixelShader* ps_ = nullptr;
  ID3D11InputLayout* layout_ = nullptr;
  ID3D11Buffer* vb_ = nullptr;
  ID3D11Buffer* cb_ = nullptr;
  ID3D11BlendState* blend_ = nullptr;
  ID3D11RasterizerState* rs_ = nullptr;
  ID3D11DepthStencilState* dss_ = nullptr;
  ID3D11SamplerState* samp_ = nullptr;
  ID3D11ShaderResourceView* white_srv_ = nullptr;
  int active_star_ = 0;
};
