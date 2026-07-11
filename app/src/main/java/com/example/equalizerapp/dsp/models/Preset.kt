package com.example.equalizerapp.dsp.models

/**
 * Pure data model for an Equalizer Preset.
 */
data class Preset(
    val name: String,
    val bands: List<BandConfig>,
    val masterGain: Float
)
