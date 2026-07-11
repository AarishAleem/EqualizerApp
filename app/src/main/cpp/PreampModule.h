#ifndef EQUALIZERAPP_PREAMPMODULE_H
#define EQUALIZERAPP_PREAMPMODULE_H

#include "DSPModule.h"
#include "ParameterSmoother.h"
#include <cmath>

class PreampModule : public DSPModule {
public:
    void prepare(int sampleRate, int blockSize, int channels) override {
        smoother_.reset(gainLinear_, sampleRate, 10.0f);
    }

    void setParameter(ParameterID id, ParameterValue value) override {
        if (id == ParameterID::PreampGain) {
            gainLinear_ = powf(10.0f, value.val.f / 20.0f);
            smoother_.setTarget(gainLinear_);
        }
    }

    std::vector<ParameterID> getOwnedParameters() const override {
        return { ParameterID::PreampGain };
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

    std::string getName() const override { return "Preamp"; }

private:
    float gainLinear_ = 1.0f;
    ParameterSmoother smoother_;
};

#endif //EQUALIZERAPP_PREAMPMODULE_H
