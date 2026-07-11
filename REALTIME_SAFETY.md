# Real-Time Safety Specification

## 1. The Audio Callback Contract
The `AudioEngine::onAudioReady` callback (and all transitive calls) MUST follow strict real-time constraints to prevent audio glitches (underruns).

### Prohibited Operations:
*   **Memory Management:** No `malloc`, `free`, `new`, `delete`.
*   **Synchronization:** No `std::mutex`, `std::lock_guard`, `std::condition_variable`.
*   **I/O:** No file system access, network access, or `__android_log_print`.
*   **JNI:** No calls into Java/Kotlin from the callback thread.
*   **Blocking:** No `sleep`, `yield`, or long-running non-deterministic loops.
*   **Exceptions:** No throwing or catching exceptions.

## 2. Callback Call Graph
```
AudioEngine::onAudioReady
├── ExecutionPlan::load (atomic)
├── ParameterQueue::pop (lock-free)
│   └── ModuleRegistry::dispatch (O(1) lookup)
│       └── DSPModule::setParameter
├── ProcessContext setup (stack only)
├── GraphExecutor::execute
│   └── DSPModule::process
│       ├── PreampModule::process (arithmetic only)
│       ├── HighPassModule::process (arithmetic only)
│       ├── ParametricEQModule::process (arithmetic only)
│       ├── StereoWidthModule::process (arithmetic only)
│       ├── CrossfeedModule::process (arithmetic only)
│       └── OutputGainModule::process (arithmetic only)
└── Re-interleaving loop (arithmetic only)
```

## 3. Verified Constraints
*   **Allocation:** PASS. All memory (Arena, Modules, Queue) is pre-allocated during initialization or `installGraph`.
*   **Locking:** PASS. Inter-thread communication is handled via a Lock-Free SPSC Ring Buffer.
*   **Determinism:** PASS. The execution plan is pre-compiled and executed sequentially.
