package com.example.equalizerapp.dsp.models

/**
 * High-level wrapper for audio data to be used in the DSP chain.
 */
class AudioBuffer(val size: Int) {
    val data = FloatArray(size)

    fun clear() {
        data.fill(0f)
    }
}
