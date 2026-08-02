#pragma once

#include "game_types.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>

inline constexpr int kIronAtomicNumber = 26;

struct GameState {
  double energy = 0.0;
  double protons = 0.0;
  double neutrons = 0.0;
  double electrons = 0.0;
  double star_dust = 0.0;

  double click_power = 1.0;
  double crit_chance = 0.05;
  double crit_multiplier = 2.0;
  double auto_eps = 0.0;
  double auto_click_mult = 1.0;
  double annihilation_mult = 2.0;

  // Energy cost to materialize one particle.
  double price_proton = 10.0;
  double price_neutron = 12.0;
  double price_electron = 5.0;

  StarType star_type = StarType::BrownDwarf;
  CraftBatch craft_batch = CraftBatch::x1;
  GameTab tab = GameTab::Star;

  std::vector<Element> elements;
  std::vector<UpgradeDef> upgrades;

  std::mt19937 rng{std::random_device{}()};

  void InitNewGame() {
    energy = 0.0;
    protons = 0.0;
    neutrons = 0.0;
    electrons = 0.0;
    star_dust = 0.0;
    click_power = 1.0;
    crit_chance = 0.05;
    crit_multiplier = 2.0;
    auto_eps = 0.0;
    auto_click_mult = 1.0;
    annihilation_mult = 2.0;
    star_type = StarType::BrownDwarf;
    craft_batch = CraftBatch::x1;
    tab = GameTab::Star;
    elements = CreateDefaultElements();
    upgrades = CreateDefaultUpgrades();
    UnlockElementsForStar();
  }

  static std::vector<Element> CreateDefaultElements() {
    return {
        Element{"Hydrogen", L"Hydrogen", 1, 1, 1, 1, 8.0, 1.35, 0.0, 0, 0, 0, true, false},
        Element{"Helium", L"Helium", 2, 2, 2, 2, 40.0, 1.25, 0.0, 0, 0, 0, true, false},
        Element{"Carbon", L"Carbon", 6, 6, 6, 6, 220.0, 1.18, 0.0, 0, 0, 0, false, false},
        Element{"Oxygen", L"Oxygen", 8, 8, 8, 8, 480.0, 1.12, 0.0, 0, 0, 0, false, false},
        Element{"Silicon", L"Silicon", 14, 14, 14, 14, 1800.0, 1.05, 0.0, 0, 0, 0, false, false},
        Element{"Iron", L"Iron", 26, 26, 30, 26, 12000.0, 1.0, 0.0, 0, 0, 0, false, false},
        Element{"Gold", L"Gold", 79, 79, 118, 79, 250000.0, 0.0, 0.0, 0, 0, 0, false, false},
    };
  }

  static std::vector<UpgradeDef> CreateDefaultUpgrades() {
    std::vector<UpgradeDef> list;

    list.push_back(UpgradeDef{
        "click_p", L"Proton Injector", L"+1 eV click power",
        {{ResourceKind::Proton, "", 100}}, 1.4, 100, UpgradeEffect::ClickPower, 1.0, 0});

    list.push_back(UpgradeDef{
        "click_n", L"Neutron Channel", L"+2 eV click power",
        {{ResourceKind::Neutron, "", 80}}, 1.45, 100, UpgradeEffect::ClickPower, 2.0, 0});

    list.push_back(UpgradeDef{
        "click_e", L"Electron Lens", L"+0.5 eV click power",
        {{ResourceKind::Electron, "", 120}}, 1.35, 100, UpgradeEffect::ClickPower, 0.5, 0});

    list.push_back(UpgradeDef{
        "crit_he", L"Helium Core Focus", L"+1% crit chance",
        {{ResourceKind::Nucleus, "Helium", 5}}, 1.5, 40, UpgradeEffect::CritChance, 0.01, 0});

    list.push_back(UpgradeDef{
        "crit_c", L"Carbon Lattice", L"+0.2 crit multiplier",
        {{ResourceKind::Nucleus, "Carbon", 8}}, 1.55, 30, UpgradeEffect::CritMultiplier, 0.2, 0});

    list.push_back(UpgradeDef{
        "auto_h", L"Hydrogen Farm", L"+0.5 EPS",
        {{ResourceKind::Atom, "Hydrogen", 10}}, 1.4, 80, UpgradeEffect::AutoEps, 0.5, 0});

    list.push_back(UpgradeDef{
        "auto_he", L"Helium Turbine", L"+2 EPS",
        {{ResourceKind::Atom, "Helium", 8}}, 1.45, 60, UpgradeEffect::AutoEps, 2.0, 0});

    list.push_back(UpgradeDef{
        "auto_o", L"Oxygen Reactor", L"+12 EPS",
        {{ResourceKind::Atom, "Oxygen", 6}}, 1.5, 40, UpgradeEffect::AutoEps, 12.0, 0});

    list.push_back(UpgradeDef{
        "quantum_cpu", L"Quantum Processor", L"Auto-click x10 multiplier",
        {{ResourceKind::Electron, "", 10000},
         {ResourceKind::Nucleus, "Silicon", 500},
         {ResourceKind::Atom, "Gold", 50}},
        2.2, 5, UpgradeEffect::AutoClickMult, 10.0, 0});

    list.push_back(UpgradeDef{
        "annihilation", L"Annihilation Coil", L"+0.25 isotope cashout multiplier",
        {{ResourceKind::Isotope, "Helium", 3}, {ResourceKind::Energy, "", 5000}},
        1.6, 20, UpgradeEffect::AnnihilationMult, 0.25, 0});

    return list;
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

  double EffectiveMutationChance(const Element& el) const {
    return std::clamp(el.mutation_chance + star_dust * 0.001, 0.0, 0.95);
  }

  double EffectiveKeff(const Element& el) const {
    return std::max(0.0, el.k_eff_base);
  }

  double ActivationEnergy(const Element& el) const {
    if (el.atomic_number <= kIronAtomicNumber) {
      return el.energy_activation;
    }
    const int over = el.atomic_number - kIronAtomicNumber;
    return el.energy_activation * std::pow(1.18, static_cast<double>(over));
  }

  // Energy-equivalent craft cost used for refund / annihilation.
  double CraftEnergyCost(const Element& el) const {
    return el.protons_needed * price_proton + el.neutrons_needed * price_neutron +
           el.electrons_needed * price_electron + ActivationEnergy(el);
  }

  double NucleusEnergyCost(const Element& el) const {
    return el.protons_needed * price_proton + el.neutrons_needed * price_neutron +
           ActivationEnergy(el);
  }

  double Eps() const { return auto_eps * auto_click_mult; }

  void Tick(float dt) {
    if (dt > 0.f) {
      energy += Eps() * static_cast<double>(dt);
    }
  }

  double ClickStar() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double gain = click_power;
    if (dist(rng) < crit_chance) {
      gain *= crit_multiplier;
    }
    energy += gain;
    return gain;
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

  int ResolveBatchCount(int max_affordable) const {
    if (max_affordable <= 0) {
      return 0;
    }
    const int mult = BatchMultiplier(craft_batch);
    if (mult == 0) {
      return max_affordable;
    }
    return std::min(mult, max_affordable);
  }

  int MaxNucleusCraft(const Element& el) const {
    if (!el.unlocked || el.atomic_number > StarMaxAtomicNumber(star_type)) {
      return 0;
    }
    const double e_act = ActivationEnergy(el);
    if (el.protons_needed <= 0 || e_act <= 0.0) {
      return 0;
    }
    const int by_p = static_cast<int>(protons / el.protons_needed);
    const int by_n =
        el.neutrons_needed > 0
            ? static_cast<int>(neutrons / el.neutrons_needed)
            : by_p;
    const int by_e = static_cast<int>(energy / e_act);
    return std::max(0, std::min({by_p, by_n, by_e}));
  }

  int MaxAtomCraft(const Element& el) const {
    if (!el.unlocked || el.atomic_number > StarMaxAtomicNumber(star_type)) {
      return 0;
    }
    if (el.electrons_needed <= 0) {
      return 0;
    }
    const double e_act = ActivationEnergy(el) * 0.35;
    if (e_act <= 0.0) {
      return 0;
    }
    const int by_nuc = static_cast<int>(el.nucleus_count);
    const int by_e = static_cast<int>(electrons / el.electrons_needed);
    const int by_energy = static_cast<int>(energy / e_act);
    return std::max(0, std::min({by_nuc, by_e, by_energy}));
  }

  bool CraftNucleus(Element& el, int requested) {
    const int max_n = MaxNucleusCraft(el);
    const int count = ResolveBatchCount(std::min(requested > 0 ? requested : max_n, max_n));
    if (count <= 0) {
      return false;
    }

    const double e_act = ActivationEnergy(el);
    const double spend_e = e_act * count;
    protons -= el.protons_needed * count;
    neutrons -= el.neutrons_needed * count;
    energy -= spend_e;

    el.nucleus_count += count;

    if (el.atomic_number <= kIronAtomicNumber) {
      const double cost = NucleusEnergyCost(el) * count;
      energy += cost * EffectiveKeff(el);
    }
    return true;
  }

  bool CraftAtom(Element& el, int requested) {
    const int max_n = MaxAtomCraft(el);
    const int count = ResolveBatchCount(std::min(requested > 0 ? requested : max_n, max_n));
    if (count <= 0) {
      return false;
    }

    const double e_act = ActivationEnergy(el) * 0.35;
    el.nucleus_count -= count;
    electrons -= el.electrons_needed * count;
    energy -= e_act * count;

    std::uniform_real_distribution<double> dist(0.0, 1.0);
    const double mut = EffectiveMutationChance(el);
    int atoms = 0;
    int isotopes = 0;
    for (int i = 0; i < count; ++i) {
      if (dist(rng) <= mut) {
        ++isotopes;
      } else {
        ++atoms;
      }
    }
    el.atom_count += atoms;
    el.isotope_count += isotopes;
    if (isotopes > 0) {
      el.isotope_discovered = true;
    }

    if (el.atomic_number <= kIronAtomicNumber) {
      const double cost = CraftEnergyCost(el) * count;
      energy += cost * EffectiveKeff(el);
    }
    return true;
  }

  bool IsotopeDiscovered(const std::string& element_id) const {
    const Element* el = FindElement(element_id);
    return el && el->isotope_discovered;
  }

  // Tech upgrades that spend an undiscovered isotope stay hidden.
  bool IsUpgradeVisible(const UpgradeDef& up) const {
    for (const auto& c : up.base_costs) {
      if (c.kind == ResourceKind::Isotope && !IsotopeDiscovered(c.element_id)) {
        return false;
      }
    }
    return true;
  }

  bool AnnihilateIsotopes(Element& el, int requested) {
    const int available = static_cast<int>(el.isotope_count);
    const int count = ResolveBatchCount(std::min(requested > 0 ? requested : available, available));
    if (count <= 0) {
      return false;
    }
    el.isotope_count -= count;
    energy += count * CraftEnergyCost(el) * annihilation_mult;
    return true;
  }

  bool InvestIsotopes(Element& el, int requested) {
    const int available = static_cast<int>(el.isotope_count);
    const int count = ResolveBatchCount(std::min(requested > 0 ? requested : available, available));
    if (count <= 0) {
      return false;
    }
    el.isotope_count -= count;
    el.k_eff_base += 0.01 * count;
    el.mutation_chance += 0.002 * count;
    el.mutation_chance = std::min(el.mutation_chance, 0.75);
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
    return base.amount * std::pow(up.cost_scale, static_cast<double>(up.level));
  }

  bool CanAffordUpgrade(const UpgradeDef& up) const {
    if (up.level >= up.max_level) {
      return false;
    }
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
    ApplyUpgradeEffect(up);
    return true;
  }

  void ApplyUpgradeEffect(const UpgradeDef& up) {
    switch (up.effect) {
      case UpgradeEffect::ClickPower:
        click_power += up.effect_per_level;
        break;
      case UpgradeEffect::CritChance:
        crit_chance = std::min(0.9, crit_chance + up.effect_per_level);
        break;
      case UpgradeEffect::CritMultiplier:
        crit_multiplier += up.effect_per_level;
        break;
      case UpgradeEffect::AutoEps:
        auto_eps += up.effect_per_level;
        break;
      case UpgradeEffect::AutoClickMult:
        auto_click_mult += up.effect_per_level;
        break;
      case UpgradeEffect::AnnihilationMult:
        annihilation_mult += up.effect_per_level;
        break;
    }
  }

  bool CanTriggerSupernova() const {
    // Require synthesizing something at the current star's cap.
    const int max_z = StarMaxAtomicNumber(star_type);
    for (const auto& el : elements) {
      if (el.atomic_number == max_z &&
          (el.atom_count >= 1.0 || el.nucleus_count >= 1.0)) {
        return true;
      }
    }
    return false;
  }

  bool TriggerSupernova() {
    if (star_type == StarType::NeutronStar || !CanTriggerSupernova()) {
      return false;
    }

    star_dust += StarDustReward(star_type);

    energy = 0.0;
    protons = 0.0;
    neutrons = 0.0;
    electrons = 0.0;

    for (auto& el : elements) {
      el.atom_count = 0.0;
      el.nucleus_count = 0.0;
      el.isotope_count = 0.0;
      // Keep k_eff / mutation investments and unlock progress via star dust.
    }

    // Soft-reset click/auto but keep a fraction via star dust.
    click_power = 1.0 + star_dust * 0.05;
    crit_chance = 0.05;
    crit_multiplier = 2.0;
    auto_eps = star_dust * 0.1;
    auto_click_mult = 1.0;
    annihilation_mult = 2.0;

    for (auto& up : upgrades) {
      up.level = 0;
    }

    star_type = static_cast<StarType>(static_cast<int>(star_type) + 1);
    UnlockElementsForStar();
    return true;
  }
};
