# PowerEQ Master - Real-Time Safety Contract

## 1. Audio Thread Restrictions
 транзитивно reachable from `AudioEngine::onAudioReady()` MUST NOT:
- Allocate or free heap memory (`new`, `delete`, `malloc`, `free`).
- Acquire blocking mutexes or locks.
- Wait on condition variables or perform unbounded loops.
- Perform JNI calls.
- Perform File I/O or Logging.
- Throw or catch exceptions.

## 2. Plan Lifecycle and Quiescence
The engine uses a **Single-Reader Sequence Protocol** to manage `ExecutionPlan` lifetimes:
- **Audio Thread** announces block entry (`blocksStarted_`) and block exit (`blocksCompleted_`).
- **Control Thread** tags retired plans with the current `blocksStarted_` value.
- **Reclamation** occurs only when `blocksCompleted_ >= retirementBlockId`.
- This ensures a plan is never deleted while the audio thread is within its use interval.

## 3. Oboe Callback Quiescence
The engine relies on the **Oboe 1.10.0** synchronous lifecycle contract:
- `oboe::AudioStream::close()` is a blocking operation.
- Per Oboe specifications, once `close()` returns:
  - All data callbacks have completed.
  - No future data callbacks will be initiated.
- This allows safe destruction of the `AudioEngine` and its processing arenas immediately after `close()`.

## 4. Real-Time Performance Hazards
- **Coefficient Calculation:** Current BiQuad parameter updates involve `sin`, `cos`, and `pow`. These are deterministic but computationally expensive. RP-002 will move these to the control side or use efficient interpolation.
- **Parameter Queue:** The queue is structurally bounded to 1024 events. Draining this in a single callback is safe but should be monitored for worst-case timing.
