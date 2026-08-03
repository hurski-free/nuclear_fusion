package com.nuclearfusion.game

import kotlin.math.floor
import kotlin.math.pow

enum class ResourceKind {
    Energy, Proton, Neutron, Electron, Nucleus, Atom, Isotope, StarDust
}

data class ResourceCost(
    val kind: ResourceKind,
    val elementId: String = "",
    val amount: Double = 0.0,
)

enum class StarType {
    BrownDwarf, YellowDwarf, BlueGiant, NeutronStar;

    fun displayName(): String = when (this) {
        BrownDwarf -> "Brown Dwarf"
        YellowDwarf -> "Yellow Dwarf"
        BlueGiant -> "Blue Giant"
        NeutronStar -> "Neutron Star"
    }
}

const val SUPERNOVA_ATOM_GOAL = 100.0
const val PRESTIGE_GOLD_PER_DUST = 10.0
const val IRON_ATOMIC_NUMBER = 26

fun starMaxAtomicNumber(type: StarType): Int = when (type) {
    StarType.BrownDwarf -> 2
    StarType.YellowDwarf -> 8
    StarType.BlueGiant -> 26
    StarType.NeutronStar -> 92
}

fun starDustReward(type: StarType): Double = when (type) {
    StarType.BrownDwarf -> 1.0
    StarType.YellowDwarf -> 5.0
    StarType.BlueGiant -> 25.0
    StarType.NeutronStar -> 100.0
}

fun elementSymbol(id: String): String = when (id) {
    "Hydrogen" -> "H"
    "Helium" -> "He"
    "Carbon" -> "C"
    "Oxygen" -> "O"
    "Silicon" -> "Si"
    "Iron" -> "Fe"
    "Nickel" -> "Ni"
    "Silver" -> "Ag"
    "Xenon" -> "Xe"
    "Gold" -> "Au"
    else -> "?"
}

enum class CraftBatch {
    X1, X10, X100, Max;

    fun multiplier(): Int = when (this) {
        X1 -> 1
        X10 -> 10
        X100 -> 100
        Max -> 0
    }

    fun label(): String = when (this) {
        X1 -> "x1"
        X10 -> "x10"
        X100 -> "x100"
        Max -> "Max"
    }
}

enum class UpgradeEffect {
    ClickPower, CritChance, CritMultiplier, AutoEps, AutoClickMult, IsotopeEpsMult
}

fun upgradeGrade(level: Int): Int = when {
    level < 10 -> 0
    level < 25 -> 1
    level < 50 -> 2
    level < 100 -> 3
    else -> 3 + level / 100
}

fun upgradeGradeMultiplier(grade: Int): Double =
    if (grade <= 0) 1.0 else 2.0.pow(grade.toDouble())

fun upgradeUsesGrade(effect: UpgradeEffect): Boolean =
    effect == UpgradeEffect.ClickPower ||
        effect == UpgradeEffect.AutoEps ||
        effect == UpgradeEffect.IsotopeEpsMult

data class Element(
    val id: String,
    val name: String,
    val atomicNumber: Int,
    val protonsNeeded: Int,
    val neutronsNeeded: Int,
    val electronsNeeded: Int,
    val energyActivation: Double,
    var atomCount: Double = 0.0,
    var nucleusCount: Double = 0.0,
    var isotopeCount: Double = 0.0,
    var unlocked: Boolean = false,
    var isotopeDiscovered: Boolean = false,
)

data class UpgradeDef(
    val id: String,
    val name: String,
    val description: String,
    val baseCosts: List<ResourceCost>,
    val costScale: Double,
    val effect: UpgradeEffect,
    val effectPerLevel: Double,
    var level: Int = 0,
)

fun upgradeScaledEffect(up: UpgradeDef): Double {
    if (up.level <= 0) return 0.0
    val gradeMult =
        if (upgradeUsesGrade(up.effect)) upgradeGradeMultiplier(upgradeGrade(up.level)) else 1.0
    return up.effectPerLevel * up.level * gradeMult
}

data class ClickResult(val gain: Double, val crit: Boolean)

fun formatCompact(v: Double): String {
    val a = kotlin.math.abs(v)
    return when {
        a >= 1e12 -> String.format("%.2fT", v / 1e12)
        a >= 1e9 -> String.format("%.2fB", v / 1e9)
        a >= 1e6 -> String.format("%.2fM", v / 1e6)
        a >= 1000.0 -> String.format("%.1fK", v / 1000.0)
        a >= 10.0 && kotlin.math.abs(v - floor(v)) > 1e-6 -> String.format("%.1f", v)
        a >= 1.0 && kotlin.math.abs(v - floor(v)) > 1e-6 -> String.format("%.2f", v)
        else -> String.format("%.0f", v)
    }
}
