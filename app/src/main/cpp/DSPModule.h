#ifndef EQUALIZERAPP_DSPMODULE_H
#define EQUALIZERAPP_DSPMODULE_H

#include "ParameterID.h"
#include "ParameterValue.h"
#include "ProcessSpec.h"
#include "ProcessContext.h"
#include <string>
#include <vector>

/**
 * Richer lifecycle states for production-grade module management.
 */
enum class ModuleState {
    Active,     // Processing and contributing to output
    Bypassed,   // Passing input to output without modification
    Suspended   // Muted, zero output
};

/**
 * Abstract base class for all native DSP modules.
 */
class DSPModule {
public:
    virtual ~DSPModule() = default;

    /**
     * Preparation contract. Must be called off the real-time thread.
     */
    virtual void prepare(const ProcessSpec& spec) = 0;

    /**
     * Processing contract. Must be real-time safe.
     */
    virtual void process(ProcessContext& context) = 0;

    /**
     * Reset contract. Clears delay lines and internal history.
     */
    virtual void reset() = 0;

    /**
     * Parameter entry point.
     */
    virtual void setParameter(ParameterID id, ParameterValue value) = 0;

    /**
     * Parameters owned by this specific instance.
     */
    virtual std::vector<ParameterID> getOwnedParameters() const = 0;

    virtual std::string getName() const = 0;

    virtual void setState(ModuleState state) { state_ = state; }
    virtual ModuleState getState() const { return state_; }

    virtual uint32_t getLatencySamples() const { return 0; }

    // DEFECT 3 FIX: Clone method to allow materializing runtime instances
    // from logical graph nodes without shared state risk.
    virtual std::unique_ptr<DSPModule> clone() const = 0;

protected:
    ModuleState state_ = ModuleState::Active;
};

#endif //EQUALIZERAPP_DSPMODULE_H
