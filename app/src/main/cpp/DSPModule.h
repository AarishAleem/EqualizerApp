#ifndef EQUALIZERAPP_DSPMODULE_H
#define EQUALIZERAPP_DSPMODULE_H

#include "ParameterID.h"
#include "ParameterValue.h"
#include <string>
#include <vector>

/**
 * Richer lifecycle states for production-grade module management.
 */
enum class ModuleState {
    Active,     // Processing and contributing to output
    Bypassed,   // Passing input to output without modification
    Suspended   // Muted, zero output (e.g., when no signal detected)
};

/**
 * Abstract base class for all native DSP modules.
 */
class DSPModule {
public:
    virtual ~DSPModule() = default;

    virtual void prepare(int sampleRate, int blockSize, int channels) = 0;
    virtual void process(float* left, float* right, int numFrames) = 0;
    virtual void reset() = 0;

    /**
     * Unified parameter entry point.
     * Now strictly routes only owned parameters via the registry.
     */
    virtual void setParameter(ParameterID id, ParameterValue value) = 0;

    /**
     * Called during registry initialization to map parameters to destinations.
     */
    virtual std::vector<ParameterID> getOwnedParameters() const = 0;

    virtual std::string getName() const = 0;

    virtual void setState(ModuleState state) { state_ = state; }
    virtual ModuleState getState() const { return state_; }

    // Legacy support for simple boolean toggle
    virtual void setEnabled(bool enabled) {
        state_ = enabled ? ModuleState::Active : ModuleState::Bypassed;
    }
    virtual bool isEnabled() const { return state_ == ModuleState::Active; }

protected:
    ModuleState state_ = ModuleState::Active;
};

#endif //EQUALIZERAPP_DSPMODULE_H
