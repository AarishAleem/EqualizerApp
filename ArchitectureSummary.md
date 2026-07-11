# PowerEQ Master: Architecture Specification (v5 Final)

## 1. Core Architecture
The system uses a **Decoupled Configuration & Real-Time Processing** model.

### 1.1 Command Path (Control Flow)
- **UI/Kotlin**: Modifies `DSPConfigurationManager`.
- **JNI**: shallow bridge to `ParameterManager`.
- **ParameterManager**: Stores snapshots in a contiguous `std::array` (O(1)) and pushes events to an **SPSC Lock-Free Queue**.
- **Audio Thread**: Drains the queue at the start of every callback.

### 1.2 Processing Path (Hot Path)
- **ModuleRegistry**: routes `ParameterID` to specific `DSPModule` instances using a deterministic `std::array` lookup table.
- **DSPPipeline**: Owns the lifecycle of all `DSPModule` instances and executes them sequentially.
- **Audio Callback**: Guaranteed Real-Time safe (no allocations, no locks, no I/O).

## 2. Ownership Model
- **AudioEngine**: Owns the `DSPPipeline`, `ModuleRegistry`, and `ParameterManager`.
- **DSPPipeline**: Owns `std::unique_ptr<DSPModule>` instances.
- **ModuleRegistry**: Holds **raw pointers** to modules for routing. It acts as a subscriber table and does not manage memory.

## 3. Threading Model
- **UI Thread**: Producer of parameter updates.
- **Audio Thread**: Exclusive consumer of the `ParameterQueue` and exclusive writer to DSP states.
- **Synchronization**: Achieved via an **SPSC Lock-Free Ring Buffer**.

## 4. Parameter Management
- **Lookup**: O(1) deterministic access via `ParameterID` index.
- **Storage**: Fixed-size memory layout to prevent cache misses.
- **Smoothing**: Integrated `ParameterSmoother` for gain, width, and crossfeed to prevent zipper noise.
- **Overflow Policy**: **Drop Newest**. If the queue is saturated, incoming UI updates are rejected until the next processing block.

## 5. Audio Configuration
- **Source of Truth**: Negotiated Oboe stream parameters (Sample Rate, Channels, Burst Size).
- **Initialization**: Pipeline is "Prepared" twice: once with defaults during creation, and once with actual values before the stream starts.

## 6. Directory Structure
- `Parameter*.h`: The lock-free command infrastructure.
- `*Module.h`: Concrete DSP implementations.
- `DSPPipeline.cpp/h`: Sequence manager.
- `ModuleRegistry.h`: Routing manager.
- `AudioEngine.cpp/h`: Root orchestrator.
