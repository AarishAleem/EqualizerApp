#include <jni.h>
#include <string>
#include <vector>
#include <cmath>
#include <complex>
#include <android/log.h>

#define TAG "EqualizerNative"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

const double PI = 3.14159265358979323846;

struct FilterParams {
    int type; // 0: Peaking, 1: LP, 2: HP, 3: BP, 4: LS, 5: HS
    double f0;
    double gainDb;
    double Q;
};

// Precise BiQuad Magnitude calculation
double getMagnitude(double f, double fs, const FilterParams& p) {
    double w0 = 2.0 * PI * p.f0 / fs;
    double alpha = sin(w0) / (2.0 * p.Q);
    double A = pow(10.0, p.gainDb / 40.0);

    double b0, b1, b2, a0, a1, a2;

    switch (p.type) {
        case 1: // Low Pass
            b0 = (1.0 - cos(w0)) / 2.0;
            b1 = 1.0 - cos(w0);
            b2 = (1.0 - cos(w0)) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos(w0);
            a2 = 1.0 - alpha;
            break;
        case 2: // High Pass
            b0 = (1.0 + cos(w0)) / 2.0;
            b1 = -(1.0 + cos(w0));
            b2 = (1.0 + cos(w0)) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos(w0);
            a2 = 1.0 - alpha;
            break;
        case 3: // Band Pass
            b0 = alpha;
            b1 = 0;
            b2 = -alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos(w0);
            a2 = 1.0 - alpha;
            break;
        case 4: // Low Shelf
            b0 = A * ((A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha);
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos(w0));
            b2 = A * ((A + 1.0) - (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha);
            a0 = (A + 1.0) + (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos(w0));
            a2 = (A + 1.0) + (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha;
            break;
        case 5: // High Shelf
            b0 = A * ((A + 1.0) + (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha);
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos(w0));
            b2 = A * ((A + 1.0) + (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha);
            a0 = (A + 1.0) - (A - 1.0) * cos(w0) + 2.0 * sqrt(A) * alpha;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos(w0));
            a2 = (A + 1.0) - (A - 1.0) * cos(w0) - 2.0 * sqrt(A) * alpha;
            break;
        default: // Peaking (0)
            b0 = 1.0 + alpha * A;
            b1 = -2.0 * cos(w0);
            b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1 = -2.0 * cos(w0);
            a2 = 1.0 - alpha / A;
            break;
    }

    double w = 2.0 * PI * f / fs;
    std::complex<double> ejw = std::polar(1.0, w);
    std::complex<double> ej2w = std::polar(1.0, 2.0 * w);

    std::complex<double> num = b0 + b1 / ejw + b2 / ej2w;
    std::complex<double> den = a0 + a1 / ejw + a2 / ej2w;

    return 20.0 * log10(std::abs(num / den));
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_example_equalizerapp_MainActivity_calculateFrequencyResponse(
        JNIEnv* env,
        jobject /* this */,
        jfloatArray frequencies,
        jfloatArray bandFreqs,
        jfloatArray bandGains,
        jfloatArray bandQs,
        jintArray bandTypes,
        jfloat masterGain) {

    jsize freqCount = env->GetArrayLength(frequencies);
    jsize bandCount = env->GetArrayLength(bandFreqs);

    jfloat* freqs = env->GetFloatArrayElements(frequencies, nullptr);
    jfloat* bFreqs = env->GetFloatArrayElements(bandFreqs, nullptr);
    jfloat* bGains = env->GetFloatArrayElements(bandGains, nullptr);
    jfloat* bQs = env->GetFloatArrayElements(bandQs, nullptr);
    jint* bTypes = env->GetIntArrayElements(bandTypes, nullptr);

    std::vector<float> response(freqCount, masterGain);
    const float fs = 48000.0f;

    for (int i = 0; i < freqCount; ++i) {
        double f = (double)freqs[i];
        for (int b = 0; b < bandCount; ++b) {
            FilterParams p = { (int)bTypes[b], (double)bFreqs[b], (double)bGains[b], (double)bQs[b] };
            response[i] += (float)getMagnitude(f, fs, p);
        }
    }

    jfloatArray result = env->NewFloatArray(freqCount);
    env->SetFloatArrayRegion(result, 0, freqCount, response.data());

    env->ReleaseFloatArrayElements(frequencies, freqs, 0);
    env->ReleaseFloatArrayElements(bandFreqs, bFreqs, 0);
    env->ReleaseFloatArrayElements(bandGains, bGains, 0);
    env->ReleaseFloatArrayElements(bandQs, bQs, 0);
    env->ReleaseIntArrayElements(bandTypes, bTypes, 0);

    return result;
}
