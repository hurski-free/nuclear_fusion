#pragma once

#include "simple_ui.h"
#include "game_types.h"

#include <string>
#include <unordered_map>

struct GameAssets {
  int proton = -1;
  int neutron = -1;
  int electron = -1;
  int energy = -1;
  int supernova = -1;
  int grade = -1;
  int nucleus = -1;
  int atom = -1;
  int isotope = -1;
  int autoclick = -1;
  int star_icons[4] = {-1, -1, -1, -1};
  std::unordered_map<std::string, int> elements;
  std::unordered_map<std::string, int> upgrades;

  void Load(UiContext* ctx) {
    if (!ctx || proton >= 0) {
      return;
    }
    proton = ui_load_texture(ctx, L"assets\\icons\\proton.png");
    neutron = ui_load_texture(ctx, L"assets\\icons\\neutron.png");
    electron = ui_load_texture(ctx, L"assets\\icons\\electron.png");
    energy = ui_load_texture(ctx, L"assets\\icons\\energy.png");
    supernova = ui_load_texture(ctx, L"assets\\icons\\supernova.png");
    grade = ui_load_texture(ctx, L"assets\\icons\\grade.png");
    nucleus = ui_load_texture(ctx, L"assets\\icons\\nucleus.png");
    atom = ui_load_texture(ctx, L"assets\\icons\\atom.png");
    isotope = ui_load_texture(ctx, L"assets\\icons\\isotope.png");
    autoclick = ui_load_texture(ctx, L"assets\\icons\\autoclick.png");

    star_icons[0] =
        ui_load_texture(ctx, StarTypeImagePath(StarType::BrownDwarf));
    star_icons[1] =
        ui_load_texture(ctx, StarTypeImagePath(StarType::YellowDwarf));
    star_icons[2] =
        ui_load_texture(ctx, StarTypeImagePath(StarType::BlueGiant));
    star_icons[3] =
        ui_load_texture(ctx, StarTypeImagePath(StarType::NeutronStar));

    const char* ids[] = {"Hydrogen", "Helium",     "Carbon",  "Oxygen",
                         "Silicon",  "Iron",       "Nickel",  "Silver",
                         "Xenon",    "Gadolinium", "Tungsten", "Gold"};
    const wchar_t* files[] = {
        L"assets\\images\\elements\\hydrogen.png",
        L"assets\\images\\elements\\helium.png",
        L"assets\\images\\elements\\carbon.png",
        L"assets\\images\\elements\\oxygen.png",
        L"assets\\images\\elements\\silicon.png",
        L"assets\\images\\elements\\iron.png",
        L"assets\\images\\elements\\nickel.png",
        L"assets\\images\\elements\\silver.png",
        L"assets\\images\\elements\\xenon.png",
        L"assets\\images\\elements\\gadolinium.png",
        L"assets\\images\\elements\\tungsten.png",
        L"assets\\images\\elements\\gold.png"};
    for (int i = 0; i < 12; ++i) {
      elements[ids[i]] = ui_load_texture(ctx, files[i]);
    }

    const char* up_ids[] = {
        "click_p",
        "click_n",
        "click_e",
        "nuc_Hydrogen",
        "atom_Hydrogen",
        "nuc_Helium",
        "atom_Helium",
        "nuc_Carbon",
        "atom_Carbon",
        "nuc_Oxygen",
        "atom_Oxygen",
        "nuc_Silicon",
        "atom_Silicon",
        "nuc_Iron",
        "atom_Iron",
        "nuc_Nickel",
        "atom_Nickel",
        "nuc_Silver",
        "atom_Silver",
        "nuc_Xenon",
        "atom_Xenon",
        "nuc_Gadolinium",
        "atom_Gadolinium",
        "nuc_Tungsten",
        "atom_Tungsten",
        "nuc_Gold",
        "atom_Gold",
        "crit_amplifier",
        "tungsten_catalyst",
        "quantum_cpu",
        "boost_dust_eps",
        "boost_click_e",
        "boost_click_p",
        "annihilation_Hydrogen",
        "annihilation_Helium",
        "annihilation_Carbon",
        "annihilation_Oxygen",
        "annihilation_Silicon",
        "annihilation_Iron",
        "annihilation_Nickel",
        "annihilation_Silver",
        "annihilation_Xenon",
        "annihilation_Gadolinium",
        "annihilation_Tungsten",
        "annihilation_Gold"};
    const wchar_t* up_files[] = {
        L"assets\\icons\\upgrades\\click_p.png",
        L"assets\\icons\\upgrades\\click_n.png",
        L"assets\\icons\\upgrades\\click_e.png",
        L"assets\\icons\\upgrades\\nuc_Hydrogen.png",
        L"assets\\icons\\upgrades\\atom_Hydrogen.png",
        L"assets\\icons\\upgrades\\nuc_Helium.png",
        L"assets\\icons\\upgrades\\atom_Helium.png",
        L"assets\\icons\\upgrades\\nuc_Carbon.png",
        L"assets\\icons\\upgrades\\atom_Carbon.png",
        L"assets\\icons\\upgrades\\nuc_Oxygen.png",
        L"assets\\icons\\upgrades\\atom_Oxygen.png",
        L"assets\\icons\\upgrades\\nuc_Silicon.png",
        L"assets\\icons\\upgrades\\atom_Silicon.png",
        L"assets\\icons\\upgrades\\nuc_Iron.png",
        L"assets\\icons\\upgrades\\atom_Iron.png",
        L"assets\\icons\\upgrades\\nuc_Nickel.png",
        L"assets\\icons\\upgrades\\atom_Nickel.png",
        L"assets\\icons\\upgrades\\nuc_Silver.png",
        L"assets\\icons\\upgrades\\atom_Silver.png",
        L"assets\\icons\\upgrades\\nuc_Xenon.png",
        L"assets\\icons\\upgrades\\atom_Xenon.png",
        L"assets\\icons\\upgrades\\nuc_Gadolinium.png",
        L"assets\\icons\\upgrades\\atom_Gadolinium.png",
        L"assets\\icons\\upgrades\\nuc_Tungsten.png",
        L"assets\\icons\\upgrades\\atom_Tungsten.png",
        L"assets\\icons\\upgrades\\nuc_Gold.png",
        L"assets\\icons\\upgrades\\atom_Gold.png",
        L"assets\\icons\\upgrades\\crit_amplifier.png",
        L"assets\\icons\\upgrades\\tungsten_catalyst.png",
        L"assets\\icons\\upgrades\\quantum_cpu.png",
        L"assets\\icons\\energy.png",
        L"assets\\icons\\upgrades\\click_e.png",
        L"assets\\icons\\upgrades\\click_p.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png",
        L"assets\\icons\\upgrades\\annihilation.png"};
    for (int i = 0; i < 45; ++i) {
      upgrades[up_ids[i]] = ui_load_texture(ctx, up_files[i]);
    }
  }

  int StarIcon(StarType type) const {
    const int i = static_cast<int>(type);
    if (i < 0 || i > 3) {
      return star_icons[0];
    }
    return star_icons[i];
  }

  int ElementIcon(const std::string& id) const {
    const auto it = elements.find(id);
    return it == elements.end() ? -1 : it->second;
  }

  int UpgradeIcon(const std::string& id) const {
    const auto it = upgrades.find(id);
    if (it != upgrades.end() && it->second >= 0) {
      return it->second;
    }
    if (id.rfind("nuc_", 0) == 0) {
      return ElementIcon(id.substr(4));
    }
    if (id.rfind("atom_", 0) == 0) {
      return ElementIcon(id.substr(5));
    }
    if (id.rfind("annihilation", 0) == 0) {
      const auto coil = upgrades.find("annihilation_Helium");
      if (coil != upgrades.end() && coil->second >= 0) {
        return coil->second;
      }
    }
    return grade;
  }
};
