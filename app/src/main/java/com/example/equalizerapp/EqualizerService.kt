package com.example.equalizerapp

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.media.audiofx.DynamicsProcessing
import android.media.audiofx.Virtualizer
import android.media.audiofx.Visualizer
import android.os.Binder
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import java.util.HashMap
import kotlin.math.max

class EqualizerService : Service() {
    private val binder = LocalBinder()
    private val activeEffects = HashMap<Int, DynamicsProcessing>()
    private val activeVirtualizers = HashMap<Int, Virtualizer>()
    
    private var currentConfigs: List<MainActivity.BandConfig> = ArrayList()
    private var masterGain: Float = 0f
    private var masterVolume: Float = 100f
    private var surroundStrength: Float = 0f
    private var channelDelay: Float = 0f
    
    private var visualizer: Visualizer? = null
    private var fftData: FloatArray = FloatArray(64)

    inner class LocalBinder : Binder() {
        fun getService(): EqualizerService = this@EqualizerService
    }

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        startForeground(1, getNotification())
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            applyEffectToSession(0)
        }
        setupVisualizer(0)
    }

    private fun setupVisualizer(sessionId: Int) {
        try {
            visualizer?.release()
            visualizer = Visualizer(sessionId).apply {
                captureSize = Visualizer.getCaptureSizeRange()[1]
                setDataCaptureListener(object : Visualizer.OnDataCaptureListener {
                    override fun onWaveFormDataCapture(v: Visualizer?, waveform: ByteArray?, samplingRate: Int) {}
                    override fun onFftDataCapture(v: Visualizer?, fft: ByteArray?, samplingRate: Int) {
                        fft?.let {
                            val points = 64
                            val result = FloatArray(points)
                            for (i in 0 until points) {
                                val r = it[i * 2].toFloat()
                                val im = it[i * 2 + 1].toFloat()
                                result[i] = kotlin.math.sqrt(r * r + im * im)
                            }
                            fftData = result
                        }
                    }
                }, Visualizer.getMaxCaptureRate() / 2, false, true)
                enabled = true
            }
        } catch (e: Exception) {
            Log.e(TAG, "Visualizer error: ${e.message}")
        }
    }

    fun getRealTimeFft(): FloatArray = fftData

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        intent?.let {
            val sessionId = it.getIntExtra("session_id", -1)
            if (sessionId != -1) {
                when (it.action) {
                    "OPEN_SESSION" -> {
                        applyEffectToSession(sessionId)
                        if (sessionId != 0) setupVisualizer(sessionId)
                    }
                    "CLOSE_SESSION" -> removeEffectFromSession(sessionId)
                }
            }
        }
        return START_STICKY
    }

    fun updateConfigs(configs: List<MainActivity.BandConfig>, masterGain: Float) {
        val oldSize = currentConfigs.size
        this.currentConfigs = configs.sortedBy { it.frequency }
        this.masterGain = masterGain
        
        Log.d(TAG, "Updating configs: bands=${currentConfigs.size}, masterGain=$masterGain")
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            if (oldSize != currentConfigs.size) {
                val sessions = ArrayList(activeEffects.keys)
                for (sessionId in sessions) {
                    removeEffectFromSession(sessionId)
                    applyEffectToSession(sessionId)
                }
            } else {
                for (dp in activeEffects.values) {
                    applyConfigsToDynamicsProcessing(dp)
                }
            }
        }
    }

    fun updateEffects(volume: Float, surround: Float, delay: Float) {
        this.masterVolume = volume
        this.surroundStrength = surround
        this.channelDelay = delay
        
        Log.d(TAG, "Updating effects: vol=$volume, surround=$surround, delay=$delay")
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            activeEffects.values.forEach { applyConfigsToDynamicsProcessing(it) }
        }
        
        activeVirtualizers.values.forEach {
            try {
                it.enabled = surroundStrength > 0
                if (it.strengthSupported) {
                    it.setStrength(surround.toInt().toShort())
                }
            } catch (e: Exception) { }
        }
    }

    private fun applyEffectToSession(sessionId: Int) {
        // DynamicsProcessing - Use 2 channels (Stereo) for headsets
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && !activeEffects.containsKey(sessionId)) {
            try {
                val builder = DynamicsProcessing.Config.Builder(
                    0, 
                    2, // Channel count set to 2 for stereo sound support
                    true, 0,
                    false, 0,
                    true, max(1, currentConfigs.size),
                    true
                )
                val dp = DynamicsProcessing(0, sessionId, builder.build())
                dp.enabled = true
                applyConfigsToDynamicsProcessing(dp)
                activeEffects[sessionId] = dp
                Log.d(TAG, "Applied DynamicsProcessing (Stereo) to session: $sessionId")
            } catch (e: Exception) {
                Log.e(TAG, "DP Error: ${e.message}")
            }
        }

        // Virtualizer (Surround)
        if (!activeVirtualizers.containsKey(sessionId)) {
            try {
                val v = Virtualizer(0, sessionId)
                v.enabled = surroundStrength > 0
                if (v.strengthSupported) {
                    v.setStrength(surroundStrength.toInt().toShort())
                }
                activeVirtualizers[sessionId] = v
                Log.d(TAG, "Applied Virtualizer to session: $sessionId")
            } catch (e: Exception) {
                Log.e(TAG, "Virtualizer Error: ${e.message}")
            }
        }
    }

    private fun removeEffectFromSession(sessionId: Int) {
        activeEffects.remove(sessionId)?.release()
        activeVirtualizers.remove(sessionId)?.release()
    }

    private fun applyConfigsToDynamicsProcessing(dp: DynamicsProcessing?) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P || dp == null) return

        try {
            // 1. Map Master Volume (0-100) to a dB range (-60dB to 0dB)
            // 0 volume = -60dB (near silence), 100 volume = 0dB (unity)
            val volumeDb = if (masterVolume <= 0f) -100f else (masterVolume - 100f) * 0.6f
            
            // 2. Apply Input Gain (Master Gain from EQ screen)
            // We apply the user's "Master Gain" here at the input
            dp.setInputGainAllChannelsTo(masterGain)

            // 3. Setup EQ bands
            for (i in currentConfigs.indices) {
                val config = currentConfigs[i]
                val eqBand = DynamicsProcessing.EqBand(true, config.frequency, config.gain)
                dp.setPostEqBandAllChannelsTo(i, eqBand)
            }

            // 4. Configure Limiter with Post-Gain for Volume
            // We use the Limiter's postGain for the "Main Volume" to ensure it's the last stage
            val limiter = DynamicsProcessing.Limiter(
                true, // inUse
                true, // enabled
                0,    // linkGroup
                1f,   // attackTime (ms)
                60f,  // releaseTime (ms)
                10f,  // ratio
                -0.1f, // threshold (dB) - keep it near 0 to prevent hard clipping
                volumeDb // postGain (dB) - our Master Volume
            )
            dp.setLimiterAllChannelsTo(limiter)
            
            Log.d(TAG, "Applied DP: InputGain=$masterGain, VolumeDb=$volumeDb")
        } catch (e: Exception) { 
            Log.e(TAG, "Apply config error: ${e.message}")
        }
    }

    override fun onBind(intent: Intent?): IBinder = binder

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val serviceChannel = NotificationChannel(
                CHANNEL_ID,
                "PowerEQ Master Service",
                NotificationManager.IMPORTANCE_LOW
            )
            val manager = getSystemService(NotificationManager::class.java)
            manager?.createNotificationChannel(serviceChannel)
        }
    }

    private fun getNotification(): Notification {
        val notificationIntent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, notificationIntent, 
            PendingIntent.FLAG_IMMUTABLE
        )

        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("PowerEQ Master")
            .setContentText("Global Audio Engine Active")
            .setSmallIcon(android.R.drawable.ic_media_play)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setContentIntent(pendingIntent)
            .build()
    }

    override fun onDestroy() {
        activeEffects.values.forEach { it.release() }
        activeVirtualizers.values.forEach { it.release() }
        activeEffects.clear()
        activeVirtualizers.clear()
        visualizer?.release()
        super.onDestroy()
    }

    companion object {
        private const val TAG = "EqualizerService"
        private const val CHANNEL_ID = "EqualizerChannel"
    }
}
