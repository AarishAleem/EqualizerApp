#ifndef EQUALIZERAPP_AUDIOENGINE_H
#define EQUALIZERAPP_AUDIOENGINE_H

#include <oboe/Oboe.h>
#include "Biquad.h"
#include "DSPModule.h"

class AudioEngine : public oboe::AudioStreamDataCallback {
public:
    AudioEngine();
    ~AudioEngine();

    void start();
    void stop();

    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream *audioStream,
            void *audioData,
            int32_t numFrames) override;

    void setBand(int index, FilterType type, double freq, double gainDb, double Q);
    void setMasterGain(float gainDb);
    void setSurround(float widthPercent, float crossfeedPercent);
    void setEnabled(bool enabled) { enabled_ = enabled; }

private:
    std::shared_ptr<oboe::AudioStream> stream_;

    // The Modular DSP Chain
    std::vector<std::unique_ptr<DSPModule>> modules_;

    // Pointers to specific modules for fast access/updating
    PreampModule* preamp_ = nullptr;
    // We will add more pointers as we implement HPF, EQ, etc.

    // Temporary: keeping these until EQ is modularized in next steps
    MultiBandEqualizer equalizerL_;
    MultiBandEqualizer equalizerR_;

    // Surround Params (Temporary until modularized)
    float stereoWidth_ = 1.0f;
    float crossfeed_ = 0.0f;

    bool enabled_ = true;

    // For testing: a simple oscillator
    double phase_ = 0.0;
    double phaseIncrement_ = 0.0;
};

#endif //EQUALIZERAPP_AUDIOENGINE_H
