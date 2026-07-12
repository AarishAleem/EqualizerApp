#ifndef EQUALIZERAPP_RESPONSETESTS_H
#define EQUALIZERAPP_RESPONSETESTS_H

#include "../Biquad.h"
#include "../FrequencyResponseAnalyzer.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

#define EXPECT_NEAR(actual, expected, tol, message) \
    do { \
        if (std::abs((actual) - (expected)) > (tol)) { \
            std::cerr << "FAIL: " << message << " (Expected " << expected << ", got " << actual << ")" << std::endl; \
            return false; \
        } \
    } while (0)

class ResponseTests {
public:
    static bool runAll() {
        bool success = true;
        success &= testGoldenPeakingEQ();
        success &= testMultiBandCascade();
        return success;
    }

private:
    static bool testGoldenPeakingEQ() {
        const double fs = 48000.0;
        const double f0 = 1000.0;
        const float gain = 6.0f;

        auto getWidth = [&](float Q) -> double {
            Biquad b;
            b.configure(FilterType::Peak, f0, gain, Q, fs);

            // Numerical bandwidth at +3dB (half boost)
            double target = 3.0;
            double fLow = 0, fHigh = 0;

            // Search lower
            for (double f = f0; f > 20; f -= 1.0) {
                if (b.getMagnitude(f, fs) < target) {
                    fLow = f;
                    break;
                }
            }
            // Search higher
            for (double f = f0; f < 20000; f += 1.0) {
                if (b.getMagnitude(f, fs) < target) {
                    fHigh = f;
                    break;
                }
            }
            return std::log2(fHigh / fLow);
        };

        double w05 = getWidth(0.5f);
        double w10 = getWidth(1.0f);
        double w50 = getWidth(5.0f);

        std::cout << "Bandwidth (Octaves) @ 3dB: Q0.5=" << w05 << ", Q1.0=" << w10 << ", Q5.0=" << w50 << std::endl;

        if (!(w05 > w10 && w10 > w50)) {
            std::cerr << "FAIL: Bandwidth narrowing failed" << std::endl;
            return false;
        }

        return true;
    }

    static bool testMultiBandCascade() {
        FrequencyResponseAnalyzer analyzer;
        std::vector<float> freqs = { 1000.0f };
        std::vector<float> bFreqs = { 1000.0f, 1000.0f };
        std::vector<float> bGains = { 6.0f, 6.0f };
        std::vector<float> bQs = { 1.0f, 1.0f };
        std::vector<int> bTypes = { static_cast<int>(FilterType::Peak), static_cast<int>(FilterType::Peak) };

        auto res = analyzer.calculateResponse(freqs, bFreqs, bGains, bQs, bTypes, 0.0f);

        // Cascade of two +6dB peaks should be +12dB
        EXPECT_NEAR(res[0], 12.0f, 0.1f, "Multi-band cascade @ 1kHz");

        return true;
    }
};

#endif //EQUALIZERAPP_RESPONSETESTS_H
