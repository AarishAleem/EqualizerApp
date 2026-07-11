#ifndef EQUALIZERAPP_AUDIOTAP_H
#define EQUALIZERAPP_AUDIOTAP_H

#include "ProcessContext.h"

/**
 * Read-only observation contract for signal monitoring.
 */
class AudioTapSink {
public:
    virtual ~AudioTapSink() = default;
    virtual void consume(const ProcessContext& context) = 0;
};

/**
 * Example Tap that counts processed frames.
 */
class FrameCounterTap : public AudioTapSink {
public:
    void consume(const ProcessContext& context) override {
        frameCount_ += context.numFrames;
    }
    uint64_t getCount() const { return frameCount_; }
private:
    uint64_t frameCount_ = 0;
};

#endif //EQUALIZERAPP_AUDIOTAP_H
