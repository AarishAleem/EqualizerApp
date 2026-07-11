#include "GraphCompiler.h"
#include <unordered_set>
#include <algorithm>

CompilationResult GraphCompiler::compile(const AudioGraph& graph, const ProcessSpec& spec) {
    if (!spec.isValid()) {
        return {CompilationResultCode::InvalidProcessSpec, "Invalid ProcessSpec provided", nullptr};
    }

    std::vector<NodeId> order;
    if (!validateChain(graph, order)) {
        return {CompilationResultCode::InvalidLogicalGraph, "Topology validation failed", nullptr};
    }

    auto plan = std::make_unique<ExecutionPlan>(spec);

    const auto& descriptors = graph.getNodes();
    for (NodeId id : order) {
        auto it = std::find_if(descriptors.begin(), descriptors.end(), [id](const NodeDescriptor& d) {
            return d.id == id;
        });

        if (it != descriptors.end()) {
            // Note: In a production DAG, we would clone modules or use factories.
            // For RP-001 V1, we transfer ownership from the temporary logical graph.
            // THIS REQUIRES THE CALLER TO REBUILD THE LOGICAL GRAPH FOR NEW COMPILATIONS.
            // This satisfies the "Minimal Coupling" and "Deterministic Execution" rules.

            auto& modulePtr = const_cast<std::unique_ptr<DSPModule>&>(it->module);
            if (!modulePtr) {
                return {CompilationResultCode::InvalidLogicalGraph, "Node has null module", nullptr};
            }

            modulePtr->prepare(spec);

            GraphOperation op;
            op.type = GraphOpType::Process;
            op.module = modulePtr.get();

            plan->addModule(std::move(modulePtr));
            plan->addOperation(op);
        }
    }

    return {CompilationResultCode::Success, "Success", std::move(plan)};
}

bool GraphCompiler::validateChain(const AudioGraph& graph, std::vector<NodeId>& outOrder) {
    const auto& nodes = graph.getNodes();
    if (nodes.empty()) return true;

    // V1 Policy: Chain starting from a node that has no incoming link.
    // In RP-001 we assume the first added node is the start if not specified.
    // Let's implement basic adjacency traversal to find the chain and detect cycles.

    std::unordered_set<NodeId> visited;
    NodeId current = nodes[0].id; // Simple start assumption for V1

    while (current != ReservedNodes::Invalid) {
        if (visited.count(current)) return false; // Cycle
        visited.insert(current);
        outOrder.push_back(current);

        auto it = std::find_if(nodes.begin(), nodes.end(), [current](const NodeDescriptor& d) {
            return d.id == current;
        });

        if (it == nodes.end()) break;
        current = it->next;
    }

    // Check if all transforming nodes are included
    return visited.size() == nodes.size();
}
