#ifndef EQUALIZERAPP_PARAMETERQUEUE_H
#define EQUALIZERAPP_PARAMETERQUEUE_H

#include <atomic>
#include <vector>
#include "ParameterEvent.h"

/**
 * A Single-Producer Single-Consumer (SPSC) Lock-Free Queue.
 * Optimized for UI thread (Producer) and Audio Thread (Consumer) interaction.
 * Uses a fixed-size ring buffer to guarantee zero allocation in the audio thread.
 *
 * Overflow Policy: Drop Newest.
 * If the queue is full, push() returns false and the event is rejected.
 * This strategy is chosen because in a real-time system, processing stale/older
 * updates is often better than dropping them, and the UI can retry or coalesce
 * updates if the queue is saturated.
 */
class ParameterQueue {
public:
    explicit ParameterQueue(size_t capacity = 1024)
        : buffer_(capacity), capacity_(capacity), head_(0), tail_(0) {}

    /**
     * Push a new event into the queue.
     * Called by the UI thread (Producer).
     */
    bool push(const ParameterEvent& event) {
        size_t currentTail = tail_.load(std::memory_order_relaxed);
        size_t nextTail = (currentTail + 1) % capacity_;

        if (nextTail == head_.load(std::memory_order_acquire)) {
            return false; // Queue full
        }

        buffer_[currentTail] = event;
        tail_.store(nextTail, std::memory_order_release);
        return true;
    }

    /**
     * Pop the next event from the queue.
     * Called by the Audio thread (Consumer).
     */
    bool pop(ParameterEvent& event) {
        size_t currentHead = head_.load(std::memory_order_relaxed);
        if (currentHead == tail_.load(std::memory_order_acquire)) {
            return false; // Queue empty
        }

        event = buffer_[currentHead];
        head_.store((currentHead + 1) % capacity_, std::memory_order_release);
        return true;
    }

private:
    std::vector<ParameterEvent> buffer_;
    size_t capacity_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};

#endif //EQUALIZERAPP_PARAMETERQUEUE_H
