#ifndef EQUALIZERAPP_GRAPHEXECUTOR_H
#define EQUALIZERAPP_GRAPHEXECUTOR_H

#include "ExecutionPlan.h"
#include "ProcessContext.h"
#include <atomic>

/**
 * Small real-time executor.
 * Strictly decoupled from graph construction and Oboe lifecycle.
 */
class GraphExecutor {
public:
    void execute(ExecutionPlan* plan, ProcessContext& context) {
        if (!plan) return;

        // RT-safe bound check
        if (context.numFrames > plan->getSpec().maxFramesPerBlock) {
            context.limitExceeded = true;
            return;
        }

        const auto& operations = plan->getOperations();
        for (const auto& op : operations) {
            if (op.type == GraphOpType::Process && op.module) {
                op.module->process(context);
            } else if (op.type == GraphOpType::Tap && op.tap) {
                op.tap->consume(context);
            }
        }
    }
};

#endif //EQUALIZERAPP_GRAPHEXECUTOR_H
