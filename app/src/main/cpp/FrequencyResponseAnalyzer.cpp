#include "FrequencyResponseAnalyzer.h"
#include <cmath>

std::vector<float> FrequencyResponseAnalyzer::calculateResponse(
    const std::vector<float>& frequencies,
    const std::vector<float>& bandFreqs,
    const std::vector<float>& bandGains,
    const std::vector<float>& bandQs,
    const std::vector<int>& bandTypes,
    float masterGainDb
) {
    std::vector<float> response(frequencies.size(), masterGainDb);
    const double fs = 48000.0;

    Biquad tempBiquad;
    for (size_t i = 0; i < frequencies.size(); ++i) {
        double f = (double)frequencies[i];
        for (size_t b = 0; b < bandFreqs.size(); ++b) {
            tempBiquad.configure(
                static_cast<FilterType>(bandTypes[b]),
                bandFreqs[b],
                bandGains[b],
                bandQs[b],
                fs
            );
            response[i] += (float)tempBiquad.getMagnitude(f, fs);
        }
    }
    return response;
}
