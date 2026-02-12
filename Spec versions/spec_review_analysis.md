# FlowZone Spec v1.2 Review & Analysis

## 1. Architecture Review

### Current State
- **Hybrid:** C++ JUCE Engine + React/TS Web UI.
- **Communication:** WebSocket (State) + Binary Stream (Visuals).
- **Isolation:** VST3 hosting in "SandboxManager".

### Critique & Improvements
- **VST Hosting:** The spec mentions "VST3 child processes" but isn't specific about the cardinality.
    - *Risk:* One process per plugin scales poorly (resource heavy). One process for *all* plugins risks one bad apple crashing the whole rack.
    - *Proposal:* **Single Auxiliary Host Process**. Use a single "PluginHost" process that loads all VSTs. If it crashes, all VSTs die, but the main Engine (and basic backing tracks/looping) survives. This is the best balance of stability vs. complexity for a v1.
- **Protocol:** JSON is fine for control, but "StateBroadcast" can get heavy.
    - *Proposal:* **Shared Types / Schema Definition**. Explicitly define the Protocol Buffers or TypeScript interfaces to be generated/shared. This ensures the C++ backend and TS frontend never diverge.
- **Local vs. Remote Transport:**
    - *Observation:* Using WebSocket for localhost (WebBrowserComponent) is efficient enough, but for "Feature Extraction" (visuals), we can do better on local.
    - *Proposal:* **Hybrid Transport**. Keep WebSocket for simplicity/uniformity for now (KISS), but note that the internal server should bind to `127.0.0.1` optimized loopback for the embedded view.

## 2. Reliability & Failure Handling

### Current State
- `CrashGuard` for startup loops.
- `DiskWriter` on separate thread.
- `SandboxManager` for VSTs.

### Critique & Improvements
- **Disk I/O Safety:**
    - *Risk:* "Increase RAM buffer size dynamically" is a memory leak risk if disk hangs indefinitely.
    - *Proposal:* **Backpressure Handling**. Define a strict generic `RingBuffer` size (e.g., 256MB). If full, **drop audio packets** and flag a "Disk Overload" error to UI. Never crash via OOM.
- **VST Recovery Strategy:**
    - *Gap:* "Attempts to restart worker" logic needs a "Graceful Bypass" state.
    - *Proposal:* If Host Process dies, the Engine instantly switches VST nodes to **Bypass Mode** (dry signal passes through or silence if instrument). It then attempts restart. On success, it restores state and crossfades wet signal back in.
- **State Reconciliation:**
    - *Gap:* What if the Client sends a command based on an old state?
    - *Proposal:* **Optimistic UI with Rollback**. The UI updates immediately. If the Engine rejects the command (e.g., "Slot locked"), it sends a `NACK` or a fresh State update that forces the UI to revert.

## 3. Performance & Scalability

### Current State
- Feature-based extraction (RMS/Spectrum) sent to client.
- 20-30fps target.

### Critique & Improvements
- **Frame Rate:**
    - *critique:* 20-30fps is "laggy" for a premium workstation feel.
    - *Proposal:* Target **60fps** for the Visual Stream. The data (RMS + 16-band spectrum * 8 slots) is tiny.
        - Math: 8 slots * (1 float RMS + 16 floats Spectrum) * 4 bytes = ~544 bytes per frame.
        - At 60fps, that's ~32KB/s. Trivial even for bad WiFi.
        - **Decision:** Increase target to 60fps.
- **Latency Management:**
    - *Proposal:* Add **"Network Latency Compensation"** for visual meters on remote devices. Timestamp audio features. Client delays display slightly to match its own audio playback buffer if necessary, or just render ASAP for "feel".

## 4. Clarity & Implementability (for Agents)

### Current State
- "CommandQueue", "StateVector" mentioned abstractly.
- "Project Structure" is "Monorepo" but vague.

### Critique & Improvements
- **Explicit Directory Map:** precise paths (e.g., `src/engine`, `src/client`) help agents place files correctly.
- **Command Schema:** Define the exact JSON list.
    - `SET_VOL { slot: int, gain: float }`
    - `TRIGGER_SLOT { slot: int, quantized: bool }`
- **Component Lifecycle:** explicit "Init -> Start -> Stop -> Teardown" sequence for the Engine classes.

## 5. Operational Robustness

### Current State
- Simple constraints.

### Critique & Improvements
- **Telemtry/Logging:**
    - *Proposal:* **Structured Logging**. `Logger::log(Level, "Component", "Message", {json_metadata})`.
    - *UI:* "Dev Console" in Settings to view Engine logs in real-time.
- **Watchdogs:**
    - *Proposal:* **Engine Heartbeat**. The Frontend should expect a "ping" every 1s. If missed -> "Connection Lost" overlay.
    - *Proposal:* **Audio Thread Watchdog**. If the Audio Callback takes > 80% of block time, log "CPU Overload" count.

## Summary of Changes for v1.3
1.  **Strict Typed Protocol:** Add specific schemas for Commands and State.
2.  **Architecture:** Clarify VST Host as a single separate process with "Graceful Bypass" recovery.
3.  **Performance:** Increase Visualization target to 60fps; validate bandwidth calculation.
4.  **Reliability:** Strict RingBuffer limits for recording (no infinite ram expansion).
5.  **Agents:** Add Directory Map and explicit API endpoints.
6.  **Robustness:** Add Logging/Telemetry specifications.
