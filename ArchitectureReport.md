# PowerEQ Master: Architecture Report (Pre-Production Review)

## 1. DSP Processing Pipeline
The engine follows a strictly decoupled Command/Data flow model.

**High-Level Path:**
Android UI
    ↓
DSPConfigurationManager (Kotlin)
    ↓
JNI Bridge (native-lib.cpp)
    ↓
ParameterManager (Native)
    ↓
ParameterQueue (SPSC Lock-Free)
    ↓
Audio Thread (onAudioReady callback)
    ↓
ParameterDispatcher (Internal to AudioEngine)
    ↓
ModuleRegistry (Routing)
    ↓
DSPPipeline (Execution)
    ↓
DSPModules (Processing)
    ↓
Output (Oboe Stream)

## 2. Module Registration Flow
- **Creation:** Modules are created as `std::unique_ptr` inside the `AudioEngine` constructor.
- **Ownership:** The `DSPPipeline` instance owns the modules.
- **Registration:** Modules register themselves with the `ModuleRegistry` during initialization by declaring their `ownedParameters`.
- **Destruction:** Managed automatically via RAII through the `unique_ptr` in the pipeline.

## 3. Parameter Flow
- **UI Thread:** Submits float updates to `ParameterManager`.
- **Queue:** Events are stored in a fixed-size ring buffer (SPSC).
- **Audio Thread:** At the start of each block, the thread drains the queue.
- **Registry:** Routes specific `ParameterID` events ONLY to the subscriber modules.

## 4. Thread Ownership
- **UI Thread:** Responsible for state persistence, UI reactivity, and generating Command events.
- **Audio Thread:** Exclusive ownership of the DSP state and audio buffers. Performant, deterministic, and lock-free.

## 5. Memory Ownership
- **AudioEngine:** Singleton orchestrator. Owns Pipeline, Registry, and Queue.
- **DSPPipeline:** Owns the `DSPModule` lifecycle.
- **ModuleRegistry:** Reference-only routing table (maps IDs to raw pointers).
- **ParameterManager:** Owns the contiguous parameter cache for UI synchronization.

## 6. Future Extension Points
The architecture is ready for:
- **Convolution:** Register a `ConvolutionModule` with the registry to handle IR swapping via `ParameterID`.
- **Dynamic EQ:** Insert an `EnvelopeFollower` into the pipeline and route its output to specific EQ bands via the registry.
- **Analyzers:** Add a `TapModule` at any point in the pipeline to copy buffers to a separate analysis thread without blocking.
