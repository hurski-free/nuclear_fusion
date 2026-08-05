#pragma once

#include "game_types.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <string>

inline constexpr int kIronAtomicNumber = 26;

struct GameState {
  double energy = 0.0;
  double protons = 0.0;
  double neutrons = 0.0;
  double electrons = 0.0;
  double star_dust = 0.0;
  // Unlocks D Tech after the first Prestige (not Supernova).
  int prestige_count = 0;
  // Gold atoms required for the next Prestige Dust (survives Prestige reset).
  // Starts at kPrestigeGoldPerDust; each claimed Dust raises it by +1.
  double prestige_gold_next = kPrestigeGoldPerDust;

  double click_power = 1.0;
  double crit_chance = 0.0;
  double crit_multiplier = 2.0;
  double auto_eps = 0.0;
  double auto_click_mult = 1.0;
  // Flat EPS from D Tech DustFlatEps upgrades (not scaled by dust EPS mult).
  double dust_flat_eps = 0.0;


  // Energy cost to materialize one particle.
  double price_proton = 10.0;
  double price_neutron = 12.0;
  double price_electron = 5.0;

  StarType star_type = StarType::BrownDwarf;
  CraftBatch craft_batch = CraftBatch::x1;  // Lab nucleus/atom craft
  CraftBatch buy_batch = CraftBatch::x1;    // p/n/e materialize
  AutoBuyMode auto_buy = AutoBuyMode::None;

  std::vector<Element> elements;
  std::vector<UpgradeDef> upgrades;

  std::mt19937 rng{std::random_device{}()};

  void InitNewGame() {
    energy = 0.0;
    protons = 0.0;
    neutrons = 0.0;
    electrons = 0.0;
    star_dust = 0.0;
    prestige_count = 0;
    prestige_gold_next = kPrestigeGoldPerDust;
    click_power = 1.0;
    crit_chance = 0.0;
    crit_multiplier = 2.0;
    auto_eps = 0.0;
    auto_click_mult = 1.0;
    dust_flat_eps = 0.0;
    star_type = StarType::BrownDwarf;
    craft_batch = CraftBatch::x1;
    buy_batch = CraftBatch::x1;
    auto_buy = AutoBuyMode::None;
    elements = CreateDefaultElements();
    upgrades = CreateDefaultUpgrades();
    UnlockElementsForStar();
  }

  static std::vector<Element> CreateDefaultElements() {
    return {
        Element{"Hydrogen", L"Hydrogen", 1, 1, 1, 1, 8.0, 0, 0, 0, true, false},
        Element{"Helium", L"Helium", 2, 2, 2, 2, 40.0, 0, 0, 0, true, false},
        Element{"Carbon", L"Carbon", 6, 6, 6, 6, 220.0, 0, 0, 0, false, false},
        Element{"Oxygen", L"Oxygen", 8, 8, 8, 8, 480.0, 0, 0, 0, false, false},
        Element{"Silicon", L"Silicon", 14, 14, 14, 14, 1800.0, 0, 0, 0, false,
                false},
        Element{"Iron", L"Iron", 26, 26, 30, 26, 12000.0, 0, 0, 0, false, false},
        // Mid ladder toward Gold (Neutron Star).
        Element{"Nickel", L"Nickel", 28, 28, 30, 28, 20000.0, 0, 0, 0, false,
                false},
        Element{"Silver", L"Silver", 47, 47, 60, 47, 55000.0, 0, 0, 0, false,
                false},
        Element{"Xenon", L"Xenon", 54, 54, 78, 54, 110000.0, 0, 0, 0, false,
                false},
        // Between Xe and W on the Neutron Star ladder.
        Element{"Gadolinium", L"Gadolinium", 64, 64, 94, 64, 145000.0, 0, 0, 0,
                false, false},
        Element{"Tungsten", L"Tungsten", 74, 74, 110, 74, 180000.0, 0, 0, 0,
                false, false},
        Element{"Gold", L"Gold", 79, 79, 118, 79, 250000.0, 0, 0, 0, false,
                false},
    };
  }

  static std::vector<UpgradeDef> CreateDefaultUpgrades() {
    std::vector<UpgradeDef> list;

    // Particle tree (display order: electron → proton → neutron).
    list.push_back(UpgradeDef{
        "click_e", L"Electron Lens", L"+0.2 EPS",
        {{ResourceKind::Electron, "", 5}}, 1.5, UpgradeEffect::AutoEps, 0.2,
        0});

    list.push_back(UpgradeDef{
        "click_p", L"Proton Injector", L"+1 eV click power",
        {{ResourceKind::Proton, "", 5}}, 1.5, UpgradeEffect::ClickPower, 1.0,
        0});

    list.push_back(UpgradeDef{
        "click_n", L"Neutron Channel", L"+1% crit chance (max 100%)",
        {{ResourceKind::Neutron, "", 5}}, 1.5, UpgradeEffect::CritChance, 0.01,
        0});

    // Per element by Z: nucleus (click) -> atom (EPS) -> isotope coil.
    list.push_back(UpgradeDef{
        "nuc_Hydrogen", L"Hydrogen Core", L"+2 eV click power",
        {{ResourceKind::Nucleus, "Hydrogen", 5}}, 1.5, UpgradeEffect::ClickPower,
        2.0, 0});
    list.push_back(UpgradeDef{
        "atom_Hydrogen", L"Hydrogen Farm", L"+2.5 EPS",
        {{ResourceKind::Atom, "Hydrogen", 5}}, 1.5, UpgradeEffect::AutoEps, 2.5,
        0});
    // Coil bonuses: same +0.01 isotope EPS mult per level for every element.
    list.push_back(UpgradeDef{
        "annihilation_Hydrogen", L"H Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Hydrogen", 3},
         {ResourceKind::Energy, "", 2000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Helium", L"Helium Core", L"+10 eV click power",
        {{ResourceKind::Nucleus, "Helium", 5}}, 1.5, UpgradeEffect::ClickPower,
        10.0, 0});
    list.push_back(UpgradeDef{
        "atom_Helium", L"Helium Turbine", L"+5 EPS",
        {{ResourceKind::Atom, "Helium", 5}}, 1.5, UpgradeEffect::AutoEps, 5.0,
        0});
    list.push_back(UpgradeDef{
        "annihilation_Helium", L"He Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Helium", 3},
         {ResourceKind::Energy, "", 5000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Carbon", L"Carbon Core", L"+30 eV click power",
        {{ResourceKind::Nucleus, "Carbon", 5}}, 1.5, UpgradeEffect::ClickPower,
        30.0, 0});
    list.push_back(UpgradeDef{
        "atom_Carbon", L"Carbon Lattice", L"+15 EPS",
        {{ResourceKind::Atom, "Carbon", 5}}, 1.5, UpgradeEffect::AutoEps, 15.0,
        0});
    list.push_back(UpgradeDef{
        "annihilation_Carbon", L"C Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Carbon", 3},
         {ResourceKind::Energy, "", 12000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Oxygen", L"Oxygen Core", L"+50 eV click power",
        {{ResourceKind::Nucleus, "Oxygen", 5}}, 1.5, UpgradeEffect::ClickPower,
        50.0, 0});
    list.push_back(UpgradeDef{
        "atom_Oxygen", L"Oxygen Reactor", L"+30 EPS",
        {{ResourceKind::Atom, "Oxygen", 5}}, 1.5, UpgradeEffect::AutoEps, 30.0,
        0});
    list.push_back(UpgradeDef{
        "annihilation_Oxygen", L"O Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Oxygen", 3},
         {ResourceKind::Energy, "", 20000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Silicon", L"Silicon Core", L"+90 eV click power",
        {{ResourceKind::Nucleus, "Silicon", 5}}, 1.5, UpgradeEffect::ClickPower,
        90.0, 0});
    list.push_back(UpgradeDef{
        "atom_Silicon", L"Silicon Array", L"+60 EPS",
        {{ResourceKind::Atom, "Silicon", 5}}, 1.5, UpgradeEffect::AutoEps, 60.0,
        0});
    list.push_back(UpgradeDef{
        "annihilation_Silicon", L"Si Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Silicon", 3},
         {ResourceKind::Energy, "", 40000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Iron", L"Iron Core", L"+180 eV click power",
        {{ResourceKind::Nucleus, "Iron", 5}}, 1.5, UpgradeEffect::ClickPower,
        180.0, 0});
    list.push_back(UpgradeDef{
        "atom_Iron", L"Iron Forge", L"+120 EPS",
        {{ResourceKind::Atom, "Iron", 5}}, 1.5, UpgradeEffect::AutoEps, 120.0,
        0});
    list.push_back(UpgradeDef{
        "annihilation_Iron", L"Fe Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Iron", 3},
         {ResourceKind::Energy, "", 80000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Nickel", L"Nickel Core", L"+350 eV click power",
        {{ResourceKind::Nucleus, "Nickel", 5}}, 1.5, UpgradeEffect::ClickPower,
        350.0, 0});
    list.push_back(UpgradeDef{
        "atom_Nickel", L"Nickel Stack", L"+250 EPS",
        {{ResourceKind::Atom, "Nickel", 5}}, 1.5, UpgradeEffect::AutoEps, 250.0,
        0});
    list.push_back(UpgradeDef{
        "annihilation_Nickel", L"Ni Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Nickel", 3},
         {ResourceKind::Energy, "", 100000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Silver", L"Silver Core", L"+5000 eV click power",
        {{ResourceKind::Nucleus, "Silver", 5}}, 1.5, UpgradeEffect::ClickPower,
        5000.0, 0});
    list.push_back(UpgradeDef{
        "atom_Silver", L"Silver Circuit", L"+3500 EPS",
        {{ResourceKind::Atom, "Silver", 5}}, 1.5, UpgradeEffect::AutoEps, 3500.0,
        0});
    list.push_back(UpgradeDef{
        "annihilation_Silver", L"Ag Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Silver", 3},
         {ResourceKind::Energy, "", 140000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Xenon", L"Xenon Core", L"+20000 eV click power",
        {{ResourceKind::Nucleus, "Xenon", 5}}, 1.5, UpgradeEffect::ClickPower,
        20000.0, 0});
    list.push_back(UpgradeDef{
        "atom_Xenon", L"Xenon Chamber", L"+15000 EPS",
        {{ResourceKind::Atom, "Xenon", 5}}, 1.5, UpgradeEffect::AutoEps, 15000.0,
        0});
    list.push_back(UpgradeDef{
        "annihilation_Xenon", L"Xe Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Xenon", 3},
         {ResourceKind::Energy, "", 170000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Gadolinium", L"Gadolinium Core", L"+100K eV click power",
        {{ResourceKind::Nucleus, "Gadolinium", 5}}, 1.5,
        UpgradeEffect::ClickPower, 100000.0, 0});
    list.push_back(UpgradeDef{
        "atom_Gadolinium", L"Gadolinium Lattice", L"+70K EPS",
        {{ResourceKind::Atom, "Gadolinium", 5}}, 1.5, UpgradeEffect::AutoEps,
        70000.0, 0});
    list.push_back(UpgradeDef{
        "annihilation_Gadolinium", L"Gd Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Gadolinium", 3},
         {ResourceKind::Energy, "", 177000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Tungsten", L"Tungsten Core", L"+500K eV click power",
        {{ResourceKind::Nucleus, "Tungsten", 5}}, 1.5, UpgradeEffect::ClickPower,
        500000.0, 0});
    list.push_back(UpgradeDef{
        "atom_Tungsten", L"Tungsten Forge", L"+300K EPS",
        {{ResourceKind::Atom, "Tungsten", 5}}, 1.5, UpgradeEffect::AutoEps,
        300000.0, 0});
    list.push_back(UpgradeDef{
        "annihilation_Tungsten", L"W Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Tungsten", 3},
         {ResourceKind::Energy, "", 185000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    list.push_back(UpgradeDef{
        "nuc_Gold", L"Gold Core", L"+1.5M eV click power",
        {{ResourceKind::Nucleus, "Gold", 5}}, 1.5, UpgradeEffect::ClickPower,
        1500000.0, 0});
    list.push_back(UpgradeDef{
        "atom_Gold", L"Gold Dynamo", L"+1M EPS",
        {{ResourceKind::Atom, "Gold", 5}}, 1.5, UpgradeEffect::AutoEps,
        1000000.0, 0});
    list.push_back(UpgradeDef{
        "annihilation_Gold", L"Au Annihilation Coil",
        L"+0.01 isotope EPS multiplier",
        {{ResourceKind::Isotope, "Gold", 3},
         {ResourceKind::Energy, "", 200000}},
        1.6, UpgradeEffect::IsotopeEpsMult, 0.01, 0});

    // Special multi-cost upgrades last.
    list.push_back(UpgradeDef{
        "crit_amplifier", L"Critical Cascade", L"+1 crit multiplier",
        {{ResourceKind::Atom, "Hydrogen", 10},
         {ResourceKind::Atom, "Helium", 10},
         {ResourceKind::Atom, "Carbon", 10},
         {ResourceKind::Atom, "Oxygen", 10},
         {ResourceKind::Atom, "Silicon", 10},
         {ResourceKind::Atom, "Iron", 10}},
        1.9, UpgradeEffect::CritMultiplier, 1.0, 0});

    list.push_back(UpgradeDef{
        "tungsten_catalyst", L"Tungsten Catalyst",
        L"-2% nucleus/atom eV craft cost",
        {{ResourceKind::Atom, "Tungsten", 10},
         {ResourceKind::Energy, "", 5000000000.0}},
        1.8, UpgradeEffect::CraftEnergyDiscount, 0.02, 0});

    list.push_back(UpgradeDef{
        "quantum_cpu", L"Quantum Processor", L"+10 click multiplier",
        {{ResourceKind::Electron, "", 10000000000.0},
         {ResourceKind::Nucleus, "Silicon", 50000000.0},
         {ResourceKind::Atom, "Gold", 25}},
        2.2, UpgradeEffect::AutoClickMult, 10.0, 0});

    // D Tech (dust) — persist through resets; shown only after first prestige.
    list.push_back(UpgradeDef{
        "boost_dust_eps", L"Dust Dynamo", L"+10 EPS (permanent)",
        {{ResourceKind::StarDust, "", 1}}, 1.0, UpgradeEffect::DustFlatEps, 10.0,
        0, true});
    list.push_back(UpgradeDef{
        "boost_click_e", L"Electron Lens Boost",
        L"x10 Electron Lens EPS (permanent)",
        {{ResourceKind::StarDust, "", 25}}, 2.0,
        UpgradeEffect::ElectronLensBoost, 10.0, 0, true});
    list.push_back(UpgradeDef{
        "boost_click_p", L"Proton Injector Boost",
        L"x10 Proton Injector click (permanent)",
        {{ResourceKind::StarDust, "", 25}}, 2.0,
        UpgradeEffect::ProtonInjectorBoost, 10.0, 0, true});

    return list;
  }

  // D Tech (dust upgrades) unlocks once Dust exists in the run cycle —
  // after the first Supernova (or Prestige on Neutron Star).
  bool DTechUnlocked() const {
    return star_type != StarType::BrownDwarf || prestige_count > 0;
  }

  void UnlockElementsForStar() {
    const int max_z = StarMaxAtomicNumber(star_type);
    for (auto& el : elements) {
      if (el.atomic_number <= max_z) {
        el.unlocked = true;
      }
    }
  }

  Element* FindElement(const std::string& id) {
    for (auto& el : elements) {
      if (el.id == id) {
        return &el;
      }
    }
    return nullptr;
  }

  const Element* FindElement(const std::string& id) const {
    for (const auto& el : elements) {
      if (el.id == id) {
        return &el;
      }
    }
    return nullptr;
  }

  // Mutation chance comes only from star dust prestige.
  double EffectiveMutationChance(const Element&) const {
    return std::clamp(star_dust * 0.001, 0.0, 0.50);
  }

  double ActivationEnergy(const Element& el) const {
    if (el.atomic_number <= kIronAtomicNumber) {
      return el.energy_activation;
    }
    const int over = el.atomic_number - kIronAtomicNumber;
    return el.energy_activation * std::pow(1.18, static_cast<double>(over));
  }

  // Multiplier for nucleus/atom craft eV after Tungsten Catalyst (cap 90%).
  double CraftEnergyMult() const {
    double discount = 0.0;
    for (const auto& up : upgrades) {
      if (up.effect == UpgradeEffect::CraftEnergyDiscount) {
        discount += UpgradeScaledEffect(up);
      }
    }
    return 1.0 - std::min(discount, 0.90);
  }

  double NucleusCraftEnergy(const Element& el) const {
    return ActivationEnergy(el) * CraftEnergyMult();
  }

  double AtomCraftEnergy(const Element& el) const {
    return NucleusCraftEnergy(el) * 0.35;
  }

  // Heavier isotopes scale superlinearly so late elements outpace cheap H spam.
  // Per isotope: 0.01 * Z^2.2 eV/s (Z=1 → 0.01, Z=26 → ~12.2, Z=79 → ~168).
  double IsotopeEpsPer(const Element& el) const {
    const double z = static_cast<double>(el.atomic_number);
    return 0.01 * std::pow(z, 2.2);
  }

  // Each Annihilation Coil buffs only its own element's isotopes: 1 + coil bonus.
  double ElementIsotopeMult(const Element& el) const {
    const std::string coil_id = "annihilation_" + el.id;
    for (const auto& up : upgrades) {
      if (up.id == coil_id) {
        return 1.0 + UpgradeScaledEffect(up);
      }
    }
    return 1.0;
  }

  double ElementIsotopeEps(const Element& el) const {
    return el.isotope_count * IsotopeEpsPer(el) * ElementIsotopeMult(el);
  }

  double IsotopeEps() const {
    double sum = 0.0;
    for (const auto& el : elements) {
      if (el.isotope_count > 0.0) {
        sum += ElementIsotopeEps(el);
      }
    }
    return sum;
  }

  double Eps() const {
    // Flat +Dust EPS and Dust Dynamo; no passive dust EPS/click multipliers.
    return auto_eps + IsotopeEps() + star_dust + dust_flat_eps;
  }

  // One star click worth of eV before crit (includes Quantum Processor mult).
  double EffectiveClickPower() const {
    return click_power * auto_click_mult;
  }

  void Tick(float dt) {
    if (dt > 0.f) {
      energy += Eps() * static_cast<double>(dt);
    }
  }

  ClickResult ClickStar() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    ClickResult result;
    result.gain = EffectiveClickPower();
    if (dist(rng) < crit_chance) {
      result.gain *= crit_multiplier;
      result.crit = true;
    }
    energy += result.gain;
    return result;
  }

  bool MaterializeProton(double amount) {
    const double cost = amount * price_proton;
    if (amount <= 0.0 || energy < cost) {
      return false;
    }
    energy -= cost;
    protons += amount;
    return true;
  }

  bool MaterializeNeutron(double amount) {
    const double cost = amount * price_neutron;
    if (amount <= 0.0 || energy < cost) {
      return false;
    }
    energy -= cost;
    neutrons += amount;
    return true;
  }

  bool MaterializeElectron(double amount) {
    const double cost = amount * price_electron;
    if (amount <= 0.0 || energy < cost) {
      return false;
    }
    energy -= cost;
    electrons += amount;
    return true;
  }

  double MaxMaterialize(ResourceKind kind) const {
    double price = price_proton;
    if (kind == ResourceKind::Neutron) {
      price = price_neutron;
    } else if (kind == ResourceKind::Electron) {
      price = price_electron;
    }
    if (price <= 0.0) {
      return 0.0;
    }
    return std::floor(energy / price);
  }

  int ResolveBatchCount(int max_affordable, CraftBatch batch) const {
    if (max_affordable <= 0) {
      return 0;
    }
    const int mult = BatchMultiplier(batch);
    if (mult == 0) {
      return max_affordable;
    }
    return std::min(mult, max_affordable);
  }

  // Double path for particle Max buys (counts can exceed int32 ~2.1e9).
  double ResolveBatchAmount(double max_affordable, CraftBatch batch) const {
    if (!(max_affordable > 0.0)) {
      return 0.0;
    }
    const double capped = std::floor(max_affordable);
    const int mult = BatchMultiplier(batch);
    if (mult == 0) {
      return capped;
    }
    return std::min(static_cast<double>(mult), capped);
  }

  int ResolveCraftBatchCount(int max_affordable) const {
    return ResolveBatchCount(max_affordable, craft_batch);
  }

  double ResolveCraftBatchAmount(double max_affordable) const {
    return ResolveBatchAmount(max_affordable, craft_batch);
  }

  double ResolveParticleBuyAmount(ResourceKind kind) const {
    return ResolveBatchAmount(MaxMaterialize(kind), buy_batch);
  }

  double PairUnitPrice() const { return price_proton + price_neutron; }

  double TrioUnitPrice() const {
    return price_proton + price_neutron + price_electron;
  }

  double MaxMaterializePair() const {
    const double unit = PairUnitPrice();
    if (unit <= 0.0) {
      return 0.0;
    }
    return std::floor(energy / unit);
  }

  double MaxMaterializeTrio() const {
    const double unit = TrioUnitPrice();
    if (unit <= 0.0) {
      return 0.0;
    }
    return std::floor(energy / unit);
  }

  double ResolvePairBuyAmount() const {
    return ResolveBatchAmount(MaxMaterializePair(), buy_batch);
  }

  double ResolveTrioBuyAmount() const {
    return ResolveBatchAmount(MaxMaterializeTrio(), buy_batch);
  }

  bool MaterializePair(double amount) {
    const double cost = amount * PairUnitPrice();
    if (amount <= 0.0 || energy < cost) {
      return false;
    }
    energy -= cost;
    protons += amount;
    neutrons += amount;
    return true;
  }

  bool MaterializeTrio(double amount) {
    const double cost = amount * TrioUnitPrice();
    if (amount <= 0.0 || energy < cost) {
      return false;
    }
    energy -= cost;
    protons += amount;
    neutrons += amount;
    electrons += amount;
    return true;
  }

  // Auto-buy always spends for Max affordable of the selected mode.
  bool TickAutoBuy() {
    switch (auto_buy) {
      case AutoBuyMode::Proton:
        return MaterializeProton(MaxMaterialize(ResourceKind::Proton));
      case AutoBuyMode::Neutron:
        return MaterializeNeutron(MaxMaterialize(ResourceKind::Neutron));
      case AutoBuyMode::Electron:
        return MaterializeElectron(MaxMaterialize(ResourceKind::Electron));
      case AutoBuyMode::ProtonNeutron:
        return MaterializePair(MaxMaterializePair());
      case AutoBuyMode::All:
        return MaterializeTrio(MaxMaterializeTrio());
      case AutoBuyMode::None:
        break;
    }
    return false;
  }

  // Avoid UB/overflow when casting huge doubles to int.
  static int FloorToCount(double v) {
    if (!(v > 0.0) || !std::isfinite(v)) {
      return 0;
    }
    const double lim = static_cast<double>((std::numeric_limits<int>::max)());
    if (v >= lim) {
      return (std::numeric_limits<int>::max)();
    }
    return static_cast<int>(std::floor(v));
  }

  double MaxNucleusCraft(const Element& el) const {
    if (!el.unlocked || el.atomic_number > StarMaxAtomicNumber(star_type)) {
      return 0.0;
    }
    const double e_act = NucleusCraftEnergy(el);
    if (el.protons_needed <= 0 || e_act <= 0.0) {
      return 0.0;
    }
    const double by_p = protons / el.protons_needed;
    const double by_n =
        el.neutrons_needed > 0 ? neutrons / el.neutrons_needed : by_p;
    const double by_e = energy / e_act;
    const double m = std::min({by_p, by_n, by_e});
    return (m > 0.0 && std::isfinite(m)) ? std::floor(m) : 0.0;
  }

  double MaxAtomCraft(const Element& el) const {
    if (!el.unlocked || el.atomic_number > StarMaxAtomicNumber(star_type)) {
      return 0.0;
    }
    if (el.electrons_needed <= 0) {
      return 0.0;
    }
    const double e_act = AtomCraftEnergy(el);
    if (e_act <= 0.0) {
      return 0.0;
    }
    const double by_nuc = el.nucleus_count;
    const double by_e = electrons / el.electrons_needed;
    const double by_energy = energy / e_act;
    const double m = std::min({by_nuc, by_e, by_energy});
    return (m > 0.0 && std::isfinite(m)) ? std::floor(m) : 0.0;
  }

  bool CraftNucleus(Element& el, double requested = 0.0) {
    const double max_n = MaxNucleusCraft(el);
    const double want = requested > 0.0 ? requested : max_n;
    const double count = ResolveCraftBatchAmount(std::min(want, max_n));
    if (!(count > 0.0)) {
      return false;
    }

    const double e_act = NucleusCraftEnergy(el);
    const double spend_e = e_act * count;
    protons -= el.protons_needed * count;
    neutrons -= el.neutrons_needed * count;
    energy -= spend_e;

    el.nucleus_count += count;
    return true;
  }

  // Sample Binomial(n, p) without an O(n) Bernoulli loop.
  int SampleBinomial(int n, double p) {
    if (n <= 0 || p <= 0.0) {
      return 0;
    }
    if (p >= 1.0) {
      return n;
    }
    std::binomial_distribution<int> dist(n, p);
    return dist(rng);
  }

  // Isotope rolls for large craft batches (beyond int32).
  double SampleIsotopeCount(double n, double p) {
    if (!(n > 0.0) || p <= 0.0) {
      return 0.0;
    }
    n = std::floor(n);
    if (!(n > 0.0)) {
      return 0.0;
    }
    if (p >= 1.0) {
      return n;
    }
    // Exact binomial while n fits comfortably in int.
    constexpr double kExactLimit = 100000.0;
    if (n <= kExactLimit) {
      return static_cast<double>(SampleBinomial(static_cast<int>(n), p));
    }
    // Normal approximation for huge Max crafts.
    const double mean = n * p;
    const double var = n * p * (1.0 - p);
    if (var < 1e-12) {
      return std::min(n, std::floor(mean + 0.5));
    }
    std::normal_distribution<double> dist(mean, std::sqrt(var));
    return std::clamp(std::round(dist(rng)), 0.0, n);
  }

  bool CraftAtom(Element& el, double requested = 0.0) {
    const double max_n = MaxAtomCraft(el);
    const double want = requested > 0.0 ? requested : max_n;
    const double count = ResolveCraftBatchAmount(std::min(want, max_n));
    if (!(count > 0.0)) {
      return false;
    }

    const double e_act = AtomCraftEnergy(el);
    el.nucleus_count -= count;
    electrons -= el.electrons_needed * count;
    energy -= e_act * count;

    const double mut = EffectiveMutationChance(el);
    const double isotopes = SampleIsotopeCount(count, mut);
    const double atoms = count - isotopes;
    el.atom_count += atoms;
    el.isotope_count += isotopes;
    if (isotopes > 0.0) {
      el.isotope_discovered = true;
    }
    return true;
  }

  bool IsotopeDiscovered(const std::string& element_id) const {
    const Element* el = FindElement(element_id);
    return el && el->isotope_discovered;
  }

  // Tech upgrades that spend an undiscovered isotope stay hidden.
  // Nucleus/atom upgrades appear only after the element is unlocked.
  bool IsUpgradeVisible(const UpgradeDef& up) const {
    for (const auto& c : up.base_costs) {
      if (c.kind == ResourceKind::Isotope && !IsotopeDiscovered(c.element_id)) {
        return false;
      }
      if ((c.kind == ResourceKind::Nucleus || c.kind == ResourceKind::Atom) &&
          !c.element_id.empty()) {
        const Element* el = FindElement(c.element_id);
        if (!el || !el->unlocked) {
          return false;
        }
      }
    }
    return true;
  }

  double ResourceAmount(const ResourceCost& cost) const {
    switch (cost.kind) {
      case ResourceKind::Energy:
        return energy;
      case ResourceKind::Proton:
        return protons;
      case ResourceKind::Neutron:
        return neutrons;
      case ResourceKind::Electron:
        return electrons;
      case ResourceKind::StarDust:
        return star_dust;
      case ResourceKind::Nucleus:
      case ResourceKind::Atom:
      case ResourceKind::Isotope: {
        const Element* el = FindElement(cost.element_id);
        if (!el) {
          return 0.0;
        }
        if (cost.kind == ResourceKind::Nucleus) {
          return el->nucleus_count;
        }
        if (cost.kind == ResourceKind::Atom) {
          return el->atom_count;
        }
        return el->isotope_count;
      }
    }
    return 0.0;
  }

  void SpendResource(const ResourceCost& cost, double amount) {
    switch (cost.kind) {
      case ResourceKind::Energy:
        energy -= amount;
        break;
      case ResourceKind::Proton:
        protons -= amount;
        break;
      case ResourceKind::Neutron:
        neutrons -= amount;
        break;
      case ResourceKind::Electron:
        electrons -= amount;
        break;
      case ResourceKind::StarDust:
        star_dust -= amount;
        break;
      case ResourceKind::Nucleus:
      case ResourceKind::Atom:
      case ResourceKind::Isotope: {
        Element* el = FindElement(cost.element_id);
        if (!el) {
          return;
        }
        if (cost.kind == ResourceKind::Nucleus) {
          el->nucleus_count -= amount;
        } else if (cost.kind == ResourceKind::Atom) {
          el->atom_count -= amount;
        } else {
          el->isotope_count -= amount;
        }
        break;
      }
    }
  }

  double ScaledCostAmount(const UpgradeDef& up, const ResourceCost& base) const {
    const double raw =
        base.amount * std::pow(up.cost_scale, static_cast<double>(up.level));
    return std::max(1.0, std::round(raw));
  }

  bool CanAffordUpgrade(const UpgradeDef& up) const {
    for (const auto& c : up.base_costs) {
      if (ResourceAmount(c) + 1e-9 < ScaledCostAmount(up, c)) {
        return false;
      }
    }
    return true;
  }

  bool BuyUpgrade(UpgradeDef& up) {
    if (!CanAffordUpgrade(up)) {
      return false;
    }
    for (const auto& c : up.base_costs) {
      SpendResource(c, ScaledCostAmount(up, c));
    }
    ++up.level;
    // Full recalc so grade thresholds apply correctly.
    RecalculateFromUpgrades();
    return true;
  }

  double DustBoostMultiplier(UpgradeEffect boost_effect) const {
    for (const auto& up : upgrades) {
      if (up.effect == boost_effect && up.level > 0) {
        return std::pow(up.effect_per_level, static_cast<double>(up.level));
      }
    }
    return 1.0;
  }

  void ApplyUpgradeTotal(const UpgradeDef& up) {
    if (up.level <= 0) {
      return;
    }
    // Dust boosts are applied as multipliers on click_e / click_p below.
    if (up.effect == UpgradeEffect::ElectronLensBoost ||
        up.effect == UpgradeEffect::ProtonInjectorBoost) {
      return;
    }
    double total = UpgradeScaledEffect(up);
    if (up.id == "click_e") {
      total *= DustBoostMultiplier(UpgradeEffect::ElectronLensBoost);
    } else if (up.id == "click_p") {
      total *= DustBoostMultiplier(UpgradeEffect::ProtonInjectorBoost);
    }
    switch (up.effect) {
      case UpgradeEffect::ClickPower:
        // Accumulated into click_power as tech click strength X, then scaled by Dust.
        click_power += total;
        break;
      case UpgradeEffect::CritChance:
        crit_chance = std::min(1.0, crit_chance + total);
        break;
      case UpgradeEffect::CritMultiplier:
        crit_multiplier += total;
        break;
      case UpgradeEffect::AutoEps:
        auto_eps += total;
        break;
      case UpgradeEffect::AutoClickMult:
        auto_click_mult += total;
        break;
      case UpgradeEffect::IsotopeEpsMult:
        // Applied per-element in ElementIsotopeMult / ElementIsotopeEps.
        break;
      case UpgradeEffect::CraftEnergyDiscount:
        // Applied in CraftEnergyMult / NucleusCraftEnergy / AtomCraftEnergy.
        break;
      case UpgradeEffect::DustFlatEps:
        dust_flat_eps += total;
        break;
      case UpgradeEffect::ElectronLensBoost:
      case UpgradeEffect::ProtonInjectorBoost:
        break;
    }
  }

  // Prestige-aware base stats, then re-apply all purchased upgrade levels.
  void RecalculateFromUpgrades() {
    // X starts at 0 and becomes click power from Tech ClickPower upgrades.
    click_power = 0.0;
    crit_chance = 0.0;
    crit_multiplier = 2.0;
    // Tech AutoEps only here; flat +Dust EPS is applied in Eps().
    auto_eps = 0.0;
    auto_click_mult = 1.0;
    dust_flat_eps = 0.0;

    for (const auto& up : upgrades) {
      ApplyUpgradeTotal(up);
    }

    // Innate base click of 1 is included so early game still works at Dust 0.
    click_power = 1.0 + click_power;
  }

  bool CanTriggerSupernova() const {
    // Require 100 atoms of the current star's cap element.
    const int max_z = StarMaxAtomicNumber(star_type);
    for (const auto& el : elements) {
      if (el.atomic_number == max_z && el.atom_count + 1e-9 >= kSupernovaAtomGoal) {
        return true;
      }
    }
    return false;
  }

  void SoftResetRun() {
    energy = 0.0;
    protons = 0.0;
    neutrons = 0.0;
    electrons = 0.0;

    for (auto& el : elements) {
      el.atom_count = 0.0;
      el.nucleus_count = 0.0;
      el.isotope_count = 0.0;
      // Isotope discoveries persist across soft resets.
    }

    for (auto& up : upgrades) {
      if (!up.persist_on_reset) {
        up.level = 0;
      }
    }
  }

  bool TriggerSupernova() {
    if (star_type == StarType::NeutronStar || !CanTriggerSupernova()) {
      return false;
    }

    star_dust += StarDustReward(star_type);
    SoftResetRun();

    star_type = static_cast<StarType>(static_cast<int>(star_type) + 1);
    RecalculateFromUpgrades();
    UnlockElementsForStar();
    return true;
  }

  // How many Dust the current Gold stock can buy with escalating costs
  // (next, next+1, next+2, ...). Does not mutate prestige_gold_next.
  double PrestigeDustReward() const {
    if (star_type != StarType::NeutronStar) {
      return 0.0;
    }
    const Element* gold = FindElement("Gold");
    if (!gold) {
      return 0.0;
    }
    double remaining = gold->atom_count;
    double cost = prestige_gold_next;
    double dust = 0.0;
    constexpr int kMaxDustPerPrestige = 1000000;
    for (int i = 0; i < kMaxDustPerPrestige; ++i) {
      if (remaining + 1e-9 < cost) {
        break;
      }
      remaining -= cost;
      dust += 1.0;
      cost += 1.0;
    }
    return dust;
  }

  // Cumulative Gold atoms needed for `dust_count` Dust from the current next cost.
  double PrestigeGoldForDustCount(int dust_count) const {
    if (dust_count <= 0) {
      return 0.0;
    }
    double sum = 0.0;
    double cost = prestige_gold_next;
    for (int i = 0; i < dust_count; ++i) {
      sum += cost;
      cost += 1.0;
    }
    return sum;
  }

  bool CanTriggerPrestige() const {
    return PrestigeDustReward() + 1e-9 >= 1.0;
  }

  bool TriggerPrestige() {
    if (!CanTriggerPrestige()) {
      return false;
    }

    const double reward = PrestigeDustReward();
    star_dust += reward;
    prestige_gold_next += reward;
    ++prestige_count;
    SoftResetRun();
    // Neutron Star stays; unlocks already cover Gold.
    RecalculateFromUpgrades();
    UnlockElementsForStar();
    return true;
  }
};
