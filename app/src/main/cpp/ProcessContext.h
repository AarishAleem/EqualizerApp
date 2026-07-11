#ifndef EQUALIZERAPP_PROCESSCONTEXT_H
#define EQUALIZERAPP_PROCESSCONTEXT_H

#include <cstdint>

/**
 * Context for a single processing callback.
 * Explicitly distinguishes between external (Oboe) and internal (Arena) memory.
 */
struct ProcessContext {
    // Current valid frame count for this block
    uint32_t numFrames = 0;

    // External interleaved buffer (if applicable)
    float* externalInterleaved = nullptr;

    // Planar pointers for current block
    float* left = nullptr;
    float* right = nullptr;

    // Diagnostic flags
    bool limitExceeded = false;
};

#endif //EQUALIZERAPP_PROCESSCONTEXT_H
