#pragma once

#include "game_state.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace save_io {

inline constexpr uint32_t kMagic = 0x4E465331u;  // "NF1S"
inline constexpr uint32_t kVersion = 8u;
inline constexpr uint8_t kXorKey[8] = {0xA5, 0x3C, 0x77, 0x19,
                                       0xE2, 0x5B, 0x91, 0x0D};

inline std::wstring GameSavePath() {
  wchar_t module_path[MAX_PATH] = {};
  const DWORD len = GetModuleFileNameW(nullptr, module_path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return L"save.dat";
  }
  std::wstring path(module_path, len);
  const size_t slash = path.find_last_of(L"\\/");
  if (slash == std::wstring::npos) {
    return L"save.dat";
  }
  return path.substr(0, slash + 1) + L"save.dat";
}

inline bool SaveExists() {
  FILE* file = _wfopen(GameSavePath().c_str(), L"rb");
  if (!file) {
    return false;
  }
  std::fclose(file);
  return true;
}

inline bool DeleteSave() {
  const std::wstring path = GameSavePath();
  if (DeleteFileW(path.c_str())) {
    return true;
  }
  return GetLastError() == ERROR_FILE_NOT_FOUND;
}

inline uint32_t Crc32(const uint8_t* data, size_t size) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (int b = 0; b < 8; ++b) {
      const uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

inline void XorBuffer(std::vector<uint8_t>& buf) {
  for (size_t i = 0; i < buf.size(); ++i) {
    buf[i] ^= kXorKey[i & 7];
  }
}

struct Writer {
  std::vector<uint8_t> data;

  template <typename T>
  void Write(const T& v) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto* p = reinterpret_cast<const uint8_t*>(&v);
    data.insert(data.end(), p, p + sizeof(T));
  }

  void WriteString(const std::string& s) {
    const uint32_t n = static_cast<uint32_t>(s.size());
    Write(n);
    data.insert(data.end(), s.begin(), s.end());
  }
};

struct Reader {
  const uint8_t* p = nullptr;
  const uint8_t* end = nullptr;

  bool Ok() const { return p && p <= end; }

  template <typename T>
  bool Read(T& v) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (static_cast<size_t>(end - p) < sizeof(T)) {
      return false;
    }
    std::memcpy(&v, p, sizeof(T));
    p += sizeof(T);
    return true;
  }

  bool ReadString(std::string& s) {
    uint32_t n = 0;
    if (!Read(n) || static_cast<size_t>(end - p) < n) {
      return false;
    }
    s.assign(reinterpret_cast<const char*>(p),
             reinterpret_cast<const char*>(p) + n);
    p += n;
    return true;
  }
};

inline StarType ClampStarType(int32_t raw) {
  if (raw < 0) {
    return StarType::BrownDwarf;
  }
  if (raw > static_cast<int32_t>(StarType::NeutronStar)) {
    return StarType::NeutronStar;
  }
  return static_cast<StarType>(raw);
}

}  // namespace save_io

inline bool SaveGame(const GameState& g) {
  using namespace save_io;
  Writer w;
  w.Write(kMagic);
  w.Write(kVersion);

  // Resources + prestige; derived combat/EPS stats are recomputed from upgrades.
  w.Write(g.energy);
  w.Write(g.protons);
  w.Write(g.neutrons);
  w.Write(g.electrons);
  w.Write(g.star_dust);
  w.Write(static_cast<int32_t>(g.star_type));
  w.Write(static_cast<int32_t>(g.craft_batch));
  w.Write(static_cast<int32_t>(g.buy_batch));
  w.Write(static_cast<int32_t>(g.prestige_count));
  w.Write(g.prestige_gold_next);
  w.Write(static_cast<int32_t>(g.auto_buy));
  w.WriteString(g.auto_tech_id);

  w.Write(static_cast<uint32_t>(g.elements.size()));
  for (const auto& el : g.elements) {
    w.WriteString(el.id);
    w.Write(el.atom_count);
    w.Write(el.nucleus_count);
    w.Write(el.isotope_count);
    w.Write(static_cast<uint8_t>(el.unlocked ? 1 : 0));
    w.Write(static_cast<uint8_t>(el.isotope_discovered ? 1 : 0));
  }

  w.Write(static_cast<uint32_t>(g.upgrades.size()));
  for (const auto& up : g.upgrades) {
    w.WriteString(up.id);
    w.Write(static_cast<int32_t>(up.level));
  }

  const uint32_t crc = Crc32(w.data.data(), w.data.size());
  w.Write(crc);

  XorBuffer(w.data);

  FILE* file = _wfopen(GameSavePath().c_str(), L"wb");
  if (!file) {
    return false;
  }
  const size_t written = std::fwrite(w.data.data(), 1, w.data.size(), file);
  std::fclose(file);
  return written == w.data.size();
}

inline bool LoadGame(GameState& g) {
  using namespace save_io;

  FILE* file = _wfopen(GameSavePath().c_str(), L"rb");
  if (!file) {
    return false;
  }

  std::fseek(file, 0, SEEK_END);
  const long sz = std::ftell(file);
  std::fseek(file, 0, SEEK_SET);
  if (sz <= 0) {
    std::fclose(file);
    return false;
  }

  std::vector<uint8_t> buf(static_cast<size_t>(sz));
  if (std::fread(buf.data(), 1, buf.size(), file) != buf.size()) {
    std::fclose(file);
    return false;
  }
  std::fclose(file);

  XorBuffer(buf);
  if (buf.size() < sizeof(uint32_t) * 3) {
    return false;
  }

  uint32_t stored_crc = 0;
  std::memcpy(&stored_crc, buf.data() + buf.size() - sizeof(uint32_t),
              sizeof(uint32_t));
  const uint32_t calc = Crc32(buf.data(), buf.size() - sizeof(uint32_t));
  if (stored_crc != calc) {
    return false;
  }

  Reader r{buf.data(), buf.data() + buf.size() - sizeof(uint32_t)};
  uint32_t magic = 0;
  uint32_t version = 0;
  if (!r.Read(magic) || magic != kMagic || !r.Read(version) || version < 2u ||
      version > kVersion) {
    return false;
  }

  if (g.elements.empty()) {
    g.InitNewGame();
  }

  auto clamp_batch = [](int32_t raw) -> CraftBatch {
    if (raw < 0 || raw > static_cast<int32_t>(CraftBatch::Max)) {
      return CraftBatch::x1;
    }
    return static_cast<CraftBatch>(raw);
  };

  int32_t star_type = 0;
  g.prestige_count = 0;
  g.prestige_gold_next = kPrestigeGoldPerDust;
  g.auto_buy = AutoBuyMode::None;
  g.auto_tech_id.clear();
  if (version >= 3u) {
    int32_t craft_batch = 0;
    int32_t buy_batch = 0;
    if (!r.Read(g.energy) || !r.Read(g.protons) || !r.Read(g.neutrons) ||
        !r.Read(g.electrons) || !r.Read(g.star_dust) || !r.Read(star_type) ||
        !r.Read(craft_batch)) {
      return false;
    }
    if (version >= 4u) {
      if (!r.Read(buy_batch)) {
        return false;
      }
    }
    if (version >= 5u) {
      int32_t prestige = 0;
      if (!r.Read(prestige)) {
        return false;
      }
      g.prestige_count = prestige < 0 ? 0 : prestige;
    }
    if (version >= 6u) {
      if (!r.Read(g.prestige_gold_next)) {
        return false;
      }
      if (g.prestige_gold_next < kPrestigeGoldPerDust) {
        g.prestige_gold_next = kPrestigeGoldPerDust;
      }
    }
    if (version >= 7u) {
      int32_t auto_buy = 0;
      if (!r.Read(auto_buy)) {
        return false;
      }
      if (auto_buy < static_cast<int32_t>(AutoBuyMode::None) ||
          auto_buy > static_cast<int32_t>(AutoBuyMode::All)) {
        auto_buy = static_cast<int32_t>(AutoBuyMode::None);
      }
      g.auto_buy = static_cast<AutoBuyMode>(auto_buy);
    }
    if (version >= 8u) {
      if (!r.ReadString(g.auto_tech_id)) {
        return false;
      }
    }
    g.star_type = ClampStarType(star_type);
    g.craft_batch = clamp_batch(craft_batch);
    g.buy_batch = version >= 4u ? clamp_batch(buy_batch) : CraftBatch::x1;
  } else {
    // v2: skip obsolete derived stats + keff/mutation fields.
    double discard = 0.0;
    if (!r.Read(g.energy) || !r.Read(g.protons) || !r.Read(g.neutrons) ||
        !r.Read(g.electrons) || !r.Read(g.star_dust) || !r.Read(discard) ||
        !r.Read(discard) || !r.Read(discard) || !r.Read(discard) ||
        !r.Read(discard) || !r.Read(discard) || !r.Read(star_type)) {
      return false;
    }
    g.star_type = ClampStarType(star_type);
    g.craft_batch = CraftBatch::x1;
    g.buy_batch = CraftBatch::x1;
  }

  uint32_t el_count = 0;
  if (!r.Read(el_count)) {
    return false;
  }
  for (uint32_t i = 0; i < el_count; ++i) {
    std::string id;
    double atoms = 0, nuclei = 0, isotopes = 0;
    uint8_t unlocked = 0, discovered = 0;
    if (version >= 3u) {
      if (!r.ReadString(id) || !r.Read(atoms) || !r.Read(nuclei) ||
          !r.Read(isotopes) || !r.Read(unlocked) || !r.Read(discovered)) {
        return false;
      }
    } else {
      double keff = 0, mut = 0;
      if (!r.ReadString(id) || !r.Read(atoms) || !r.Read(nuclei) ||
          !r.Read(isotopes) || !r.Read(keff) || !r.Read(mut) ||
          !r.Read(unlocked) || !r.Read(discovered)) {
        return false;
      }
    }
    Element* el = g.FindElement(id);
    if (!el) {
      continue;
    }
    el->atom_count = atoms;
    el->nucleus_count = nuclei;
    el->isotope_count = isotopes;
    el->unlocked = unlocked != 0;
    el->isotope_discovered = discovered != 0 || isotopes > 0.0;
  }

  uint32_t up_count = 0;
  if (!r.Read(up_count)) {
    return false;
  }
  for (uint32_t i = 0; i < up_count; ++i) {
    std::string id;
    int32_t level = 0;
    if (!r.ReadString(id) || !r.Read(level)) {
      return false;
    }
    if (level < 0) {
      level = 0;
    }
    // Legacy single coil id → Helium coil.
    if (id == "annihilation") {
      id = "annihilation_Helium";
    }
    for (auto& up : g.upgrades) {
      if (up.id == id) {
        up.level = level;
        break;
      }
    }
  }

  // Pre-v5 saves: unlock D Tech if the player already bought dust upgrades.
  if (version < 5u && g.prestige_count <= 0) {
    for (const auto& up : g.upgrades) {
      if (up.persist_on_reset && up.level > 0) {
        g.prestige_count = 1;
        break;
      }
    }
  }

  // Drop invalid / D Tech auto-buy targets from older or corrupted saves.
  if (!g.auto_tech_id.empty()) {
    bool ok = false;
    for (const auto& up : g.upgrades) {
      if (up.id == g.auto_tech_id && !up.persist_on_reset) {
        ok = true;
        break;
      }
    }
    if (!ok || !g.AutoTechUnlocked()) {
      g.auto_tech_id.clear();
    }
  }

  g.UnlockElementsForStar();
  g.RecalculateFromUpgrades();
  return true;
}
