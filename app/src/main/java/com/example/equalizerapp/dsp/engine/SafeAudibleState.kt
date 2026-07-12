package com.example.equalizerapp.dsp.engine

/**
 * Immutable validated state for the audible bridge.
 */
data class SafeAudibleState(
    val masterGain: Float,
    val isEnabled: Boolean,
    val bands: List<SafeBandConfig>,
    val limiter: SafeLimiterConfig
)

data class SafeBandConfig(
    val index: Int,
    val frequency: Float,
    val gain: Float,
    val q: Float,
    val typeIndex: Int,
    val enabled: Boolean
)

data class SafeLimiterConfig(
    val threshold: Float,
    val ratio: Float,
    val attackMs: Float,
    val releaseMs: Float,
    val postGain: Float,
    val enabled: Boolean
)
