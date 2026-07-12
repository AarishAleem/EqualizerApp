#ifndef EQUALIZERAPP_GRAPHCOMPILER_H
#define EQUALIZERAPP_GRAPHCOMPILER_H

#include "AudioGraph.h"
#include "ExecutionPlan.h"
#include <string>

/**
 * Structured diagnostics for compilation results.
 */
enum class CompilationResultCode {
    Success = 0,
    InvalidProcessSpec,
    MissingNodes,
    MissingEdgeDestination,
    UnsupportedSelfLoop,
    CycleDetected,
    MissingStartNode,
    AmbiguousStartNode,
    DisconnectedNodes,
    NodeMaterializationFailure,
    UnknownError
};

struct CompilationResult {
    CompilationResultCode code;
    std::string diagnosticText;
    std::unique_ptr<ExecutionPlan> plan;

    bool isSuccess() const { return code == CompilationResultCode::Success; }
};

/**
 * Handles validation and materialization of the real-time execution plan.
 */
class GraphCompiler {
public:
    CompilationResult compile(const AudioGraph& graph, const ProcessSpec& spec);

private:
    CompilationResultCode validateAndOrder(const AudioGraph& graph, std::vector<NodeId>& outOrder);
};

#endif //EQUALIZERAPP_GRAPHCOMPILER_H
