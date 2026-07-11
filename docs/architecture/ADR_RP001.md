# Architectural Decision Records: RP-001

## ADR-001: Chain-first Graph Topology
*   **Status:** Accepted
*   **Context:** Need a foundation for modular processing.
*   **Decision:** V1 supports only a single ordered chain of processors.
*   **Alternatives:** Full DAG (Too complex for initial milestone).
*   **Consequences:** Structured validation rejects branches/merges.

## ADR-002: Compile Off the Audio Thread
*   **Status:** Accepted
*   **Context:** Graph validation and processor materialization are expensive and non-deterministic.
*   **Decision:** Use a `GraphCompiler` class to build `ExecutionPlan` on the control thread.
*   **Alternatives:** Lazy compilation (Violates RT-safety).

## ADR-003: Immutable Active Execution Plan
*   **Status:** Accepted
*   **Context:** Structural mutation during processing causes races.
*   **Decision:** The `ExecutionPlan` structure is immutable once installed.
*   **Alternatives:** Mutex-protected topology (Causes priority inversion).

## ADR-004: Quiescent Plan Installation
*   **Status:** Accepted
*   **Context:** Safely swapping plans without lock-free hazard pointers.
*   **Decision:** In V1, stop/start Oboe stream or use a control mutex to ensure the callback is idle during swap.
*   **Alternatives:** RCU (Complexity too high for V1).

## ADR-005: Preallocated Buffer Arena
*   **Status:** Accepted
*   **Context:** Need scratch memory for de-interleaving and graph routing.
*   **Decision:** `AudioEngine` owns pre-allocated `leftArena_` and `rightArena_` sized for worst-case.
*   **Alternatives:** Pool-based allocation (Non-deterministic).

## ADR-011: Execution Plans are ProcessSpec-bound
*   **Status:** Accepted
*   **Context:** DSP modules depend on sample rate and block size.
*   **Decision:** A plan is invalid if the current `ProcessSpec` differs from the one used at compilation.
*   **Alternatives:** Dynamic re-preparation (Non-deterministic).
