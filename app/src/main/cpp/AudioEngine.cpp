#include "AudioEngine.h"
#include <android/log.h>

#include "PreampModule.h"
#include "HighPassModule.h"
#include "ParametricEQModule.h"
#include "StereoWidthModule.h"
#include "CrossfeedModule.h"
#include "OutputGainModule.h"

#define TAG "AudioEngine"

AudioEngine::AudioEngine() : parameterManager_(queue_) {
    initializePipeline();
}

AudioEngine::~AudioEngine() {
    stop();
}

void AudioEngine::initializePipeline() {
    /**
     * Ownership Model:
     * pipeline_ owns the unique_ptr to the module.
     * registry_ holds a raw pointer for routing (subscriber model).
     */

    auto preamp = std::make_unique<PreampModule>();
    registry_.registerModule(preamp.get());
    pipeline_.addModule(std::move(preamp));

    auto hpf = std::make_unique<HighPassModule>();
    registry_.registerModule(hpf.get());
    pipeline_.addModule(std::move(hpf));

    auto eq = std::make_unique<ParametricEQModule>(12, 48000.0);
    registry_.registerModule(eq.get());
    pipeline_.addModule(std::move(eq));

    auto width = std::make_unique<StereoWidthModule>();
    registry_.registerModule(width.get());
    pipeline_.addModule(std::move(width));

    auto crossfeed = std::make_unique<CrossfeedModule>();
    registry_.registerModule(crossfeed.get());
    pipeline_.addModule(std::move(crossfeed));

    auto outputGain = std::make_unique<OutputGainModule>();
    registry_.registerModule(outputGain.get());
    pipeline_.addModule(std::move(outputGain));

    // Initial prepare with defaults
    pipeline_.prepare(sampleRate_, blockSize_, channels_);
}

void AudioEngine::start() {
    oboe::AudioStreamBuilder builder;
    builder.setFormat(oboe::AudioFormat::Float)
        ->setChannelCount(oboe::ChannelCount::Stereo)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setSharingMode(oboe::SharingMode::Exclusive)
        ->setSampleRate(48000)
        ->setDataCallback(this);

    oboe::Result result = builder.openStream(stream_);
    if (result != oboe::Result::OK) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Error opening stream: %s", oboe::convertToText(result));
        return;
    }

    // Task 4: Source of Truth from Negotiated Stream
    sampleRate_ = stream_->getSampleRate();
    channels_ = stream_->getChannelCount();
    blockSize_ = stream_->getFramesPerBurst();

    // Re-prepare pipeline with actual values
    pipeline_.prepare(sampleRate_, blockSize_, channels_);

    result = stream_->requestStart();
    if (result != oboe::Result::OK) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Error starting stream: %s", oboe::convertToText(result));
    }
}

void AudioEngine::stop() {
    if (stream_) {
        stream_->requestStop();
        stream_->close();
        stream_.reset();
    }
}

oboe::DataCallbackResult AudioEngine::onAudioReady(
        oboe::AudioStream *audioStream,
        void *audioData,
        int32_t numFrames) {

    // --- Task 8: RT Audit - Lock-Free, No Allocations ---

    // 1. Drain Parameter Queue
    ParameterEvent event;
    while (queue_.pop(event)) {
        if (event.id == ParameterID::Bypass) {
            enabled_ = event.value.val.f > 0.5f;
        } else {
            // Task 1: Registry is the ONLY routing mechanism
            registry_.dispatch(event.id, event.value);
        }
    }

    // 2. Audio Processing
    float *output = static_cast<float *>(audioData);

    // Initial source: silence
    for (int i = 0; i < numFrames * 2; ++i) output[i] = 0.0f;

    if (enabled_) {
        // Task 8: Stack allocation is safe for small buffers
        static float leftBuffer[2048];
        static float rightBuffer[2048];
        int frames = std::min(numFrames, 2048);

        pipeline_.process(leftBuffer, rightBuffer, frames);

        for (int i = 0; i < frames; ++i) {
            output[i * 2] = leftBuffer[i];
            output[i * 2 + 1] = rightBuffer[i];
        }
    }

    return oboe::DataCallbackResult::Continue;
}
