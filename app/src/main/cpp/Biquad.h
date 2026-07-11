#ifndef EQUALIZERAPP_BIQUAD_H
#define EQUALIZERAPP_BIQUAD_H

#include <cmath>
#include <vector>
#include <complex>

enum class FilterType {
    Peak = 0,
    LowShelf = 1,
    HighShelf = 2,
    LowPass = 3,
    HighPass = 4,
    BandPass = 5,
    Notch = 6
};

class Biquad {
public:
    Biquad();
    void configure(FilterType type, double freq, double gainDb, double Q, double sampleRate);
    float process(float sample);
    void processBuffer(const float* input, float* output, int numSamples);
    double getMagnitude(double freq, double sampleRate);

private:
    void updateCoefficients();

    FilterType type_;
    double freq_;
    double gainDb_;
    double Q_;
    double sampleRate_;

    // Coefficients
    double b0, b1, b2, a0, a1, a2;
    // State variables
    double z1, z2;
};

class MultiBandEqualizer {
public:
    MultiBandEqualizer(int numBands, double sampleRate);
    void setBand(int index, FilterType type, double freq, double gainDb, double Q);
    void setSampleRate(double sampleRate);
    float process(float sample);
    void process(const float* input, float* output, int numSamples, float masterGainDb);

private:
    std::vector<Biquad> bands_;
    double sampleRate_;
};

#endif //EQUALIZERAPP_BIQUAD_H
