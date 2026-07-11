#include "DSPPipeline.h"

void DSPPipeline::addModule(std::unique_ptr<DSPModule> module) {
    modules_.push_back(std::move(module));
}

void DSPPipeline::prepare(int sampleRate, int blockSize, int channels) {
    for (auto& module : modules_) {
        module->prepare(sampleRate, blockSize, channels);
    }
}

void DSPPipeline::process(float* left, float* right, int numFrames) {
    for (auto& module : modules_) {
        if (module->isEnabled()) {
            module->process(left, right, numFrames);
        }
    }
}

void DSPPipeline::reset() {
    for (auto& module : modules_) {
        module->reset();
    }
}
