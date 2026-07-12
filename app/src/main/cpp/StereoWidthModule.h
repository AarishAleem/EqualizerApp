#ifndef EQUALIZERAPP_STEREOWIDTHMODULE_H
#define EQUALIZERAPP_STEREOWIDTHMODULE_H

#include "DSPModule.h"
#include "ParameterSmoother.h"

class StereoWidthModule : public DSPModule {
public:
    void prepare(const ProcessSpec& spec) override {
        smoother_.reset(width_, spec.sampleRate, 20.0f);
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

    void process(ProcessContext& context) override {
        if (state_ == ModuleState::Bypassed) return;

        for (uint32_t i = 0; i < context.numFrames; ++i) {
            float l = context.left[i];
            float r = context.right[i];
            float w = smoother_.getNextValue();

            float mid = (l + r) * 0.5f;
            float side = (l - r) * 0.5f;
            side *= w;

            context.left[i] = mid + side;
            context.right[i] = mid - side;
        }
    }

    void reset() override {
        smoother_.skip();
    }

    std::string getName() const override { return "Stereo Width"; }

    std::unique_ptr<DSPModule> clone() const override {
        auto c = std::make_unique<StereoWidthModule>();
        c->width_ = this->width_;
        c->state_ = this->state_;
        return c;
    }

private:
    float width_ = 1.0f;
    ParameterSmoother smoother_;
};

#endif //EQUALIZERAPP_STEREOWIDTHMODULE_H
