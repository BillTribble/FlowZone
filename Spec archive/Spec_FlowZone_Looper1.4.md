# **Technical Design Doc: FlowZone**

**Version:** 1.4 (Architectural Hardening & Reliability Focus)  
**Target Framework:** JUCE 8.0.x (C++20) + React 18.3.1 (TypeScript 5.x)  
**Platform:** macOS (Standalone App, Silicon Native)  
**Repository Structure:** Monorepo  

## **1. Goals & Non-Goals**

### **1.1. Goals**

*   **Identity:** A macOS-first retrospective looping workstation.
    *   **Core Principle:** **"Destructive Creativity, Safe Capture."** Active loop slots are constantly merged/overwritten to keep flow moving, but *every* committed loop is saved to disk history.
*   **Hybrid Architecture:** Native C++ Audio Engine (Source of Truth) + React Web UI (Stateless View).
    *   **Local:** `juce::WebBrowserComponent` (embedded).
    *   **Remote:** Internal WebServer (Background Thread) for phone/tablet control.
    *   **Unified State:** All clients see the exact same session state, synced via WebSocket.
*   **Strict Decoupling:** 
    *   **Audio Priority:** The Audio Thread is sacred. It must never block on UI, Network, or Disk I/O.
    *   **Crash Resilience:** UI crashes do not stop audio. Plugin crashes do not kill the main engine.
*   **Premium Visualization:** 
    *   **Throttled Stream:** Target 30fps "Best Effort" bi-directional feature stream.
    *   **Adaptive degradation:** If Audio Load > 70%, visualization frame rate drops automatically to prioritize audio processing.
*   **Operational Robustness:**
    *   **Crash Guard:** Detects boot loops and offers "Safe Mode".
    *   **Plugin Isolation:** VST3s run in separate child processes.
    *   **Data Integrity:** Dynamic RAM buffering for disk writes. **Never** drop audio packets due to slow disk unless RAM is exhausted.
    *   **Observability:** Structured JSON logs, telemetry stream, and performance metrics.
*   **Agent-Optimized:** Codebase structured for clarity, with single-source-of-truth schemas.

### **1.2. Non-Goals**
*   **Windows/Linux Support:** macOS only for V1.
*   **Cloud Sync:** Local network sharing only.
*   **Complex DAW Features:** No piano roll, no automation lanes.

---

## **2. System Architecture**

### **2.1. Component Diagram**

```mermaid
graph TD
    User[User] -->|Interacts| UI[React UI]
    UI -->|WebSocket JSON| WebServer[CivetWeb (Background Thread)]
    UI <--|Binary Stream| FeatureStream[Feature Extractor]
    
    subgraph "Main Process (C++)"
        WebServer -->|Cmd Queue| Engine[FlowEngine (Realtime Priority)]
        
        Engine -->|State Update| Broadcaster[StateBroadcaster]
        Broadcaster -->|Diff/Patch| WebServer
        
        Engine <-->|ProcessBlock| Transport[Transport Service]
        Engine -->|Audio Blocks| FeatureStream
        Engine -->|Audio Blocks| DiskWriter[DiskWriter Thread]
        
        DiskWriter -->|Write| FileSystem[SSD Storage]
    end
    
    subgraph "IPC Layer"
        SharedMem[Shared Memory RingBuffers]
    end

    subgraph "Child Processes (One per Manufacturer)"
        HostA[Host: FabFilter]
        HostB[Host: Arturia]
        Engine <-->|Audio/Events| SharedMem <--> HostA
        Engine <-->|Audio/Events| SharedMem <--> HostB
    end
```

### **2.2. High-Level Components**

#### **A. Main Process (C++ macOS App)**
*   **`FlowEngine`:** The singleton `juce::AudioProcessor`.
    *   **Role:** Real-time audio processing DAG.
    *   **Priority:** `Realtime` (approx. macOS `UserInteractive` QoS).
*   **`TransportService`:** 
    *   **Role:** Manages Transport state (Play/Pause, BPM, Phase). 
    *   **Isolation:** Decoupled from Engine for independent testing.
*   **`CommandDispatcher`:** 
    *   **Role:** Routes incoming commands to appropriate handlers.
    *   **Mechanism:** Lock-free SPSC FIFO.
*   **`StateBroadcaster`:** 
    *   **Mechanism:** Maintains a `StateRevisionID`.
    *   **Protocol:** Sends **JSON Patches** (RFC 6902 style) for small updates (vol/pan), and **Full Snapshots** on connection/re-sync.
    *   **Recovery:** If a client misses a revision, they request a Full Snapshot.
*   **`FeatureExtractor`:** 
    *   **Optimization:** Double-buffered atomic exchange.
    *   **Rate:** Target 30fps.
*   **`DiskWriter`:** Background thread.
    *   **Reliability:** 
        1.  **Primary:** 256MB RingBuffer.
        2.  **Fallback:** If buffer fills, allocate **temporary overflow blocks** in RAM (up to 1GB).
        3.  **Failure:** Only if Overflow > 1GB, stop recording and trigger `ERR_DISK_CRITICAL`. **Never silently drop samples.**
*   **`PluginProcessManager`:** 
    *   **Grouping:** Manufacturer-based.
    *   **Watchdog:** 250ms heartbeat (high frequency for better responsiveness).
    *   **Hot-Swap:** Monitors VST3 folders for instant availability.

#### **B. Secondary Processes (Manufacturer-Isolated Hosts)**
*   **`PluginHostApp`:** Lightweight headless JUCE app.
*   **Failure Isolation:** 
    *   **Crash:** Detected via Pipe closure.
    *   **Hang:** Detected via Heartbeat timeout (>1s).
    *   **Recovery:** Bypass nodes -> Respawn Host -> Restore State. Max 3 retries per session.

#### **C. Frontend (React Web App)**
*   **Stack:** React 18.3.1, TypeScript, Vite.
*   **Rendering:** 
    *   **UI:** React DOM.
    *   **Visualizers:** Canvas API (via `requestAnimationFrame`).
*   **Resilience:**
    *   **Connection Lost:** UI shows "Reconnecting..." overlay. Audio continues.
    *   **Reconnection:** UI requests `GET_FULL_STATE` to sync.

---

## **3. Data Model & Protocol**

### **3.1. Single Source of Truth**

To ensure implementability by agents, all commands and state shapes must be defined in a **Schema Registry** (e.g., `src/shared/protocol/schema.ts` or a JSON schema) that acts as the contract.

### **3.2. Command Schema (TypeScript Definition)**

```typescript
// Commands sent from Client -> Engine
// Must match C++ decoding logic EXACTLY.
type Command = 
  | { cmd: 'SET_VOL'; slot: number; val: number; reqId: string } // val: 0.0 - 1.0
  | { cmd: 'TRIGGER_SLOT'; slot: number; quantized: boolean }
  | { cmd: 'LOAD_VST'; slot: number; pluginId: string }
  | { cmd: 'SET_TEMPO'; bpm: number }
  | { cmd: 'UNDO'; scope: 'SESSION' }
  | { cmd: 'PANIC'; scope: 'ALL' | 'ENGINE' }; // Immediate silence/reset

// Server Responses
type ServerMessage = 
  | { type: 'STATE_FULL'; data: AppState }
  | { type: 'STATE_PATCH'; ops: JsonPatchOp[]; revId: number }
  | { type: 'ERROR'; code: number; msg: string; reqId?: string };
```

### **3.3. App State**

```typescript
interface AppState {
  meta: {
    revisionId: number;
    serverTime: number; // For jitter compensation
    mode: 'NORMAL' | 'SAFE_MODE';
    version: string;
  };
  transport: {
    bpm: number;
    isPlaying: boolean;
    barPhase: number;
  };
  slots: Array<{
    id: string; // UUID
    state: 'EMPTY' | 'RECORDING' | 'PLAYING' | 'MUTED';
    volume: number;
    pan: number; // -1.0 to 1.0
    name: string;
    pluginChain: PluginInstance[];
  }>;
  system: {
    cpuLoad: number; // 0.0 - 1.0
    diskBufferUsage: number; // 0.0 - 1.0
    memoryUsageMB: number;
  };
}
```

### **3.4. Binary Visualization Stream**

*   **Format:** Raw Float32 Array (Little Endian).
*   **Transmission:** Separate WebSocket Channel or Binary frames on main socket.
*   **Optimization:**
    *   Client sends `ACK` after every 30 frames.
    *   Server tracks `frames_in_flight`.
    *   If `frames_in_flight > 5`, Server drops frame (User sees stutter, but audio is safe).

---

## **4. File System & Reliability**

### **4.1. Directory Structure**

Strict adherence for Agent clarity.

```
/FlowZone_Root
├── /cmake
├── /src
│   ├── /engine          # Core Audio Logic
│   │   ├── /transport   # TransportService
│   │   ├── /ipc         # SharedMemoryManager
│   │   └── FlowEngine.cpp
│   ├── /host            # Child Process Host
│   ├── /shared          # Shared Types & Schemas
│   │   ├── /utils
│   │   ├── /protocol    # schema.ts / commands.h (Generated or synced)
│   │   └── /errors      # ErrorCodes.h
│   └── /web_client      # React App
├── /assets
└── CMakeLists.txt
```

### **4.2. Failure Handling Strategies**

| Scenario | Severity | Response | Recovery Action |
| :--- | :--- | :--- | :--- |
| **VST Crash** | High | Bypass nodes, Toast UI "Plugin Died". | Auto-respawn host. |
| **VST Hang** | High | Heartbeat timeout (>1s). | Kill SIGKILL, then respawn. |
| **Disk Slow** | Medium | Buffer in RAM (Overflow). | If RAM > 1GB, Stop Rec & Alert. |
| **Boot Loop** | Critical | 3 Crashes < 60s. | Enter **SAFE MODE**. |
| **Web Server Crash** | Low | UI disconnects. Audio continues. | Watchdog restarts Server thread. |

### **4.3. Session Safety (Undo/Redo)**

*   **Action History:** Every destructive command (Record Over, Delete, Clear) pushes the *previous* state to a `HistoryStack`.
*   **Snapshot:** State snapshots are lightweight JSON. Audio files are **never deleted** immediately, only marked for "Cleanup on Exit" or "Garbage Collection".
*   **Implementation:**
    *   `SessionStateManager` holds `std::deque<AppState> undoStack`.
    *   On `UNDO`, restore state from stack and reload referenced audio files.

---

## **5. Operational Robustness (Observability)**

### **5.1. Error Codes (Shared/Errors.h)**

Agents must strictly use these codes.

*   `1000-1999`: System (Boot, Config, Filesystem)
    *   `1001`: `ERR_DISK_FULL`
    *   `1002`: `ERR_DISK_CRITICAL` (Write failed)
*   `2000-2999`: Audio Engine
    *   `2001`: `ERR_AUDIO_DROPOUT` (XRUN)
*   `3000-3999`: Plugins
    *   `3001`: `ERR_PLUGIN_CRASH`
    *   `3002`: `ERR_PLUGIN_TIMEOUT`

### **5.2. Telemetry**
*   **Local Metric Store:** A circular buffer of the last 10 minutes of performance metrics (CPU, RAM, FPS).
*   **Developer Overlay:** UI can request `GET_METRICS_HISTORY` to render a graph for debugging performance issues.

---

## **6. Agent Implementation Guide**

**How to add a new command (e.g., `MUTE_SLOT`):**

1.  **Define Protocol:** Update `src/shared/protocol/schema.ts` to include `{ cmd: 'MUTE_SLOT', ... }`.
2.  **Update C++:** Add handler in `FlowEngine::handleCommand()`.
3.  **Update State:** Update `AppState` struct to include `muted` bool.
4.  **Update UI:** Add dispatch function in React and update Component to reflect `state.muted`.

**Rule:** Never change the `CommandQueue` implementation logic itself, only add new types of commands.
