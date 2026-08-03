package com.nuclearfusion.game

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject

object SaveStore {
    private const val PREFS = "nuclear_fusion_save"
    private const val KEY = "state_json"

    fun load(context: Context, into: GameState) {
        val raw = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).getString(KEY, null)
        if (raw.isNullOrEmpty()) {
            into.initNewGame()
            return
        }
        try {
            into.initNewGame()
            val o = JSONObject(raw)
            into.energy = o.optDouble("energy", 0.0)
            into.protons = o.optDouble("protons", 0.0)
            into.neutrons = o.optDouble("neutrons", 0.0)
            into.electrons = o.optDouble("electrons", 0.0)
            into.starDust = o.optDouble("starDust", 0.0)
            into.starType = StarType.entries.getOrElse(o.optInt("starType", 0)) { StarType.BrownDwarf }
            into.craftBatch = CraftBatch.entries.getOrElse(o.optInt("craftBatch", 0)) { CraftBatch.X1 }
            into.buyBatch = CraftBatch.entries.getOrElse(o.optInt("buyBatch", 0)) { CraftBatch.X1 }

            val els = o.optJSONArray("elements") ?: JSONArray()
            for (i in 0 until els.length()) {
                val e = els.getJSONObject(i)
                val id = e.getString("id")
                val el = into.findElement(id) ?: continue
                el.atomCount = e.optDouble("atoms", 0.0)
                el.nucleusCount = e.optDouble("nuclei", 0.0)
                el.isotopeCount = e.optDouble("isotopes", 0.0)
                el.unlocked = e.optBoolean("unlocked", el.unlocked)
                el.isotopeDiscovered = e.optBoolean("discovered", false) || el.isotopeCount > 0.0
            }

            val ups = o.optJSONArray("upgrades") ?: JSONArray()
            for (i in 0 until ups.length()) {
                val u = ups.getJSONObject(i)
                val id = u.getString("id")
                val up = into.upgrades.find { it.id == id } ?: continue
                up.level = u.optInt("level", 0).coerceAtLeast(0)
            }

            into.unlockElementsForStar()
            into.recalculateFromUpgrades()
        } catch (_: Exception) {
            into.initNewGame()
        }
    }

    fun save(context: Context, state: GameState) {
        val o = JSONObject()
        o.put("energy", state.energy)
        o.put("protons", state.protons)
        o.put("neutrons", state.neutrons)
        o.put("electrons", state.electrons)
        o.put("starDust", state.starDust)
        o.put("starType", state.starType.ordinal)
        o.put("craftBatch", state.craftBatch.ordinal)
        o.put("buyBatch", state.buyBatch.ordinal)

        val els = JSONArray()
        for (el in state.elements) {
            els.put(
                JSONObject()
                    .put("id", el.id)
                    .put("atoms", el.atomCount)
                    .put("nuclei", el.nucleusCount)
                    .put("isotopes", el.isotopeCount)
                    .put("unlocked", el.unlocked)
                    .put("discovered", el.isotopeDiscovered),
            )
        }
        o.put("elements", els)

        val ups = JSONArray()
        for (up in state.upgrades) {
            ups.put(JSONObject().put("id", up.id).put("level", up.level))
        }
        o.put("upgrades", ups)

        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            .edit()
            .putString(KEY, o.toString())
            .apply()
    }

    fun clear(context: Context) {
        context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().clear().apply()
    }
}
