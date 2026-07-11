#ifndef EQUALIZERAPP_AUDIOENGINE_H
#define EQUALIZERAPP_AUDIOENGINE_H

#include <oboe/Oboe.h>
#include "DSPPipeline.h"
#include "ModuleRegistry.h"
#include "ParameterManager.h"

/**
 * The core native audio engine using Oboe.
 * Orchestrates the DSP pipeline and handles the real-time audio callback.
 */
class AudioEngine : public oboe::AudioStreamDataCallback {
public:
    AudioEngine();
    ~AudioEngine();

    void start();
    void stop();

    // Real-time audio callback
    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream *audioStream,
            void *audioData,
            int32_t numFrames) override;

    // External parameter interface (UI thread)
    ParameterManager& getParameterManager() { return parameterManager_; }

private:
    void initializePipeline();

    std::shared_ptr<oboe::AudioStream> stream_;

    // Parameter Management System
    ParameterQueue queue_;
    ParameterManager parameterManager_;

    // Modular Routing and Processing
    ModuleRegistry registry_;

    /**
     * The DSP Pipeline owns all DSPModule instances.
     * Ownership: AudioEngine -> DSPPipeline -> std::vector<std::unique_ptr<DSPModule>>
     */
    DSPPipeline pipeline_;

    // Audio stream configuration (Source of truth negotiated at runtime)
    int sampleRate_ = 48000;
    int blockSize_ = 512;
    int channels_ = 2;

    bool enabled_ = true;
};

#endif //EQUALIZERAPP_AUDIOENGINE_H
