#include "Biquad.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Biquad::Biquad() : type_(FilterType::Peak), freq_(1000.0), gainDb_(0.0), Q_(0.707), sampleRate_(48000.0),
                   b0(1.0), b1(0.0), b2(0.0), a0(1.0), a1(0.0), a2(0.0), z1(0.0), z2(0.0) {
    updateCoefficients();
}

void Biquad::configure(FilterType type, double freq, double gainDb, double Q, double sampleRate) {
    type_ = type;
    freq_ = freq;
    gainDb_ = gainDb;
    Q_ = Q;
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void Biquad::updateCoefficients() {
    double w0 = 2.0 * M_PI * freq_ / sampleRate_;
    double alpha = sin(w0) / (2.0 * Q_);
    double A = pow(10.0, gainDb_ / 40.0);
    double cosW0 = cos(w0);

    switch (type_) {
        case FilterType::LowPass:
            b0 = (1.0 - cosW0) / 2.0;
            b1 = 1.0 - cosW0;
            b2 = (1.0 - cosW0) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosW0;
            a2 = 1.0 - alpha;
            break;
        case FilterType::HighPass:
            b0 = (1.0 + cosW0) / 2.0;
            b1 = -(1.0 + cosW0);
            b2 = (1.0 + cosW0) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosW0;
            a2 = 1.0 - alpha;
            break;
        case FilterType::BandPass:
            b0 = alpha;
            b1 = 0;
            b2 = -alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosW0;
            a2 = 1.0 - alpha;
            break;
        case FilterType::Notch:
            b0 = 1.0;
            b1 = -2.0 * cosW0;
            b2 = 1.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosW0;
            a2 = 1.0 - alpha;
            break;
        case FilterType::Peak:
            b0 = 1.0 + alpha * A;
            b1 = -2.0 * cosW0;
            b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1 = -2.0 * cosW0;
            a2 = 1.0 - alpha / A;
            break;
        case FilterType::LowShelf:
            b0 = A * ((A + 1.0) - (A - 1.0) * cosW0 + 2.0 * sqrt(A) * alpha);
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
            b2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - 2.0 * sqrt(A) * alpha);
            a0 = (A + 1.0) + (A - 1.0) * cosW0 + 2.0 * sqrt(A) * alpha;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
            a2 = (A + 1.0) + (A - 1.0) * cosW0 - 2.0 * sqrt(A) * alpha;
            break;
        case FilterType::HighShelf:
            b0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + 2.0 * sqrt(A) * alpha);
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
            b2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - 2.0 * sqrt(A) * alpha);
            a0 = (A + 1.0) - (A - 1.0) * cosW0 + 2.0 * sqrt(A) * alpha;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
            a2 = (A + 1.0) - (A - 1.0) * cosW0 - 2.0 * sqrt(A) * alpha;
            break;
    }

    // Normalize coefficients
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
}

float Biquad::process(float sample) {
    double out = b0 * sample + z1;
    z1 = b1 * sample - a1 * out + z2;
    z2 = b2 * sample - a2 * out;
    return (float)out;
}

void Biquad::processBuffer(const float* input, float* output, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process(input[i]);
    }
}

double Biquad::getMagnitude(double freq, double sampleRate) {
    double w = 2.0 * M_PI * freq / sampleRate;
    std::complex<double> ejw = std::polar(1.0, -w);
    std::complex<double> ej2w = std::polar(1.0, -2.0 * w);

    std::complex<double> num = b0 + b1 * ejw + b2 * ej2w;
    std::complex<double> den = 1.0 + a1 * ejw + a2 * ej2w;

    return 20.0 * log10(std::abs(num / den));
}

MultiBandEqualizer::MultiBandEqualizer(int numBands, double sampleRate) : sampleRate_(sampleRate) {
    bands_.resize(numBands);
}

void MultiBandEqualizer::setBand(int index, FilterType type, double freq, double gainDb, double Q) {
    if (index >= 0 && index < (int)bands_.size()) {
        bands_[index].configure(type, freq, gainDb, Q, sampleRate_);
    }
}

float MultiBandEqualizer::process(float sample) {
    for (auto& band : bands_) {
        sample = band.process(sample);
    }
    return sample;
}

void MultiBandEqualizer::process(const float* input, float* output, int numSamples, float masterGainDb) {
    float masterGain = powf(10.0f, masterGainDb / 20.0f);
    for (int i = 0; i < numSamples; ++i) {
        output[i] = process(input[i] * masterGain);
    }
}
