#ifndef EQUALIZERAPP_OUTPUTGAINMODULE_H
#define EQUALIZERAPP_OUTPUTGAINMODULE_H

#include "DSPModule.h"
#include "ParameterSmoother.h"
#include <cmath>

class OutputGainModule : public DSPModule {
public:
    void prepare(int sampleRate, int blockSize, int channels) override {
        smoother_.reset(gainLinear_, sampleRate, 10.0f);
    }

    void setParameter(ParameterID id, ParameterValue value) override {
        if (id == ParameterID::OutputVolume) {
            float db = (value.val.f <= 0.0f) ? -100.0f : (value.val.f - 100.0f) * 0.6f;
            gainLinear_ = powf(10.0f, db / 20.0f);
            smoother_.setTarget(gainLinear_);
        }
    }

    std::vector<ParameterID> getOwnedParameters() const override {
        return { ParameterID::OutputVolume };
    }

    void process(float* left, float* right, int numFrames) override {
        if (state_ == ModuleState::Bypassed) return;
        for (int i = 0; i < numFrames; ++i) {
            float g = smoother_.getNextValue();
            left[i] *= g;
            right[i] *= g;
        }
    }

    void reset() override {}

    std::string getName() const override { return "Output Gain"; }

private:
    float gainLinear_ = 1.0f;
    ParameterSmoother smoother_;
};

#endif //EQUALIZERAPP_OUTPUTGAINMODULE_H
