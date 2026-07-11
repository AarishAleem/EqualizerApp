#ifndef EQUALIZERAPP_MODULEREGISTRY_H
#define EQUALIZERAPP_MODULEREGISTRY_H

#include <vector>
#include <array>
#include "DSPModule.h"

/**
 * Handles the routing of ParameterEvents to specific DSPModule instances.
 * Uses a deterministic O(1) lookup table to ensure real-time safety.
 *
 * Ownership:
 * - ModuleRegistry holds RAW pointers (references) to modules.
 * - It DOES NOT own, delete, or allocate modules.
 * - Modules are owned by the DSPPipeline.
 */
class ModuleRegistry {
public:
    ModuleRegistry() = default;

    /**
     * Registers a module and maps its declared parameters in the routing table.
     * Called once during engine initialization.
     */
    void registerModule(DSPModule* module) {
        if (!module) return;

        auto params = module->getOwnedParameters();
        for (auto id : params) {
            uint32_t index = static_cast<uint32_t>(id);
            if (index < kMaxParameterID) {
                routingTable_[index].push_back(module);
            }
        }
    }

    /**
     * Routes a parameter change only to the registered subscriber(s).
     * Deterministic O(1) lookup without hashing.
     * Called in the high-priority audio thread.
     */
    void dispatch(ParameterID id, ParameterValue value) {
        uint32_t index = static_cast<uint32_t>(id);
        if (index >= kMaxParameterID) return;

        // Iterating over a small, fixed list of pointers is cache-friendly and deterministic.
        for (auto* module : routingTable_[index]) {
            module->setParameter(id, value);
        }
    }

    /**
     * Clears the routing table.
     */
    void clear() {
        for (auto& subscribers : routingTable_) {
            subscribers.clear();
        }
    }

private:
    // Mappings: ParameterID (Index) -> List of interested Modules
    // Using std::vector here for flexibility during initialization.
    // Since registration is fixed, this doesn't allocate during processing.
    std::array<std::vector<DSPModule*>, kMaxParameterID> routingTable_;
};

#endif //EQUALIZERAPP_MODULEREGISTRY_H
