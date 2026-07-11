# RP-001 Architecture Overview: Native Processing Graph

## 1. The Logical Graph (`AudioGraph`)
Configuration-space representation. Defines "what" to process and the "order" of modules. Uses stable `NodeId`s.

## 2. The Compiler (`GraphCompiler`)
Transformation stage. Validates topology (cycle detection, chain integrity) and materializes `DSPModule` instances. It binds nodes to a specific `ProcessSpec`.

## 3. The Execution Plan (`ExecutionPlan`)
Real-time optimized structure. Contains pre-ordered `GraphOperation`s and owns the prepared runtime instances.

## 4. The Executor (`GraphExecutor`)
The "Heartbeat". A minimal class that iterates the `ExecutionPlan` operations and calls `process()` on each. Decoupled from Android and Oboe.

## 5. Process Contract
*   `ProcessSpec`: Sample rate, channel count, max block size.
*   `ProcessContext`: Active buffer pointers and current frame count.
*   `DSPModule`: Virtual interface for `prepare`, `process`, and `reset`.

## 6. Oboe Boundary
Oboe is treated as a transport. The `AudioEngine` is the bridge, converting Oboe callback data into a `ProcessContext` and invoking the `GraphExecutor`.
