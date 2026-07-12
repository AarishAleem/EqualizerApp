#ifndef EQUALIZERAPP_PARAMETRICEQMODULE_H
#define EQUALIZERAPP_PARAMETRICEQMODULE_H

#include "DSPModule.h"
#include "Biquad.h"
#include <android/log.h>
#include <algorithm>

class ParametricEQModule : public DSPModule {
public:
    static constexpr int MAX_BANDS = 12;

    ParametricEQModule(int numBands)
        : eqL_(sanitizeBandCount(numBands), 48000.0),
          eqR_(sanitizeBandCount(numBands), 48000.0),
          numBands_(sanitizeBandCount(numBands)) {

        if (numBands > MAX_BANDS || numBands < 1) {
            __android_log_print(ANDROID_LOG_WARN, "ParametricEQ", "Requested %d bands, sanitized to %d", numBands, numBands_);
        }
    }

    void prepare(const ProcessSpec& spec) override {
        eqL_.setSampleRate(spec.sampleRate);
        eqR_.setSampleRate(spec.sampleRate);

        for (int i = 0; i < numBands_; ++i) {
            updateBand(i);
        }
    }

    void setParameter(ParameterID id, ParameterValue value) override {
        uint32_t pid = static_cast<uint32_t>(id);
        if (pid < 100) return;

        int bandIdx = (pid - 100) / 10;
        int paramType = (pid - 100) % 10;

        if (bandIdx >= 0 && bandIdx < numBands_) {
            auto& config = configs_[bandIdx];
            switch (paramType) {
                case 0: config.type = static_cast<FilterType>(static_cast<int>(value.val.f)); break;
                case 1: config.freq = value.val.f; break;
                case 2: config.gain = value.val.f; break;
                case 3: config.q = value.val.f; break;
                default: break;
            }
            updateBand(bandIdx);
        }
    }

    std::vector<ParameterID> getOwnedParameters() const override {
        std::vector<ParameterID> params;
        for (int i = 0; i < numBands_; ++i) {
            uint32_t base = 100 + i * 10;
            params.push_back(static_cast<ParameterID>(base + 0));
            params.push_back(static_cast<ParameterID>(base + 1));
            params.push_back(static_cast<ParameterID>(base + 2));
            params.push_back(static_cast<ParameterID>(base + 3));
        }
        return params;
    }

    void process(ProcessContext& context) override {
        if (state_ == ModuleState::Bypassed) return;

        for (uint32_t i = 0; i < context.numFrames; ++i) {
            context.left[i] = eqL_.process(context.left[i]);
            context.right[i] = eqR_.process(context.right[i]);
        }
    }

    void reset() override {}

    std::string getName() const override { return "Parametric EQ"; }

    std::unique_ptr<DSPModule> clone() const override {
        auto c = std::make_unique<ParametricEQModule>(numBands_);
        for (int i = 0; i < numBands_; ++i) {
            c->configs_[i] = this->configs_[i];
        }
        c->state_ = this->state_;
        return c;
    }

private:
    static int sanitizeBandCount(int n) {
        if (n < 1) return 1;
        if (n > MAX_BANDS) return MAX_BANDS;
        return n;
    }

    void updateBand(int index) {
        if (index < 0 || index >= MAX_BANDS) return;
        auto& c = configs_[index];
        eqL_.setBand(index, c.type, c.freq, c.gain, c.q);
        eqR_.setBand(index, c.type, c.freq, c.gain, c.q);
    }

    struct BandCache {
        FilterType type = FilterType::Peak;
        float freq = 1000.0f;
        float gain = 0.0f;
        float q = 0.707f;
    };

    MultiBandEqualizer eqL_;
    MultiBandEqualizer eqR_;
    BandCache configs_[MAX_BANDS];
    int numBands_;
};

#endif //EQUALIZERAPP_PARAMETRICEQMODULE_H
