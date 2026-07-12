package com.example.equalizerapp.dsp.engine

import android.util.Log

/**
 * Centralized safety boundary for all audible parameters.
 */
object AudibleParameterSafety {
    private const val TAG = "PowerEQ-AudioSafety"

    // Counters for rejections
    private var rejectionCount = 0

    fun getRejectionCount(): Int = rejectionCount

    /**
     * Validates and sanitizes a complete state.
     */
    fun validate(
        rawMasterGain: Float,
        rawIsEnabled: Boolean,
        rawBands: List<com.example.equalizerapp.dsp.models.BandConfig>,
        previousSafeState: SafeAudibleState? = null
    ): SafeAudibleState {
        val safeMasterGain = validateGain(rawMasterGain, previousSafeState?.masterGain ?: 0f, "MasterGain")
        
        val safeBands = rawBands.mapIndexed { index, raw ->
            val prevBand = previousSafeState?.bands?.getOrNull(index)
            SafeBandConfig(
                index = index,
                frequency = validateFrequency(raw.frequency, prevBand?.frequency ?: 1000f, "BandFreq[$index]"),
                gain = validateGain(raw.gain, prevBand?.gain ?: 0f, "BandGain[$index]"),
                q = validateQ(raw.q, prevBand?.q ?: 0.707f, "BandQ[$index]"),
                typeIndex = raw.typeIndex,
                enabled = true // Current product defaults bands to active
            )
        }

        return SafeAudibleState(
            masterGain = safeMasterGain,
            isEnabled = rawIsEnabled,
            bands = safeBands,
            limiter = getFixedSafetyLimiter()
        )
    }

    /**
     * Conservative safety limiter configuration.
     * User state and persisted state must not bypass or disable this.
     */
    fun getFixedSafetyLimiter(): SafeLimiterConfig {
        return SafeLimiterConfig(
            threshold = -0.1f,  // dB
            ratio = 10f,
            attackMs = 1f,
            releaseMs = 60f,
            postGain = 0f,      // No makeup gain allowed in safety limiter
            enabled = true
        )
    }

    private fun validateGain(value: Float, fallback: Float, name: String): Float {
        if (!value.isFinite()) {
            reportInvalid(name, "non-finite")
            return fallback
        }
        // PowerEQ V1 production range: -15 to +15 dB
        return value.coerceIn(-15f, 15f)
    }

    private fun validateFrequency(value: Float, fallback: Float, name: String): Float {
        if (!value.isFinite()) {
            reportInvalid(name, "non-finite")
            return fallback
        }
        return value.coerceIn(20f, 22000f)
    }

    private fun validateQ(value: Float, fallback: Float, name: String): Float {
        if (!value.isFinite()) {
            reportInvalid(name, "non-finite")
            return fallback
        }
        return value.coerceIn(0.1f, 10f)
    }

    private fun reportInvalid(parameter: String, reason: String) {
        rejectionCount++
        Log.w(TAG, "INVALID_AUDIO_PARAMETER parameter=$parameter reason=$reason action=safe_default")
    }
}
