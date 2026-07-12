#include "AudioEngine.h"
#include <android/log.h>
#include <thread>
#include <algorithm>

#include "PreampModule.h"
#include "HighPassModule.h"
#include "ParametricEQModule.h"
#include "StereoWidthModule.h"
#include "CrossfeedModule.h"
#include "OutputGainModule.h"

#define TAG "AudioEngine"

AudioEngine::AudioEngine() : parameterManager_(queue_) {
    arenaCapacity_ = 2048;
    leftArena_ = new float[arenaCapacity_];
    rightArena_ = new float[arenaCapacity_];

    currentStreamSpec_ = {48000.0, arenaCapacity_, 2};

    buildAndInstallDefaultGraph();
}

AudioEngine::~AudioEngine() {
    quiesceAndStop();

    // Final cleanup
    ExecutionPlan* p = activePlan_.exchange(nullptr, std::memory_order_relaxed);
    delete p;

    std::lock_guard<std::mutex> lock(retiredPlansMutex_);
    for (auto& rp : retiredPlans_) {
        delete rp.plan;
    }
    retiredPlans_.clear();

    delete[] leftArena_;
    delete[] rightArena_;
}

void AudioEngine::quiesceAndStop() {
    desiredState_.store(EngineState::Stopped, std::memory_order_release);

    std::lock_guard<std::mutex> lock(streamMutex_);
    if (stream_) {
        stream_->requestStop();
        stream_->close();
        stream_.reset();
    }

    // Ensure no reader is active
    while (readerActiveGeneration_.load(std::memory_order_acquire) != kReaderInactive) {
        std::this_thread::yield();
    }
}

void AudioEngine::buildAndInstallDefaultGraph() {
    AudioGraph graph;

    graph.addNode(10, std::make_unique<PreampModule>());
    graph.addNode(20, std::make_unique<HighPassModule>());
    graph.addNode(30, std::make_unique<ParametricEQModule>(12));
    graph.addNode(40, std::make_unique<StereoWidthModule>());
    graph.addNode(50, std::make_unique<CrossfeedModule>());
    graph.addNode(60, std::make_unique<OutputGainModule>());

    graph.setLink(10, 20);
    graph.setLink(20, 30);
    graph.setLink(30, 40);
    graph.setLink(40, 50);
    graph.setLink(50, 60);

    installGraph(graph);
}

CompilationResult AudioEngine::installGraph(AudioGraph& graph) {
    GraphCompiler compiler;
    auto result = compiler.compile(graph, currentStreamSpec_);

    if (result.isSuccess()) {
        std::lock_guard<std::mutex> lock(publicationMutex_);

        ExecutionPlan* newPlan = result.plan.release();

        // 1. Enter Publication Transition (ODD)
        publicationSequence_.fetch_add(1, std::memory_order_acq_rel);

        // 2. Publish New Plan
        ExecutionPlan* oldPlan = activePlan_.exchange(newPlan, std::memory_order_acq_rel);

        // 3. Complete Publication Transition (EVEN)
        uint64_t stableSeq = publicationSequence_.fetch_add(1, std::memory_order_release) + 1;

        if (oldPlan) {
            std::lock_guard<std::mutex> retiredLock(retiredPlansMutex_);
            retiredPlans_.push_back({oldPlan, stableSeq});
        }

        reclaimRetiredPlans();
    }
    return result;
}

void AudioEngine::reclaimRetiredPlans() {
    // Current state of the audio reader
    uint64_t currentReaderGen = readerActiveGeneration_.load(std::memory_order_acquire);

    std::lock_guard<std::mutex> lock(retiredPlansMutex_);

    while (!retiredPlans_.empty()) {
        const auto& rp = retiredPlans_.front();

        // A retired plan is safe to delete if:
        // 1. No reader is active (gen == 0)
        // 2. The active reader holds a generation NEWER than the retirement generation
        if (currentReaderGen == kReaderInactive || currentReaderGen > rp.retirementGeneration) {
            delete rp.plan;
            retiredPlans_.pop_front();
        } else {
            break;
        }
    }
}

ExecutionPlan* AudioEngine::acquirePlanForAudioThread() {
    // Blocker 3: Bounded retry without yield
    for (int i = 0; i < 3; ++i) {
        // 1. Read Publication Sequence
        uint64_t s1 = publicationSequence_.load(std::memory_order_acquire);

        // 2. Publication in progress? (Odd value)
        if (s1 % 2 != 0) {
            continue;
        }

        // 3. Announce Reader Generation
        readerActiveGeneration_.store(s1, std::memory_order_release);

        // 4. Load Active Plan
        ExecutionPlan* plan = activePlan_.load(std::memory_order_acquire);

#ifdef POWEREQ_NATIVE_TESTS
        // Hook for deterministic race testing
        if (testPauseAcquisition_.load()) {
            testAcquisitionReachedPause_.store(true);
            while (!testResumeAcquisition_.load()) {
                // Spinning in test mode is allowed for synchronization
            }
        }
#endif

        // 5. Re-read Sequence to ensure stability
        uint64_t s2 = publicationSequence_.load(std::memory_order_acquire);

        if (s1 == s2) {
            return plan;
        }

        // 6. Collision: Clear announcement and retry
        readerActiveGeneration_.store(kReaderInactive, std::memory_order_release);
    }
    return nullptr; // Bounded failure results in silence for this block
}

void AudioEngine::releasePlanForAudioThread() {
    readerActiveGeneration_.store(kReaderInactive, std::memory_order_release);
}

void AudioEngine::start() {
    std::lock_guard<std::mutex> lock(streamMutex_);
    if (desiredState_.load(std::memory_order_acquire) == EngineState::Running) return;

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

    updateStreamSpec();

    desiredState_.store(EngineState::Running, std::memory_order_release);
    stream_->requestStart();
}

void AudioEngine::stop() {
    quiesceAndStop();
    reclaimRetiredPlans();
}

void AudioEngine::updateStreamSpec() {
    if (!stream_) return;

    ProcessSpec newSpec;
    newSpec.sampleRate = stream_->getSampleRate();
    newSpec.numChannels = stream_->getChannelCount();
    newSpec.maxFramesPerBlock = stream_->getFramesPerBurst();

    if (newSpec != currentStreamSpec_) {
        currentStreamSpec_ = newSpec;
        buildAndInstallDefaultGraph();
    }
}

oboe::DataCallbackResult AudioEngine::onAudioReady(
        oboe::AudioStream *audioStream,
        void *audioData,
        int32_t numFrames) {

    ExecutionPlan* plan = acquirePlanForAudioThread();

    float* output = static_cast<float *>(audioData);

    // Blocker 3: Handle null plan (bypass/silence)
    if (!plan || !enabled_.load(std::memory_order_relaxed)) {
        if (output) {
            for (int i = 0; i < numFrames * 2; ++i) output[i] = 0.0f;
        }
        releasePlanForAudioThread();
        return oboe::DataCallbackResult::Continue;
    }

    // 1. Drain Parameter Queue
    ParameterEvent event;
    while (queue_.pop(event)) {
        plan->getRegistry().dispatch(event.id, event.value);
    }

    // 2. Setup Context
    ProcessContext context;
    context.numFrames = static_cast<uint32_t>(numFrames);

    if (context.numFrames > arenaCapacity_) {
        boundViolationCount_.fetch_add(1, std::memory_order_relaxed);
        if (output) {
            for (int i = 0; i < numFrames * 2; ++i) output[i] = 0.0f;
        }
        releasePlanForAudioThread();
        return oboe::DataCallbackResult::Continue;
    }

    for (int i = 0; i < numFrames; ++i) {
        leftArena_[i] = 0.0f;
        rightArena_[i] = 0.0f;
    }

    context.left = leftArena_;
    context.right = rightArena_;

    // 3. Execute Graph
    executor_.execute(plan, context);

    // 4. Re-interleave
    if (output) {
        for (int i = 0; i < numFrames; ++i) {
            output[i * 2] = leftArena_[i];
            output[i * 2 + 1] = rightArena_[i];
        }
    }

    releasePlanForAudioThread();

    return oboe::DataCallbackResult::Continue;
}
