package com.example.equalizerapp.dsp.engine

import com.example.equalizerapp.dsp.models.BandConfig
import com.example.equalizerapp.dsp.models.ProcessingContext
import java.util.TreeSet
import kotlin.math.pow

/**
 * The primary controller for the modular DSP system.
 */
class DSPConfigurationManager private constructor() {
    private val context = ProcessingContext()

    init {
        System.loadLibrary("equalizerapp")
    }

    companion object {
        val instance: DSPConfigurationManager by lazy { DSPConfigurationManager() }
    }

    fun setEnabled(enabled: Boolean) { context.isEnabled = enabled }
    fun isEnabled(): Boolean = context.isEnabled
    fun setMasterGain(gainDb: Float) { context.masterGain = gainDb }
    fun getMasterGain(): Float = context.masterGain
    fun setMasterVolume(volume: Float) { context.masterVolume = volume }
    fun setSurround(strength: Float, crossfeed: Float) {
        context.surroundStrength = strength
        context.crossfeed = crossfeed
    }
    fun setChannelDelay(delay: Float) { context.channelDelay = delay }
    fun getContext(): ProcessingContext = context

    /**
     * Calculates frequency response with high-Q refinement.
     * Returns Pair(frequencies, magnitudes)
     */
    fun getFrequencyResponse(bandConfigs: List<BandConfig>): Pair<FloatArray, FloatArray> {
        val points = TreeSet<Float>()
        
        val minF = 20f
        val maxF = 22000f
        val baseCount = 100
        for (i in 0 until baseCount) {
            points.add(minF * (maxF / minF).toDouble().pow(i.toDouble() / (baseCount - 1)).toFloat())
        }

        for (band in bandConfigs) {
            val f0 = band.frequency
            points.add(f0 * 0.95f)
            points.add(f0 * 0.98f)
            points.add(f0 * 0.99f)
            points.add(f0)
            points.add(f0 * 1.01f)
            points.add(f0 * 1.02f)
            points.add(f0 * 1.05f)
        }

        val frequencies = points.filter { it in 20f..22000f }.toFloatArray()
        
        val bFreqs = bandConfigs.map { it.frequency }.toFloatArray()
        val bGains = bandConfigs.map { it.gain }.toFloatArray()
        val bQs = bandConfigs.map { it.q }.toFloatArray()
        val bTypes = bandConfigs.map { it.typeIndex }.toIntArray()

        val magnitudes = calculateFrequencyResponseNative(
            frequencies, bFreqs, bGains, bQs, bTypes, context.masterGain
        )
        
        return Pair(frequencies, magnitudes)
    }

    private external fun calculateFrequencyResponseNative(
        frequencies: FloatArray,
        bandFreqs: FloatArray,
        bandGains: FloatArray,
        bandQs: FloatArray,
        bandTypes: IntArray,
        masterGain: Float
    ): FloatArray
}
