#include <jni.h>
#include <string>
#include <vector>
#include <cmath>
#include <android/log.h>
#include "AudioEngine.h"

#define TAG "EqualizerNative"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

AudioEngine* gAudioEngine = nullptr;

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
    const double fs = 48000.0;

    Biquad temporaryBiquad;
    for (int i = 0; i < freqCount; ++i) {
        double f = (double)freqs[i];
        for (int b = 0; b < bandCount; ++b) {
            temporaryBiquad.configure((FilterType)bTypes[b], bFreqs[b], bGains[b], bQs[b], fs);
            response[i] += (float)temporaryBiquad.getMagnitude(f, fs);
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

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_createNativeEngine(JNIEnv* env, jobject /* this */, jint numBands, jdouble sampleRate) {
    if (gAudioEngine) delete gAudioEngine;
    gAudioEngine = new AudioEngine();
    LOGD("Native Audio Engine with Oboe created");
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_configureNativeBand(
        JNIEnv* env, jobject /* this */, jint index, jint type, jdouble freq, jdouble gain, jdouble q) {
    if (gAudioEngine) {
        gAudioEngine->setBand(index, (FilterType)type, freq, gain, q);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_setNativeMasterGain(JNIEnv* env, jobject /* this */, jfloat gain) {
    if (gAudioEngine) {
        gAudioEngine->setMasterGain(gain);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_setNativeSurround(JNIEnv* env, jobject /* this */, jfloat width, jfloat crossfeed) {
    if (gAudioEngine) {
        gAudioEngine->setSurround(width, crossfeed);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_setNativeEnabled(JNIEnv* env, jobject /* this */, jboolean enabled) {
    if (gAudioEngine) {
        gAudioEngine->setEnabled(enabled);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_startNativeStream(JNIEnv* env, jobject /* this */) {
    if (gAudioEngine) gAudioEngine->start();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_stopNativeStream(JNIEnv* env, jobject /* this */) {
    if (gAudioEngine) gAudioEngine->stop();
}
