#include <jni.h>
#include <vector>
#include <android/log.h>
#include <mutex>
#include <memory>
#include <atomic>
#include "AudioEngine.h"
#include "FrequencyResponseAnalyzer.h"

#define TAG "EqualizerNative"

/**
 * Generation-Safe JNI Handle Registry.
 * Replaces raw pointer handles with monotonic IDs.
 */
class JNIHandleRegistry {
public:
    static JNIHandleRegistry& instance() {
        static JNIHandleRegistry i;
        return i;
    }

    jlong createEngine() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (activeEngine_) {
            __android_log_print(ANDROID_LOG_ERROR, TAG, "Rejecting duplicate engine creation.");
            return 0;
        }

        activeEngine_ = std::make_unique<AudioEngine>();
        activeHandleId_ = ++nextHandleId_;
        return static_cast<jlong>(activeHandleId_);
    }

    void destroyEngine(jlong handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isValid(handle)) {
            activeEngine_.reset();
            activeHandleId_ = 0;
        } else {
             __android_log_print(ANDROID_LOG_ERROR, TAG, "Rejecting destroy of invalid handle: %lld", handle);
        }
    }

    AudioEngine* lookup(jlong handle) {
        // Must be called with lock held or via withEngine helper
        if (isValid(handle)) {
            return activeEngine_.get();
        }
        return nullptr;
    }

    std::mutex& getMutex() { return mutex_; }

private:
    bool isValid(jlong handle) const {
        return handle != 0 && handle == static_cast<jlong>(activeHandleId_);
    }

    std::mutex mutex_;
    std::unique_ptr<AudioEngine> activeEngine_;
    uint64_t activeHandleId_ = 0;
    uint64_t nextHandleId_ = 0;
};

template<typename F>
static void withEngine(jlong handle, F&& func) {
    auto& reg = JNIHandleRegistry::instance();
    std::lock_guard<std::mutex> lock(reg.getMutex());
    AudioEngine* engine = reg.lookup(handle);
    if (engine) {
        func(engine);
    } else {
        __android_log_print(ANDROID_LOG_WARN, TAG, "JNI call with stale handle: %lld", handle);
    }
}

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

extern "C" JNIEXPORT jlong JNICALL
Java_com_example_equalizerapp_EqualizerService_createNativeEngine(JNIEnv* env, jobject /* this */, jint numBands, jdouble sampleRate) {
    return JNIHandleRegistry::instance().createEngine();
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_destroyNativeEngine(JNIEnv* env, jobject /* this */, jlong handle) {
    JNIHandleRegistry::instance().destroyEngine(handle);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_configureNativeBand(
        JNIEnv* env, jobject /* this */, jlong handle, jint index, jint type, jdouble freq, jdouble gain, jdouble q) {
    withEngine(handle, [&](AudioEngine* engine) {
        auto& pm = engine->getParameterManager();
        uint32_t base = 100 + index * 10;
        pm.setParameter(static_cast<ParameterID>(base + 0), static_cast<float>(type));
        pm.setParameter(static_cast<ParameterID>(base + 1), static_cast<float>(freq));
        pm.setParameter(static_cast<ParameterID>(base + 2), static_cast<float>(gain));
        pm.setParameter(static_cast<ParameterID>(base + 3), static_cast<float>(q));
    });
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_setNativeMasterGain(JNIEnv* env, jobject /* this */, jlong handle, jfloat gain) {
    withEngine(handle, [&](AudioEngine* engine) {
        engine->getParameterManager().setParameter(ParameterID::PreampGain, gain);
    });
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_setNativeSurround(JNIEnv* env, jobject /* this */, jlong handle, jfloat width, jfloat crossfeed) {
    withEngine(handle, [&](AudioEngine* engine) {
        engine->getParameterManager().setParameter(ParameterID::StereoWidth, width);
        engine->getParameterManager().setParameter(ParameterID::Crossfeed, crossfeed);
    });
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_setNativeEnabled(JNIEnv* env, jobject /* this */, jlong handle, jboolean enabled) {
    withEngine(handle, [&](AudioEngine* engine) {
        engine->setEnabled(enabled == JNI_TRUE);
    });
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_startNativeStream(JNIEnv* env, jobject /* this */, jlong handle) {
    withEngine(handle, [&](AudioEngine* engine) {
        engine->start();
    });
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_equalizerapp_EqualizerService_stopNativeStream(JNIEnv* env, jobject /* this */, jlong handle) {
    withEngine(handle, [&](AudioEngine* engine) {
        engine->stop();
    });
}

#ifdef POWEREQ_NATIVE_TESTS
#include "tests/GraphTests.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_example_equalizerapp_EqualizerService_runNativeTests(JNIEnv* env, jobject /* this */) {
    return static_cast<jint>(GraphTests::runAll());
}
#endif
