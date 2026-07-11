#ifndef EQUALIZERAPP_PARAMETRICEQMODULE_H
#define EQUALIZERAPP_PARAMETRICEQMODULE_H

#include "DSPModule.h"
#include "Biquad.h"

class ParametricEQModule : public DSPModule {
public:
    ParametricEQModule(int numBands, double sampleRate)
        : eqL_(numBands, sampleRate), eqR_(numBands, sampleRate) {}

    void prepare(int sampleRate, int blockSize, int channels) override {
        eqL_.setSampleRate(static_cast<double>(sampleRate));
        eqR_.setSampleRate(static_cast<double>(sampleRate));

        // Update filters for all bands to reflect current sample rate
        for (int i = 0; i < 12; ++i) {
            updateBand(i);
        }
    }

    void setParameter(ParameterID id, ParameterValue value) override {
        uint32_t pid = static_cast<uint32_t>(id);
        if (pid < 100) return;

        int bandIdx = (pid - 100) / 10;
        int paramType = (pid - 100) % 10;

        if (bandIdx >= 0 && bandIdx < 12) {
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
        for (int i = 0; i < 12; ++i) {
            uint32_t base = 100 + i * 10;
            params.push_back(static_cast<ParameterID>(base + 0));
            params.push_back(static_cast<ParameterID>(base + 1));
            params.push_back(static_cast<ParameterID>(base + 2));
            params.push_back(static_cast<ParameterID>(base + 3));
        }
        return params;
    }

    void process(float* left, float* right, int numFrames) override {
        if (state_ == ModuleState::Bypassed) return;
        for (int i = 0; i < numFrames; ++i) {
            left[i] = eqL_.process(left[i]);
            right[i] = eqR_.process(right[i]);
        }
    }

    void reset() override {}

    std::string getName() const override { return "Parametric EQ"; }

private:
    void updateBand(int index) {
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
    BandCache configs_[12];
};

#endif //EQUALIZERAPP_PARAMETRICEQMODULE_H
