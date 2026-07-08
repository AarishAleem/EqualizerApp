#include "AudioEngine.h"
#include <android/log.h>

#define TAG "AudioEngine"

AudioEngine::AudioEngine()
    : equalizerL_(12, 48000.0),
      equalizerR_(12, 48000.0) {

    // Initialize the chain
    auto preamp = std::make_unique<PreampModule>();
    preamp_ = preamp.get();
    modules_.push_back(std::move(preamp));

    // As we modularize HPF, EQ, etc., we will add them here in order
}

AudioEngine::~AudioEngine() {
    stop();
}

void AudioEngine::setBand(int index, FilterType type, double freq, double gainDb, double Q) {
    equalizerL_.setBand(index, type, freq, gainDb, Q);
    equalizerR_.setBand(index, type, freq, gainDb, Q);
}

void AudioEngine::setMasterGain(float gainDb) {
    if (preamp_) preamp_->setGain(gainDb);
}

void AudioEngine::setSurround(float widthPercent, float crossfeedPercent) {
    stereoWidth_ = 1.0f + (widthPercent / 100.0f);
    crossfeed_ = (crossfeedPercent / 333.3f);
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

    float *output = static_cast<float *>(audioData);

    for (int i = 0; i < numFrames; ++i) {
        float left = 0.0f;
        float right = 0.0f;

        if (!enabled_) {
            output[i * 2] = left;
            output[i * 2 + 1] = right;
            continue;
        }

        // 1. Process Modular Chain
        for (auto& module : modules_) {
            module->process(left, right);
        }

        // 2. Process non-modularized parts (Temporary)
        left = equalizerL_.process(left);
        right = equalizerR_.process(right);

        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;
        side *= stereoWidth_;
        left = mid + side;
        right = mid - side;

        float finalLeft = left + (right * crossfeed_);
        float finalRight = right + (left * crossfeed_);

        output[i * 2] = finalLeft;
        output[i * 2 + 1] = finalRight;
    }

    return oboe::DataCallbackResult::Continue;
}
