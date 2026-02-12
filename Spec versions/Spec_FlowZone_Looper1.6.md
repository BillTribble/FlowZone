# **Technical Design Doc: FlowZone**

**Version:** 1.6 (Unified Architecture + Product Specification)  
**Target Framework:** JUCE 8.0.x (C++20) + React 18.3.1 (TypeScript 5.x)  
**Platform:** macOS (Standalone App, Silicon Native)  
**Repository Structure:** Monorepo  

---

## **1. Goals & Non-Goals**

### **1.1. Goals**

*   **Identity:** A macOS-first retrospective looping workstation named **FlowZone**.
    *   **Inspiration:** Inspired by the "flow machine" workflow (e.g., Tim Exile's *Endlesss*), designed to eliminate "loop paralysis."
    *   **Core Principle:** **"Destructive Creativity, Safe Capture."** Active loop slots are constantly merged/overwritten to keep flow moving, but *every* committed loop is saved to disk history.
    *   **Visual History:** A persistent, chronological "Riff Feed" is always visible (Sidebar on Desktop, Split/Drawer on Mobile), allowing users to drag past ideas back into the active flow.
*   **Hybrid Architecture:** Native C++ Audio Engine (Source of Truth) + React Web UI (Stateless View).
    *   **Local:** `juce::WebBrowserComponent` (embedded).
    *   **Remote:** Internal WebServer (Background Thread) for phone/tablet control.
    *   **Unified State:** All clients see the exact same session state, synced via WebSocket.
*   **Strict Decoupling:** 
    *   **Audio Priority:** The Audio Thread is sacred. It must never block on UI, Network, or Disk I/O.
    *   **Crash Resilience:** UI crashes do not stop audio. Plugin crashes do not kill the main engine.
*   **Premium Visualization:** 
    *   **Throttled Stream:** Target 30fps "Best Effort" bi-directional feature stream.
    *   **Adaptive Degradation:** Stepped CPU-aware throttling (see §3.6).
*   **Operational Robustness:**
    *   **Crash Guard:** Named component that detects boot loops and enforces Graduated Safe Mode (see §2.2.F).
    *   **Plugin Isolation:** VST3s run in separate child processes with exponential-backoff respawning.
    *   **Data Integrity:** Tiered RAM buffering for disk writes. **Never** silently drop audio samples. On failure, flush a partial file (see §4.2).
    *   **Observability:** Structured JSONL logs with mandatory schema, HTTP health endpoint, telemetry stream, and performance metrics.
*   **Agent-Optimized:** Codebase structured for clarity, with single-source-of-truth schemas and a step-by-step implementation recipe (§6).
*   **Responsive Design:** The UI must adapt fluidly to the client device's form factor (Phone vs. Tablet/Desktop).
*   **Centralized Audio Processing:** All audio processing happens on the main macOS computer.
*   **Retrospective "Always-On" Capture:** Implement a lock-free circular buffer (~60s).
*   **Hybrid Sound Engine:** VST3 Hosting, Internal Procedural Instruments, Mic Input Processing.
*   **Microtuning Support:** Internal synths must support microtuning via `.scl` (Scala) and `.kbm` files, with standard presets (Just Intonation, Pythagorean, Slendro, Pelog, 12TET).
*   **"Configure Mode" for VST Parameters:** Users must explicitly "touch" a VST knob to expose it to the React UI.
*   **Smart Layering (Auto-Merge):** The "9th Loop" trigger automatically sums Slots 1-8 into Slot 1.
*   **FX Mode / Resampling:** Route specific layers through FX and resample into a new layer.

### **1.2. Non-Goals**
*   **Windows/Linux Support:** macOS only for V1.
*   **Cloud Sync:** Local network sharing only.
*   **Complex DAW Features:** No piano roll, no automation lanes.

### **1.3. Future Goals (V2 Stretch)**
*   **Ableton Link Integration:** Global phase-sync across devices and applications.
*   **Windows/Linux Ports**
*   **Tap Tempo Cold Start:** Time-between-taps sets session tempo when transport is stopped and slots are empty.
*   **WebGL Visualizers:** GPU-accelerated visualization option.
*   **MIDI Clock Sync:** External MIDI Clock as transport source.

---

## **2. System Architecture**

### **2.1. Component Diagram**

```mermaid
graph TD
    User[User] -->|Interacts| UI[React UI]
    UI -->|WebSocket JSON| WebServer[CivetWeb - Background Thread]
    UI <--|Binary Stream| FeatureStream[Feature Extractor]
    
    subgraph "Main Process - C++"
        WebServer -->|Lock-free FIFO| CmdQueue[CommandQueue - SPSC]
        CmdQueue --> Dispatcher[CommandDispatcher]

        Dispatcher -->|Route| Engine[FlowEngine - Realtime Priority]
        Dispatcher -->|Route| Transport[TransportService]
        Dispatcher -->|Route| PluginMgr[PluginProcessManager]
        
        Engine -->|State Update| Broadcaster[StateBroadcaster]
        Broadcaster -->|Diff/Patch or Snapshot| WebServer
        
        Engine <-->|ProcessBlock| Transport
        Engine -->|Audio Blocks| FeatureStream
        Engine -->|Audio Blocks| DiskWriter[DiskWriter Thread]
        
        DiskWriter -->|Write| FileSystem[SSD Storage]

        SessionMgr[SessionStateManager] <-->|Undo/Redo/Save| Engine
        CrashGuard[CrashGuard - Startup] -->|Mode Decision| Engine
    end
    
    subgraph "IPC Layer"
        SharedMem[Shared Memory RingBuffers]
    end

    subgraph "Child Processes - One per Manufacturer"
        HostA[Host: FabFilter]
        HostB[Host: Arturia]
        Engine <-->|Audio/Events| SharedMem <--> HostA
        Engine <-->|Audio/Events| SharedMem <--> HostB
    end
```

### **2.2. High-Level Components**

#### **A. FlowEngine** — `src/engine/FlowEngine.cpp`
*   The singleton `juce::AudioProcessor`.
*   **Role:** Real-time audio processing DAG.
*   **Priority:** `Realtime` (macOS `UserInteractive` QoS).

#### **B. TransportService** — `src/engine/transport/TransportService.cpp`
*   **Role:** Manages Transport state (Play/Pause, BPM, Phase). 
*   **Isolation:** Decoupled from Engine for independent testing.

#### **C. CommandQueue** — `src/engine/CommandQueue.h`
*   **Role:** Lock-free SPSC FIFO. Moves raw command bytes from the WebSocket thread to the audio thread *without locking*.
*   **Contract:** This is a data structure only — no validation, no routing.

#### **D. CommandDispatcher** — `src/engine/CommandDispatcher.cpp`
*   **Role:** Reads from `CommandQueue` on the audio thread. Validates command payloads, rejects unknown commands, and routes to the correct handler (`FlowEngine`, `TransportService`, `PluginProcessManager`).
*   **Contract:**
    *   Validates all required fields are present and within range.
    *   Unknown `cmd` values → log warning + send `ERROR { code: 1200, msg: 'UNKNOWN_COMMAND' }`.
    *   Returns `reqId` in error/ack responses when present on the command.

#### **E. StateBroadcaster** — `src/engine/StateBroadcaster.cpp`
*   **Mechanism:** Maintains a `StateRevisionID` (monotonically increasing `uint64_t`).
*   **Protocol:**
    *   **Patch:** Sends **JSON Patches** (RFC 6902) for incremental updates.
    *   **Snapshot:** Sends a **Full Snapshot** (`STATE_FULL`) when:
        *   A new client connects.
        *   A client reconnects with a stale `revisionId`.
        *   A patch would exceed **20 operations or 4 KB** (at which point a snapshot is cheaper).
    *   **Decision:** The `StateBroadcaster` always decides patch-vs-snapshot — clients never request patches.

#### **F. CrashGuard** — `src/engine/CrashGuard.cpp`
*   **Role:** Detects boot loops and enforces Graduated Safe Mode at startup.
*   **Mechanism:**
    1.  On launch, reads crash counter and context from `~/Library/Application Support/FlowZone/.crash_sentinel`.
    2.  Increments counter immediately on boot.
    3.  Clears counter after **60 seconds** of stable runtime.
    4.  Determines Safe Mode level based on crash context.
*   **Graduated Safe Mode:**

| Level | Trigger | Behavior |
| :--- | :--- | :--- |
| **Level 1: Plugin-Only** | Plugin crash loop (same manufacturer 3× in 60s) | Disable all VSTs. Engine + audio I/O continue normally. |
| **Level 2: Audio Reset** | Audio driver failure at boot | Disable VSTs + reset to default audio device. |
| **Level 3: Full Recovery** | Config corruption or crash counter ≥ 3 | Disable VSTs, default audio, no session auto-load. Factory defaults offered. |

#### **G. SessionStateManager** — `src/engine/SessionStateManager.cpp`
*   **Role:** Undo/redo stack management, auto-save, crash-recovery snapshots, session loading.
*   **Implementation:**
    *   Holds `std::deque<AppState> undoStack` (max 50 entries).
    *   Every destructive command (Record Over, Delete, Clear) pushes the *previous* state to the stack.
    *   Audio files are **never deleted immediately** — only marked for garbage collection on clean exit.
    *   **Auto-save:** Writes a recovery snapshot every 30 seconds to `~/Library/Application Support/FlowZone/backups/autosave.json`.

#### **H. FeatureExtractor** — `src/engine/FeatureExtractor.cpp`
*   **Optimization:** Double-buffered atomic exchange.
*   **Rate:** Target 30fps (subject to CPU-aware degradation per §3.6).

#### **I. DiskWriter** — `src/engine/DiskWriter.cpp`
*   Background thread with tiered reliability (see §4.2 for full failure strategy):
    1.  **Primary:** 256MB RingBuffer.
    2.  **Overflow:** Temporary RAM blocks (up to 1GB).
    3.  **Critical:** Flush partial file, stop recording, alert UI.

#### **J. PluginProcessManager** — `src/engine/PluginProcessManager.cpp`
*   **Grouping:** One child process per VST Manufacturer.
*   **Watchdog:** Pings each process every **500ms**. Hang detected after **3 consecutive missed heartbeats (1.5s)**.
    *   *Timing rationale:* At 48kHz / 512 buffer, a single audio callback is ~10.7ms. A 1.5s timeout allows for ~140 callback periods — far beyond any legitimate processing delay.
*   **Hot-Swap:** Monitors VST3 folders for file changes. New plugins added to available list without engine reboot.

#### **K. Secondary Processes (Manufacturer-Isolated Hosts)**
*   **`PluginHostApp`:** Lightweight headless JUCE app in `src/host/`.
*   **Failure Isolation:** If a VST crashes, only the process for that manufacturer dies.
*   **Recovery with Exponential Backoff:**
    1.  Retry 1: Immediate.
    2.  Retry 2: After **2 seconds**.
    3.  Retry 3: After **10 seconds**.
    4.  After retry 3: Mark manufacturer as **"Suspended"** for the session. User can manually re-enable from the plugin settings panel.
*   **On respawn:** Bypass affected nodes (fade out over 50ms to avoid clicks) → respawn host → restore last known plugin state.

#### **L. Frontend (React Web App)** — `src/web_client/`
*   **Stack:** React 18.3.1, TypeScript, Vite.
*   **Rendering:** 
    *   **UI:** React DOM.
    *   **Visualizers:** Canvas API (via `requestAnimationFrame`).
*   **Connection Lifecycle:**
    1.  **Connect:** Client → opens WebSocket with `{ clientId: <UUID> }`.
    2.  **Handshake:** Server → sends `STATE_FULL` with current `revisionId` and `protocolVersion`.
    3.  **Version check:** If `protocolVersion` doesn't match, server sends `ERROR { code: 1100, msg: 'PROTOCOL_MISMATCH' }` and closes.
    4.  **Steady state:** Server → `STATE_PATCH` ops. Client → commands.
    5.  **Disconnect:** Client shows "Reconnecting…" overlay. Audio continues unaffected. Reconnect uses exponential backoff: 100ms → 200ms → 400ms → … → max 5s.
    6.  **Reconnect:** Client → `WS_RECONNECT` with last known `revisionId`. Server sends diff if revision is recent, or full snapshot if stale.
    7.  **Optional PIN auth:** If `config.json` has `"requirePin": true`, client must send `{ cmd: 'AUTH', pin: '…' }` before any other command is accepted. Prevents accidental LAN access during live performance.

#### **M. Internal Audio Engines (Native C++)**
*   **`InternalSynth`:**
    *   **Engines:** Drums, Bass, Leads.
    *   **Tuning:** Implementation of MTS-ESP or internal frequency mapping for `.scl` support.
    *   **Presets:** Just Intonation, Pythagorean, Slendro, Pelog, 12TET (Default).
*   **`InternalFX`:** Core & Keymasher banks.
*   **`MicProcessor`:** Input chains.

### **2.3. Thread Priority Table**

| Thread | macOS QoS | Rationale |
| :--- | :--- | :--- |
| Audio Thread | `UserInteractive` | Real-time audio processing. Must never be preempted. |
| DiskWriter | `UserInitiated` | File I/O is time-sensitive during recording. |
| WebServer | `Default` | HTTP/WS handling is normal priority. |
| FeatureExtractor | `Utility` | Visualization is expendable — sacrificed under load. |
| Plugin Watchdog | `Default` | Must run reliably but not at audio priority. |

### **2.4. Component Lifecycle**

**Startup sequence:**
```
CrashGuard → Config Load → Audio Device Init → FlowEngine → TransportService
→ PluginProcessManager → DiskWriter → WebServer → StateBroadcaster
→ FeatureExtractor → SessionStateManager (load last session if not Safe Mode)
```

**Shutdown sequence:**
```
SessionStateManager (auto-save) → FeatureExtractor → StateBroadcaster
→ WebServer → DiskWriter → PluginProcessManager → FlowEngine → Audio Device
→ CrashGuard (clear sentinel)
```

---

## **3. Data Model & Protocol**

### **3.1. Single Source of Truth**

All commands and state shapes are defined in a **Schema Registry** that acts as the contract between C++ and TypeScript:
*   **TypeScript:** `src/shared/protocol/schema.ts`
*   **C++ Mirror:** `src/shared/protocol/commands.h`

These must be kept in sync. Changes to one **must** be reflected in the other.

### **3.2. Command Schema (TypeScript Definition)**

```typescript
// Commands sent from Client -> Engine
// Must match C++ decoding logic EXACTLY.
type Command = 
  | { cmd: 'SET_VOL'; slot: number; val: number; reqId: string }   // val: 0.0 - 1.0
  | { cmd: 'TRIGGER_SLOT'; slot: number; quantized: boolean }
  | { cmd: 'LOAD_VST'; slot: number; pluginId: string }
  | { cmd: 'SET_TEMPO'; bpm: number }                              // bpm: 20.0 - 300.0
  | { cmd: 'UNDO'; scope: 'SESSION' }
  | { cmd: 'PANIC'; scope: 'ALL' | 'ENGINE' }                     // Immediate silence/reset
  | { cmd: 'AUTH'; pin: string };                                   // Optional PIN auth

// Server Responses
type ServerMessage = 
  | { type: 'STATE_FULL'; data: AppState }
  | { type: 'STATE_PATCH'; ops: JsonPatchOp[]; revId: number }
  | { type: 'ACK'; reqId: string }
  | { type: 'ERROR'; code: number; msg: string; reqId?: string };
```

### **3.3. Command Error Matrix**

Every command has a defined set of possible error responses:

| Command | Possible Errors | Client Behavior |
| :--- | :--- | :--- |
| `SET_VOL` | None (always succeeds) | Optimistic update; reconcile on next state patch. |
| `TRIGGER_SLOT` | `2010: SLOT_BUSY` | Show toast "Slot busy, try again." |
| `LOAD_VST` | `3001: PLUGIN_CRASH`, `3010: PLUGIN_NOT_FOUND` | Show error in slot UI panel. |
| `SET_TEMPO` | `2020: TEMPO_OUT_OF_RANGE` | Clamp UI slider to valid range (20–300 BPM). |
| `UNDO` | `4001: NOTHING_TO_UNDO` | Disable undo button. |
| `PANIC` | None (always succeeds) | Full UI reset animation. |
| `AUTH` | `1101: AUTH_FAILED` | Show "Incorrect PIN" prompt. |

### **3.4. App State**

```typescript
interface AppState {
  meta: {
    revisionId: number;
    protocolVersion: number;      // Must match client expectation
    serverTime: number;           // For jitter compensation
    mode: 'NORMAL' | 'SAFE_MODE';
    version: string;              // e.g. "1.0.0"
  };
  transport: {
    bpm: number;
    isPlaying: boolean;
    barPhase: number;              // 0.0 - 1.0
  };
  slots: Array<{
    id: string;                    // UUID
    state: 'EMPTY' | 'RECORDING' | 'PLAYING' | 'MUTED';
    volume: number;                // 0.0 - 1.0
    pan: number;                   // -1.0 to 1.0
    name: string;
    pluginChain: PluginInstance[];
    lastError?: number;            // Error code if slot has an active error
  }>;
  system: {
    cpuLoad: number;               // 0.0 - 1.0 (DSP time / buffer time)
    diskBufferUsage: number;       // 0.0 - 1.0
    memoryUsageMB: number;
    activePluginHosts: number;
  };
}
```

### **3.5. RiffSnapshot (C++)**

```cpp
struct RiffSnapshot {
    juce::Uuid id;
    int64_t timestamp;
    int64_t stateRevisionId;  // For reconciliation
    double tempo;
    int rootKey;
    // ... RigState ...
    std::array<SlotState, 8> slots;
};
```

### **3.6. Audio Persistence Strategy**

*   **Recording Format:** FLAC (Lossless) 24-bit / 44.1kHz or 48kHz (Default).
*   **Storage Option:** User preference to save as MP3 320kbps (via LAME or system encoder) to save disk space.
*   **Naming Convention:** `project_name/audio/riff_{timestamp}_slot_{index}.flac`

### **3.7. Binary Visualization Stream**

*   **Format:** Raw Float32 Array (Little Endian).
*   **Header:** `[Magic:4][FrameId:4][Timestamp:8]`
*   **Payload:** `[MasterRMS_L][MasterRMS_R][Slot1_RMS][Slot1_Spec_1...16]...`
*   **Transmission:** Separate binary WebSocket channel.
*   **Backpressure Control:**
    *   Server maintains a per-client `pendingFrameCount` (incremented on send, decremented on ACK).
    *   Client sends a **4-byte ACK** after *every received frame*.
    *   If `pendingFrameCount > 3`, server skips sending the next frame.
    *   Fast clients get ~30fps; slow clients (phone on WiFi) gracefully degrade.

### **3.8. Adaptive Visualization Degradation**

| CPU Load (DSP/Buffer %) | Visualization FPS | Feature Stream Content |
| :--- | :--- | :--- |
| < 60% | 30 fps | Full (RMS + 16-band spectrum) |
| 60–75% | 15 fps | Reduced (RMS only) |
| 75–90% | 5 fps | RMS only |
| > 90% | 0 fps (paused) | Disabled; UI shows "HIGH LOAD" badge |

*   **CPU Load** is defined as: `(DSP processing time per callback) / (audio buffer duration) × 100`.

### **3.9. Optimistic UI Pattern**

For low-latency commands (`SET_VOL`, `SET_PAN`, `SET_TEMPO`), the client updates its local state optimistically before receiving server confirmation. On the next `STATE_PATCH`, the client reconciles. If the server sends an `ERROR`, the client reverts to the last confirmed state. Apply this consistently to all "always succeeds" or "range-clampable" commands.

---

## **4. File System & Reliability**

### **4.1. Directory Structure**

Strict adherence for Agent clarity.

```
/FlowZone_Root
├── /cmake
├── /src
│   ├── /engine              # Core Audio Logic
│   │   ├── /transport       # TransportService
│   │   ├── /ipc             # SharedMemoryManager
│   │   ├── FlowEngine.cpp
│   │   ├── CommandQueue.h
│   │   ├── CommandDispatcher.cpp
│   │   ├── StateBroadcaster.cpp
│   │   ├── SessionStateManager.cpp
│   │   ├── CrashGuard.cpp
│   │   ├── FeatureExtractor.cpp
│   │   ├── DiskWriter.cpp
│   │   └── PluginProcessManager.cpp
│   ├── /host                # Child Process Host (PluginHostApp)
│   ├── /shared              # Shared Types & Schemas
│   │   ├── /protocol        # schema.ts / commands.h
│   │   ├── /errors          # ErrorCodes.h
│   │   └── /utils
│   └── /web_client          # React App
│       └── /src
│           ├── /hooks       # useCommands.ts, useAppState.ts
│           ├── /components
│           └── /lib         # WebSocket client, binary stream decoder
├── /tests
│   ├── /engine              # C++ unit tests
│   └── /web_client          # React component tests
├── /assets
└── CMakeLists.txt
```

### **4.2. DiskWriter Failure Strategy (Tiered)**

| Tier | Condition | Action | User Impact |
| :--- | :--- | :--- | :--- |
| **1. Normal** | Ring buffer < 80% | Write to disk normally. | None. |
| **2. Warning** | Ring buffer > 80% | Log warning. UI shows "Disk Slow" badge. | Awareness. |
| **3. Overflow** | Ring buffer full | Allocate temporary overflow blocks in RAM (up to 1GB). | Recording continues; RAM pressure increases. |
| **4. Critical** | Overflow > 1GB | Flush all remaining data to `*_PARTIAL.flac`. Stop recording. Send `ERR_DISK_CRITICAL` with `{ partialFilePath }`. | Partial recording saved. Audio playback continues. |

> **Principle:** Audio playback of already-loaded loops is **never** affected by disk failures. Only *new recording* stops.

### **4.3. IPC Shared Memory Sizing**

*   Ring buffer size per plugin host = **4 × audio buffer size**.
    *   Example: 512 samples × 2 channels × 4 bytes × 4 = **16,384 bytes**.
*   If the ring buffer fills (writer catches up to reader), the **writer drops the frame** and increments a `dropCount` metric.
*   A `dropCount > 10 per second` triggers `ERR_PLUGIN_OVERLOAD` and is surfaced in the UI.

### **4.4. General Failure Handling Strategies**

| Scenario | Severity | Detection | Response | Recovery |
| :--- | :--- | :--- | :--- | :--- |
| **VST Crash** | High | IPC Broken Pipe | Bypass nodes (50ms fade), Toast UI. | Respawn w/ exponential backoff (§2.2.K). |
| **VST Hang** | High | 3 missed heartbeats (1.5s) | Kill SIGKILL. | Respawn w/ exponential backoff. |
| **Disk Slow** | Medium | Buffer > 80% | UI badge "Disk Slow". | If > 1GB overflow → partial save + stop rec. |
| **Boot Loop** | Critical | CrashGuard counter ≥ 3 | Enter **Graduated Safe Mode**. | See §2.2.F for level-appropriate response. |
| **Web Server Crash** | Low | Thread watchdog | Audio continues. UI disconnects. | Restart WebServer thread. |
| **Config Corrupt** | Medium | JSON parse failure | Log error. | Load `state.backup.1.json`. |
| **Config Outdated** | Low | `configVersion` mismatch | Run migration function. | If migration fails → load defaults, archive as `config.legacy.json`. |

### **4.5. Config File Versioning**

*   `config.json` includes a `configVersion: number` field.
*   On load:
    1.  Parse JSON. If parse fails → load backup.
    2.  Check `configVersion`. If older than current app version → run a migration function.
    3.  If migration fails → load factory defaults. Archive old config as `config.legacy.json`.

---

## **5. Operational Robustness (Observability)**

### **5.1. Error Codes (`src/shared/errors/ErrorCodes.h`)**

Agents must strictly use these codes. New codes must be registered here before use.

*   `1000-1099`: System (Boot, Config, Filesystem)
    *   `1001`: `ERR_DISK_FULL`
    *   `1002`: `ERR_DISK_CRITICAL` (Write failed / overflow exceeded)
    *   `1010`: `ERR_CONFIG_CORRUPT`
    *   `1011`: `ERR_CONFIG_MIGRATION_FAILED`
*   `1100-1199`: Protocol
    *   `1100`: `ERR_PROTOCOL_MISMATCH`
    *   `1101`: `ERR_AUTH_FAILED`
    *   `1200`: `ERR_UNKNOWN_COMMAND`
*   `2000-2999`: Audio Engine
    *   `2001`: `ERR_AUDIO_DROPOUT` (XRUN)
    *   `2010`: `ERR_SLOT_BUSY`
    *   `2020`: `ERR_TEMPO_OUT_OF_RANGE`
*   `3000-3999`: Plugins
    *   `3001`: `ERR_PLUGIN_CRASH`
    *   `3002`: `ERR_PLUGIN_TIMEOUT`
    *   `3010`: `ERR_PLUGIN_NOT_FOUND`
    *   `3020`: `ERR_PLUGIN_OVERLOAD` (IPC ring buffer drops)
*   `4000-4999`: Session
    *   `4001`: `ERR_NOTHING_TO_UNDO`

### **5.2. Structured Log Format (JSONL)**

All components must emit logs in this format. One JSON object per line (`*.jsonl` files).

```json
{
  "ts": "2026-02-12T10:30:00.000Z",
  "level": "INFO",
  "component": "FlowEngine",
  "event": "audio_dropout",
  "msg": "XRUN detected on callback 14523",
  "data": { "callbackDuration_ms": 12.3, "bufferDuration_ms": 10.7 }
}
```

*   **Valid `level` values:** `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`.
*   **Valid `component` values:** `FlowEngine`, `DiskWriter`, `PluginHost`, `WebServer`, `CrashGuard`, `SessionMgr`, `Transport`, `CommandDispatcher`.
*   **Storage:** `~/Library/Logs/FlowZone/engine.jsonl`
*   **Rotation:** Max 10MB per file, keep last 5 files.

### **5.3. Telemetry & Metrics**
*   **Local Metric Store:** A circular buffer of the last 10 minutes of performance metrics (CPU, RAM, FPS, disk buffer %).
*   **Developer Overlay:** UI can request `GET_METRICS_HISTORY` to render a performance graph.

### **5.4. HTTP Health Endpoint**

CivetWeb exposes a simple health check for external monitoring:

*   **`GET /api/health`** — Returns:

```json
{
  "status": "ok",
  "uptime_s": 3600,
  "cpu_load": 0.45,
  "disk_buffer_pct": 0.12,
  "active_plugin_hosts": 3,
  "safe_mode": false,
  "version": "1.0.0",
  "protocol_version": 1
}
```

*   **Status logic:**
    *   `"ok"` — Normal operation.
    *   `"degraded"` — CPU > 75% or disk buffer > 80%.
    *   `"critical"` — Safe mode active or `ERR_DISK_CRITICAL`.

---

## **6. Agent Implementation Guide**

### **6.1. How to Add a New Command (Step-by-Step)**

Example: Adding `MUTE_SLOT`.

| Step | File | Action |
| :--- | :--- | :--- |
| 1. Schema (TS) | `src/shared/protocol/schema.ts` | Add `{ cmd: 'MUTE_SLOT'; slot: number }` to `Command` union. |
| 2. Schema (C++) | `src/shared/protocol/commands.h` | Add `MUTE_SLOT` to `enum CommandType` and corresponding struct fields. |
| 3. Error codes | `src/shared/errors/ErrorCodes.h` | Register any new error codes if the command can fail. |
| 4. Dispatcher | `src/engine/CommandDispatcher.cpp` | Add `case CommandType::MUTE_SLOT:` in `dispatch()`, calling the handler. |
| 5. Handler | `src/engine/FlowEngine.cpp` | Add `void handleMuteSlot(int slotIndex)` method. Update slot state. |
| 6. State | Both `schema.ts` and C++ `AppState` | If observable state changes, update the state struct. |
| 7. UI Hook | `src/web_client/src/hooks/useCommands.ts` | Add `muteSlot(slot: number)` typed dispatch helper. |
| 8. UI Component | `src/web_client/src/components/SlotControls.tsx` | Wire up mute button to dispatch helper. Reflect `state === 'MUTED'`. |
| 9. Error matrix | §3.3 of this spec | Document possible errors for the new command. |
| 10. Test | `tests/engine/test_commands.cpp` | Push `MUTE_SLOT` through `CommandDispatcher`, assert slot state changes to `MUTED`. |

### **6.2. Rules for Agents**

1.  **Never** change the `CommandQueue` implementation logic — only add new command types.
2.  **Never** add an error code without registering it in `ErrorCodes.h` first.
3.  **Never** add a command without updating both the TypeScript and C++ schema files.
4.  **Always** update the error matrix (§3.3) when adding a command that can fail.
5.  **Always** add a unit test for new command handlers.

### **6.3. Naming Conventions**

| Language | Classes | Methods/Functions | Member Variables | Files |
| :--- | :--- | :--- | :--- | :--- |
| **C++** | `PascalCase` | `camelCase` | `m_memberName` | `PascalCase.cpp/.h` |
| **TypeScript** | `PascalCase` | `camelCase` | `camelCase` | `kebab-case.ts/.tsx` |

---

## **7. UI Specification (React)**

### **7.1. Visual Language & Design System**

*   **Base Aesthetic:** **Functional Studio Minimalism.**
    *   **Panel-Based Grouping:** Avoid harsh high-contrast borders. Use subtle differences in background brightness.
    *   **Soft Geometry:** UI elements should feel precise but not sharp. Use small, consistent border-radii (2px - 4px).
    *   **Vector Precision:** All meters and graphs use fine, anti-aliased vector lines (1px width).
    *   **Inline Visuals:** Knobs and sliders are "filled" with color to indicate value.
*   **Theme Modes:**
    *   **Dark (Default):** "Studio Dark Grey" backgrounds (`#2D2D2D` to `#383838`). Text is off-white (`#DCDCDC`).
    *   **Mid:** "Classic Grey" backgrounds (`#555555` to `#777777`).
    *   **Light:** "Daylight" backgrounds (`#F2F2F2`).
    *   **Match System:** Auto-switch based on OS.
*   **Typography:** **Inter** (sans-serif) used exclusively.
*   **Color Palette (Tertiary Highlights):**
    *   **Indigo (Blue-Violet):** Drums/Percussion.
    *   **Teal (Blue-Green):** Instruments/Synths.
    *   **Amber (Yellow-Orange):** External Audio Input.
    *   **Vermilion (Red-Orange):** Combined/Summed tracks or FX.
    *   **Chartreuse (Yellow-Green):** Transport & Sync.

### **7.2. Navigation & Views**

#### **Phone Mode (`< 768px`)**

*   **Navigation:** Bottom Tab Bar (Dash | Inst | FX | Mixer | **Settings**).
*   **Dashboard:** Vertical Stack.
*   **Mixer:** Scrollable.

#### **Tablet/Desktop Mode (`>= 768px`)**

*   **Navigation:** Top Header / Sidebar.
*   **Dashboard:** Grid Layout.
*   **Mixer:** Full 8-fader view.
*   **Session History:** Persistent Right Sidebar. Displays chronological list of Riffs (Waveform + Metadata). Drag-and-drop to Slots.

### **7.3. JSON Layout Specification**

```json
{
  "app_identity": {
    "name": "FlowZone",
    "theme_mode": "System",
    "typography": "Inter",
    "primary_accent": "#E65100",
    "style_guide": {
      "corner_radius": "3px",
      "border_width": "0px",
      "gap": "4px",
      "density": "high"
    },
    "palette": {
      "bg_app": "#252525",
      "bg_panel": "#333333",
      "drums": "#5E35B1",
      "synth": "#00897B",
      "input": "#FFB300",
      "fx": "#E65100",
      "sync": "#7CB342"
    }
  },
  "layout_hierarchy": [
    {
      "component": "ResponsiveContainer",
      "routes": {
        "dashboard": { 
          "mobile": ["Stack", "HistoryDrawer"], 
          "desktop": ["Grid", "HistorySidebar"] 
        },
        "settings": ["SettingsTabs"]
      }
    }
  ]
}
```

### **7.4. Settings Panel Specification**

The settings view is divided into four tabs. Changes made here sync to the Engine immediately via WebSocket.

#### **A. Interface (Look & Feel)**

*   **Zoom Level:** Global UI scaling percentage.
    *   *Controls:* Slider [50% - 200%]. (Default: 100%).
    *   *Implementation:* CSS `html { font-size: X% }` using REM units for all layout.
*   **Theme:** Color scheme selector.
    *   *Options:* Dark | Mid | Light | Match System.
*   **Font Size:** Base text scaling independent of layout zoom.
    *   *Options:* Small | Medium (Default) | Large.
*   **Reduce Motion:** Accessibility toggle.
    *   *Action:* Disables canvas visualizer animations and smooth scrolling.

#### **B. Audio (Engine Configuration)**

*   **Driver Type:** CoreAudio only for V1.
*   **Input Device:** Dropdown selector for Hardware Input.
*   **Output Device:** Dropdown selector for Hardware Output.
*   **Sample Rate:** Dropdown [44.1kHz | 48kHz | 88.2kHz | 96kHz].
*   **Buffer Size:** Dropdown [64 | 128 | 256 | 512 | 1024].
    *   *Note:* Critical for latency management.
*   **Input Channels:** Checkbox matrix to enable/disable specific inputs from the interface (1-8).

#### **C. MIDI & Sync**

*   **MIDI Inputs:** List of detected ports with "Active" checkboxes.
*   **Clock Source:** Radio button [Internal | External MIDI Clock].
    *   *Note:* Ableton Link clock source deferred to V2 (see §1.3).

#### **D. Library & VST**

*   **VST3 Search Paths:** List of directories.
    *   *Actions:* Add Path, Remove Path.
*   **Scan:**
    *   *Button:* "Rescan All Plugins".
    *   *Toggle:* "Scan on Startup".
*   **Storage Location:** Path selector for where Recordings/Project History are saved.

### **7.5. Safe Mode Recovery UI**

When the app enters Safe Mode (any level), the UI presents a specific "Recovery" dashboard:

*   Current Safe Mode level indicator (Level 1/2/3).
*   [ ] Toggle: Disable All VSTs (Level 1+)
*   [ ] Toggle: Reset Audio Driver (Level 2+)
*   [ ] Button: "Factory Reset Config" (Level 3)
*   [ ] "Resume Normal Operation" once user has addressed the issue.

---

## **7.6. Mobile Layout Reference (JSON)**

Summarized from mobile design reference materials.

See [Mobile_Layout_Reference.md](file:///Users/tribble/Sites/FlowZone/Spec%20versions/Mobile_Layout_Reference.md) for the full JSON structure and layout details.

## **8. Task Breakdown**

### **Phase 1: Core Foundation**
1.  **Task 1.1:** CMake Setup with aggressive compiler warnings (`-Wall -Wextra`) + CrashGuard sentinel.
2.  **Task 1.2:** Implement `SharedMemoryManager` (RingBuffers) & `PluginProcessManager` (Child spawning).
3.  **Task 1.3:** Build `TransportService` with unit tests (independent of Audio).
4.  **Task 1.4:** Implement `FlowEngine` audio graph.
5.  **Task 1.5:** `StateBroadcaster` with patch/snapshot protocol.
6.  **Task 1.6:** `DiskWriter` with tiered failure mode.
7.  **Task 1.7:** `CommandQueue` + `CommandDispatcher`.
8.  **Task 1.8:** `SessionStateManager` (undo/redo/autosave).

### **Phase 2: Frontend Infrastructure**
9.  **Task 2.1:** Vite + React + TS setup.
10. **Task 2.2:** WebSocket client with full connection lifecycle.
11. **Task 2.3:** Binary stream decoder + visualization canvas.
12. **Task 2.4:** Responsive layout wrapper.

### **Phase 3: Internal Engines**
13. **Task 3.1:** Implement `InternalSynth` (with Microtuning/Scala support) & `InternalFX`.
14. **Task 3.2:** Expose parameters via Configure Mode.

### **Phase 4: Sandboxing & VST**
15. **Task 4.1:** Implement `PluginHostApp`.
16. **Task 4.2:** Implement IPC with shared memory.

### **Phase 5: UI Implementation**
17. **Task 5.1:** Build Dashboard (responsive Grid/Stack).
18. **Task 5.2:** Build Instruments, FX, Mixer views.
19. **Task 5.3:** Build Settings panel (4 tabs).
20. **Task 5.4:** Safe Mode recovery UI (graduated levels).

### **Phase 6: Polish & Verification**
21. **Task 6.1:** Error boundary integration.
22. **Task 6.2:** Chaos monkey testing (random child process kills).
23. **Task 6.3:** Health endpoint + developer overlay.
24. **Task 6.4:** Log rotation + telemetry verification.
