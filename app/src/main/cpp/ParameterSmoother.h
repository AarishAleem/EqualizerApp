#ifndef EQUALIZERAPP_PARAMETERSMOOTHER_H
#define EQUALIZERAPP_PARAMETERSMOOTHER_H

#include <cmath>

/**
 * A real-time safe linear parameter smoother.
 * Prevents "zipper noise" by interpolating changes over a specific number of samples.
 */
class ParameterSmoother {
public:
    ParameterSmoother() = default;

    /**
     * Initializes the smoother state.
     */
    void reset(float initialValue, double sampleRate, float timeMs) {
        currentValue_ = initialValue;
        targetValue_ = initialValue;
        steps_ = static_cast<size_t>(timeMs * 0.001 * sampleRate);
        remainingSteps_ = 0;
        stepSize_ = 0.0f;
    }

    /**
     * Sets a new target value to smooth towards.
     */
    void setTarget(float target) {
        if (target == targetValue_) return;
        targetValue_ = target;
        if (steps_ > 0) {
            stepSize_ = (targetValue_ - currentValue_) / static_cast<float>(steps_);
            remainingSteps_ = steps_;
        } else {
            currentValue_ = targetValue_;
            remainingSteps_ = 0;
        }
    }

    /**
     * Returns the next smoothed sample value.
     */
    inline float getNextValue() {
        if (remainingSteps_ > 0) {
            currentValue_ += stepSize_;
            --remainingSteps_;
            if (remainingSteps_ == 0) currentValue_ = targetValue_;
        }
        return currentValue_;
    }

    /**
     * Returns the current value (even if still smoothing).
     */
    inline float getCurrentValue() const { return currentValue_; }

    /**
     * Immediately snaps to the target value, bypassing smoothing.
     */
    void skip() {
        currentValue_ = targetValue_;
        remainingSteps_ = 0;
    }

private:
    float currentValue_ = 0.0f;
    float targetValue_ = 0.0f;
    float stepSize_ = 0.0f;
    size_t steps_ = 0;
    size_t remainingSteps_ = 0;
};

#endif //EQUALIZERAPP_PARAMETERSMOOTHER_H
