package com.example.equalizerapp

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.media.audiofx.AudioEffect
import android.util.Log

class AudioSessionReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        val action = intent.action
        val sessionId = intent.getIntExtra(AudioEffect.EXTRA_AUDIO_SESSION, -1)
        val packageName = intent.getStringExtra(AudioEffect.EXTRA_PACKAGE_NAME)

        Log.d(TAG, "Action: $action, Session ID: $sessionId, Package: $packageName")

        if (sessionId != -1) {
            val serviceIntent = Intent(context, EqualizerService::class.java).apply {
                putExtra("session_id", sessionId)
                putExtra("package_name", packageName)
                this.action = when (action) {
                    AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION -> "OPEN_SESSION"
                    AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION -> "CLOSE_SESSION"
                    else -> null
                }
            }
            
            if (serviceIntent.action != null) {
                context.startService(serviceIntent)
            }
        }
    }

    companion object {
        private const val TAG = "AudioSessionReceiver"
    }
}
