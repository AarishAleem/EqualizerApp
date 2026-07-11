#ifndef EQUALIZERAPP_NODEID_H
#define EQUALIZERAPP_NODEID_H

#include <cstdint>

/**
 * Unique identifier for a logical graph node.
 * Stable across compilation and not dependent on memory addresses.
 */
using NodeId = uint32_t;

namespace ReservedNodes {
    constexpr NodeId Invalid = 0;
    constexpr NodeId Input = 1;
    constexpr NodeId Output = 0xFFFFFFFF;
}

#endif //EQUALIZERAPP_NODEID_H
