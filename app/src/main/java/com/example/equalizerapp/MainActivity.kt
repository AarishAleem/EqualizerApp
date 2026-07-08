package com.example.equalizerapp

import android.Manifest
import android.content.*
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.IBinder
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.*
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.core.content.ContextCompat
import com.example.equalizerapp.ui.screens.EffectsScreen
import com.example.equalizerapp.ui.screens.EqualizerScreen
import com.example.equalizerapp.ui.screens.SurroundScreen
import com.example.equalizerapp.ui.theme.PowerEQTheme
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlin.math.pow

@OptIn(ExperimentalMaterial3Api::class)
class MainActivity : ComponentActivity() {

    private var equalizerService: EqualizerService? = null
    private var isBound by mutableStateOf(false)
    private val bandConfigs = mutableStateListOf<BandConfig>()
    private var isEngineEnabled by mutableStateOf(true)
    private var graphData by mutableStateOf(floatArrayOf())
    private var realTimeFft by mutableStateOf(floatArrayOf())
    private var masterGain by mutableFloatStateOf(0f)
    
    private var masterVolume by mutableFloatStateOf(100f)
    private var surroundStrength by mutableFloatStateOf(0f)
    private var crossfeed by mutableFloatStateOf(0f)
    private var channelDelay by mutableFloatStateOf(0f)

    private val presets = mutableStateListOf<Preset>()
    private var currentPresetName by mutableStateOf("Custom")

    private var updateJob: Job? = null

    sealed class Screen(val route: String, val icon: androidx.compose.ui.graphics.vector.ImageVector, val label: String) {
        object Equalizer : Screen("equalizer", Icons.Default.Tune, "EQ")
        object Effects : Screen("effects", Icons.Default.Settings, "FX")
        object Surround : Screen("surround", Icons.Default.SurroundSound, "3D")
    }

    private var currentScreen by mutableStateOf<Screen>(Screen.Equalizer)

    companion object {
        init {
            System.loadLibrary("equalizerapp")
        }
    }

    private external fun calculateFrequencyResponse(
        frequencies: FloatArray,
        bandFreqs: FloatArray,
        bandGains: FloatArray,
        bandQs: FloatArray,
        bandTypes: IntArray,
        masterGain: Float
    ): FloatArray

    data class BandConfig(
        var frequency: Float,
        var gain: Float,
        var q: Float,
        var typeIndex: Int = 0 // 0: Peak, 1: LP, 2: HP, 3: BP, 4: LS, 5: HS
    )

    data class Preset(
        val name: String,
        val bands: List<BandConfig>,
        val masterGain: Float
    )

    private val connection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            val binder = service as EqualizerService.LocalBinder
            equalizerService = binder.getService()
            isBound = true
            equalizerService?.updateConfigs(bandConfigs, masterGain)
            equalizerService?.updateEffects(masterVolume, surroundStrength, channelDelay)
        }
        override fun onServiceDisconnected(name: ComponentName?) {
            isBound = false
            equalizerService = null
        }
    }

    private val requestPermissionLauncher = registerForActivityResult(ActivityResultContracts.RequestPermission()) { }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            requestPermissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
        }

        loadState()
        // If first run, initialize presets
        if (presets.isEmpty()) {
            presets.addAll(getFactoryPresets())
            applyPreset(presets[0]) // Apply Flat by default
        }
        
        if (bandConfigs.isEmpty()) initDefaultBands()

        updateGraphImmediate()

        val serviceIntent = Intent(this, EqualizerService::class.java)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) startForegroundService(serviceIntent) else startService(serviceIntent)
        bindService(serviceIntent, connection, BIND_AUTO_CREATE)

        setContent {
            PowerEQTheme {
                Scaffold(
                    topBar = {
                        CenterAlignedTopAppBar(
                            title = { Text("PowerEQ Master", style = MaterialTheme.typography.titleMedium, color = Color.White) },
                            navigationIcon = {
                                IconButton(onClick = { 
                                    isEngineEnabled = !isEngineEnabled
                                    notifyService()
                                    saveState()
                                }) {
                                    Icon(
                                        imageVector = Icons.Default.PowerSettingsNew,
                                        contentDescription = "Toggle Power",
                                        tint = if (isEngineEnabled) Color.Cyan else Color.Gray
                                    )
                                }
                            },
                            colors = TopAppBarDefaults.centerAlignedTopAppBarColors(
                                containerColor = Color.Black
                            )
                        )
                    },
                    bottomBar = {
                        NavigationBar(containerColor = MaterialTheme.colorScheme.secondary) {
                            val items = listOf(Screen.Equalizer, Screen.Surround, Screen.Effects)
                            items.forEach { screen ->
                                NavigationBarItem(
                                    icon = { Icon(screen.icon, contentDescription = screen.label) },
                                    label = { Text(screen.label) },
                                    selected = currentScreen == screen,
                                    onClick = { currentScreen = screen }
                                )
                            }
                        }
                    }
                ) { innerPadding ->
                    Surface(
                        modifier = Modifier.fillMaxSize().padding(innerPadding),
                        color = MaterialTheme.colorScheme.background
                    ) {
                        LaunchedEffect(isBound) {
                            while (isBound) {
                                realTimeFft = equalizerService?.getRealTimeFft() ?: floatArrayOf()
                                delay(50)
                            }
                        }

                        val scope = rememberCoroutineScope()

                        Crossfade(targetState = currentScreen, label = "screenTransition") { screen ->
                            when (screen) {
                                is Screen.Equalizer -> {
                                    EqualizerScreen(
                                        bandConfigs = bandConfigs,
                                        graphData = graphData,
                                        realTimeFft = realTimeFft,
                                        masterGain = masterGain,
                                        presets = presets,
                                        currentPresetName = currentPresetName,
                                        onMasterGainChanged = { masterGain = it; saveAndNotifyDebounced(scope) },
                                        onAddBand = { if (bandConfigs.size < 12) { bandConfigs.add(BandConfig(1000f, 0f, 0.7f, 0)); saveAndNotifyImmediate() } },
                                        onRemoveBand = { index -> if (bandConfigs.size > 1) { bandConfigs.removeAt(index); saveAndNotifyImmediate() } },
                                        onConfigChanged = { saveAndNotifyDebounced(scope) },
                                        onPresetSelected = { applyPreset(it) },
                                        onSavePreset = { saveNewPreset(it) },
                                        onResetPreset = { resetCurrentPreset() }
                                    )
                                }
                                is Screen.Effects -> {
                                    EffectsScreen(
                                        masterVolume = masterVolume,
                                        onVolumeChange = { masterVolume = it; saveAndNotifyEffectsDebounced(scope) },
                                        surroundStrength = surroundStrength,
                                        onSurroundChange = { surroundStrength = it; saveAndNotifyEffectsDebounced(scope) },
                                        channelDelay = channelDelay,
                                        onDelayChange = { channelDelay = it; saveAndNotifyEffectsDebounced(scope) }
                                    )
                                }
                                is Screen.Surround -> {
                                    SurroundScreen(
                                        width = surroundStrength,
                                        onWidthChange = { surroundStrength = it; saveAndNotifyEffectsDebounced(scope) },
                                        crossfeed = crossfeed,
                                        onCrossfeedChange = { crossfeed = it; saveAndNotifyEffectsDebounced(scope) }
                                    )
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    private fun initDefaultBands() {
        bandConfigs.addAll(listOf(
            BandConfig(60f, 0f, 0.7f, 0),
            BandConfig(230f, 0f, 0.7f, 0),
            BandConfig(910f, 0f, 0.7f, 0),
            BandConfig(3600f, 0f, 0.7f, 0),
            BandConfig(14000f, 0f, 0.7f, 0)
        ))
    }

    private fun initPredefinedPresets() {
        presets.addAll(getFactoryPresets())
    }

    private fun getFactoryPresets(): List<Preset> {
        return listOf(
            Preset("Flat", listOf(BandConfig(60f, 0f, 0.7f, 0), BandConfig(230f, 0f, 0.7f, 0), BandConfig(910f, 0f, 0.7f, 0), BandConfig(3600f, 0f, 0.7f, 0), BandConfig(14000f, 0f, 0.7f, 0)), 0f),
            Preset("Bass Boost", listOf(BandConfig(60f, 7f, 0.6f, 0), BandConfig(150f, 4f, 0.7f, 0), BandConfig(400f, 1f, 0.7f, 0)), 0f),
            Preset("Treble Boost", listOf(BandConfig(5000f, 3f, 0.7f, 0), BandConfig(10000f, 6f, 0.7f, 0), BandConfig(16000f, 8f, 0.5f, 0)), 0f),
            Preset("Harman Target", listOf(
                BandConfig(20f, 6f, 0.4f, 0),
                BandConfig(105f, 3f, 0.7f, 0),
                BandConfig(3000f, 2f, 1.0f, 0),
                BandConfig(10000f, -2f, 0.7f, 0),
                BandConfig(20000f, -5f, 0.5f, 0)
            ), -3f)
        )
    }

    private fun resetCurrentPreset() {
        val factory = getFactoryPresets().find { it.name == currentPresetName }
        if (factory != null) {
            applyPreset(factory)
            saveAndNotifyImmediate()
        } else {
            // For custom presets, reset to a flat 5-band state
            bandConfigs.clear()
            initDefaultBands()
            masterGain = 0f
            saveAndNotifyImmediate()
        }
    }

    private fun applyPreset(preset: Preset) {
        bandConfigs.clear()
        bandConfigs.addAll(preset.bands.map { it.copy() })
        masterGain = preset.masterGain
        currentPresetName = preset.name
        updateGraphImmediate()
        notifyService()
        saveState()
    }

    private fun saveNewPreset(name: String) {
        val newPreset = Preset(name, bandConfigs.toList().map { it.copy() }, masterGain)
        presets.removeAll { it.name == name }
        presets.add(newPreset)
        currentPresetName = name
        saveState()
    }

    private fun saveAndNotifyImmediate() {
        updateActivePresetInMemory()
        updateGraphImmediate()
        notifyService()
        saveState()
    }

    private fun saveAndNotifyDebounced(scope: kotlinx.coroutines.CoroutineScope) {
        updateGraphImmediate()
        updateJob?.cancel()
        updateJob = scope.launch {
            delay(10)
            updateActivePresetInMemory()
            notifyService()
            saveState()
        }
    }

    private fun updateActivePresetInMemory() {
        val activeIdx = presets.indexOfFirst { it.name == currentPresetName }
        if (activeIdx != -1) {
            presets[activeIdx] = Preset(currentPresetName, bandConfigs.toList().map { it.copy() }, masterGain)
        }
    }

    private fun saveAndNotifyEffectsDebounced(scope: kotlinx.coroutines.CoroutineScope) {
        updateJob?.cancel()
        updateJob = scope.launch {
            delay(10)
            if (isBound) equalizerService?.updateEffects(masterVolume, surroundStrength, channelDelay, crossfeed)
            saveState()
        }
    }

    private fun saveState() {
        val sp = getSharedPreferences("eq_prefs", MODE_PRIVATE)
        val gson = Gson()
        sp.edit().apply {
            putBoolean("engine_enabled", isEngineEnabled)
            putString("bands", gson.toJson(bandConfigs.toList()))
            putFloat("master_gain", masterGain)
            putFloat("master_volume", masterVolume)
            putFloat("surround_strength", surroundStrength)
            putFloat("crossfeed", crossfeed)
            putFloat("channel_delay", channelDelay)
            putString("presets", gson.toJson(presets.toList()))
            putString("current_preset", currentPresetName)
            apply()
        }
    }

    private fun loadState() {
        val sp = getSharedPreferences("eq_prefs", MODE_PRIVATE)
        val gson = Gson()
        isEngineEnabled = sp.getBoolean("engine_enabled", true)
        val bandsJson = sp.getString("bands", null)
        if (bandsJson != null) {
            val type = object : TypeToken<List<BandConfig>>() {}.type
            bandConfigs.clear()
            bandConfigs.addAll(gson.fromJson<List<BandConfig>>(bandsJson, type))
        }
        masterGain = sp.getFloat("master_gain", 0f)
        masterVolume = sp.getFloat("master_volume", 100f)
        surroundStrength = sp.getFloat("surround_strength", 0f)
        crossfeed = sp.getFloat("crossfeed", 0f)
        channelDelay = sp.getFloat("channel_delay", 0f)
        currentPresetName = sp.getString("current_preset", "Custom") ?: "Custom"
        
        val presetsJson = sp.getString("presets", null)
        if (presetsJson != null) {
            val type = object : TypeToken<List<Preset>>() {}.type
            presets.clear()
            presets.addAll(gson.fromJson<List<Preset>>(presetsJson, type))
        }
    }

    private fun updateGraphImmediate() {
        val freqPoints = 120
        val frequencies = FloatArray(freqPoints)
        val minF = 20f
        val maxF = 22000f
        for (i in 0 until freqPoints) frequencies[i] = minF * (maxF / minF).toDouble().pow(i.toDouble() / (freqPoints - 1)).toFloat()
        
        val bFreqs = bandConfigs.map { it.frequency }.toFloatArray()
        val bGains = bandConfigs.map { it.gain }.toFloatArray()
        val bQs = bandConfigs.map { it.q }.toFloatArray()
        val bTypes = bandConfigs.map { it.typeIndex }.toIntArray()

        graphData = calculateFrequencyResponse(frequencies, bFreqs, bGains, bQs, bTypes, masterGain)
    }

    private fun notifyService() {
        if (isBound) {
            equalizerService?.updateConfigs(bandConfigs, masterGain)
            equalizerService?.setEngineEnabled(isEngineEnabled)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        if (isBound) unbindService(connection)
    }
}
