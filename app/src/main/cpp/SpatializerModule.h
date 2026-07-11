#ifndef EQUALIZERAPP_SPATIALIZERMODULE_H
#define EQUALIZERAPP_SPATIALIZERMODULE_H

#include "DSPModule.h"

class SpatializerModule : public DSPModule {
public:
    void setParameters(float widthPercent, float crossfeedPercent) {
        // widthPercent 0..100 maps to 1.0..2.0 (1.0 is neutral)
        stereoWidth_ = 1.0f + (widthPercent / 100.0f);
        // crossfeed 0..100 maps to 0.0..0.3
        crossfeed_ = (crossfeedPercent / 333.3f);
    }

    void prepare(int sampleRate, int blockSize, int channels) override {}

    void process(float* left, float* right, int numFrames) override {
        for (int i = 0; i < numFrames; ++i) {
            float l = left[i];
            float r = right[i];

            // 1. Stereo Widening (Mid/Side)
            float mid = (l + r) * 0.5f;
            float side = (l - r) * 0.5f;
            side *= stereoWidth_;

            l = mid + side;
            r = mid - side;

            // 2. Crossfeed
            left[i] = l + (r * crossfeed_);
            right[i] = r + (l * crossfeed_);
        }
    }

    void reset() override {}

    std::string getName() const override { return "Spatializer"; }

private:
    float stereoWidth_ = 1.0f;
    float crossfeed_ = 0.0f;
};

#endif //EQUALIZERAPP_SPATIALIZERMODULE_H
