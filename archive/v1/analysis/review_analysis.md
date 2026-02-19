# Review Analysis of Spec_FlowZone_Looper1.3.md

## 1. Architecture

**Current State:**
- Native C++ Engine + React UI via Internal WebServer (WebSocket).
- Shared Memory IPC for VST3 hosting.
- Strict thread decoupling.

**Critique & Improvements:**
- **IPC for UI:** While WebSockets are convenient, they introduce serialization overhead. For a local desktop app, this is acceptable for control messages but suboptimal for high-frequency visualization.
    - *Decision:* Keep WebSocket for control/state (simplicity/remotability). Keep the "Binary Stream" over WebSocket but clarify the "backpressure" mechanism.
    - *Revision:* Explicitly define the "Dispatcher" model. Commands should be dispatched to the Message Thread or specific worker threads, never blocking the Audio Thread.
- **State Management:** The "StateBroadcaster" pushes full or partial updates?
    - *Refinement:* Define a "Patch" protocol (JSON Patch or similar) to avoid sending the entire `AppState` tree on every minor change (like a volume fader tweak). This reduces GC pressure in the React frontend.

## 2. Reliability & Failure Handling

**Current State:**
- Plugin process isolation.
- Disk writer drops packets on overload.
- Boot loop detection -> Safe Mode.

**Critique & Improvements:**
- **Disk Overload:** "Dropping packets" silently corrupts a recording. This is unacceptable for a "Workstation".
    - *Change:* Implement a **Dynamic Buffer Expansion** with a hard RAM limit (e.g., up to 1GB). If disk falls behind, buffer in RAM. If RAM limit is hit, *then* stop recording and notify user. This is better than writing a corrupted file.
- **Watchdog:** 1s heartbeat for plugins is too slow for a real-time feeling.
    - *Change:* 250ms heartbeat.
- **UI Disconnection:**
    - *Change:* Explicit rule: If WebSocket disconnects (UI crash/refresh), **Audio Engine continues running**. Reconnection triggers a "Full State Sync".

## 3. Performance & Scalability

**Current State:**
- 30fps visualization target.
- Manufacturer-based process grouping.

**Critique & Improvements:**
- **Frame Rate:** 30fps is functional but not "premium".
    - *Change:* Default to **60fps**. Downgrade to 30fps dynamically *only* if CPU load > 70%.
- **Audio Thread:**
    - *Refinement:* Explicitly mandate **Lock-Free Circular Buffers** for all variable updates (Volume, Pan, etc.) to prevent atomics contention on the hot path.

## 4. Clarity for Coding Agents

**Current State:**
- Strong directory structure.
- TypeScript definitions provided.

**Critique & Improvements:**
- **Command Protocol:**
    - *Change:* Add a `Schema Registry` requirement. A `commands.json` or `protocol.ts` file that acts as the single source of truth for both C++ and TS generation.
    - *Change:* Explicit steps for adding a new feature: 1. Update Schema, 2. Update C++ Handler, 3. Update Reducer, 4. Update UI.
- **Error Codes:**
    - *Change:* Centralize error codes in a `Shared/Errors.h` file so agents don't make up magic numbers.

## 5. Operational Robustness

**Current State:**
- JSON Logs.
- Config backups.

**Critique & Improvements:**
- **Session History:**
    - *Change:* Implement **Session Snapshots** (Undo/Redo). Every "Destructive" action (Delete Loop, Clear) creates a diff snapshot.
- **Observability:**
    - *Change:* Add "Performance Metrics" to the binary stream (Engine CPU, GUI FPS, Disk IOPS).
- **Rollback:**
    - *Change:* The `config.json` backup strategy is good, but let's formalize a `backup_restored` flag in the logs for traceability.

## Proposed Plan for Spec 1.4

I will rewrite the spec to include:
1.  **Hybrid Patch/Snapshot Protocol:** For efficient UI updates.
2.  **Dynamic Disk Buffering:** To prioritize data integrity over memory thriftiness.
3.  **Agent Implementation Guide:** A distinct section on "How to add a feature".
4.  **60fps Target:** With adaptive degradation.
5.  **Session Undo/Redo:** Architectural support for state rollback.
