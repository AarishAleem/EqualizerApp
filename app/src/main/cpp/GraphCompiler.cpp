#include "GraphCompiler.h"
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <set>
#include <queue>

CompilationResult GraphCompiler::compile(const AudioGraph& graph, const ProcessSpec& spec) {
    if (!spec.isValid()) {
        return {CompilationResultCode::InvalidProcessSpec, "Invalid ProcessSpec", nullptr};
    }

    const auto& nodes = graph.getNodes();
    if (nodes.empty()) {
        return {CompilationResultCode::MissingNodes, "Graph is empty", nullptr};
    }

    std::vector<NodeId> order;
    CompilationResultCode validationCode = validateAndOrder(graph, order);
    if (validationCode != CompilationResultCode::Success) {
        return {validationCode, "Graph validation failed", nullptr};
    }

    auto plan = std::make_unique<ExecutionPlan>(spec);

    for (NodeId id : order) {
        auto it = std::find_if(nodes.begin(), nodes.end(), [id](const NodeDescriptor& d) {
            return d.id == id;
        });

        if (it != nodes.end()) {
            auto moduleInstance = it->moduleTemplate->clone();
            if (!moduleInstance) {
                return {CompilationResultCode::NodeMaterializationFailure, "Materialization failed", nullptr};
            }

            moduleInstance->prepare(spec);

            GraphOperation op;
            op.type = GraphOpType::Process;
            op.module = moduleInstance.get();

            plan->addModule(std::move(moduleInstance));
            plan->addOperation(op);

            if (it->tap) {
                GraphOperation tapOp;
                tapOp.type = GraphOpType::Tap;
                tapOp.tap = it->tap;
                plan->addOperation(tapOp);
            }
        }
    }

    return {CompilationResultCode::Success, "Success", std::move(plan)};
}

CompilationResultCode GraphCompiler::validateAndOrder(const AudioGraph& graph, std::vector<NodeId>& outOrder) {
    const auto& nodes = graph.getNodes();

    // 1. Establish validated ID set
    std::set<NodeId> allIds;
    for (const auto& n : nodes) {
        allIds.insert(n.id);
    }

    // 2. Validate Edge Endpoints and Self-Loops
    std::unordered_map<NodeId, int> inDegrees;
    for (const auto& n : allIds) inDegrees[n] = 0;

    for (const auto& n : nodes) {
        if (n.next != ReservedNodes::Invalid) {
            if (n.next == n.id) return CompilationResultCode::UnsupportedSelfLoop;
            if (allIds.find(n.next) == allIds.end()) return CompilationResultCode::MissingEdgeDestination;
            inDegrees[n.next]++;
        }
    }

    // 3. Full-Graph Cycle Detection (Kahn's Algorithm)
    std::queue<NodeId> q;
    // Use set for deterministic start if multiple roots were allowed,
    // but Kahn's typically uses a queue. For single-root V1 policy,
    // we use a priority queue or sort start nodes to ensure determinism.
    std::set<NodeId> zeroInNodes;
    for (const auto& entry : inDegrees) {
        if (entry.second == 0) zeroInNodes.insert(entry.first);
    }

    std::vector<NodeId> topologicalOrder;
    std::unordered_map<NodeId, int> tempInDegrees = inDegrees;

    // Use a work list that is sorted for determinism
    std::set<NodeId> workSet = zeroInNodes;

    while (!workSet.empty()) {
        NodeId u = *workSet.begin();
        workSet.erase(workSet.begin());
        topologicalOrder.push_back(u);

        // Find neighbors (for RP-001 each node has at most one next)
        auto it = std::find_if(nodes.begin(), nodes.end(), [u](const NodeDescriptor& d) {
            return d.id == u;
        });

        if (it != nodes.end() && it->next != ReservedNodes::Invalid) {
            NodeId v = it->next;
            tempInDegrees[v]--;
            if (tempInDegrees[v] == 0) {
                workSet.insert(v);
            }
        }
    }

    if (topologicalOrder.size() != allIds.size()) {
        return CompilationResultCode::CycleDetected;
    }

    // 4. V1 Policy: Single Start Node
    if (zeroInNodes.empty()) return CompilationResultCode::MissingStartNode;
    if (zeroInNodes.size() > 1) return CompilationResultCode::AmbiguousStartNode;

    // 5. Connectivity: Ensure all nodes are reachable from the single root
    // Since it's a single chain/path for now, Kahn's order IS the path.
    // However, if there are multiple chains, topologicalOrder might be > 1 chain.
    // But since zeroInNodes.size() == 1, topologicalOrder MUST be a single connected component.

    outOrder = topologicalOrder;
    return CompilationResultCode::Success;
}
