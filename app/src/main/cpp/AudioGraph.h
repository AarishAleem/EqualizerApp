#ifndef EQUALIZERAPP_AUDIOGRAPH_H
#define EQUALIZERAPP_AUDIOGRAPH_H

#include "NodeId.h"
#include "DSPModule.h"
#include "AudioTap.h"
#include <vector>
#include <memory>
#include <set>

/**
 * Result codes for graph mutation operations.
 */
enum class GraphMutationResult {
    Success = 0,
    InvalidNodeId,
    DuplicateNodeId,
    MissingEdgeSource
};

/**
 * Logical representation of the processing topology.
 * For RP-001 V1, this supports a single chain of nodes.
 */
struct NodeDescriptor {
    NodeId id;
    std::unique_ptr<DSPModule> moduleTemplate;
    AudioTapSink* tap = nullptr;
    NodeId next = ReservedNodes::Invalid;

    NodeDescriptor(NodeId id, std::unique_ptr<DSPModule> mod)
        : id(id), moduleTemplate(std::move(mod)) {}
};

class AudioGraph {
public:
    AudioGraph() = default;

    AudioGraph(const AudioGraph&) = delete;
    AudioGraph& operator=(const AudioGraph&) = delete;

    GraphMutationResult addNode(NodeId id, std::unique_ptr<DSPModule> module) {
        if (id == ReservedNodes::Invalid) return GraphMutationResult::InvalidNodeId;

        for (const auto& n : nodes_) {
            if (n.id == id) return GraphMutationResult::DuplicateNodeId;
        }

        nodes_.emplace_back(id, std::move(module));
        return GraphMutationResult::Success;
    }

    GraphMutationResult setLink(NodeId from, NodeId to) {
        for (auto& node : nodes_) {
            if (node.id == from) {
                node.next = to;
                return GraphMutationResult::Success;
            }
        }
        return GraphMutationResult::MissingEdgeSource;
    }

    const std::vector<NodeDescriptor>& getNodes() const { return nodes_; }
    std::vector<NodeDescriptor>& getNodesMutable() { return nodes_; }

    void clear() {
        nodes_.clear();
    }

private:
    std::vector<NodeDescriptor> nodes_;
};

#endif //EQUALIZERAPP_AUDIOGRAPH_H
