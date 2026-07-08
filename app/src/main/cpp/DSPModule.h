#ifndef EQUALIZERAPP_DSPMODULE_H
#define EQUALIZERAPP_DSPMODULE_H

#include <vector>
#include <string>

// Abstract base class for all DSP modules
class DSPModule {
public:
    virtual ~DSPModule() = default;

    // Process a single stereo frame (L and R)
    virtual void process(float& left, float& right) = 0;

    virtual void setEnabled(bool enabled) { enabled_ = enabled; }
    virtual bool isEnabled() const { return enabled_; }
    virtual std::string getName() const = 0;

protected:
    bool enabled_ = true;
};

// Simple Preamp Module (the first step in our chain)
class PreampModule : public DSPModule {
public:
    void setGain(float gainDb) {
        gainLinear_ = powf(10.0f, gainDb / 20.0f);
    }

    void process(float& left, float& right) override {
        if (!enabled_) return;
        left *= gainLinear_;
        right *= gainLinear_;
    }

    std::string getName() const override { return "Preamp"; }

private:
    float gainLinear_ = 1.0f;
};

#endif //EQUALIZERAPP_DSPMODULE_H
