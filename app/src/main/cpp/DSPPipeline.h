#ifndef EQUALIZERAPP_DSPPIPELINE_H
#define EQUALIZERAPP_DSPPIPELINE_H

#include <vector>
#include <memory>
#include "DSPModule.h"

/**
 * Manages a sequential chain of DSP modules.
 * Responsible for initialization, execution, and resource management.
 */
class DSPPipeline {
public:
    DSPPipeline() = default;
    ~DSPPipeline() = default;

    // Fixed registration during initialization
    void addModule(std::unique_ptr<DSPModule> module);

    void prepare(int sampleRate, int blockSize, int channels);
    void process(float* left, float* right, int numFrames);
    void reset();

private:
    std::vector<std::unique_ptr<DSPModule>> modules_;
};

#endif //EQUALIZERAPP_DSPPIPELINE_H
