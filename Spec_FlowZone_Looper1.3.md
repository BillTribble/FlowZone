# **Technical Design Doc: FlowZone**

**Version:** 1.4 (Architectural Hardening & Reliability Focus)  
**Target Framework:** JUCE 8.0.x (C++20) + React 18.3.1 (TypeScript 5.x)  
**Platform:** macOS (Standalone App, Silicon Native)  
**Repository Structure:** Monorepo  

## **1. Goals & Non-Goals**

### **1.1. Goals**

*   **Identity:** A macOS-first retrospective looping workstation.
    *   **Core Principle:** **"Destructive Creativity, Non-Destructive Storage."** Active loop slots are constantly merged/overwritten to keep flow moving, but *every* committed loop is saved to disk history.
*   **Hybrid Architecture:** Native C++ Audio Engine (Source of Truth) + React Web UI (Stateless View).
    *   **Local:** `juce::WebBrowserComponent` (embedded).
    *   **Remote:** Internal WebServer (Background Thread) for phone/tablet control.
    *   **Unified State:** All clients see the exact same session state, synced via WebSocket.
*   **Strict Decoupling:** 
    *   **Audio Priority:** The Audio Thread is sacred. It must never block on UI, Network, or Disk I/O.
    *   **Crash Resilience:** UI crashes do not stop audio. Plugin crashes do not kill the main engine.
*   **Efficient Visualization:** 
    *   **Throttled Stream:** Target 30fps "Best Effort" bi-directional feature stream.
    *   **Adaptive degradation:** If Audio Load > 70%, visualization frame rate drops automatically to prioritize audio processing.
*   **Operational Robustness:**
    *   **Crash Guard:** Detects boot loops and offers "Safe Mode".
    *   **Plugin Isolation:** VST3s run in separate child processes to protect the main engine.
    *   **Disk Safety:** Fixed-size ring buffers for recording to prevent OOM.
    *   **Observability:** Structured JSON logs and strict error codes.
*   **Global Phase-Sync:** Ableton Link integration.

### **1.2. Non-Goals**

*   **Windows/Linux Support:** macOS only for V1 to reduce platform-specific complexity.
*   **Cloud Sync:** Local network sharing only.
*   **Complex DAW Features:** No piano roll, no automation lanes. Pure audio looping/mangling.

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
        Broadcaster -->|Notify| WebServer
        
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
    *   **Priority:** `Realtime` (approx. macOS `UserInteractive` QoS for audio thread).
*   **`TransportService`:** 
    *   **Role:** Manages Transport state (Play/Pause, BPM, Phase). 
    *   **Isolation:** Decoupled from Engine to allow independent testing and Link synchronization logic.
*   **`CommandQueue`:** Lock-free SPSC FIFO for incoming JSON commands from the WebServer.
*   **`StateBroadcaster`:** Manages state diffing and WebSocket pushes. Uses `StateRevisionID` for reconciliation.
*   **`FeatureExtractor`:** Analyzes audio to produce visualization data.
    *   **Optimization:** Downsamples audio to ~100Hz analysis rate before feature extraction to savx`e CPU.
    *   **Priority:** `Background` thread.
*   **`DiskWriter`:** Background thread with a **256MB RingBuffer**.
    *   **Failure Mode:** If buffer fills (disk too slow), it **drops packets** and alerts UI ("Disk Handling Error" - `ERR_DISK_OVERLOAD`), rather than allocating indefinite RAM.
*   **`PluginProcessManager`:** Manages a pool of child processes, keyed by **VST Manufacturer**.
    *   *Strategy:* All plugins from the same manufacturer (e.g., "FabFilter") share a single process.
    *   *Rationale:* Plugins from the same vendor often share resources/libraries. Grouping them balances isolation with resource overhead.
    *   *Watchdog:* Pings each process every 1s.
    *   *VST Folder Monitor:* Hot-swaps new plugins.
        *   **Action:** Monitors VST3 folders for file changes (New/Deleted).
        *   **Response:** Instantly scans and adds the new plugin to the available list *without* requiring an engine reboot.

#### **B. Secondary Processes (Manufacturer-Isolated Hosts)**
*   **`PluginHostApp`:** A lightweight headless JUCE app that loads VST3s.
*   **Failure Isolation:** If a VST crashes, **only the process for that manufacturer dies**.
    *   **Recovery:** 
        1. Main Process detects pipe closure or heartbeat miss.
        2. Bypasses affected nodes (fades out to avoid clicks).
        3. Attempts simple respawn of host.
        4. If respawn fails 3x, marks Manufacturer as "Blacklisted" for the session.

#### **C. Frontend (React Web App)**
*   **Stack:** React 18.3.1, TypeScript, Vite, Tailwind CSS.
*   **Rendering:** Canvas API for meters (performance).
*   **Protocol:** Strictly typed JSON commands.

---

## **3. Data Model & Protocol**

### **3.1. Command Schema (TypeScript Definition)**

Alignment between C++ `structs` and TS `interfaces` is enforced via code generation or strict manual validation.

```typescript
// Commands sent from Client -> Engine
type Command = 
  | { cmd: 'SET_VOL'; slot: number; val: number; reqId: string }
  | { cmd: 'TRIGGER_SLOT'; slot: number; quantized: boolean }
  | { cmd: 'LOAD_VST'; slot: number; pluginId: string }
  | { cmd: 'SET_TEMPO'; bpm: number }
  | { cmd: 'PANIC'; scope: 'ALL' | 'ENGINE' }; // Immediate silence/reset

// State sent from Engine -> Client
interface AppState {
  meta: {
    revisionId: number;
    serverTime: number; // For jitter compensation
    mode: 'NORMAL' | 'SAFE_MODE';
  };
  transport: {
    bpm: number;
    isPlaying: boolean;
    barPhase: number; // 0.0 - 1.0
  };
  slots: Array<{
    id: string;
    state: 'EMPTY' | 'RECORDING' | 'PLAYING' | 'MUTED';
    volume: number;
    name: string;
    lastError?: number; // Error code if specific slot failed
  }>;
  system: {
    cpuLoad: number;
    diskBufferUsage: number;
    activePluginHosts: number;
  };
}
```

### **3.2. Binary Visualization Stream**

*   **Format:** Raw Float32 Array.
*   **Header:** `[Magic:4][FrameId:4][Timestamp:8]`
*   **Payload:** `[MasterRMS_L][MasterRMS_R][Slot1_RMS][Slot1_Spec_1...16]...`
*   **Bandwidth Control:** If `WS_SEND_QUEUE` > 2 frames, the server drops the oldest frame to prevent TCP backpressure affecting the specific client.

---

## **4. File System & Reliability**

### **4.1. Directory Structure**

Strict adherence for Agent implementation clarity.

```
/FlowZone_Root
├── /cmake
├── /src
│   ├── /engine          # Core Audio Logic
│   │   ├── /transport   # TransportService
│   │   ├── /ipc         # SharedMemoryManager
│   │   └── FlowEngine.cpp
│   ├── /host            # Child Process Host
│   ├── /shared          # Shared Types (C++ Primitives)
│   └── /web_client      # React App
├── /assets
└── CMakeLists.txt
```

### **4.2. User Data Paths (macOS Standard)**

| Data Type | Path | Purpose |
| :--- | :--- | :--- |
| **Config** | `~/Library/Application Support/FlowZone/config.json` | Active state. |
| **Backups** | `~/Library/Application Support/FlowZone/backups/` | Rolling snapshots (`state.backup.1.json`). |
| **Logs** | `~/Library/Logs/FlowZone/engine.json.log` | Structured logs. |
| **Recordings** | `~/Music/FlowZone/Library/{YYYY-MM-DD}/` | Audio content. |

### **4.3. Failure Handling Strategies**

| Scenario | Detection | Response | Recovery Action |
| :--- | :--- | :--- | :--- |
| **VST Crash** | IPC Broken Pipe | Bypass nodes, show Toast. | Auto-respawn host (max 3 tries). |
| **VST Hang** | Heartbeat > 100ms | Mark as "Unresponsive". | Kill process, then respawn. |
| **Disk Overload**| Buffer > 95% | Drop incoming packets. | Alert: `ERR_DISK_SLOW`. User must stop recording. |
| **Config Corrupt**| JSON Parse Fail | Log error. | Load `state.backup.1.json`. |
| **Boot Loop** | 3 Crashes < 60s | **SAFE MODE**. | Disable VSTs, Reset Audio Device to Default. |

---

## **5. Settings & Observability**

### **5.1. Observability (Developer Tab)**
*   **Live Log Tail:** Websocket subscription to log stream.
*   **Process Monitor:** List of active child processes with CPU/RAM usage.
*   **IPC Stats:** Shared Memory buffer health (read/write pointer distance).

### **5.2. Safe Mode**
If the app enters Safe Mode (due to crash loops), the UI presents a specific "Recovery" dashboard:
*   [ ] Toggle: Disable All VSTs
*   [ ] Toggle: Reset Audio Driver
*   [ ] Button: "Factory Reset Config"

---

## **6. Task Breakdown**

### **Phase 1: Core Foundation (Robustness)**
1.  **Task 1.1:** CMake Setup with aggressive compiler warnings (`-Wall -Wextra`).
2.  **Task 1.2:** Implement `SharedMemoryManager` (RingBuffers) & `ProcessManager` (Child spawning).
3.  **Task 1.3:** Build `TransportService` with unit tests (independent of Audio).

### **Phase 2: Engine & State**
4.  **Task 2.1:** Implement `FlowEngine` audio graph.
5.  **Task 2.2:** `StateBroadcaster` with "Snapshot/Restore" logic.
6.  **Task 2.3:** `DiskWriter` with "Drop Packet" failure mode simulation.

### **Phase 3: Frontend & Polish**
7.  **Task 3.1:** React shell with `ErrorBoundary` components.
8.  **Task 3.2:** Binary WebSocket consumer.
9.  **Task 3.3:** "Safe Mode" UI flow.
10. **Task 3.4:** Chaos Monkey Testing (Randomly kill child processes to verify recovery).

