#ifndef EQUALIZERAPP_STEREOWIDTHMODULE_H
#define EQUALIZERAPP_STEREOWIDTHMODULE_H

#include "DSPModule.h"
#include "ParameterSmoother.h"

class StereoWidthModule : public DSPModule {
public:
    void prepare(int sampleRate, int blockSize, int channels) override {
        smoother_.reset(width_, sampleRate, 20.0f);
    }

    void setParameter(ParameterID id, ParameterValue value) override {
        if (id == ParameterID::StereoWidth) {
            width_ = 1.0f + (value.val.f / 100.0f);
            smoother_.setTarget(width_);
        }
    }

    std::vector<ParameterID> getOwnedParameters() const override {
        return { ParameterID::StereoWidth };
    }

    void process(float* left, float* right, int numFrames) override {
        if (state_ == ModuleState::Bypassed) return;
        for (int i = 0; i < numFrames; ++i) {
            float l = left[i];
            float r = right[i];
            float w = smoother_.getNextValue();

            float mid = (l + r) * 0.5f;
            float side = (l - r) * 0.5f;
            side *= w;

            left[i] = mid + side;
            right[i] = mid - side;
        }
    }

    void reset() override {}

    std::string getName() const override { return "Stereo Width"; }

private:
    float width_ = 1.0f;
    ParameterSmoother smoother_;
};

#endif //EQUALIZERAPP_STEREOWIDTHMODULE_H
