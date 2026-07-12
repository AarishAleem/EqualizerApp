package com.example.equalizerapp.dsp.engine

import android.media.audiofx.DynamicsProcessing
import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi

/**
 * Production implementation of the audible processor using Android's DynamicsProcessing.
 */
@RequiresApi(Build.VERSION_CODES.P)
class DynamicsProcessingProcessor(
    val sessionId: Int,
    val generationId: Long,
    private val dp: DynamicsProcessing
) : AudibleProcessorAdapter {

    override fun release() {
        dp.release()
    }

    override fun setEnabled(enabled: Boolean) {
        dp.enabled = enabled
    }

    override fun applyState(state: SafeAudibleState) {
        try {
            dp.setInputGainAllChannelsTo(state.masterGain)

            state.bands.forEach { band ->
                val eqBand = DynamicsProcessing.EqBand(true, band.frequency, band.gain)
                dp.setPostEqBandAllChannelsTo(band.index, eqBand)
            }

            val l = state.limiter
            val limiter = DynamicsProcessing.Limiter(
                l.enabled, true, 0, l.attackMs, l.releaseMs, l.ratio, l.threshold, l.postGain
            )
            dp.setLimiterAllChannelsTo(limiter)
        } catch (e: Exception) {
            Log.e("PowerEQ-AudioSafety", "Failed to apply state to processor generation $generationId", e)
        }
    }
}
