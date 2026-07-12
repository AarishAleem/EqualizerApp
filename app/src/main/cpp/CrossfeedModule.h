#ifndef EQUALIZERAPP_CROSSFEEDMODULE_H
#define EQUALIZERAPP_CROSSFEEDMODULE_H

#include "DSPModule.h"
#include "ParameterSmoother.h"

class CrossfeedModule : public DSPModule {
public:
    void prepare(const ProcessSpec& spec) override {
        smoother_.reset(crossfeed_, spec.sampleRate, 20.0f);
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

    void process(ProcessContext& context) override {
        if (state_ == ModuleState::Bypassed) return;

        for (uint32_t i = 0; i < context.numFrames; ++i) {
            float l = context.left[i];
            float r = context.right[i];
            float c = smoother_.getNextValue();

            context.left[i] = l + (r * c);
            context.right[i] = r + (l * c);
        }
    }

    void reset() override {
        smoother_.skip();
    }

    std::string getName() const override { return "Crossfeed"; }

    std::unique_ptr<DSPModule> clone() const override {
        auto c = std::make_unique<CrossfeedModule>();
        c->crossfeed_ = this->crossfeed_;
        c->state_ = this->state_;
        return c;
    }

private:
    float crossfeed_ = 0.0f;
    ParameterSmoother smoother_;
};

#endif //EQUALIZERAPP_CROSSFEEDMODULE_H
