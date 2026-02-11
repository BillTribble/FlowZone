# **Technical Design Doc: FlowZone**

**Version:** 1.3 (Revised for Robustness & Clarity)
**Target Framework:** JUCE 8 (C++20) + React 18
**Platform:** macOS (Standalone App)
**Repository Structure:** Monorepo

## **1. Goals & Non-Goals**

### **1.1. Goals**

*   **Identity:** A macOS-first retrospective looping workstation.
    *   **Core Principle:** **"Destructive Creativity, Non-Destructive Storage."** Active loop slots are constantly merged/overwritten to keep flow moving, but *every* committed loop is saved to disk history.
*   **Hybrid Architecture:** Native C++ Audio Engine (Source of Truth) + React Web UI (Stateless View).
    *   **Local:** `juce::WebBrowserComponent` (embedded).
    *   **Remote:** Internal WebServer (LAN) for phone/tablet control.
    *   **Unified State:** All clients see the exact same session state, synced via WebSocket.
*   **Strict Decoupling:** Engine runs independently of UI. UI crashes do not stop audio.
*   **Efficient Visualization:** 20-30fps "Best Effort" bi-directional feature stream.
    *   **Audio First:** Visual rendering is strictly secondary to audio processing. If CPU load is high, the visualization frame rate drops automatically.
*   **Operational Robustness:**
    *   **Crash Guard:** Detects boot loops.
    *   **Plugin Isolation:** VST3s run in a separate process to protect the main engine.
    *   **Disk Safety:** Fixed-size ring buffers for recording to prevent OOM.
*   **Global Phase-Sync:** Ableton Link integration.

### **1.2. Non-Goals**

*   **Windows/Linux Support:** macOS only for V1.
*   **Cloud Sync:** Local network sharing only.
*   **Complex DAW Features:** No piano roll, no automation lanes. Pure audio looping/mangling.

---

## **2. System Architecture**

### **2.1. Component Diagram**

```mermaid
graph TD
    User[User] -->|Interacts| UI[React UI (WebBrowser/Safari)]
    UI -->|WebSocket JSON| Server[CivetWeb Server]
    UI <--|Binary Stream (60fps)| FeatureStream[Feature Extractor]
    
    subgraph "Main Process (C++)"
        Server -->|CommandQueue| Engine[FlowEngine (Audio Thread)]
        Engine -->|State Update| Broadcaster[StateBroadcaster]
        Broadcaster -->|Notify| Server
        
        Engine -->|Audio Blocks| FeatureStream
        Engine -->|Audio Blocks| DiskWriter[DiskWriter Thread]
        
        DiskWriter -->|Write| FileSystem[SSD Storage]
    end
    
    subgraph "Child Processes (One per Manufacturer)"
        HostA[Host: FabFilter]
        HostB[Host: Arturia]
        Engine <-->|IPC Shared Mem| HostA
        Engine <-->|IPC Shared Mem| HostB
    end
```

### **2.2. High-Level Components**

#### **A. Main Process (C++ macOS App)**
*   **`FlowEngine`:** The singleton `juce::AudioProcessor`.
*   **`CommandQueue`:** Lock-free FIFO for incoming JSON commands.
*   **`StateBroadcaster`:** Manages state diffing and WebSocket pushes. Uses `StateRevisionID` for reconciliation.
*   **`FeatureExtractor`:** Analyzes audio to produce visualization data (target 30fps).
    *   *Priority:* Runs on a low-priority thread. Skips processing if the ring buffer is behind.
*   **`DiskWriter`:** Background thread with a **256MB RingBuffer**.
    *   *Failure Mode:* If buffer fills (disk too slow), it **drops packets** and alerts UI ("Disk Handling Error"), rather than allocating indefinite RAM.
*   **`PluginProcessManager`:** Manages a pool of child processes, keyed by **VST Manufacturer**.
    *   *Strategy:* All plugins from the same manufacturer (e.g., "FabFilter") share a single process.
    *   *Rationale:* Plugins from the same vendor often share resources/libraries. Grouping them balances isolation with resource overhead.
    *   *Watchdog:* Pings each process every 1s.
    *   *VST Folder Monitor:* Hot-swaps new plugins.
        *   **Action:** Monitors VST3 folders for file changes (New/Deleted).
        *   **Response:** Instantly scans and adds the new plugin to the available list *without* requiring an engine reboot.

#### **B. Secondary Processes (Manufacturer-Isolated Hosts)**
*   **`PluginHostApp`:** A lightweight headless JUCE app that loads VST3s.
*   **Failure Isolation:** If a VST crashes, **only the process for that manufacturer dies**. Ideally, other VSTs (from other vendors) keep running.
    *   *Recovery:* The Main Process detects the specific pipe closure, bypasses *only* the affected nodes, and attempts to respawn that specific host.

#### **C. Frontend (React Web App)**
*   **Stack:** React 18, TypeScript, Vite, Tailwind CSS.
*   **Rendering:** Canvas API for meters (performance).
*   **Protocol:** Strictly typed JSON commands (see Section 3).

---

## **3. Data Model & Protocol**

### **3.1. Command Schema (TypeScript Definition)**

This schema ensures alignment between C++ and TS Key-values.

```typescript
// Commands sent from Client -> Engine
type Command = 
  | { cmd: 'SET_VOL'; slot: number; val: number } // val: 0.0 - 1.0
  | { cmd: 'TRIGGER_SLOT'; slot: number; quantized: boolean }
  | { cmd: 'LOAD_VST'; slot: number; pluginId: string }
  | { cmd: 'SET_TEMPO'; bpm: number }
  | { cmd: 'CLEAR_ALL' };

// State sent from Engine -> Client
interface AppState {
  revisionId: number;
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
  }>;
  cpuLoad: number;
  diskBufferUsage: number; // 0.0 - 1.0
}
```

### **3.2. Binary Visualization Stream**

*   **Format:** Raw Float32 Array, sent ~30 times/second via WebSocket (Binary Type).
*   **Header:** `[Magic:4][FrameId:4]`
*   **Payload:** `[MasterRMS_L][MasterRMS_R][Slot1_RMS][Slot1_Spec_1...16]...[Slot8_RMS][Slot8_Spec_1...16]`
*   **Bandwidth:** ~16KB/sec. Ultra-lightweight.

---

## **4. File System & Directory Structure**

Strict adherence to this structure for Agent clarity.

```
/FlowZone_Root
├── /cmake               # CMake modules
├── /src
│   ├── /engine          # C++ Audio Engine
│   │   ├── FlowEngine.cpp
│   │   ├── DiskWriter.cpp
│   │   └── ...
│   ├── /host            # C++ Plugin Host App
│   │   └── PluginHost.cpp
│   ├── /shared          # Shared C++ Types/Constants
│   └── /web_client      # React App
│       ├── /src
│       │   ├── /components
│       │   ├── /hooks
│       │   └── /types   # Shared Protocol Types
│       ├── package.json
│       └── vite.config.ts
├── /assets              # Images, Fonts
└── CMakeLists.txt
```

### **4.1. Audio Persistence**
*   **Format:** FLAC 24-bit / 48kHz.
*   **Directory Structure:** Riffs are automatically dropped into folders organized by date.
*   **Path:** `~/Music/FlowZone/Library/{YYYY-MM-DD}/`
*   **Naming:** `Riff_{Timestamp}_{SessionID}_{SlotIndex}.flac`

---

---

## **5. Settings Panel Specification**

The settings view is divided into tabs. Changes sync immediately to the Engine.

### **5.1. VST & Library**
*   **VST Overview List:**
    *   **Top Section (Issues):** specific area highlighting plugins that failed to scan or have crashed recently.
        *   *Actions:* "Retry Scan" button for individual failures. "Ignore" to hide.
    *   **Main Section (Working):** Standard list of active plugins grouped by Manufacturer.
*   **Monitor Status:**
    *   *Visual:* "Folder Monitor Active" indicator.
    *   *Real-time:* When a new plugin is dropped into the VST3 folder, it instantly appears in this list without a restart.

### **5.2. Audio & MIDI**
*   **Audio:** Driver, Device, Sample Rate, Buffer Size (Critical for latency).
*   **MIDI:** Input Ports (Active/Inactive), Clock Source.
*   **Link:** Ableton Link Sync Toggle.

---

## **6. Operational Robustness & Observability**

### **6.1. Telemetry & Logging**
*   **Structured Logging:** The Engine writes strict JSON-lines logs to `~/Library/Logs/FlowZone/engine.log`.
    *   `{"ts": 123456789, "level": "INFO", "component": "VST", "msg": "Loaded plugin X"}`
*   **In-App Console:** The "Settings" panel includes a "Developer" tab that tails this log file in real-time for debugging without Xcode.

### **6.2. Failure Handling Strategies**

| Failure Scenario | Detection | Response | User Feedback |
| :--- | :--- | :--- | :--- |
| **VST Crash** | IPC Pipe Broken | (1) Bypass VST nodes **only for that manufacturer**. (2) Respawn Host. (3) Reload plugins. | Toast: "Relieved 'FabFilter' Host" |
| **Disk Overload**| Buffer > 95% | (1) Drop new audio packets. (2) Keep playing. || Red Status Icon: "Disk Too Slow" |
| **Network Lag** | WebSocket Ping > 500ms | UI freezes meters, shows "Reconnecting..." | Overlay with spinner. |
| **Startup Crash**| 3 crashes in < 30s | Boot into "Safe Mode" (No Audio, No VSTs). | Prompt: "Reset Settings?" |

---

## **7. Task Breakdown**

### **Phase 1: Core Foundation (C++)**
1.  **Task 1.1:** Setup CMake Monorepo with `Engine`, `Host`, and `WebClient` targets.
2.  **Task 1.2:** Implement `FlowEngine` + `DiskWriter` (RingBuffer logic).
3.  **Task 1.3:** Implement `PluginHost` Process + IPC (Shared Memory).
4.  **Task 1.4:** Build JSON `CommandQueue` & `StateBroadcaster`.

### **Phase 2: Frontend & Visuals**
5.  **Task 2.1:** React + Vite setup. Define `Protocol.ts`.
6.  **Task 2.2:** Real-time Canvas Visualizer (Binary Stream consumer).
7.  **Task 2.3:** Dashboard Layout (Responsive Grid).

### **Phase 3: Integration & Polish**
8.  **Task 3.1:** Connect VST Parameters to UI.
9.  **Task 3.2:** Build Settings Panel (VST Watchdog Status, Scan Retry Logic).
10. **Task 3.3:** Implement Telemetry/Console.
11. **Task 3.4:** Stress Test (simulate disk stall, VST crash).
