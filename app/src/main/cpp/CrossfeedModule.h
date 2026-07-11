#ifndef EQUALIZERAPP_CROSSFEEDMODULE_H
#define EQUALIZERAPP_CROSSFEEDMODULE_H

#include "DSPModule.h"
#include "ParameterSmoother.h"

class CrossfeedModule : public DSPModule {
public:
    void prepare(int sampleRate, int blockSize, int channels) override {
        smoother_.reset(crossfeed_, sampleRate, 20.0f);
    }

    void setParameter(ParameterID id, ParameterValue value) override {
        if (id == ParameterID::Crossfeed) {
            crossfeed_ = (value.val.f / 333.3f);
            smoother_.setTarget(crossfeed_);
        }
    }

    std::vector<ParameterID> getOwnedParameters() const override {
        return { ParameterID::Crossfeed };
    }

    void process(float* left, float* right, int numFrames) override {
        if (state_ == ModuleState::Bypassed) return;
        for (int i = 0; i < numFrames; ++i) {
            float l = left[i];
            float r = right[i];
            float c = smoother_.getNextValue();

            left[i] = l + (r * c);
            right[i] = r + (l * c);
        }
    }

    void reset() override {}

    std::string getName() const override { return "Crossfeed"; }

private:
    float crossfeed_ = 0.0f;
    ParameterSmoother smoother_;
};

#endif //EQUALIZERAPP_CROSSFEEDMODULE_H
