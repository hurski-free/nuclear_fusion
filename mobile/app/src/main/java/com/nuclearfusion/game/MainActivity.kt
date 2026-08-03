package com.nuclearfusion.game

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.runtime.DisposableEffect
import androidx.lifecycle.viewmodel.compose.viewModel
import com.nuclearfusion.game.ui.GameScreen
import com.nuclearfusion.game.ui.GameViewModel
import com.nuclearfusion.game.ui.GameViewModelFactory
import com.nuclearfusion.game.ui.NuclearTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val vm: GameViewModel = viewModel(factory = GameViewModelFactory(applicationContext))
            DisposableEffect(Unit) {
                onDispose { vm.persist() }
            }
            NuclearTheme {
                GameScreen(vm)
            }
        }
    }

    override fun onPause() {
        super.onPause()
        // Best-effort save when backgrounded; ViewModel may still be alive.
    }
}
