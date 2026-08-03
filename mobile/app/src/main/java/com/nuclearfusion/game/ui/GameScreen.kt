package com.nuclearfusion.game.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.key
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.nuclearfusion.game.ResourceKind
import com.nuclearfusion.game.SUPERNOVA_ATOM_GOAL
import com.nuclearfusion.game.StarType
import com.nuclearfusion.game.elementSymbol
import com.nuclearfusion.game.formatCompact
import com.nuclearfusion.game.starDustReward
import com.nuclearfusion.game.starMaxAtomicNumber
import com.nuclearfusion.game.upgradeGrade
import com.nuclearfusion.game.upgradeScaledEffect
import com.nuclearfusion.game.upgradeUsesGrade

private val Phosphor = Color(0xFF59E672)
private val PhosphorDim = Color(0xFF2E8A45)
private val Bg = Color(0xFF020A05)
private val Panel = Color(0xFF06140B)
private val Warn = Color(0xFFFF8C59)
private val StarCore = Color(0xFFFFE878)

@Composable
fun NuclearTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = darkColorScheme(
            primary = Phosphor,
            onPrimary = Color.Black,
            background = Bg,
            surface = Panel,
            onBackground = Phosphor,
            onSurface = Phosphor,
            secondary = PhosphorDim,
            error = Warn,
        ),
        content = content,
    )
}

@Composable
fun GameScreen(vm: GameViewModel) {
    key(vm.revision) {
        Scaffold(
            containerColor = Bg,
            bottomBar = {
                NavigationBar(containerColor = Panel) {
                    GameTab.entries.forEach { t ->
                        NavigationBarItem(
                            selected = vm.tab == t,
                            onClick = { vm.selectTab(t) },
                            colors = NavigationBarItemDefaults.colors(
                                selectedIconColor = Phosphor,
                                selectedTextColor = Phosphor,
                                indicatorColor = PhosphorDim.copy(alpha = 0.35f),
                                unselectedIconColor = PhosphorDim,
                                unselectedTextColor = PhosphorDim,
                            ),
                            icon = { Text(t.name.take(1), color = if (vm.tab == t) Phosphor else PhosphorDim) },
                            label = { Text(t.name) },
                        )
                    }
                }
            },
        ) { pad ->
            Box(Modifier.fillMaxSize().padding(pad)) {
                when (vm.tab) {
                    GameTab.Play -> PlayTab(vm)
                    GameTab.Lab -> LabTab(vm)
                    GameTab.Tech -> TechTab(vm)
                    GameTab.More -> MoreTab(vm)
                }
            }
        }
    }

    if (vm.showResetDialog) {
        AlertDialog(
            onDismissRequest = { vm.dismissReset() },
            title = { Text("Reset progress?", color = Warn) },
            text = {
                Text(
                    "This will erase your save and start a new game. Dust, star, Tech and Lab progress will be lost.",
                    color = Phosphor,
                )
            },
            confirmButton = {
                TextButton(onClick = { vm.confirmReset() }) {
                    Text("Reset", color = Warn)
                }
            },
            dismissButton = {
                TextButton(onClick = { vm.dismissReset() }) {
                    Text("Cancel", color = Phosphor)
                }
            },
            containerColor = Panel,
        )
    }
}

@Composable
private fun PlayTab(vm: GameViewModel) {
    val g = vm.state
    Column(
        Modifier
            .fillMaxSize()
            .background(
                Brush.verticalGradient(listOf(Bg, Color(0xFF041A0C), Bg)),
            )
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Text("Nuclear Fusion", color = Phosphor, fontSize = 26.sp, fontWeight = FontWeight.Bold)
        Spacer(Modifier.height(8.dp))
        HudBlock(vm)
        Spacer(Modifier.height(20.dp))

        Box(contentAlignment = Alignment.Center, modifier = Modifier.weight(1f)) {
            Box(
                modifier = Modifier
                    .size(200.dp)
                    .clip(CircleShape)
                    .background(
                        Brush.radialGradient(listOf(StarCore, Color(0xFFFFAA33), Color(0xFF225522))),
                    )
                    .border(3.dp, Phosphor, CircleShape)
                    .clickable { vm.clickStar() },
                contentAlignment = Alignment.Center,
            ) {
                Text("TAP", color = Color(0xFF1A1000), fontWeight = FontWeight.Bold, fontSize = 22.sp)
            }
            Column(Modifier.align(Alignment.TopCenter)) {
                vm.floatTexts.takeLast(4).forEach { ft ->
                    Text(
                        ft.text,
                        color = if (ft.crit) Warn else Phosphor,
                        fontWeight = FontWeight.Bold,
                        fontSize = 18.sp,
                    )
                }
            }
        }

        Text("Star: ${g.starType.displayName()}", color = PhosphorDim)
        Spacer(Modifier.height(8.dp))
        AscendButton(vm)
        Spacer(Modifier.height(12.dp))
        ParticleRow(vm)
    }
}

@Composable
private fun HudBlock(vm: GameViewModel) {
    val g = vm.state
    Column(
        Modifier
            .fillMaxWidth()
            .border(1.dp, PhosphorDim, RoundedCornerShape(8.dp))
            .background(Panel)
            .padding(12.dp),
    ) {
        Text("Energy(eV): ${formatCompact(g.energy)}", color = Phosphor, fontSize = 18.sp)
        Text(
            "EPS: ${formatCompact(g.eps())}/s   Click: ${formatCompact(g.clickPower)}",
            color = PhosphorDim,
            fontSize = 14.sp,
        )
        Text(
            "Crit: ${formatCompact(g.critChance * 100)}%  x${"%.1f".format(g.critMultiplier)}   Dust: ${formatCompact(g.starDust)}",
            color = PhosphorDim,
            fontSize = 14.sp,
        )
        Text(
            "p ${formatCompact(g.protons)}   n ${formatCompact(g.neutrons)}   e ${formatCompact(g.electrons)}",
            color = Phosphor,
            fontSize = 14.sp,
        )
    }
}

@Composable
private fun AscendButton(vm: GameViewModel) {
    val g = vm.state
    val prestige = g.starType == StarType.NeutronStar
    val ready = if (prestige) g.canTriggerPrestige() else g.canTriggerSupernova()
    val label = if (prestige) {
        "Prestige (+${formatCompact(g.prestigeDustReward())} Dust)"
    } else {
        val maxZ = starMaxAtomicNumber(g.starType)
        val cap = g.elements.find { it.atomicNumber == maxZ }
        val have = cap?.atomCount ?: 0.0
        "Supernova ${formatCompact(have)}/${formatCompact(SUPERNOVA_ATOM_GOAL)} (+${formatCompact(starDustReward(g.starType))} Dust)"
    }
    Button(
        onClick = { vm.tryAscend() },
        enabled = ready,
        colors = ButtonDefaults.buttonColors(
            containerColor = PhosphorDim,
            contentColor = Color.Black,
            disabledContainerColor = Color(0xFF1A2A1A),
            disabledContentColor = Color(0xFF556655),
        ),
        modifier = Modifier.fillMaxWidth(),
    ) {
        Text(label, textAlign = TextAlign.Center)
    }
}

@Composable
private fun ParticleRow(vm: GameViewModel) {
    val g = vm.state
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        TextButton(onClick = { vm.cycleBuyBatch() }, modifier = Modifier.weight(0.7f)) {
            Text("Buy ${g.buyBatch.label()}", color = Phosphor, fontSize = 13.sp)
        }
        listOf(
            ResourceKind.Proton to "p",
            ResourceKind.Neutron to "n",
            ResourceKind.Electron to "e",
        ).forEach { (kind, tag) ->
            val amt = g.resolveParticleBuyAmount(kind)
            Button(
                onClick = { vm.buyParticle(kind) },
                enabled = amt > 0.0,
                colors = ButtonDefaults.buttonColors(containerColor = PhosphorDim, contentColor = Color.Black),
                contentPadding = PaddingValues(6.dp),
                modifier = Modifier.weight(1f),
            ) {
                Text("$tag\n${formatCompact(if (amt > 0) amt else 1.0)}", fontSize = 11.sp, textAlign = TextAlign.Center)
            }
        }
    }
}

@Composable
private fun LabTab(vm: GameViewModel) {
    val g = vm.state
    Column(Modifier.fillMaxSize().padding(12.dp)) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("Lab", color = Phosphor, fontSize = 22.sp, fontWeight = FontWeight.Bold, modifier = Modifier.weight(1f))
            TextButton(onClick = { vm.cycleCraftBatch() }) {
                Text("Batch ${g.craftBatch.label()}", color = Phosphor)
            }
        }
        LazyColumn(verticalArrangement = Arrangement.spacedBy(10.dp)) {
            itemsIndexed(g.elements) { index, el ->
                if (!el.unlocked) return@itemsIndexed
                Column(
                    Modifier
                        .fillMaxWidth()
                        .border(1.dp, PhosphorDim, RoundedCornerShape(8.dp))
                        .background(Panel)
                        .padding(10.dp),
                ) {
                    Text("${el.name} (${elementSymbol(el.id)})", color = Phosphor, fontWeight = FontWeight.Bold)
                    Text(
                        "nuc ${formatCompact(el.nucleusCount)}  atom ${formatCompact(el.atomCount)}  iso ${formatCompact(el.isotopeCount)}",
                        color = PhosphorDim,
                        fontSize = 13.sp,
                    )
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        Button(
                            onClick = { vm.craftNucleus(index) },
                            enabled = g.resolveCraftBatchCount(g.maxNucleusCraft(el)) > 0,
                            colors = ButtonDefaults.buttonColors(containerColor = PhosphorDim, contentColor = Color.Black),
                            modifier = Modifier.weight(1f),
                        ) { Text("Nucleus", fontSize = 13.sp) }
                        Button(
                            onClick = { vm.craftAtom(index) },
                            enabled = g.resolveCraftBatchCount(g.maxAtomCraft(el)) > 0,
                            colors = ButtonDefaults.buttonColors(containerColor = PhosphorDim, contentColor = Color.Black),
                            modifier = Modifier.weight(1f),
                        ) { Text("Atom", fontSize = 13.sp) }
                    }
                }
            }
        }
    }
}

@Composable
private fun TechTab(vm: GameViewModel) {
    val g = vm.state
    Column(Modifier.fillMaxSize().padding(12.dp)) {
        Text("Tech", color = Phosphor, fontSize = 22.sp, fontWeight = FontWeight.Bold)
        Spacer(Modifier.height(8.dp))
        LazyColumn(verticalArrangement = Arrangement.spacedBy(10.dp)) {
            items(g.upgrades.filter { g.isUpgradeVisible(it) }, key = { it.id }) { up ->
                Column(
                    Modifier
                        .fillMaxWidth()
                        .border(1.dp, PhosphorDim, RoundedCornerShape(8.dp))
                        .background(Panel)
                        .padding(10.dp),
                ) {
                    val grade = if (upgradeUsesGrade(up.effect)) upgradeGrade(up.level) else 0
                    Text(
                        "${up.name}  [${up.level}]" + if (grade > 0) "  grade x$grade" else "",
                        color = Phosphor,
                        fontWeight = FontWeight.Bold,
                    )
                    Text(up.description, color = PhosphorDim, fontSize = 13.sp)
                    Text("Total: ${formatCompact(upgradeScaledEffect(up))}", color = PhosphorDim, fontSize = 12.sp)
                    up.baseCosts.forEach { c ->
                        val need = g.scaledCostAmount(up, c)
                        val have = g.resourceAmount(c)
                        val tag = when (c.kind) {
                            ResourceKind.Energy -> "E"
                            ResourceKind.Proton -> "p"
                            ResourceKind.Neutron -> "n"
                            ResourceKind.Electron -> "e"
                            ResourceKind.Nucleus -> "nuc(${elementSymbol(c.elementId)})"
                            ResourceKind.Atom -> "atom(${elementSymbol(c.elementId)})"
                            ResourceKind.Isotope -> "iso(${elementSymbol(c.elementId)})"
                            ResourceKind.StarDust -> "dust"
                        }
                        Text(
                            "${formatCompact(have)}/${formatCompact(need)} $tag",
                            color = if (have + 1e-9 >= need) Phosphor else Warn,
                            fontSize = 12.sp,
                        )
                    }
                    Button(
                        onClick = { vm.buyUpgrade(up) },
                        enabled = g.canAffordUpgrade(up),
                        colors = ButtonDefaults.buttonColors(containerColor = PhosphorDim, contentColor = Color.Black),
                    ) { Text("Buy") }
                }
            }
        }
    }
}

@Composable
private fun MoreTab(vm: GameViewModel) {
    Column(Modifier.fillMaxSize().padding(16.dp)) {
        Text("More", color = Phosphor, fontSize = 22.sp, fontWeight = FontWeight.Bold)
        Spacer(Modifier.height(16.dp))
        Text("HUD refresh", color = Phosphor)
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            listOf(0.25f, 0.5f, 1.0f).forEach { sec ->
                val selected = kotlin.math.abs(vm.hudIntervalSec - sec) < 0.01f
                Button(
                    onClick = { vm.setHudInterval(sec) },
                    colors = ButtonDefaults.buttonColors(
                        containerColor = if (selected) Phosphor else PhosphorDim,
                        contentColor = Color.Black,
                    ),
                ) { Text("${sec}s") }
            }
        }
        Spacer(Modifier.height(24.dp))
        Button(
            onClick = { vm.openReset() },
            colors = ButtonDefaults.buttonColors(containerColor = Warn, contentColor = Color.Black),
            modifier = Modifier.fillMaxWidth(),
        ) { Text("Reset progress") }
        Spacer(Modifier.height(12.dp))
        Text(
            "Mobile port of Nuclear Fusion. Economy matches the desktop build; star rendering is simplified for touch.",
            color = PhosphorDim,
            fontSize = 13.sp,
        )
    }
}
