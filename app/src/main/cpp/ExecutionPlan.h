#ifndef EQUALIZERAPP_EXECUTIONPLAN_H
#define EQUALIZERAPP_EXECUTIONPLAN_H

#include "ProcessSpec.h"
#include "DSPModule.h"
#include "ModuleRegistry.h"
#include "AudioTap.h"
#include <vector>
#include <memory>

/**
 * Minimal operation types for the precompiled execution plan.
 */
enum class GraphOpType {
    Process,
    Tap
};

struct GraphOperation {
    GraphOpType type;
    DSPModule* module = nullptr;
    AudioTapSink* tap = nullptr;
};

/**
 * Precompiled, real-time readable representation of the processing work.
 * Structural metadata is immutable once active.
 */
class ExecutionPlan {
public:
    ExecutionPlan(const ProcessSpec& spec) : spec_(spec) {}

    const ProcessSpec& getSpec() const { return spec_; }

    void addOperation(const GraphOperation& op) {
        operations_.push_back(op);
    }

    void addModule(std::unique_ptr<DSPModule> module) {
        registry_.registerModule(module.get());
        modules_.push_back(std::move(module));
    }

    const std::vector<GraphOperation>& getOperations() const { return operations_; }

    ModuleRegistry& getRegistry() { return registry_; }

    void reset() {
        for (auto& module : modules_) {
            module->reset();
        }
    }

private:
    ProcessSpec spec_;
    std::vector<std::unique_ptr<DSPModule>> modules_; // Owns the instances for this plan
    ModuleRegistry registry_; // routes parameters for this plan's modules
    std::vector<GraphOperation> operations_;
};

#endif //EQUALIZERAPP_EXECUTIONPLAN_H
