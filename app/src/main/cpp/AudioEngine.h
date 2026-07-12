#ifndef EQUALIZERAPP_AUDIOENGINE_H
#define EQUALIZERAPP_AUDIOENGINE_H

#include <oboe/Oboe.h>
#include <atomic>
#include <mutex>
#include "GraphExecutor.h"
#include "GraphCompiler.h"
#include "ParameterManager.h"
#include <deque>

/**
 * High-level Engine States.
 */
enum class EngineState {
    Stopped,
    Running
};

/**
 * The native audio orchestrator.
 * Connects Oboe transport to the modular processing graph.
 */
class AudioEngine : public oboe::AudioStreamDataCallback {
    friend class GraphTests;
public:
    AudioEngine();
    ~AudioEngine();

    // Lifecycle
    void start();
    void stop();

    // Graph Management
    CompilationResult installGraph(AudioGraph& graph);

    // Real-time audio callback
    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream *audioStream,
            void *audioData,
            int32_t numFrames) override;

    // Parameter access
    ParameterManager& getParameterManager() { return parameterManager_; }
    void setEnabled(bool enabled) { enabled_.store(enabled); }

    // Internal lifetime protocol
    ExecutionPlan* acquirePlanForAudioThread();
    void releasePlanForAudioThread();

private:
    void updateStreamSpec();
    void buildAndInstallDefaultGraph();

    // Quiescence and Reclamation
    void reclaimRetiredPlans();
    void quiesceAndStop();

    // Oboe
    std::shared_ptr<oboe::AudioStream> stream_;
    ProcessSpec currentStreamSpec_;

    // Concurrency / Lifecycle
    std::atomic<EngineState> desiredState_{EngineState::Stopped};
    std::mutex streamMutex_;
    std::mutex publicationMutex_; // Serializes control-side plan publication

    // Graph / Execution
    std::atomic<ExecutionPlan*> activePlan_{nullptr};

    struct RetiredPlan {
        ExecutionPlan* plan;
        uint64_t retirementGeneration; // Stable even sequence value
    };

    // Retired plans waiting for reclamation
    std::mutex retiredPlansMutex_;
    std::deque<RetiredPlan> retiredPlans_;

    // Single-Reader Quiescence Protocol (Seqlock-inspired)
    // 1. Control increments this to ODD when starting publication
    // 2. Control increments this to EVEN when publication is stable
    std::atomic<uint64_t> publicationSequence_{0};

    // Announced generation of the single audio reader
    // Written by Audio Thread, Read by Control Thread
    std::atomic<uint64_t> readerActiveGeneration_{0};

    // Constant for inactive reader
    static constexpr uint64_t kReaderInactive = 0;

    GraphExecutor executor_;

    // Legacy support infrastructure
    ParameterQueue queue_;
    ParameterManager parameterManager_;

    // Performance Diagnostics
    std::atomic<uint64_t> boundViolationCount_{0};

    // Buffer management
    float* leftArena_ = nullptr;
    float* rightArena_ = nullptr;
    uint32_t arenaCapacity_ = 0;

    std::atomic<bool> enabled_{true};

#ifdef POWEREQ_NATIVE_TESTS
public:
    // Hooks for deterministic concurrency testing
    std::atomic<bool> testPauseAcquisition_{false};
    std::atomic<bool> testResumeAcquisition_{false};
    std::atomic<bool> testAcquisitionReachedPause_{false};
#endif
};

#endif //EQUALIZERAPP_AUDIOENGINE_H
