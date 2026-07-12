#ifndef EQUALIZERAPP_HIGHPASSMODULE_H
#define EQUALIZERAPP_HIGHPASSMODULE_H

#include "DSPModule.h"
#include "Biquad.h"

class HighPassModule : public DSPModule {
public:
    void prepare(const ProcessSpec& spec) override {
        sampleRate_ = spec.sampleRate;
        update();
    }

    void setParameter(ParameterID id, ParameterValue value) override {
        if (id == ParameterID::HighPassFreq) {
            frequency_ = value.val.f;
            update();
        }
    }

    std::vector<ParameterID> getOwnedParameters() const override {
        return { ParameterID::HighPassFreq };
    }

    void process(ProcessContext& context) override {
        if (state_ == ModuleState::Bypassed) return;

        for (uint32_t i = 0; i < context.numFrames; ++i) {
            context.left[i] = biquadL_.process(context.left[i]);
            context.right[i] = biquadR_.process(context.right[i]);
        }
    }

    void reset() override {
        // Reset biquad states if needed
    }

    std::string getName() const override { return "High Pass Filter"; }

    std::unique_ptr<DSPModule> clone() const override {
        auto c = std::make_unique<HighPassModule>();
        c->frequency_ = this->frequency_;
        c->state_ = this->state_;
        return c;
    }

private:
    void update() {
        biquadL_.configure(FilterType::HighPass, frequency_, 0.0, 0.707, sampleRate_);
        biquadR_.configure(FilterType::HighPass, frequency_, 0.0, 0.707, sampleRate_);
    }

    Biquad biquadL_, biquadR_;
    float frequency_ = 20.0f;
    double sampleRate_ = 48000;
};

#endif //EQUALIZERAPP_HIGHPASSMODULE_H
