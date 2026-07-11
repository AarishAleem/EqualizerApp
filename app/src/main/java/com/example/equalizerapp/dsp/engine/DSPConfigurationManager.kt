package com.example.equalizerapp.dsp.engine

import com.example.equalizerapp.dsp.models.BandConfig
import com.example.equalizerapp.dsp.models.ProcessingContext
import kotlin.math.pow

/**
 * The primary controller for the modular DSP system.
 * This class only handles the CONFIGURATION and JNI Bridge.
 * No processing or hardcoded DSP math happens here.
 */
class DSPConfigurationManager private constructor() {
    private val context = ProcessingContext()

    init {
        // Load the native library
        System.loadLibrary("equalizerapp")
    }

    companion object {
        val instance: DSPConfigurationManager by lazy { DSPConfigurationManager() }
    }

    // --- Lifecycle ---

    fun prepare(sampleRate: Int, channels: Int) {
        // For Oboe initialization if needed from Kotlin
    }

    // --- Parameters ---

    fun setEnabled(enabled: Boolean) {
        context.isEnabled = enabled
    }

    fun isEnabled(): Boolean = context.isEnabled

    fun setMasterGain(gainDb: Float) {
        context.masterGain = gainDb
    }

    fun getMasterGain(): Float = context.masterGain

    fun setMasterVolume(volume: Float) {
        context.masterVolume = volume
    }

    fun setSurround(strength: Float, crossfeed: Float) {
        context.surroundStrength = strength
        context.crossfeed = crossfeed
    }

    fun setChannelDelay(delay: Float) {
        context.channelDelay = delay
    }

    fun getContext(): ProcessingContext = context

    // --- Frequency Response logic moved to Engine/Analyzer ---

    /**
     * Calculates the frequency response curve for the UI graph.
     */
    fun getFrequencyResponse(bandConfigs: List<BandConfig>): FloatArray {
        val freqPoints = 120
        val frequencies = FloatArray(freqPoints)
        val minF = 20f
        val maxF = 22000f
        
        for (i in 0 until freqPoints) {
            frequencies[i] = minF * (maxF / minF).toDouble().pow(i.toDouble() / (freqPoints - 1)).toFloat()
        }

        val bFreqs = bandConfigs.map { it.frequency }.toFloatArray()
        val bGains = bandConfigs.map { it.gain }.toFloatArray()
        val bQs = bandConfigs.map { it.q }.toFloatArray()
        val bTypes = bandConfigs.map { it.typeIndex }.toIntArray()

        return calculateFrequencyResponseNative(
            frequencies, bFreqs, bGains, bQs, bTypes, context.masterGain
        )
    }

    // --- Native Bridge ---

    private external fun calculateFrequencyResponseNative(
        frequencies: FloatArray,
        bandFreqs: FloatArray,
        bandGains: FloatArray,
        bandQs: FloatArray,
        bandTypes: IntArray,
        masterGain: Float
    ): FloatArray
}
