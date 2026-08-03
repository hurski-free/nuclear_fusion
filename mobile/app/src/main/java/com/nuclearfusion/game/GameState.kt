package com.nuclearfusion.game

import kotlin.math.floor
import kotlin.math.min
import kotlin.math.pow
import kotlin.random.Random

class GameState {
    var energy = 0.0
    var protons = 0.0
    var neutrons = 0.0
    var electrons = 0.0
    var starDust = 0.0

    var clickPower = 1.0
    var critChance = 0.0
    var critMultiplier = 2.0
    var autoEps = 0.0
    var autoClickMult = 1.0
    var isotopeEpsMult = 1.0

    var priceProton = 10.0
    var priceNeutron = 12.0
    var priceElectron = 5.0

    var starType = StarType.BrownDwarf
    var craftBatch = CraftBatch.X1
    var buyBatch = CraftBatch.X1

    var elements: MutableList<Element> = mutableListOf()
    var upgrades: MutableList<UpgradeDef> = mutableListOf()

    private val rng = Random.Default

    fun initNewGame() {
        energy = 0.0
        protons = 0.0
        neutrons = 0.0
        electrons = 0.0
        starDust = 0.0
        clickPower = 1.0
        critChance = 0.0
        critMultiplier = 2.0
        autoEps = 0.0
        autoClickMult = 1.0
        isotopeEpsMult = 1.0
        starType = StarType.BrownDwarf
        craftBatch = CraftBatch.X1
        buyBatch = CraftBatch.X1
        elements = createDefaultElements()
        upgrades = createDefaultUpgrades()
        unlockElementsForStar()
        recalculateFromUpgrades()
    }

    fun findElement(id: String): Element? = elements.find { it.id == id }

    fun unlockElementsForStar() {
        val maxZ = starMaxAtomicNumber(starType)
        elements.forEach { if (it.atomicNumber <= maxZ) it.unlocked = true }
    }

    fun effectiveMutationChance(): Double =
        (starDust * 0.001).coerceIn(0.0, 0.95)

    fun activationEnergy(el: Element): Double {
        if (el.atomicNumber <= IRON_ATOMIC_NUMBER) return el.energyActivation
        val over = el.atomicNumber - IRON_ATOMIC_NUMBER
        return el.energyActivation * 1.18.pow(over.toDouble())
    }

    fun isotopeEpsPer(el: Element): Double = 0.05 * el.atomicNumber

    fun isotopeEps(): Double {
        var sum = 0.0
        for (el in elements) {
            if (el.isotopeCount > 0.0) sum += el.isotopeCount * isotopeEpsPer(el)
        }
        return sum * isotopeEpsMult
    }

    fun eps(): Double {
        val dustMult = 1.0 + starDust * 0.02
        return (autoEps * autoClickMult + isotopeEps()) * dustMult + starDust
    }

    fun tick(dt: Float) {
        if (dt > 0f) energy += eps() * dt.toDouble()
    }

    fun clickStar(): ClickResult {
        var gain = clickPower
        val crit = rng.nextDouble() < critChance
        if (crit) gain *= critMultiplier
        energy += gain
        return ClickResult(gain, crit)
    }

    private fun materialize(kind: ResourceKind, amount: Double): Boolean {
        val price = when (kind) {
            ResourceKind.Proton -> priceProton
            ResourceKind.Neutron -> priceNeutron
            ResourceKind.Electron -> priceElectron
            else -> return false
        }
        val cost = amount * price
        if (amount <= 0.0 || energy < cost) return false
        energy -= cost
        when (kind) {
            ResourceKind.Proton -> protons += amount
            ResourceKind.Neutron -> neutrons += amount
            ResourceKind.Electron -> electrons += amount
            else -> {}
        }
        return true
    }

    fun maxMaterialize(kind: ResourceKind): Double {
        val price = when (kind) {
            ResourceKind.Proton -> priceProton
            ResourceKind.Neutron -> priceNeutron
            ResourceKind.Electron -> priceElectron
            else -> return 0.0
        }
        if (price <= 0.0) return 0.0
        return floor(energy / price)
    }

    fun resolveBatchAmount(maxAffordable: Double, batch: CraftBatch): Double {
        if (maxAffordable <= 0.0) return 0.0
        val capped = floor(maxAffordable)
        val mult = batch.multiplier()
        return if (mult == 0) capped else min(mult.toDouble(), capped)
    }

    fun resolveParticleBuyAmount(kind: ResourceKind): Double =
        resolveBatchAmount(maxMaterialize(kind), buyBatch)

    fun buyParticle(kind: ResourceKind): Boolean {
        val amount = resolveParticleBuyAmount(kind)
        return materialize(kind, amount)
    }

    private fun floorToCount(v: Double): Int {
        if (v <= 0.0 || !v.isFinite()) return 0
        if (v >= Int.MAX_VALUE.toDouble()) return Int.MAX_VALUE
        return floor(v).toInt()
    }

    fun maxNucleusCraft(el: Element): Int {
        if (!el.unlocked || el.atomicNumber > starMaxAtomicNumber(starType)) return 0
        val eAct = activationEnergy(el)
        if (el.protonsNeeded <= 0 || eAct <= 0.0) return 0
        val byP = protons / el.protonsNeeded
        val byN = if (el.neutronsNeeded > 0) neutrons / el.neutronsNeeded else byP
        val byE = energy / eAct
        return floorToCount(minOf(byP, byN, byE))
    }

    fun maxAtomCraft(el: Element): Int {
        if (!el.unlocked || el.atomicNumber > starMaxAtomicNumber(starType)) return 0
        if (el.electronsNeeded <= 0) return 0
        val eAct = activationEnergy(el) * 0.35
        if (eAct <= 0.0) return 0
        val byNuc = el.nucleusCount
        val byE = electrons / el.electronsNeeded
        val byEnergy = energy / eAct
        return floorToCount(minOf(byNuc, byE, byEnergy))
    }

    fun resolveCraftBatchCount(maxAffordable: Int): Int {
        if (maxAffordable <= 0) return 0
        val mult = craftBatch.multiplier()
        return if (mult == 0) maxAffordable else min(mult, maxAffordable)
    }

    fun craftNucleus(el: Element): Boolean {
        val maxN = maxNucleusCraft(el)
        val count = resolveCraftBatchCount(maxN)
        if (count <= 0) return false
        val spend = activationEnergy(el) * count
        protons -= el.protonsNeeded * count
        neutrons -= el.neutronsNeeded * count
        energy -= spend
        el.nucleusCount += count
        return true
    }

    private fun sampleBinomial(n: Int, p: Double): Int {
        if (n <= 0 || p <= 0.0) return 0
        if (p >= 1.0) return n
        // Approximate for large n; exact loop for small.
        if (n > 5000) {
            val mean = n * p
            val sd = kotlin.math.sqrt(n * p * (1.0 - p))
            val u1 = rng.nextDouble().coerceAtLeast(1e-12)
            val u2 = rng.nextDouble()
            val gauss = kotlin.math.sqrt(-2.0 * kotlin.math.ln(u1)) *
                kotlin.math.cos(2.0 * Math.PI * u2)
            return (mean + gauss * sd).toInt().coerceIn(0, n)
        }
        var c = 0
        repeat(n) { if (rng.nextDouble() < p) c++ }
        return c
    }

    fun craftAtom(el: Element): Boolean {
        val maxN = maxAtomCraft(el)
        val count = resolveCraftBatchCount(maxN)
        if (count <= 0) return false
        val eAct = activationEnergy(el) * 0.35
        el.nucleusCount -= count
        electrons -= el.electronsNeeded * count
        energy -= eAct * count
        val isotopes = sampleBinomial(count, effectiveMutationChance())
        el.atomCount += (count - isotopes)
        el.isotopeCount += isotopes
        if (isotopes > 0) el.isotopeDiscovered = true
        return true
    }

    fun isUpgradeVisible(up: UpgradeDef): Boolean {
        for (c in up.baseCosts) {
            if (c.kind == ResourceKind.Isotope && findElement(c.elementId)?.isotopeDiscovered != true) {
                return false
            }
            if ((c.kind == ResourceKind.Nucleus || c.kind == ResourceKind.Atom) && c.elementId.isNotEmpty()) {
                val el = findElement(c.elementId) ?: return false
                if (!el.unlocked) return false
            }
        }
        return true
    }

    fun resourceAmount(cost: ResourceCost): Double = when (cost.kind) {
        ResourceKind.Energy -> energy
        ResourceKind.Proton -> protons
        ResourceKind.Neutron -> neutrons
        ResourceKind.Electron -> electrons
        ResourceKind.StarDust -> starDust
        ResourceKind.Nucleus -> findElement(cost.elementId)?.nucleusCount ?: 0.0
        ResourceKind.Atom -> findElement(cost.elementId)?.atomCount ?: 0.0
        ResourceKind.Isotope -> findElement(cost.elementId)?.isotopeCount ?: 0.0
    }

    fun scaledCostAmount(up: UpgradeDef, base: ResourceCost): Double =
        base.amount * up.costScale.pow(up.level.toDouble())

    fun canAffordUpgrade(up: UpgradeDef): Boolean =
        up.baseCosts.all { resourceAmount(it) + 1e-9 >= scaledCostAmount(up, it) }

    private fun spendResource(cost: ResourceCost, amount: Double) {
        when (cost.kind) {
            ResourceKind.Energy -> energy -= amount
            ResourceKind.Proton -> protons -= amount
            ResourceKind.Neutron -> neutrons -= amount
            ResourceKind.Electron -> electrons -= amount
            ResourceKind.StarDust -> starDust -= amount
            ResourceKind.Nucleus -> findElement(cost.elementId)?.let { it.nucleusCount -= amount }
            ResourceKind.Atom -> findElement(cost.elementId)?.let { it.atomCount -= amount }
            ResourceKind.Isotope -> findElement(cost.elementId)?.let { it.isotopeCount -= amount }
        }
    }

    fun buyUpgrade(up: UpgradeDef): Boolean {
        if (!canAffordUpgrade(up)) return false
        for (c in up.baseCosts) spendResource(c, scaledCostAmount(up, c))
        up.level += 1
        recalculateFromUpgrades()
        return true
    }

    fun recalculateFromUpgrades() {
        clickPower = 0.0
        critChance = 0.0
        critMultiplier = 2.0
        autoEps = 0.0
        autoClickMult = 1.0
        isotopeEpsMult = 1.0
        for (up in upgrades) {
            if (up.level <= 0) continue
            val total = upgradeScaledEffect(up)
            when (up.effect) {
                UpgradeEffect.ClickPower -> clickPower += total
                UpgradeEffect.CritChance -> critChance = min(1.0, critChance + total)
                UpgradeEffect.CritMultiplier -> critMultiplier += total
                UpgradeEffect.AutoEps -> autoEps += total
                UpgradeEffect.AutoClickMult -> autoClickMult += total
                UpgradeEffect.IsotopeEpsMult -> isotopeEpsMult += total
            }
        }
        val x = 1.0 + clickPower
        clickPower = x * (1.0 + starDust * 0.05)
    }

    fun canTriggerSupernova(): Boolean {
        val maxZ = starMaxAtomicNumber(starType)
        return elements.any { it.atomicNumber == maxZ && it.atomCount + 1e-9 >= SUPERNOVA_ATOM_GOAL }
    }

    private fun softResetRun() {
        energy = 0.0
        protons = 0.0
        neutrons = 0.0
        electrons = 0.0
        for (el in elements) {
            el.atomCount = 0.0
            el.nucleusCount = 0.0
            el.isotopeCount = 0.0
        }
        for (up in upgrades) up.level = 0
    }

    fun triggerSupernova(): Boolean {
        if (starType == StarType.NeutronStar || !canTriggerSupernova()) return false
        starDust += starDustReward(starType)
        softResetRun()
        starType = StarType.entries[starType.ordinal + 1]
        recalculateFromUpgrades()
        unlockElementsForStar()
        return true
    }

    fun prestigeDustReward(): Double {
        if (starType != StarType.NeutronStar) return 0.0
        val gold = findElement("Gold") ?: return 0.0
        return floor(gold.atomCount / PRESTIGE_GOLD_PER_DUST)
    }

    fun canTriggerPrestige(): Boolean = prestigeDustReward() + 1e-9 >= 1.0

    fun triggerPrestige(): Boolean {
        if (!canTriggerPrestige()) return false
        starDust += prestigeDustReward()
        softResetRun()
        recalculateFromUpgrades()
        unlockElementsForStar()
        return true
    }

    companion object {
        fun createDefaultElements(): MutableList<Element> = mutableListOf(
            Element("Hydrogen", "Hydrogen", 1, 1, 1, 1, 8.0, unlocked = true),
            Element("Helium", "Helium", 2, 2, 2, 2, 40.0, unlocked = true),
            Element("Carbon", "Carbon", 6, 6, 6, 6, 220.0),
            Element("Oxygen", "Oxygen", 8, 8, 8, 8, 480.0),
            Element("Silicon", "Silicon", 14, 14, 14, 14, 1800.0),
            Element("Iron", "Iron", 26, 26, 30, 26, 12000.0),
            Element("Nickel", "Nickel", 28, 28, 30, 28, 20000.0),
            Element("Silver", "Silver", 47, 47, 60, 47, 55000.0),
            Element("Xenon", "Xenon", 54, 54, 78, 54, 110000.0),
            Element("Gold", "Gold", 79, 79, 118, 79, 250000.0),
        )

        fun createDefaultUpgrades(): MutableList<UpgradeDef> {
            fun atom(id: String, n: Double) = ResourceCost(ResourceKind.Atom, id, n)
            fun nuc(id: String, n: Double) = ResourceCost(ResourceKind.Nucleus, id, n)
            fun iso(id: String, n: Double) = ResourceCost(ResourceKind.Isotope, id, n)
            fun e(n: Double) = ResourceCost(ResourceKind.Energy, "", n)
            fun p(n: Double) = ResourceCost(ResourceKind.Proton, "", n)
            fun neut(n: Double) = ResourceCost(ResourceKind.Neutron, "", n)
            fun elec(n: Double) = ResourceCost(ResourceKind.Electron, "", n)

            fun coil(id: String, name: String, bonus: Double, energyCost: Double) = UpgradeDef(
                "annihilation_$id", "$name Annihilation Coil",
                "+${"%.2f".format(bonus)} isotope EPS multiplier",
                listOf(iso(id, 3.0), e(energyCost)), 1.6, UpgradeEffect.IsotopeEpsMult, bonus,
            )

            fun nucUp(id: String, title: String, desc: String, per: Double) = UpgradeDef(
                "nuc_$id", title, desc, listOf(nuc(id, 5.0)), 1.5, UpgradeEffect.ClickPower, per,
            )

            fun atomUp(id: String, title: String, desc: String, per: Double) = UpgradeDef(
                "atom_$id", title, desc, listOf(atom(id, 5.0)), 1.5, UpgradeEffect.AutoEps, per,
            )

            return mutableListOf(
                UpgradeDef("click_e", "Electron Lens", "+0.2 EPS", listOf(elec(5.0)), 1.5, UpgradeEffect.AutoEps, 0.2),
                UpgradeDef("click_p", "Proton Injector", "+1 eV click power", listOf(p(5.0)), 1.5, UpgradeEffect.ClickPower, 1.0),
                UpgradeDef("click_n", "Neutron Channel", "+1% crit chance (max 100%)", listOf(neut(5.0)), 1.5, UpgradeEffect.CritChance, 0.01),
                nucUp("Hydrogen", "Hydrogen Core", "+2 eV click power", 2.0),
                atomUp("Hydrogen", "Hydrogen Farm", "+2.5 EPS", 2.5),
                coil("Hydrogen", "H", 0.10, 2000.0),
                nucUp("Helium", "Helium Core", "+10 eV click power", 10.0),
                atomUp("Helium", "Helium Turbine", "+5 EPS", 5.0),
                coil("Helium", "He", 0.20, 5000.0),
                nucUp("Carbon", "Carbon Core", "+30 eV click power", 30.0),
                atomUp("Carbon", "Carbon Lattice", "+15 EPS", 15.0),
                coil("Carbon", "C", 0.30, 12000.0),
                nucUp("Oxygen", "Oxygen Core", "+50 eV click power", 50.0),
                atomUp("Oxygen", "Oxygen Reactor", "+30 EPS", 30.0),
                coil("Oxygen", "O", 0.40, 20000.0),
                nucUp("Silicon", "Silicon Core", "+90 eV click power", 90.0),
                atomUp("Silicon", "Silicon Array", "+60 EPS", 60.0),
                coil("Silicon", "Si", 0.50, 40000.0),
                nucUp("Iron", "Iron Core", "+180 eV click power", 180.0),
                atomUp("Iron", "Iron Forge", "+120 EPS", 120.0),
                coil("Iron", "Fe", 0.60, 80000.0),
                nucUp("Nickel", "Nickel Core", "+220 eV click power", 220.0),
                atomUp("Nickel", "Nickel Stack", "+150 EPS", 150.0),
                coil("Nickel", "Ni", 0.70, 100000.0),
                nucUp("Silver", "Silver Core", "+280 eV click power", 280.0),
                atomUp("Silver", "Silver Circuit", "+200 EPS", 200.0),
                coil("Silver", "Ag", 0.80, 140000.0),
                nucUp("Xenon", "Xenon Core", "+340 eV click power", 340.0),
                atomUp("Xenon", "Xenon Chamber", "+250 EPS", 250.0),
                coil("Xenon", "Xe", 0.90, 170000.0),
                nucUp("Gold", "Gold Core", "+400 eV click power", 400.0),
                atomUp("Gold", "Gold Dynamo", "+300 EPS", 300.0),
                coil("Gold", "Au", 1.00, 200000.0),
                UpgradeDef(
                    "crit_amplifier", "Critical Cascade", "+1 crit multiplier",
                    listOf(atom("Hydrogen", 10.0), atom("Helium", 10.0), atom("Carbon", 10.0),
                        atom("Oxygen", 10.0), atom("Silicon", 10.0), atom("Iron", 10.0)),
                    1.9, UpgradeEffect.CritMultiplier, 1.0,
                ),
                UpgradeDef(
                    "quantum_cpu", "Quantum Processor", "+10 auto-click multiplier",
                    listOf(elec(10000.0), nuc("Silicon", 500.0), atom("Gold", 50.0)),
                    2.2, UpgradeEffect.AutoClickMult, 10.0,
                ),
            )
        }
    }
}
