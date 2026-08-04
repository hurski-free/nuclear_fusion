#pragma once

#include "simple_ui.h"

#include <cmath>
#include <cwchar>
#include <string>

namespace game_ui {

// Counts / costs: whole numbers, no trailing decimals (K/M/B/T when large).
inline std::wstring FormatInt(double v) {
  wchar_t buf[64] = {};
  const double a = std::fabs(v);
  if (a >= 1.0e12) {
    std::swprintf(buf, 64, L"%.2fT", v / 1.0e12);
  } else if (a >= 1.0e9) {
    std::swprintf(buf, 64, L"%.2fB", v / 1.0e9);
  } else if (a >= 1.0e6) {
    std::swprintf(buf, 64, L"%.2fM", v / 1.0e6);
  } else if (a >= 1000.0) {
    std::swprintf(buf, 64, L"%.1fK", v / 1000.0);
  } else {
    std::swprintf(buf, 64, L"%.0f", std::round(v));
  }
  return buf;
}

// eV amounts that may be fractional (Lab craft energy costs, EPS, etc.).
inline std::wstring FormatEv(double v) {
  wchar_t buf[64] = {};
  const double a = std::fabs(v);
  if (a >= 1.0e12) {
    std::swprintf(buf, 64, L"%.2fT", v / 1.0e12);
  } else if (a >= 1.0e9) {
    std::swprintf(buf, 64, L"%.2fB", v / 1.0e9);
  } else if (a >= 1.0e6) {
    std::swprintf(buf, 64, L"%.2fM", v / 1.0e6);
  } else if (a >= 1000.0) {
    std::swprintf(buf, 64, L"%.1fK", v / 1000.0);
  } else if (std::fabs(v - std::round(v)) < 1e-6) {
    std::swprintf(buf, 64, L"%.0f", std::round(v));
  } else if (a >= 10.0) {
    std::swprintf(buf, 64, L"%.1f", v);
  } else {
    std::swprintf(buf, 64, L"%.2f", v);
  }
  return buf;
}

// Default HUD / generic formatting: integers preferred.
inline std::wstring FormatNum(double v) { return FormatInt(v); }

inline void ClearImageStyle(Image& img) {
  img.style_base = {Color{0.f, 0.f, 0.f, 0.f}, Border{}};
  img.style_hovered = img.style_base;
  img.style_active = img.style_base;
  img.style_disabled = img.style_base;
  img.transition = {};
  img.tint = {1.f, 1.f, 1.f, 1.f};
  // Linear: smooth when scaled. Nearest makes 64px icons look blocky on screen.
  img.filter = ImageFilter::Linear;
}

// Lab craft cost line: [result] = [a] N + [b] N + X eV
struct CostFormula {
  Image result{};
  Label eq{};
  Image a_icon{};
  Label a_amt{};
  Label plus1{};
  Image b_icon{};
  Label b_amt{};
  Label plus2{};
  Label energy{};
};

struct ElementRow {
  Panel card{};
  Image icon{};
  Image nuc_icon{};
  Image atom_icon{};
  Image iso_icon{};
  Button nuc_hit{};
  Button atom_hit{};
  Button iso_hit{};
  Label name{};
  Label nuc_count{};
  Label atom_count{};
  Label iso_count{};
  Label stats{};
  CostFormula cost_nucleus{};
  CostFormula cost_atom{};
  Button craft_nucleus{};
  Button craft_atom{};
  int element_index = -1;
};

struct TechCostLine {
  Image icon{};
  Label amount{};
};

struct UpgradeRow {
  static constexpr int kMaxCosts = 6;
  Panel card{};
  Image icon{};
  Button icon_hit{};
  Label title{};
  Text desc{};
  Label cost_label{};
  TechCostLine costs[kMaxCosts]{};
  Button buy{};
  Image grade_icon{};
  Label grade_label{};
  int upgrade_index = -1;
  int cost_count = 0;
};

struct FloatText {
  Label label{};
  float life = 0.f;
  float vy = -40.f;
  bool active = false;
};

}  // namespace game_ui
