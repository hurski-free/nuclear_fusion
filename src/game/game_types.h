#pragma once

#include <cmath>
#include <string>
#include <vector>

enum class ResourceKind {
  Energy,
  Proton,
  Neutron,
  Electron,
  Nucleus,
  Atom,
  Isotope,
  StarDust,
};

struct ResourceCost {
  ResourceKind kind = ResourceKind::Energy;
  std::string element_id;  // used for Nucleus / Atom / Isotope
  double amount = 0.0;
};

enum class StarType {
  BrownDwarf = 0,
  YellowDwarf = 1,
  BlueGiant = 2,
  NeutronStar = 3,
};

inline const wchar_t* StarTypeName(StarType type) {
  switch (type) {
    case StarType::BrownDwarf:
      return L"Brown Dwarf";
    case StarType::YellowDwarf:
      return L"Yellow Dwarf";
    case StarType::BlueGiant:
      return L"Blue Giant";
    case StarType::NeutronStar:
      return L"Neutron Star";
    default:
      return L"Unknown Star";
  }
}

// Relative path under the game working directory (build/).
inline const wchar_t* StarTypeImagePath(StarType type) {
  switch (type) {
    case StarType::BrownDwarf:
      return L"assets\\images\\stars\\brown_dwarf.png";
    case StarType::YellowDwarf:
      return L"assets\\images\\stars\\yellow_dwarf.png";
    case StarType::BlueGiant:
      return L"assets\\images\\stars\\blue_giant.png";
    case StarType::NeutronStar:
      return L"assets\\images\\stars\\neutron_star.png";
    default:
      return L"assets\\images\\stars\\brown_dwarf.png";
  }
}

inline constexpr double kSupernovaAtomGoal = 100.0;
// Prestige (Neutron Star): 1 Dust per this many Gold atoms in stock.
inline constexpr double kPrestigeGoldPerDust = 10.0;

// Max atomic number synthesizable on the current star.
inline int StarMaxAtomicNumber(StarType type) {
  switch (type) {
    case StarType::BrownDwarf:
      return 2;  // Helium
    case StarType::YellowDwarf:
      return 8;  // Oxygen
    case StarType::BlueGiant:
      return 26;  // Iron
    case StarType::NeutronStar:
      return 92;
    default:
      return 2;
  }
}

inline const wchar_t* ElementSymbol(const std::string& id) {
  if (id == "Hydrogen") {
    return L"H";
  }
  if (id == "Helium") {
    return L"He";
  }
  if (id == "Carbon") {
    return L"C";
  }
  if (id == "Oxygen") {
    return L"O";
  }
  if (id == "Silicon") {
    return L"Si";
  }
  if (id == "Iron") {
    return L"Fe";
  }
  if (id == "Nickel") {
    return L"Ni";
  }
  if (id == "Silver") {
    return L"Ag";
  }
  if (id == "Xenon") {
    return L"Xe";
  }
  if (id == "Gold") {
    return L"Au";
  }
  return L"?";
}

inline double StarDustReward(StarType type) {
  switch (type) {
    case StarType::BrownDwarf:
      return 1.0;
    case StarType::YellowDwarf:
      return 5.0;
    case StarType::BlueGiant:
      return 25.0;
    case StarType::NeutronStar:
      return 100.0;
    default:
      return 1.0;
  }
}

enum class CraftBatch {
  x1 = 0,
  x10 = 1,
  x100 = 2,
  Max = 3,
};

inline int BatchMultiplier(CraftBatch batch) {
  switch (batch) {
    case CraftBatch::x1:
      return 1;
    case CraftBatch::x10:
      return 10;
    case CraftBatch::x100:
      return 100;
    case CraftBatch::Max:
      return 0;  // dynamic
  }
  return 1;
}

enum class UpgradeEffect {
  ClickPower,
  CritChance,
  CritMultiplier,
  AutoEps,
  AutoClickMult,
  IsotopeEpsMult,
};

// Grade thresholds: 10, 25, 50, 100, then every +100 (200, 300, ...).
inline int UpgradeGrade(int level) {
  if (level < 10) {
    return 0;
  }
  if (level < 25) {
    return 1;
  }
  if (level < 50) {
    return 2;
  }
  if (level < 100) {
    return 3;
  }
  // level 100 -> 4, 200 -> 5, 300 -> 6, ...
  return 3 + level / 100;
}

// Extra effect multiplier from grade: base^n (grade 0 → 1).
inline double UpgradeGradeMultiplier(int grade, double base = 10.0) {
  if (grade <= 0) {
    return 1.0;
  }
  return std::pow(base, static_cast<double>(grade));
}

// Grades amplify click power, flat EPS, and isotope EPS multiplier upgrades.
inline bool UpgradeUsesGrade(UpgradeEffect effect) {
  return effect == UpgradeEffect::ClickPower ||
         effect == UpgradeEffect::AutoEps ||
         effect == UpgradeEffect::IsotopeEpsMult;
}

// Annihilation coils (isotope EPS) use 2^n; other graded upgrades use 10^n.
inline double UpgradeGradeBase(UpgradeEffect effect) {
  return effect == UpgradeEffect::IsotopeEpsMult ? 2.0 : 10.0;
}

struct Element {
  std::string id;
  std::wstring name;
  int atomic_number = 1;
  int protons_needed = 1;
  int neutrons_needed = 0;
  int electrons_needed = 1;
  double energy_activation = 10.0;
  double atom_count = 0.0;
  double nucleus_count = 0.0;
  double isotope_count = 0.0;
  bool unlocked = false;
  // Becomes true the first time this element's isotope is obtained.
  bool isotope_discovered = false;
};

struct UpgradeDef {
  std::string id;
  std::wstring name;
  std::wstring description;
  std::vector<ResourceCost> base_costs;
  double cost_scale = 1.35;
  UpgradeEffect effect = UpgradeEffect::ClickPower;
  double effect_per_level = 1.0;
  int level = 0;
};

inline double UpgradeScaledEffect(const UpgradeDef& up) {
  if (up.level <= 0) {
    return 0.0;
  }
  const double grade_mult =
      UpgradeUsesGrade(up.effect)
          ? UpgradeGradeMultiplier(UpgradeGrade(up.level),
                                   UpgradeGradeBase(up.effect))
          : 1.0;
  return up.effect_per_level * static_cast<double>(up.level) * grade_mult;
}

struct ClickResult {
  double gain = 0.0;
  bool crit = false;
};
