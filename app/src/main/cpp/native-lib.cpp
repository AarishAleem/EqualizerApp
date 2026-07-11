#include <jni.h>
#include <vector>
#include <android/log.h>
#include "AudioEngine.h"
#include "FrequencyResponseAnalyzer.h"

#define TAG "EqualizerNative"

AudioEngine* gAudioEngine = nullptr;

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_example_equalizerapp_dsp_engine_DSPConfigurationManager_calculateFrequencyResponseNative(
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

    jfloat* freqsArr = env->GetFloatArrayElements(frequencies, nullptr);
    jfloat* bFreqsArr = env->GetFloatArrayElements(bandFreqs, nullptr);
    jfloat* bGainsArr = env->GetFloatArrayElements(bandGains, nullptr);
    jfloat* bQsArr = env->GetFloatArrayElements(bandQs, nullptr);
    jint* bTypesArr = env->GetIntArrayElements(bandTypes, nullptr);

    std::vector<float> freqs(freqsArr, freqsArr + freqCount);
    std::vector<float> bFreqs(bFreqsArr, bFreqsArr + bandCount);
    std::vector<float> bGains(bGainsArr, bGainsArr + bandCount);
    std::vector<float> bQs(bQsArr, bQsArr + bandCount);
    std::vector<int> bTypes(bTypesArr, bTypesArr + bandCount);

    std::vector<float> response = FrequencyResponseAnalyzer::calculateResponse(
        freqs, bFreqs, bGains, bQs, bTypes, masterGain
    );

    jfloatArray result = env->NewFloatArray(freqCount);
    env->SetFloatArrayRegion(result, 0, freqCount, response.data());

    env->ReleaseFloatArrayElements(frequencies, freqsArr, JNI_ABORT);
    env->ReleaseFloatArrayElements(bandFreqs, bFreqsArr, JNI_ABORT);
    env->ReleaseFloatArrayElements(bandGains, bGainsArr, JNI_ABORT);
    env->ReleaseFloatArrayElements(bandQs, bQsArr, JNI_ABORT);
    env->ReleaseIntArrayElements(bandTypes, bTypesArr, JNI_ABORT);

    return result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_createNativeEngine(JNIEnv* env, jobject /* this */, jint numBands, jdouble sampleRate) {
    if (gAudioEngine) delete gAudioEngine;
    gAudioEngine = new AudioEngine();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_configureNativeBand(
        JNIEnv* env, jobject /* this */, jint index, jint type, jdouble freq, jdouble gain, jdouble q) {
    if (gAudioEngine) {
        auto& pm = gAudioEngine->getParameterManager();
        uint32_t base = 100 + index * 10;

        pm.setParameter(static_cast<ParameterID>(base + 0), static_cast<float>(type));
        pm.setParameter(static_cast<ParameterID>(base + 1), static_cast<float>(freq));
        pm.setParameter(static_cast<ParameterID>(base + 2), static_cast<float>(gain));
        pm.setParameter(static_cast<ParameterID>(base + 3), static_cast<float>(q));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_setNativeMasterGain(JNIEnv* env, jobject /* this */, jfloat gain) {
    if (gAudioEngine) {
        gAudioEngine->getParameterManager().setParameter(ParameterID::PreampGain, gain);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_setNativeSurround(JNIEnv* env, jobject /* this */, jfloat width, jfloat crossfeed) {
    if (gAudioEngine) {
        gAudioEngine->getParameterManager().setParameter(ParameterID::StereoWidth, width);
        gAudioEngine->getParameterManager().setParameter(ParameterID::Crossfeed, crossfeed);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_setNativeEnabled(JNIEnv* env, jobject /* this */, jboolean enabled) {
    if (gAudioEngine) {
        gAudioEngine->getParameterManager().setParameter(ParameterID::Bypass, enabled ? 1.0f : 0.0f);
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
