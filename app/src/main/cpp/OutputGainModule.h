#ifndef EQUALIZERAPP_OUTPUTGAINMODULE_H
#define EQUALIZERAPP_OUTPUTGAINMODULE_H

#include "DSPModule.h"
#include "ParameterSmoother.h"
#include <cmath>

class OutputGainModule : public DSPModule {
public:
    void prepare(const ProcessSpec& spec) override {
        smoother_.reset(gainLinear_, spec.sampleRate, 10.0f);
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

    void process(ProcessContext& context) override {
        if (state_ == ModuleState::Bypassed) return;

        for (uint32_t i = 0; i < context.numFrames; ++i) {
            float g = smoother_.getNextValue();
            context.left[i] *= g;
            context.right[i] *= g;
        }
    }

    void reset() override {
        smoother_.skip();
    }

    std::string getName() const override { return "Output Gain"; }

    std::unique_ptr<DSPModule> clone() const override {
        auto c = std::make_unique<OutputGainModule>();
        c->gainLinear_ = this->gainLinear_;
        c->state_ = this->state_;
        return c;
    }

private:
    float gainLinear_ = 1.0f;
    ParameterSmoother smoother_;
};

#endif //EQUALIZERAPP_OUTPUTGAINMODULE_H
