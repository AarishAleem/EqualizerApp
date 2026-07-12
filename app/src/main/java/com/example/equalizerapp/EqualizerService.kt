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
import com.example.equalizerapp.dsp.engine.AudibleParameterSafety
import com.example.equalizerapp.dsp.engine.AudibleProcessorAdapter
import com.example.equalizerapp.dsp.engine.DynamicsProcessingProcessor
import com.example.equalizerapp.dsp.engine.ProcessorOwnershipRegistry
import com.example.equalizerapp.dsp.engine.SafeAudibleState
import com.example.equalizerapp.dsp.models.BandConfig
import java.util.HashMap
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.math.max

class EqualizerService : Service() {
    private val binder = LocalBinder()
    
    // SI-001: Processor ownership and safety
    private val processorRegistry = ProcessorOwnershipRegistry<AudibleProcessorAdapter>()
    private val activeVirtualizers = HashMap<Int, Virtualizer>()
    private var lastSafeState: SafeAudibleState? = null
    private val acceptingUpdates = AtomicBoolean(true)

    private var currentConfigs: List<BandConfig> = ArrayList()
    private var masterGain: Float = 0f
    private var masterVolume: Float = 100f
    private var surroundStrength: Float = 0f
    private var isEnabled: Boolean = true
    
    private var visualizer: Visualizer? = null
    private var fftData: FloatArray = FloatArray(64)

    // Native Engine Handle
    private var nativeEngineHandle: Long = 0

    // Native EQ Engine Bridge
    private external fun createNativeEngine(numBands: Int, sampleRate: Double): Long
    private external fun configureNativeBand(handle: Long, index: Int, type: Int, freq: Double, gain: Double, q: Double)
    private external fun setNativeMasterGain(handle: Long, gain: Float)
    private external fun setNativeSurround(handle: Long, width: Float, crossfeed: Float)
    private external fun setNativeEnabled(handle: Long, enabled: Boolean)
    private external fun startNativeStream(handle: Long)
    private external fun stopNativeStream(handle: Long)
    private external fun destroyNativeEngine(handle: Long)
    private external fun runNativeTests(): Int
    
    inner class LocalBinder : Binder() {
        fun getService(): EqualizerService = this@EqualizerService
    }

    override fun onCreate() {
        super.onCreate()
        Log.d(TAG, "SERVICE_CREATE")
        createNotificationChannel()
        startForeground(1, getNotification())
        
        // Initialize Native Engine
        nativeEngineHandle = createNativeEngine(12, 48000.0)
        
        if (nativeEngineHandle != 0L) {
            startNativeStream(nativeEngineHandle)
        } else {
            Log.e(TAG, "Failed to create native audio engine.")
        }
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            applyEffectToSession(0)
        }
        setupVisualizer(0)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        intent?.let {
            val sessionId = it.getIntExtra("session_id", -1)
            if (sessionId != -1) {
                when (it.action) {
                    "OPEN_SESSION" -> {
                        Log.d("PowerEQ-AudioSafety", "SESSION_OPEN session=$sessionId")
                        applyEffectToSession(sessionId)
                        if (sessionId != 0) setupVisualizer(sessionId)
                    }
                    "CLOSE_SESSION" -> {
                        Log.d("PowerEQ-AudioSafety", "SESSION_CLOSE session=$sessionId")
                        removeEffectFromSession(sessionId)
                    }
                }
            }
        }
        return START_STICKY
    }

    fun updateConfigs(configs: List<BandConfig>, masterGain: Float) {
        if (!acceptingUpdates.get()) return

        this.currentConfigs = configs.sortedBy { it.frequency }
        this.masterGain = masterGain
        
        // Native update
        if (nativeEngineHandle != 0L) {
            setNativeMasterGain(nativeEngineHandle, masterGain)
            for (i in currentConfigs.indices) {
                val config = currentConfigs[i]
                configureNativeBand(nativeEngineHandle, i, config.typeIndex, config.frequency.toDouble(), config.gain.toDouble(), config.q.toDouble())
            }
        }
        
        // Audible update with safety boundary
        val newState = AudibleParameterSafety.validate(masterGain, isEnabled, currentConfigs, lastSafeState)
        lastSafeState = newState
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            processorRegistry.getActiveRecords().forEach { record ->
                record.processor.applyState(newState)
            }
        }
    }

    fun updateEffects(volume: Float, surround: Float, delay: Float, crossfeedValue: Float = 0f) {
        if (!acceptingUpdates.get()) return

        this.masterVolume = volume
        this.surroundStrength = surround
        
        // Native update
        if (nativeEngineHandle != 0L) {
            setNativeSurround(nativeEngineHandle, surround, crossfeedValue)
        }
        
        // Note: masterVolume mapping and Virtualizer safety will be handled in SI-001 Pass 2 if needed
    }

    fun setEngineEnabled(enabled: Boolean) {
        if (!acceptingUpdates.get()) return

        this.isEnabled = enabled
        if (nativeEngineHandle != 0L) {
            setNativeEnabled(nativeEngineHandle, enabled)
        }
        
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            processorRegistry.getActiveRecords().forEach { it.processor.setEnabled(enabled) }
        }
        activeVirtualizers.values.forEach { it.enabled = enabled && surroundStrength > 0 }
    }

    fun getRealTimeFft(): FloatArray = fftData

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

    private fun applyEffectToSession(sessionId: Int) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            try {
                processorRegistry.getOrCreateSessionProcessor(sessionId) { genId ->
                    val builder = DynamicsProcessing.Config.Builder(
                        0, 2, true, 0, false, 0, true, max(1, currentConfigs.size), true
                    )
                    val dp = DynamicsProcessing(0, sessionId, builder.build())
                    
                    val adapter = DynamicsProcessingProcessor(sessionId, genId, dp)
                    
                    // 1. apply safe baseline
                    val baseline = AudibleParameterSafety.validate(0f, false, emptyList())
                    adapter.applyState(baseline)
                    
                    // 2. apply validated state
                    lastSafeState?.let { adapter.applyState(it) }
                    
                    // 3. enable
                    adapter.setEnabled(isEnabled)
                    
                    adapter
                }
            } catch (e: Exception) {
                Log.e(TAG, "Failed to create DynamicsProcessing for session $sessionId", e)
            }
        }

        // Virtualizer (Surround)
        if (!activeVirtualizers.containsKey(sessionId)) {
            try {
                val v = Virtualizer(0, sessionId)
                v.enabled = isEnabled && surroundStrength > 0
                if (v.strengthSupported) {
                    v.setStrength(surroundStrength.toInt().toShort())
                }
                activeVirtualizers[sessionId] = v
            } catch (e: Exception) {
                Log.e(TAG, "Virtualizer Error: ${e.message}")
            }
        }
    }

    private fun removeEffectFromSession(sessionId: Int) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            processorRegistry.releaseSessionProcessor(sessionId)
        }
        activeVirtualizers.remove(sessionId)?.release()
    }

    override fun onBind(intent: Intent?): IBinder = binder

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val serviceChannel = NotificationChannel(
                CHANNEL_ID, "PowerEQ Master Service", NotificationManager.IMPORTANCE_LOW
            )
            val manager = getSystemService(NotificationManager::class.java)
            manager?.createNotificationChannel(serviceChannel)
        }
    }

    private fun getNotification(): Notification {
        val notificationIntent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, notificationIntent, PendingIntent.FLAG_IMMUTABLE
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
        Log.d(TAG, "SERVICE_DESTROY")
        
        // 1. stop updates
        acceptingUpdates.set(false)
        
        // 2. release audible processors
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            processorRegistry.releaseAll()
        }
        activeVirtualizers.values.forEach { it.release() }
        activeVirtualizers.clear()
        
        // 3. release Visualizer
        visualizer?.release()
        
        // 4. stop native engine
        if (nativeEngineHandle != 0L) {
            stopNativeStream(nativeEngineHandle)
            destroyNativeEngine(nativeEngineHandle)
            nativeEngineHandle = 0L
        }
        
        super.onDestroy()
    }

    fun dumpAudioSafetyState() {
        Log.i("PowerEQ-AudioSafety", "--- Safety Snapshot ---")
        Log.i("PowerEQ-AudioSafety", "Accepting Updates: ${acceptingUpdates.get()}")
        Log.i("PowerEQ-AudioSafety", "Native Handle Valid: ${nativeEngineHandle != 0L}")
        Log.i("PowerEQ-AudioSafety", "Live Processor Count: ${processorRegistry.getActiveRecords().size}")
        Log.i("PowerEQ-AudioSafety", "Invalid Param Rejections: ${AudibleParameterSafety.getRejectionCount()}")
    }

    companion object {
        private const val TAG = "EqualizerService"
        private const val CHANNEL_ID = "EqualizerChannel"

        init {
            System.loadLibrary("equalizerapp")
        }
    }
}
