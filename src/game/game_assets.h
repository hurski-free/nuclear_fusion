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
  int lab = -1;
  int tech = -1;
  int star_icons[4] = {-1, -1, -1, -1};
  std::unordered_map<std::string, int> elements;
  std::unordered_map<std::string, int> isotopes;

  void Load(UiContext* ctx) {
    if (!ctx || proton >= 0) {
      return;
    }
    proton = ui_load_texture(ctx, L"assets\\icons\\proton.png");
    neutron = ui_load_texture(ctx, L"assets\\icons\\neutron.png");
    electron = ui_load_texture(ctx, L"assets\\icons\\electron.png");
    energy = ui_load_texture(ctx, L"assets\\icons\\energy.png");
    supernova = ui_load_texture(ctx, L"assets\\icons\\supernova.png");
    lab = ui_load_texture(ctx, L"assets\\icons\\lab.png");
    tech = ui_load_texture(ctx, L"assets\\icons\\tech.png");

    star_icons[0] =
        ui_load_texture(ctx, StarTypeImagePath(StarType::BrownDwarf));
    star_icons[1] =
        ui_load_texture(ctx, StarTypeImagePath(StarType::YellowDwarf));
    star_icons[2] =
        ui_load_texture(ctx, StarTypeImagePath(StarType::BlueGiant));
    star_icons[3] =
        ui_load_texture(ctx, StarTypeImagePath(StarType::NeutronStar));

    const char* ids[] = {"Hydrogen", "Helium", "Carbon", "Oxygen",
                         "Silicon",  "Iron",   "Gold"};
    const wchar_t* files[] = {
        L"assets\\images\\elements\\hydrogen.png",
        L"assets\\images\\elements\\helium.png",
        L"assets\\images\\elements\\carbon.png",
        L"assets\\images\\elements\\oxygen.png",
        L"assets\\images\\elements\\silicon.png",
        L"assets\\images\\elements\\iron.png",
        L"assets\\images\\elements\\gold.png"};
    const wchar_t* iso_files[] = {
        L"assets\\images\\elements\\iso_hydrogen.png",
        L"assets\\images\\elements\\iso_helium.png",
        L"assets\\images\\elements\\iso_carbon.png",
        L"assets\\images\\elements\\iso_oxygen.png",
        L"assets\\images\\elements\\iso_silicon.png",
        L"assets\\images\\elements\\iso_iron.png",
        L"assets\\images\\elements\\iso_gold.png"};
    for (int i = 0; i < 7; ++i) {
      elements[ids[i]] = ui_load_texture(ctx, files[i]);
      isotopes[ids[i]] = ui_load_texture(ctx, iso_files[i]);
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

  int IsotopeIcon(const std::string& id) const {
    const auto it = isotopes.find(id);
    return it == isotopes.end() ? -1 : it->second;
  }
};
