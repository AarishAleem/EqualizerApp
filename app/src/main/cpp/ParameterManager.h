#ifndef EQUALIZERAPP_PARAMETERMANAGER_H
#define EQUALIZERAPP_PARAMETERMANAGER_H

#include "ParameterQueue.h"
#include <array>
#include <mutex>

/**
 * Handles incoming parameter changes from the UI/JNI.
 * Maintains current parameter values and writes events into the lock-free queue.
 *
 * Responsibilities:
 * - Deterministic O(1) storage of current parameter states.
 * - Thread-safe interface for UI queries.
 * - Producer for the real-time parameter queue.
 */
class ParameterManager {
public:
    explicit ParameterManager(ParameterQueue& queue) : queue_(queue) {
        currentValues_.fill(0.0f);
    }

    /**
     * Updates a parameter. Called by the UI thread (Producer).
     */
    void setParameter(ParameterID id, float value) {
        uint32_t index = static_cast<uint32_t>(id);
        if (index >= kMaxParameterID) return;

        {
            // Internal array update for UI queries (not real-time path)
            std::lock_guard<std::mutex> lock(mapMutex_);
            currentValues_[index] = value;
        }

        // Push event to real-time audio thread.
        // If queue is full, latest update is rejected (Drop newest strategy).
        ParameterEvent event{id, ParameterValue::fromFloat(value)};
        queue_.push(event);
    }

    /**
     * Gets the latest cached value for a parameter.
     * Called by the UI thread for display or synchronization.
     */
    float getParameter(ParameterID id) {
        uint32_t index = static_cast<uint32_t>(id);
        if (index >= kMaxParameterID) return 0.0f;

        std::lock_guard<std::mutex> lock(mapMutex_);
        return currentValues_[index];
    }

private:
    ParameterQueue& queue_;

    // Contiguous fixed-size storage for deterministic O(1) lookup.
    std::array<float, kMaxParameterID> currentValues_;
    std::mutex mapMutex_;
};

#endif //EQUALIZERAPP_PARAMETERMANAGER_H
