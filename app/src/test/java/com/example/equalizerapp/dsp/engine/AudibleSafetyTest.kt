package com.example.equalizerapp.dsp.engine

import org.junit.Assert.*
import org.junit.Test

class AudibleSafetyTest {

    class FakeProcessorAdapter : AudibleProcessorAdapter {
        var released = false
        var isEnabledInternal = false
        var lastState: SafeAudibleState? = null

        override fun release() { released = true }
        override fun setEnabled(e: Boolean) { isEnabledInternal = e }
        override fun applyState(s: SafeAudibleState) { lastState = s }
    }

    @Test
    fun testDuplicateSessionOpenDoesNotCreateDuplicateProcessor() {
        val registry = ProcessorOwnershipRegistry<FakeProcessorAdapter>()
        var factoryCalls = 0
        
        val record1 = registry.getOrCreateSessionProcessor(1) { factoryCalls++; FakeProcessorAdapter() }
        val record2 = registry.getOrCreateSessionProcessor(1) { factoryCalls++; FakeProcessorAdapter() }
        
        assertEquals(1, factoryCalls)
        assertEquals(record1, record2)
        assertEquals(1, registry.getActiveRecords().size)
    }

    @Test
    fun testSessionCloseReleasesProcessorExactlyOnce() {
        val registry = ProcessorOwnershipRegistry<FakeProcessorAdapter>()
        val record = registry.getOrCreateSessionProcessor(1) { FakeProcessorAdapter() }
        
        registry.releaseSessionProcessor(1)
        assertTrue(record.processor.released)
        assertTrue(record.isReleased())
        assertEquals(0, registry.getActiveRecords().size)
        
        // Second call should be safe no-op
        registry.releaseSessionProcessor(1)
    }

    @Test
    fun testServiceDestroyReleasesAllProcessors() {
        val registry = ProcessorOwnershipRegistry<FakeProcessorAdapter>()
        val r1 = registry.getOrCreateSessionProcessor(1) { FakeProcessorAdapter() }
        val r2 = registry.getOrCreateSessionProcessor(2) { FakeProcessorAdapter() }
        
        registry.releaseAll()
        assertTrue(r1.processor.released)
        assertTrue(r2.processor.released)
        assertEquals(0, registry.getActiveRecords().size)
    }

    @Test
    fun testInvalidGainNaNRejected() {
        val newState = AudibleParameterSafety.validate(
            rawMasterGain = Float.NaN,
            rawIsEnabled = true,
            rawBands = emptyList(),
            previousSafeState = null
        )
        assertEquals(0f, newState.masterGain)
        assertTrue(AudibleParameterSafety.getRejectionCount() > 0)
    }

    @Test
    fun testInvalidFrequencyRejected() {
        val newState = AudibleParameterSafety.validate(
            rawMasterGain = 0f,
            rawIsEnabled = true,
            rawBands = listOf(com.example.equalizerapp.dsp.models.BandConfig(100000f, 0f, 0.7f, 0))
        )
        assertEquals(22000f, newState.bands[0].frequency)
    }
    
    @Test
    fun testLimiterSafetyConfigurationCannotBeBypassed() {
        val safe = AudibleParameterSafety.getFixedSafetyLimiter()
        assertEquals(-0.1f, safe.threshold)
        assertEquals(0f, safe.postGain)
        assertTrue(safe.enabled)
    }
}
