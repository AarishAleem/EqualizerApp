package com.example.equalizerapp.dsp.models

/**
 * Pure data model for a single EQ band configuration.
 */
data class BandConfig(
    var frequency: Float,
    var gain: Float,
    var q: Float,
    var typeIndex: Int = 0 // 0: Peak, 1: LS, 2: HS, 3: LP, 4: HP, 5: BP, 6: Notch
)
