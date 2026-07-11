#ifndef EQUALIZERAPP_FREQUENCYRESPONSEANALYZER_H
#define EQUALIZERAPP_FREQUENCYRESPONSEANALYZER_H

#include <vector>
#include "Biquad.h"

/**
 * Dedicated component for calculating frequency response curves.
 * Separated from the real-time AudioEngine.
 */
class FrequencyResponseAnalyzer {
public:
    static std::vector<float> calculateResponse(
        const std::vector<float>& frequencies,
        const std::vector<float>& bandFreqs,
        const std::vector<float>& bandGains,
        const std::vector<float>& bandQs,
        const std::vector<int>& bandTypes,
        float masterGainDb
    );
};

#endif //EQUALIZERAPP_FREQUENCYRESPONSEANALYZER_H
