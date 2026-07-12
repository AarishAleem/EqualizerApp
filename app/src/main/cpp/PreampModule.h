#ifndef EQUALIZERAPP_PREAMPMODULE_H
#define EQUALIZERAPP_PREAMPMODULE_H

#include "DSPModule.h"
#include "ParameterSmoother.h"
#include <cmath>

class PreampModule : public DSPModule {
public:
    void prepare(const ProcessSpec& spec) override {
        smoother_.reset(gainLinear_, spec.sampleRate, 10.0f);
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

    std::string getName() const override { return "Preamp"; }

    std::unique_ptr<DSPModule> clone() const override {
        auto c = std::make_unique<PreampModule>();
        c->gainLinear_ = this->gainLinear_;
        c->state_ = this->state_;
        return c;
    }

private:
    float gainLinear_ = 1.0f;
    ParameterSmoother smoother_;
};

#endif //EQUALIZERAPP_PREAMPMODULE_H
