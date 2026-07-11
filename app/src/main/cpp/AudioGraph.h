#ifndef EQUALIZERAPP_AUDIOGRAPH_H
#define EQUALIZERAPP_AUDIOGRAPH_H

#include "NodeId.h"
#include "DSPModule.h"
#include <vector>
#include <memory>

/**
 * Logical representation of the processing topology.
 * For RP-001 V1, this supports a single chain of nodes.
 */
struct NodeDescriptor {
    NodeId id;
    std::unique_ptr<DSPModule> module;
    NodeId next = ReservedNodes::Invalid;
};

class AudioGraph {
public:
    AudioGraph() = default;

    // Non-copyable
    AudioGraph(const AudioGraph&) = delete;
    AudioGraph& operator=(const AudioGraph&) = delete;

    void addNode(NodeId id, std::unique_ptr<DSPModule> module) {
        nodes_.push_back({id, std::move(module), ReservedNodes::Invalid});
    }

    void setLink(NodeId from, NodeId to) {
        for (auto& node : nodes_) {
            if (node.id == from) {
                node.next = to;
                return;
            }
        }
    }

    const std::vector<NodeDescriptor>& getNodes() const { return nodes_; }

    void clear() {
        nodes_.clear();
    }

private:
    std::vector<NodeDescriptor> nodes_;
};

#endif //EQUALIZERAPP_AUDIOGRAPH_H
