package com.example.equalizerapp.dsp.engine

import android.util.Log
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicLong

/**
 * Platform-neutral processor record tracking generation and lifecycle.
 */
class ProcessorRecord<T>(
    val sessionId: Int,
    val generationId: Long,
    val processor: T
) {
    private val released = AtomicBoolean(false)
    
    fun isReleased(): Boolean = released.get()
    
    fun markReleased(): Boolean {
        return released.compareAndSet(false, true)
    }
}

/**
 * Interface for the production processor adapter.
 */
interface AudibleProcessorAdapter {
    fun release()
    fun setEnabled(enabled: Boolean)
    fun applyState(state: SafeAudibleState)
}

/**
 * Centralized ownership policy for audible processors.
 */
class ProcessorOwnershipRegistry<T : AudibleProcessorAdapter> {
    private val activeProcessors = mutableMapOf<Int, ProcessorRecord<T>>()
    private val nextGenerationId = AtomicLong(1)
    private val lock = Any()

    fun getOrCreateSessionProcessor(sessionId: Int, factory: (Long) -> T): ProcessorRecord<T> {
        synchronized(lock) {
            val existing = activeProcessors[sessionId]
            if (existing != null && !existing.isReleased()) {
                Log.d("PowerEQ-AudioSafety", "PROCESSOR_REUSE session=$sessionId processorGeneration=${existing.generationId}")
                return existing
            }

            val genId = nextGenerationId.getAndIncrement()
            val newProcessor = factory(genId)
            val record = ProcessorRecord(sessionId, genId, newProcessor)
            activeProcessors[sessionId] = record
            
            Log.d("PowerEQ-AudioSafety", "PROCESSOR_CREATE session=$sessionId processorGeneration=$genId")
            return record
        }
    }

    fun releaseSessionProcessor(sessionId: Int) {
        synchronized(lock) {
            val record = activeProcessors.remove(sessionId)
            if (record != null) {
                if (record.markReleased()) {
                    Log.d("PowerEQ-AudioSafety", "PROCESSOR_RELEASE session=$sessionId processorGeneration=${record.generationId}")
                    record.processor.release()
                }
            } else {
                Log.d("PowerEQ-AudioSafety", "SESSION_CLOSE_UNKNOWN session=$sessionId")
            }
            Unit
        }
    }

    fun releaseAll() {
        synchronized(lock) {
            val count = activeProcessors.size
            activeProcessors.values.forEach { record ->
                if (record.markReleased()) {
                    record.processor.release()
                }
            }
            activeProcessors.clear()
            Log.d("PowerEQ-AudioSafety", "SERVICE_DESTROY liveProcessorCount=$count")
            Unit
        }
    }

    fun getActiveRecords(): List<ProcessorRecord<T>> {
        synchronized(lock) {
            return activeProcessors.values.toList()
        }
    }
}
