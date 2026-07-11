#ifndef EQUALIZERAPP_HIGHPASSMODULE_H
#define EQUALIZERAPP_HIGHPASSMODULE_H

#include "DSPModule.h"
#include "Biquad.h"

class HighPassModule : public DSPModule {
public:
    void prepare(int sampleRate, int blockSize, int channels) override {
        sampleRate_ = sampleRate;
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

    void process(float* left, float* right, int numFrames) override {
        if (state_ == ModuleState::Bypassed) return;
        for (int i = 0; i < numFrames; ++i) {
            left[i] = biquadL_.process(left[i]);
            right[i] = biquadR_.process(right[i]);
        }
    }

    void reset() override {}

    std::string getName() const override { return "High Pass Filter"; }

private:
    void update() {
        biquadL_.configure(FilterType::HighPass, frequency_, 0.0, 0.707, sampleRate_);
        biquadR_.configure(FilterType::HighPass, frequency_, 0.0, 0.707, sampleRate_);
    }

    Biquad biquadL_, biquadR_;
    float frequency_ = 20.0f;
    int sampleRate_ = 48000;
};

#endif //EQUALIZERAPP_HIGHPASSMODULE_H
