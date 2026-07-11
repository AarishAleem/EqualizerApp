#ifndef EQUALIZERAPP_PROCESSSPEC_H
#define EQUALIZERAPP_PROCESSSPEC_H

#include <cstdint>

/**
 * Immutable specification for preparing DSP modules and internal buffers.
 */
struct ProcessSpec {
    double sampleRate = 0.0;
    uint32_t maxFramesPerBlock = 0;
    uint32_t numChannels = 0;

    bool isValid() const {
        return sampleRate > 0.0 && maxFramesPerBlock > 0 && numChannels > 0;
    }

    bool operator==(const ProcessSpec& other) const {
        return sampleRate == other.sampleRate &&
               maxFramesPerBlock == other.maxFramesPerBlock &&
               numChannels == other.numChannels;
    }

    bool operator!=(const ProcessSpec& other) const {
        return !(*this == other);
    }
};

#endif //EQUALIZERAPP_PROCESSSPEC_H
