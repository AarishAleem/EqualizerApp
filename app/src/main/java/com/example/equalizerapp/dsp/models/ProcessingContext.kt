package com.example.equalizerapp.dsp.models

/**
 * Encapsulates the global state of the audio engine.
 * Passed to every processor during the prepare() and process() stages.
 */
data class ProcessingContext(
    val sampleRate: Int = 48000,
    val channels: Int = 2,
    val blockSize: Int = 512,
    var isEnabled: Boolean = true,
    var masterGain: Float = 0f,
    var masterVolume: Float = 100f,
    var surroundStrength: Float = 0f,
    var crossfeed: Float = 0f,
    var channelDelay: Float = 0f
)
