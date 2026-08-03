package com.nuclearfusion.game.ui

import android.content.Context
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.nuclearfusion.game.CraftBatch
import com.nuclearfusion.game.GameState
import com.nuclearfusion.game.ResourceKind
import com.nuclearfusion.game.SaveStore
import com.nuclearfusion.game.StarType
import com.nuclearfusion.game.UpgradeDef
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

enum class GameTab { Play, Lab, Tech, More }

data class FloatText(val id: Long, val text: String, val crit: Boolean)

class GameViewModel(private val appContext: Context) : ViewModel() {
    val state = GameState().also { SaveStore.load(appContext, it) }

    var tab by mutableStateOf(GameTab.Play)
        private set
    var revision by mutableIntStateOf(0)
        private set
    var floatTexts by mutableStateOf<List<FloatText>>(emptyList())
        private set
    var showResetDialog by mutableStateOf(false)
        private set
    var hudIntervalSec by mutableFloatStateOf(0.5f)
        private set

    private var floatId = 0L
    private var tickJob: Job? = null
    private var hudJob: Job? = null

    init {
        tickJob = viewModelScope.launch {
            var last = System.nanoTime()
            while (isActive) {
                delay(50)
                val now = System.nanoTime()
                val dt = ((now - last) / 1_000_000_000.0).toFloat().coerceIn(0f, 0.25f)
                last = now
                state.tick(dt)
            }
        }
        hudJob = viewModelScope.launch {
            while (isActive) {
                delay((hudIntervalSec * 1000).toLong().coerceAtLeast(100L))
                bump()
                SaveStore.save(appContext, state)
            }
        }
    }

    private fun bump() {
        revision += 1
    }

    fun selectTab(t: GameTab) {
        tab = t
    }

    fun setHudInterval(sec: Float) {
        hudIntervalSec = sec
    }

    fun clickStar() {
        val r = state.clickStar()
        val id = ++floatId
        val label = (if (r.crit) "CRIT " else "+") + String.format("%.1f", r.gain)
        floatTexts = floatTexts + FloatText(id, label, r.crit)
        bump()
        viewModelScope.launch {
            delay(800)
            floatTexts = floatTexts.filterNot { it.id == id }
        }
    }

    fun buyParticle(kind: ResourceKind) {
        if (state.buyParticle(kind)) bump()
    }

    fun cycleBuyBatch() {
        state.buyBatch = CraftBatch.entries[(state.buyBatch.ordinal + 1) % CraftBatch.entries.size]
        bump()
    }

    fun cycleCraftBatch() {
        state.craftBatch = CraftBatch.entries[(state.craftBatch.ordinal + 1) % CraftBatch.entries.size]
        bump()
    }

    fun craftNucleus(index: Int) {
        val el = state.elements.getOrNull(index) ?: return
        if (state.craftNucleus(el)) bump()
    }

    fun craftAtom(index: Int) {
        val el = state.elements.getOrNull(index) ?: return
        if (state.craftAtom(el)) bump()
    }

    fun buyUpgrade(up: UpgradeDef) {
        if (state.buyUpgrade(up)) bump()
    }

    fun tryAscend() {
        val ok = if (state.starType == StarType.NeutronStar) {
            state.triggerPrestige()
        } else {
            state.triggerSupernova()
        }
        if (ok) {
            bump()
            SaveStore.save(appContext, state)
        }
    }

    fun openReset() {
        showResetDialog = true
    }

    fun dismissReset() {
        showResetDialog = false
    }

    fun confirmReset() {
        SaveStore.clear(appContext)
        state.initNewGame()
        showResetDialog = false
        bump()
        SaveStore.save(appContext, state)
    }

    fun persist() {
        SaveStore.save(appContext, state)
    }

    override fun onCleared() {
        persist()
        super.onCleared()
    }
}

class GameViewModelFactory(private val context: Context) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        return GameViewModel(context.applicationContext) as T
    }
}
