# **Technical Design Doc: FlowZone**

**Version:** 1.6 (Unified Architecture + Product Specification)  
**Target Framework:** JUCE 8.0.x (C++20) + React 18.3.1 (TypeScript 5.x)  
**Platform:** macOS (Standalone App + VST3 Plugin, Silicon Native)  
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
    *   **Unified State:** The embedded client receives the engine's state via WebSocket (localhost).
    *   **Remote (V2):** Internal WebServer for phone/tablet control over LAN. See §1.3 Future Goals.
*   **Multi-User Collaboration (V2):** Remote multi-user jamming is a V2 feature. V1 is single-user on the local machine. See §1.3 for plans.
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
*   **Multi-Channel Output:** 8 stereo pairs (16 channels) for DAW recording via the VST3 plugin target. Each slot maps to one stereo pair, enabling multi-track capture of live sessions.
*   **Dual Build Target:** Built simultaneously as a Standalone App and a VST3 Plugin from the same Projucer project. When running as a VST3, the app provides multi-channel output for DAW recording of riffs. VST3 hosting (Phase 7) is only available in Standalone mode.
*   **Retrospective "Always-On" Capture:** Implement a lock-free circular buffer (~96s, enough for 8 bars at 20 BPM).
*   **Hybrid Sound Engine:** VST3 Hosting, Internal Procedural Instruments, Mic Input Processing.
*   **Microtuning Support:** Internal synths must support microtuning via `.scl` (Scala) and `.kbm` files, with standard presets (Just Intonation, Pythagorean, Slendro, Pelog, 12TET).
*   **"Configure Mode" for VST Parameters:** Users must explicitly "touch" a VST knob to expose it to the React UI.
*   **Smart Layering (Auto-Merge):** The "9th Loop" trigger automatically sums Slots 1-8 into Slot 1.
*   **FX Mode / Resampling:** Route specific layers through FX and resample into a new layer (see §7.6.2 FX Mode).

### **1.2. Non-Goals**
*   **Windows/Linux Support:** macOS only for V1.
*   **Cloud Sync:** Local network sharing only.
*   **Complex DAW Features:** No piano roll, no automation lanes.

### **1.3. Future Goals (V2 Stretch)**
*   **Remote Web Server (Multi-Device):** Internal WebServer (background thread) exposing a real-time control surface on the LAN for phone/tablet control. Unified state sync via WebSocket so all clients see the same session.
*   **Multi-User Collaboration:** 2-4+ simultaneous users jamming over local WiFi. Each client (local or remote) receives a real-time control surface.
*   **Sample Engine:** Full sample playback engine with WAV/FLAC/MP3/AIFF support, playback modes (One-Shot, Gated, Looped, Sliced), per-pad sample assignment, drag-and-drop loading, and JSON preset system. See `Audio_Engine_Specifications.md` §5 for full design.
*   **MP3 / AAC Export:** User preference to save recordings as MP3 or AAC to save disk space. Requires LAME (GPL) or macOS AudioToolbox. FLAC is V1's only format.
*   **Ableton Link Integration:** Global phase-sync across devices and applications.
*   **Export Riffs:** Export riff audio (stems, mixdown) to disk.
*   **Windows/Linux Ports**
*   **Tap Tempo Cold Start:** Time-between-taps sets session tempo when transport is paused and slots are empty.
*   **WebGL Visualizers:** GPU-accelerated visualization option.
*   **MIDI Clock Sync:** External MIDI Clock as transport source.
*   **Multi-User Retrospective Buffers:** V1 uses a single global retrospective buffer (single-user). V2 will add per-user buffers for multi-user jamming — each connected user gets their own capture buffer. This requires a session settings item to configure the number of active users (2-4+), with each user's buffer pre-allocated on session start.
*   **Undo/Redo:** Session-level undo/redo. Deferred from V1 because Riff History (always-on capture + commit history) provides the primary recovery mechanism.

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

        SessionMgr[SessionStateManager] <-->|Auto-Save/Load| Engine
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
*   **Master Limiter:** A brickwall limiter is applied to the master stereo output as the final stage of `processBlock`. Active only in Standalone mode — **bypassed in VST3 multi-channel mode** so that dynamics are preserved for DAW recording. Ceiling: -0.3 dBFS, lookahead: 1ms.
*   **Bus Configuration:** In VST3 mode, declare 8 stereo output buses in `BusesProperties` (one per slot). In Standalone mode, declare a single stereo output bus (master only). Use `#if JucePlugin_Build_VST3` preprocessor guards for the bus configuration.

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
*   **Role:** Auto-save, crash-recovery snapshots, session loading, riff history management.
*   **Implementation:**
    *   Manages the riff history (commit, load, delete operations).
    *   Audio files are **never deleted immediately** — only marked for garbage collection on clean exit. `DELETE_JAM` immediately removes session metadata and riff history entries; associated audio files are marked for GC and deleted on next clean exit.
    *   **Auto-save:** Writes a recovery snapshot every 30 seconds to `~/Library/Application Support/FlowZone/backups/autosave.json`.
    *   **No Undo/Redo in V1:** Riff History serves as the primary recovery mechanism — every committed riff is preserved and can be loaded at any time. Undo/Redo is deferred to V2 (see §1.3).

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
*   **Network Configuration:**
    *   Default port: `8765` (configurable via `config.json` field `"serverPort": 8765`).
    *   React dev mode connects to `ws://localhost:8765`.
    *   Health endpoint at `http://localhost:8765/api/health`.
*   **Connection Lifecycle:**
    1.  **Connect:** Client → opens WebSocket to `ws://localhost:<serverPort>` with `{ clientId: <UUID> }`.
    2.  **Handshake:** Server → sends `STATE_FULL` with current `revisionId` and `protocolVersion`.
    3.  **Version check:** If `protocolVersion` doesn't match, server sends `ERROR { code: 1100, msg: 'PROTOCOL_MISMATCH' }` and closes.
    4.  **Steady state:** Server → `STATE_PATCH` ops. Client → commands.
    5.  **Disconnect:** Client shows "Reconnecting…" overlay. Audio continues unaffected. Reconnect uses exponential backoff: 100ms → 200ms → 400ms → … → max 5s.
    6.  **Reconnect:** Client → `WS_RECONNECT` with last known `revisionId`. Server sends diff if revision is recent, or full snapshot if stale.
    7.  **Optional PIN auth:** If `config.json` has `"requirePin": true`, client must send `{ cmd: 'AUTH', pin: '…' }` before any other command is accepted. Prevents accidental LAN access during live performance.

#### **M. Internal Audio Engines (Native C++)**

> **Detailed Specifications:** See [Audio_Engine_Specifications.md](./Audio_Engine_Specifications.md) for comprehensive implementation details of all effects, presets, and sound generators.

*   **`InternalSynth`:**
    *   **Engines:** Drums, Bass, Leads (Notes).
    *   **Notes Mode:** 12 starting presets (Sine Bell, Saw Lead, Square Bass, Triangle Pad, Pluck, Warm Pad, Bright Lead, Soft Keys, Organ, EP, Choir, Arp).
    *   **Bass Mode:** 12 starting presets (Sub, Growl, Deep, Wobble, Punch, 808, Fuzz, Reese, Smooth, Rumble, Pluck Bass, Acid).
    *   **Drum Mode:** 4 kit presets (Synthetic, Lo-Fi, Acoustic, Electronic) with 16 procedural drum sounds each.
    *   **Tuning:** Implementation of MTS-ESP or internal frequency mapping for `.scl` support.
    *   **Preset Tunings:** Just Intonation, Pythagorean, Slendro, Pelog, 12TET (Default).
*   **`InternalFX`:**
    *   **Core FX (12):** Lowpass, Highpass, Reverb, Gate, Buzz, GoTo, Saturator, Delay, Comb, Distortion, Smudge, Channel.
    *   **Infinite FX (11):** Keymasher, Ripper, Ringmod, Bitcrusher, Degrader, Pitchmod, Multicomb, Freezer, Zap Delay, Dub Delay, Compressor.
    *   All effects support XY Pad mapping (except Keymasher which uses 3×4 button grid).
*   **`SampleEngine` (V2):** Deferred to V2. See §1.3 Future Goals and `Audio_Engine_Specifications.md` §5 for full design.
*   **`MicProcessor`:**
    *   **Input Chains:** Gain control, monitor modes, built-in reverb (applied to the input signal before it enters the retrospective capture buffer).
    *   **Waveform Display:** Real-time visualization with peak detection.

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

// Valid parameter names for the SET_KNOB command (Adjust tab)
type KnobParameter = 'pitch' | 'length' | 'tone' | 'level' | 'bounce' | 'speed' | 'reverb' | 'reverb_mix' | 'room_size';

// Valid scale names for SET_KEY command
type Scale = 'major' | 'minor' | 'minor_pentatonic' | 'major_pentatonic' | 'dorian' | 'mixolydian' | 'blues' | 'chromatic';

type Command =
  // Volume & Pan
  | { cmd: 'SET_VOL'; slot: number; val: number; reqId: string }   // val: 0.0 - 1.0
  | { cmd: 'SET_PAN'; slot: number; val: number }                  // val: -1.0 to 1.0

  // Transport (Play/Pause only — no Stop. Pausing holds all slot states; resuming continues from where it left off.)
  | { cmd: 'PLAY' }
  | { cmd: 'PAUSE' }
  | { cmd: 'SET_TEMPO'; bpm: number }                              // bpm: 20.0 - 300.0
  | { cmd: 'TOGGLE_METRONOME' }
  | { cmd: 'TOGGLE_QUANTISE' }                                     // Toggle quantise on/off
  | { cmd: 'SET_KEY'; rootNote: number; scale: Scale }

  // Slot Control (no explicit record start/stop — see §3.10 Retrospective Capture)
  | { cmd: 'MUTE_SLOT'; slot: number }
  | { cmd: 'UNMUTE_SLOT'; slot: number }
  | { cmd: 'SET_LOOP_LENGTH'; bars: number }                       // 1, 2, 4, or 8. **Dual-purpose:** Sets the loop length AND immediately captures that duration from the retrospective buffer into the next empty slot. This is the primary recording mechanism — see §3.10. In FX Mode, captures FX-processed audio instead (see §7.6.2 FX Mode).

  // Performance (Pad / Note Triggering)
  | { cmd: 'NOTE_ON'; note: number }                               // Trigger a note/pad. note: MIDI note number (0-127). Pad grid maps to notes via scale/root/transpose (§7.6.2). Drums use fixed pad-to-note mapping (§4.1 Audio Engine Spec). Equivalent to external MIDI NOTE_ON.
  | { cmd: 'NOTE_OFF'; note: number }                              // Release a note/pad. Drums ignore NOTE_OFF (one-shot). Sustained synth presets (Organ, Warm Pad, etc.) require NOTE_OFF to stop sounding.

  // Mode & FX
  | { cmd: 'SELECT_MODE'; category: string; presetId: string }
  | { cmd: 'SELECT_EFFECT'; effectId: string }
  | { cmd: 'SET_KNOB'; param: KnobParameter; val: number }         // Adjust tab knob. Routed based on current activeMode.category — e.g., 'reverb_mix' in Microphone mode adjusts the mic's built-in reverb, while in Notes mode it adjusts the synth's reverb send.
  | { cmd: 'LOAD_VST'; slot: number; pluginId: string }
  | { cmd: 'SET_XY_PAD'; x: number; y: number }                    // XY pad position (0.0-1.0 each)
  | { cmd: 'FX_ENGAGE' }                                           // Finger down on XY pad — activate FX
  | { cmd: 'FX_DISENGAGE' }                                        // Finger up on XY pad — bypass FX
  | { cmd: 'SELECT_FX_SOURCE_SLOTS'; slots: number[] }             // FX Mode: select which slots to route through FX
  | { cmd: 'KEYMASHER_ACTION'; action: KeymasherButton }           // Keymasher performance button press

  // Microphone / Input
  | { cmd: 'SET_INPUT_GAIN'; val: number }                         // val: -60dB to +40dB
  | { cmd: 'TOGGLE_MONITOR_INPUT' }                                // Toggle direct monitoring on/off
  | { cmd: 'TOGGLE_MONITOR_UNTIL_LOOPED' }                         // Toggle monitor-until-looped on/off

  // Session & Riff Management
  | { cmd: 'COMMIT_RIFF' }                                         // Save the current session state (volumes, pans, mutes) as a new entry in riff history. Does NOT capture audio from the retrospective buffer — use loop length buttons (SET_LOOP_LENGTH) for audio capture. Only available from the Mixer tab, and only enabled when the user has changed mix levels or mute states since the last commit. In FX Mode, captures FX-processed audio and deletes source slots (see §7.6.2 FX Mode).
  | { cmd: 'LOAD_RIFF'; riffId: string }                           // Load riff from history
  | { cmd: 'DELETE_RIFF'; riffId: string }                         // Delete riff from history
  | { cmd: 'NEW_JAM' }                                              // Create a new jam session
  | { cmd: 'LOAD_JAM'; sessionId: string }                         // Load an existing jam session
  | { cmd: 'RENAME_JAM'; sessionId: string; name: string }         // Rename a jam session
  | { cmd: 'DELETE_JAM'; sessionId: string }                       // Delete a jam session and its audio
  // UI Settings (UI-only, stored in localStorage — not sent to engine)
  | { cmd: 'TOGGLE_NOTE_NAMES' }                                   // Toggle note name display on pads

  // Engine Settings (sent to engine via WebSocket)
  | { cmd: 'SET_AUDIO_DEVICE'; deviceType: 'input' | 'output'; deviceId: string }
  | { cmd: 'SET_SAMPLE_RATE'; rate: number }                       // 44100, 48000, 88200, 96000
  | { cmd: 'SET_BUFFER_SIZE'; size: number }                       // 16, 32, 64, 128, 256, 512, 1024
  | { cmd: 'SET_INPUT_CHANNELS'; channels: number[] }              // Enable specific input channels
  | { cmd: 'SET_OUTPUT_CHANNELS'; channels: number[] }             // Enable specific output channels
  | { cmd: 'PLAY_TEST_TONE' }                                      // Play test tone through output
  | { cmd: 'SET_MIDI_INPUT_ACTIVE'; portId: string; active: boolean }
  | { cmd: 'SET_CLOCK_SOURCE'; source: 'internal' | 'external_midi' }
  | { cmd: 'SET_VST_SEARCH_PATHS'; paths: string[] }               // Update VST3 search directories
  | { cmd: 'RESCAN_PLUGINS' }                                      // Trigger full plugin rescan
  | { cmd: 'SET_STORAGE_LOCATION'; path: string }                  // Set recordings storage path
  | { cmd: 'SET_RIFF_SWAP_MODE'; mode: 'instant' | 'swap_on_bar' } // How riff history taps switch playback

  // System
  | { cmd: 'PANIC'; scope: 'ALL' | 'ENGINE' }                     // ALL: silence + reset all slots + stop transport. ENGINE: silence audio output only, preserve state.
  | { cmd: 'AUTH'; pin: string }                                    // Optional PIN auth
  | { cmd: 'WS_RECONNECT'; revisionId: number; clientId: string }; // Client reconnection with last known state revision

// Valid Keymasher button actions
type KeymasherButton = 'repeat' | 'pitch_down' | 'pitch_rst' | 'pitch_up' | 'reverse' | 'gate' | 'scratch' | 'buzz' | 'stutter' | 'goto' | 'goto2' | 'buzz_slip';

// JSON Patch operation (RFC 6902)
interface JsonPatchOp {
  op: 'add' | 'remove' | 'replace' | 'move' | 'copy' | 'test';
  path: string;
  value?: any;
  from?: string;
}

// Server Responses
type ServerMessage =
  | { type: 'STATE_FULL'; data: AppState }
  | { type: 'STATE_PATCH'; ops: JsonPatchOp[]; revisionId: number }
  | { type: 'ACK'; reqId: string }
  | { type: 'ERROR'; code: number; msg: string; reqId?: string };
```

### **3.3. Command Error Matrix**

Every command has a defined set of possible error responses:

| Command | Possible Errors | Client Behavior |
| :--- | :--- | :--- |
| `SET_VOL` | None (always succeeds) | Optimistic update; reconcile on next state patch. |
| `SET_PAN` | None (always succeeds) | Optimistic update; reconcile on next state patch. |
| `PLAY` / `PAUSE` | None (always succeeds) | Optimistic update. |
| `SET_TEMPO` | `2020: TEMPO_OUT_OF_RANGE` | Clamp UI slider to valid range (20–300 BPM). |
| `TOGGLE_METRONOME` | None (always succeeds) | Optimistic toggle. |
| `SET_KEY` | `2030: INVALID_SCALE` | Revert to previous key/scale. Valid scales: `major`, `minor`, `minor_pentatonic`, `major_pentatonic`, `dorian`, `mixolydian`, `blues`, `chromatic`. |
| `MUTE_SLOT` / `UNMUTE_SLOT` | None (always succeeds) | Optimistic toggle. |
| `NOTE_ON` / `NOTE_OFF` | None (always succeeds) | Trigger/release immediately. Local audio feedback optional (engine is source of truth for sound). |
| `SET_LOOP_LENGTH` | `2050: INVALID_LOOP_LENGTH`, `4010: NOTHING_TO_COMMIT` | Revert to current length. If retro buffer is empty (nothing played), show brief "Nothing to capture" toast. |
| `SELECT_MODE` | `2060: PRESET_NOT_FOUND` | Show error toast. |
| `SELECT_EFFECT` | `2060: PRESET_NOT_FOUND` | Show error toast. |
| `SET_KNOB` | None (always succeeds) | Optimistic update. |
| `LOAD_VST` | `3001: PLUGIN_CRASH`, `3010: PLUGIN_NOT_FOUND` | Show error in slot UI panel. |
| `KEYMASHER_ACTION` | None (always succeeds) | Trigger immediately. |
| `COMMIT_RIFF` | `4010: NOTHING_TO_COMMIT` | Button only visible when mix state has changed. If somehow triggered with no changes, show toast. |
| `LOAD_RIFF` | `4011: RIFF_NOT_FOUND` | Show error toast. |
| `DELETE_RIFF` | `4011: RIFF_NOT_FOUND` | Show error toast. |
| `NEW_JAM` | None (always succeeds) | Navigate to new empty session. |
| `LOAD_JAM` | `4020: SESSION_NOT_FOUND` | Show error toast. |
| `RENAME_JAM` | `4020: SESSION_NOT_FOUND` | Show error toast. |
| `DELETE_JAM` | `4020: SESSION_NOT_FOUND` | Show error toast. |
| `PANIC` | None (always succeeds) | `ALL`: Full UI reset animation. `ENGINE`: silence output, preserve UI state. |
| `AUTH` | `1101: AUTH_FAILED` | Show "Incorrect PIN" prompt. |
| `TOGGLE_QUANTISE` | None (always succeeds) | Optimistic toggle. |
| `SET_XY_PAD` | None (always succeeds) | Optimistic update. |
| `FX_ENGAGE` | None (always succeeds) | Show crosshair, activate effect. |
| `FX_DISENGAGE` | None (always succeeds) | Hide crosshair, bypass effect. |
| `SELECT_FX_SOURCE_SLOTS` | `2010: SLOT_BUSY` | Show error toast. |
| `SET_INPUT_GAIN` | None (always succeeds) | Optimistic update. |
| `TOGGLE_MONITOR_INPUT` | None (always succeeds) | Optimistic toggle. |
| `TOGGLE_MONITOR_UNTIL_LOOPED` | None (always succeeds) | Optimistic toggle. |
| `TOGGLE_NOTE_NAMES` | None (always succeeds) | Optimistic toggle. |
| `SET_AUDIO_DEVICE` | `2070: AUDIO_DEVICE_NOT_FOUND` | Revert to previous device. |
| `SET_SAMPLE_RATE` | `2071: INVALID_SAMPLE_RATE` | Revert to previous rate. |
| `SET_BUFFER_SIZE` | `2072: INVALID_BUFFER_SIZE` | Revert to previous size. |
| `SET_INPUT_CHANNELS` / `SET_OUTPUT_CHANNELS` | None (always succeeds) | Optimistic update. |
| `PLAY_TEST_TONE` | None (always succeeds) | Show "Playing..." indicator. |
| `SET_MIDI_INPUT_ACTIVE` | None (always succeeds) | Optimistic toggle. |
| `SET_CLOCK_SOURCE` | None (always succeeds) | Optimistic update. |
| `SET_VST_SEARCH_PATHS` | None (always succeeds) | Update paths list. |
| `RESCAN_PLUGINS` | None (always succeeds) | Show scanning indicator. |
| `SET_STORAGE_LOCATION` | `1003: STORAGE_PATH_INVALID` | Show error toast. |
| `SET_RIFF_SWAP_MODE` | None (always succeeds) | Optimistic update. |
| `WS_RECONNECT` | `1100: PROTOCOL_MISMATCH` | Full page reload. |

### **3.4. App State**

```typescript
interface AppState {
  meta: {
    revisionId: number;
    protocolVersion: number;      // Must match client expectation
    serverTime: number;           // For jitter compensation
    mode: 'NORMAL' | 'SAFE_MODE';
    version: string;              // e.g. "1.0.0"
    isVstMode: boolean;           // True when running as VST3 plugin (read-only)
  };
  session: {
    id: string;                    // UUID
    name: string;                  // e.g., "Jam 12 Feb 2026"
    emoji: string;                 // Randomly assigned
    createdAt: number;             // Unix timestamp
  };
  transport: {
    bpm: number;
    isPlaying: boolean;
    barPhase: number;              // 0.0 - 1.0
    loopLengthBars: number;        // Current loop length (1, 2, 4, 8)
    metronomeEnabled: boolean;
    quantiseEnabled: boolean;      // Whether input is quantised to grid
    rootNote: number;              // 0-11 (C=0)
    scale: Scale;                  // e.g., 'minor_pentatonic'. See Scale type definition above.
  };
  activeMode: {
    category: string;              // 'drums' | 'notes' | 'bass' | 'fx' | 'mic' | 'ext_inst' | 'ext_fx' (sampler deferred to V2)
    presetId: string;              // Currently selected preset
    presetName: string;            // Display name
    isFxMode: boolean;             // True when FX resampling mode is active
    selectedSourceSlots: number[]; // Slots selected for FX Mode routing (empty when not in FX mode)
  };
  activeFX: {
    effectId: string;              // Currently selected effect
    effectName: string;            // Display name
    xyPosition: { x: number; y: number }; // Current XY pad state (0.0-1.0)
    isActive: boolean;             // Whether effect is currently engaged (finger down on XY pad)
  };
  mic: {
    inputGain: number;             // -60dB to +40dB
    monitorInput: boolean;         // Direct monitoring on/off
    monitorUntilLooped: boolean;   // Monitor until first loop commit
  };
  slots: Array<{
    id: string;                    // UUID
    state: 'EMPTY' | 'PLAYING' | 'MUTED';  // No RECORDING or STOPPED states. Retrospective buffer is always capturing (§3.10). Slots transition directly from EMPTY to PLAYING on commit. When transport is paused, PLAYING slots simply pause in place — they remain PLAYING and resume when transport resumes. MUTED slots stay MUTED regardless of transport state.
    volume: number;                // 0.0 - 1.0
    pan: number;                   // -1.0 to 1.0
    name: string;
    instrumentCategory: string;    // What produced this slot's audio. Values: 'drums', 'notes', 'bass', 'fx', 'mic', 'ext_inst', 'ext_fx', 'merge' (auto-merge result).
    presetId: string;              // Which preset was used. For mic recordings: 'mic_input'. For auto-merge: 'auto_merge'.
    userId: string;                // Who recorded this slot (multi-user)
    pluginChain: PluginInstance[];
    lastError?: number;            // Error code if slot has an active error
  }>;
  riffHistory: Array<{
    id: string;                    // UUID
    timestamp: number;
    name: string;
    layers: number;                // Number of active slots when committed
    colors: string[];              // Layer cake colors (source-based)
    userId: string;                // Who committed
  }>;
  settings: {
    riffSwapMode: 'instant' | 'swap_on_bar';  // How riff history taps switch playback
    bufferSize: number;            // Current buffer size (16, 32, 64, 128, 256, 512, 1024)
    sampleRate: number;            // Current sample rate (44100, 48000, 88200, 96000)
    storageLocation: string;       // Path for recordings storage
    clockSource: 'internal' | 'external_midi';
  };
  ui: {
    noteNamesEnabled: boolean;     // Show note names on pads
  };
  system: {
    cpuLoad: number;               // 0.0 - 1.0 (DSP time / buffer time)
    diskBufferUsage: number;       // 0.0 - 1.0
    memoryUsageMB: number;
    activePluginHosts: number;
  };
}

// Plugin instance definition (used in slot.pluginChain)
interface PluginInstance {
  id: string;          // Instance UUID
  pluginId: string;    // VST3 plugin ID
  manufacturer: string;
  name: string;
  bypass: boolean;
  state?: string;      // Base64-encoded VST3 state blob for recall
}
```

> **Note:** Multi-channel output routing (8 stereo pairs / 16 channels for VST3 DAW mode) is automatic: Slot N → Output Pair N. This is not user-configurable in V1 and does not appear in the UI.

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

*   **Recording Format:** FLAC (Lossless) 24-bit / 44.1kHz or 48kHz (Default). **Compression level 0** (fastest) for real-time recording. Higher compression levels can be applied during session export or idle time if storage space is a concern.
*   **Storage Option (V2):** User preference to save as MP3/AAC for disk space savings. Deferred to V2 — see §1.3. V1 uses FLAC only.
*   **Naming Convention:** `project_name/audio/riff_{timestamp}_slot_{index}.flac`

### **3.7. Binary Visualization Stream**

*   **Format:** Raw Float32 Array (Little Endian).
*   **Header:** `[Magic:4][FrameId:4][Timestamp:8]`
*   **Payload:** `[BarPhase:4][MasterRMS_L][MasterRMS_R][Slot1_RMS][Slot1_Spec_1...16]...`
*   **`BarPhase`:** A Float32 value (0.0–1.0) representing the current position within the bar. Included in every binary frame so the client can animate transport position at up to 30fps without polling JSON state. This value is **not** broadcast via JSON `STATE_PATCH` — it is exclusively delivered through the binary stream to avoid flooding the patch channel.
*   **Transmission:** Binary frames on the **same** WebSocket connection used for JSON command/state traffic. The client distinguishes frame types by WebSocket opcode (`0x1` text = JSON, `0x2` binary = visualization). This avoids the complexity of managing a second connection.
*   **Backpressure Control:**
    *   Server maintains a per-client `pendingFrameCount` (incremented on send, decremented on ACK).
    *   Client sends a **4-byte ACK** (the `FrameId` of the received frame as `uint32` Little Endian) after *every received frame*.
    *   If `pendingFrameCount > 3`, server skips sending the next frame.
    *   Fast clients get ~30fps; slow clients (phone on WiFi) gracefully degrade.

> **Note:** If a client never sends ACKs, `pendingFrameCount` will reach the threshold and the server simply stops sending frames to that client. This is **silent degradation, not an error** — the UI works fine without visualization data, it just won’t show meters/waveforms. No error is logged for this condition.

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

### **3.10. Retrospective Capture (Always-On Recording)**

FlowZone uses **always-on capture** — there is no explicit "record start" or "record stop" command. The audio engine continuously captures all played audio into a fixed-size rolling retrospective buffer.

**Buffer Design:**

*   **Size:** Pre-allocated at startup. Approximately **96 seconds** of stereo audio at the session sample rate (e.g., 48kHz × 2 channels × 4 bytes × 96s ≈ 36.9 MB). This ensures 8 bars can be captured even at the minimum tempo of 20 BPM (8 bars at 20 BPM = 96 seconds). This is a fixed allocation — the buffer never resizes.
*   **Count:** V1 uses a **single global buffer** (single-user). See §1.3 for V2 multi-user buffer plans.
*   **Content:** Continuously records the output of the currently selected instrument/mode. Old audio is overwritten as the buffer wraps.

**How recording works:**

1.  **Continuous capture:** The engine maintains the circular buffer, always recording. The user plays pads/keys/drums via `NOTE_ON`/`NOTE_OFF` commands from the React UI — the engine generates audio and everything is captured into the retrospective buffer. No arming, no record button. External MIDI input also triggers NOTE_ON/OFF through the same path.
2.  **Commit to slot via loop length tap:** When the user taps a **Loop Length Button** (1, 2, 4, or 8 Bars) in the Mode or Play tabs, the `SET_LOOP_LENGTH` command fires. This simultaneously sets the loop length to that value AND copies the most recent N bars of audio from the retrospective buffer into the next available slot. The buffer itself does not resize — only the capture window changes.
    *   **Implicit Commit:** There is no separate "Commit" button in the Mode/Play tabs. Tapping the length button *is* the commit action. `SET_LOOP_LENGTH` is the primary recording command.
    *   **Mixer Tab Commit:** The Mixer tab has a specific `COMMIT_RIFF` button for saving mix/pan changes as a new riff — this is a state-only snapshot and does **not** capture audio from the retrospective buffer.
4.  **Slot state transitions:**
    *   `EMPTY` → `PLAYING` (audio committed from retrospective buffer)
    *   `PLAYING` → `MUTED` (user mutes)
    *   `MUTED` → `PLAYING` (user unmutes)
    *   When transport is **paused**, `PLAYING` slots pause in place — they remain in `PLAYING` state and resume automatically when transport resumes. `MUTED` slots stay `MUTED` regardless of transport state. There is no `STOPPED` state.

This model ensures **zero-latency creative flow** — the user never waits for recording to start or misses the beginning of a performance. Tapping a length button instantly captures what was just played.

> **Note:** V1 does not include a per-slot clear command. Users who want to remove a single layer should load a previous riff from history. A per-slot clear could be added as a V2 feature.

---

## **4. File System & Reliability**

### **4.1. Directory Structure**

Strict adherence for Agent clarity.

```
/FlowZone_Root
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
└── FlowZone.jucer
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
    4.  If factory default loading also fails → initialize with a **hardcoded minimum configuration** compiled into the binary (44.1kHz, 512 buffer, no plugins, no session auto-load). This is the absolute fallback and cannot fail.

---

## **5. Operational Robustness (Observability)**

### **5.1. Error Codes (`src/shared/errors/ErrorCodes.h`)**

Agents must strictly use these codes. New codes must be registered here before use.

*   `1000-1099`: System (Boot, Config, Filesystem)
    *   `1001`: `ERR_DISK_FULL`
    *   `1002`: `ERR_DISK_CRITICAL` (Write failed / overflow exceeded)
    *   `1003`: `ERR_STORAGE_PATH_INVALID`
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
    *   `2030`: `ERR_INVALID_SCALE`
    *   `2040`: `ERR_NO_INPUT` (No audio input device available)
    *   `2050`: `ERR_INVALID_LOOP_LENGTH`
    *   `2060`: `ERR_PRESET_NOT_FOUND`
    *   `2070`: `ERR_AUDIO_DEVICE_NOT_FOUND`
    *   `2071`: `ERR_INVALID_SAMPLE_RATE`
    *   `2072`: `ERR_INVALID_BUFFER_SIZE`
*   `3000-3999`: Plugins
    *   `3001`: `ERR_PLUGIN_CRASH`
    *   `3002`: `ERR_PLUGIN_TIMEOUT`
    *   `3010`: `ERR_PLUGIN_NOT_FOUND`
    *   `3020`: `ERR_PLUGIN_OVERLOAD` (IPC ring buffer drops)
*   `4000-4999`: Session
    *   `4010`: `ERR_NOTHING_TO_COMMIT`
    *   `4011`: `ERR_RIFF_NOT_FOUND`
    *   `4020`: `ERR_SESSION_NOT_FOUND`

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
*   **Developer Overlay:** UI can request metrics history to render a performance graph. The `GET_METRICS_HISTORY` command will be defined as part of Phase 8 (Task 8.7) when the developer overlay is implemented.

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

*   **Navigation:** Bottom Tab Bar (Mode | Play | Adjust | Mixer). Settings accessed via "More" button in Mixer tab (§7.6.8).
*   **Dashboard:** Vertical Stack.
*   **Mixer:** Scrollable.

#### **Tablet/Desktop Mode (`>= 768px`)**

*   **Navigation:** Top Header / Sidebar.
*   **Home Screen:** Jam Manager showing all saved sessions.
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

### **7.4. Settings**

All settings are accessed via the **More Options / Settings Panel** (§7.6.8), opened from the "More" button in the Mixer tab. This is the only settings entry point on all devices. Changes sync to the Engine immediately via WebSocket.

### **7.5. Safe Mode Recovery UI**

When the app enters Safe Mode (any level), the UI presents a specific "Recovery" dashboard:

*   Current Safe Mode level indicator (Level 1/2/3).
*   [ ] Toggle: Disable All VSTs (Level 1+)
*   [ ] Toggle: Reset Audio Driver (Level 2+)
*   [ ] Button: "Factory Reset Config" (Level 3)
*   [ ] "Resume Normal Operation" once user has addressed the issue.

---

## **7.6. UI Layout Specification**

This section defines the UI layout implementation details. For the complete JSON structure, see [UI_Layout_Reference.md](./UI_Layout_Reference.md).

### **7.6.1. Global UI Elements**

#### **Header**
*   **Context Label:** [random emoji] date in simple format (This will allow the user multiple sessions with multiple riff histories. )
    *   Position: Top center
    *   Typography: Small caps, secondary color
*   **Top Bar Controls:** 3-section horizontal layout
    *   **Left Section:** Home button (navigates to Jam Manager §7.7)
    *   **Center Section:** Metronome toggle, Play/Pause toggle
    *   **Right Section:** *(Reserved for V2 — e.g., Share, Undo when implemented)*

#### **Navigation Tabs**
*   **Layout:** Horizontal tab bar
*   **Position:** Bottom of content area (above riff history indicators)
*   **Tabs:** 4 tabs in fixed order
    1.  **Mode** — Grid icon
    2.  **Play** — Wave icon — *Sub-view of Mode; shows presets + pads for the selected category*
    3.  **Adjust** — Knob icon  
    4.  **Mixer** — Sliders icon
*   **Visual State:**
    *   Active tab: Highlighted with accent color underline or fill
    *   Inactive tabs: Muted color

#### **Riff History Indicators**
*   **Display Format:** Oblong layer cake with rounded edges, layers stacked vertically.
    *   **Color Logic:** Each color represents input source. Differentiation of same colors via +/- 30% brightness.
*   **Arrangement:** Right-to-left (latest riff on right)
*   **Position:** Below navigation tabs
*   **Interaction:** 
    *   **Tap:** Jumps backwards in riff history to selected riff
    *   **Playback Behavior:** Configurable in settings
        *   `"instant"` — Playback switches immediately
        *   `"swap_on_bar"` — Waits for bar boundary before switching
*   **Visual:** User badge overlay (circular initial badge)

#### **Timeline / Waveform Area**
*   **Flow Direction:** Right-to-left
*   **Section Layout:** 4 waveform sections with decreasing temporal resolution
    1.  **1 Bar** (rightmost) — Most recent
    2.  **2 Bars** (second from right)
    3.  **4 Bars** (third from right)
    4.  **8 Bars** (leftmost) — Oldest visible
*   **Interaction:** Tap a waveform section to set loop length to that duration
*   **Visual:** Waveform rendered as filled path with accent color

#### **Loop Length Controls**
*   **Layout:** Horizontal button row
*   **Position:** Above pad grid (below timeline)
*   **Buttons:** `[8 BARS] [4 BARS] [2 BARS] [1 BAR]`
*   **Action:** Sets the loop length to the specified value AND **immediately commits** that duration from the retrospective buffer to the next empty slot.
    *   **Dual Function:** Serves as both length selector and "Capture" trigger.
    *   **Visual Feedback:** Buttons flash heavily on tap to indicate capture occurred.

#### **Toolbar**
*   **Position:** Between navigation tabs and waveform display
*   **Items:** 
    *   **Riff History Indicators** (center, see above)
    *   **Expand** button (right) — Expands to Riff History View (§7.6.7)

---

### **7.6.2. Mode Tab Layout**

The Mode tab is for **Category Selection ONLY**. It allows the user to switch between the primary performance modes (Drums, Notes, Bass, Microphone, FX).

*   **Behavior:** Selecting a category in the Mode tab performs two actions:
    1.  Fires the `SELECT_MODE` command for that category.
    2.  **Instant Switch:** The app instantly switches the UI view to the **Play tab** (§7.6.3) to show the performative controls for that category.
*   **Content:**
    *   **Category Grid:** 2×4 grid of instrument/mode categories (Drums, Notes, Bass, Ext Inst, FX, Ext FX, Microphone).
    *   **No Presets or Pads:** The Mode tab itself does not show presets or pads (V1 decision). It is purely for high-level mode selection.

#### **FX Mode (Resampling)**

When the user selects **FX** from the Mode category selector, the app enters FX Mode — a real-time resampling workflow where existing layers are processed through effects and bounced into a new slot.

**Workflow:**

1.  **Enter FX Mode:** User taps **FX** in the Mode category selector (§7.6.2). On entry, if the current session state differs from the latest riff history entry, the engine automatically commits the current state to riff history. This ensures all mix adjustments (volumes, pans, mutes) are preserved before any destructive FX operation.
2.  **Select Source Layers:** The bottom panel shows loop slot indicators as **oblong loop symbols** (with rounded edges). Each has a **square selector indicator** that toggles selection on/off. User taps to select which layers are fed through the FX chain. Uses `SELECT_FX_SOURCE_SLOTS` command.
3.  **Select Effect:** The Play tab (§7.6.3) shows the main FX selector. Only one effect is active at a time — this is an FX *selector*, not a chain. The XY pad (visible only in FX Mode) controls the selected effect's parameters in real-time.
4.  **Real-Time Processing:** While in FX Mode, selected layers are continuously routed through the active effect and output to the audio buffer. **Unselected slots continue playing normally** alongside the FX-processed audio. The user **cannot play instruments** while in FX Mode (V1).
5.  **FX Activation:** The effect is engaged only while the user's finger is held down on the XY pad (`FX_ENGAGE` on touch-down, `FX_DISENGAGE` on touch-up). When disengaged, the selected layers play through unprocessed.
6.  **Commit Resampled Layer:** When the user taps a **Loop Length Button** (1, 2, 4, or 8 Bars), the FX-processed audio is captured for that duration and written to the **next empty slot**. The source slots that were selected are then set to `EMPTY` state (destructive — this is the only option). Their audio files remain on disk per the garbage collection policy (§2.2.G). This is safe because the currently playing riff (the pre-FX state) is already the latest entry in Riff History — the user committed it when they originally recorded those layers. Loading that riff from history instantly restores the individual layers.
7.  **Auto-Merge:** If all 8 slots are full when committing the resampled layer, the standard auto-merge rule applies first (see §7.6.2.1 Auto-Merge Algorithm), then the resampled audio goes into the next available slot.

**Audio Routing in FX Mode:**

```
[Selected Slots] → [Sum] → [Active Effect (XY controlled)] → [Audio Output]
                                                            → [Record Buffer (on commit)]
[Unselected Slots] → [Normal Mix] → [Audio Output]  (continue playing unaffected)
```

**Commands:**

*   `SELECT_MODE` with `category: 'fx'` enters FX Mode
*   `SELECT_FX_SOURCE_SLOTS` chooses which slots to route through effects
*   `SELECT_EFFECT` chooses the active effect
*   `FX_ENGAGE` / `FX_DISENGAGE` activates/deactivates the effect (finger down/up on XY pad)
*   `SET_XY_PAD` updates the XY pad position in real-time
*   `SET_LOOP_LENGTH` in FX Mode captures the FX-processed audio and triggers the resample-and-replace behavior (same dual-purpose as in other modes)

> **Note:** The XY pad is only visible when in FX Mode. There is no FX chain or XY pad available in other modes (Drums, Notes, Bass, etc.) for V1. The Audio In slot has its own independent FX chain (simple reverb for vocals), which is only accessible from the Audio In / Microphone mode and is not related to FX Mode.

#### **7.6.2.1. Auto-Merge Algorithm (9th Loop Trigger)**

When a user attempts to record into a 9th slot (all 8 slots are occupied), the auto-merge fires:

1.  **Sum:** Mix the audio from Slots 1-8 into a single stereo buffer at current playback levels (respecting per-slot volume and pan).
2.  **Replace Slot 1:** Write the summed audio into Slot 1, replacing its previous content.
3.  **Clear Slots 2-8:** Set Slots 2-8 to `EMPTY` state.
4.  **Update Slot 1 Metadata:**
    *   `instrumentCategory`: `"merge"`
    *   `presetId`: `"auto_merge"`
    *   `name`: Generated name (e.g., `"Merge [timestamp]"`).
    *   `userId`: The user who triggered the merge.
5.  **Auto-Commit to Riff History:** The merged state is automatically committed as a new riff snapshot before the new recording begins. This ensures no audio is ever lost.
6.  **Proceed:** Recording begins into Slot 2 (first empty slot after merge).

> **Note:** This merge is destructive to the individual slot contents, but safe because the pre-merge state was already saved to Riff History via the auto-commit in step 5. The user can always load a previous riff to recover individual layers.

> **Implementation Note:** The auto-merge operation must be non-blocking from the audio thread's perspective. During merge, the 8 existing loops continue playing. The merge computation (summing and writing) happens on a background thread. Once complete, the slot states are atomically swapped on the next audio callback. The retrospective buffer continues capturing during the merge, so no audio is lost — the capture begins from the next available slot once the merge finalizes.

---

### **7.6.3. Play Tab Layout**

The Play tab is the **primary performance view**. It dynamically changes its content based on the active mode selected in the Mode tab.

#### **Layout: Instruments (Drums, Notes, Bass)**
Used for all melodic and rhythmic instrument modes.
*   **Preset Selector (Top):** 3×4 grid of preset buttons.
*   **Active Preset Display:** Shows selected preset + attribution.
*   **Pad Grid (Bottom):** 4×4 performative pad grid.
*   **Slot Indicators:** Performance-row of loops (mute/unmute toggles).

#### **Layout: Microphone**
*   **Audio Input Selector:** A grid or list showing available hardware audio inputs. The user selects **one and only one** active input for capture.
*   **Gain Control:** Large circular knob for `SET_INPUT_GAIN`.
*   **Monitor Toggles:** "Monitor Input" and "Monitor Until Looped".
*   **Waveform Timeline:** Real-time visualization of the input buffer.

#### **Layout: FX (Resampling)**
*   **FX Preset Selector:** Grid of available resampling effects (Keymasher, etc.). **Note:** FX presets are *only* visible when FX mode is active.
*   **XY Pad:** Large touch surface for controlling the active effect.
*   **Loop Source Selectors:** Loop indicators used as source-select toggles for the FX chain.

> **Note:** This tab was formerly labeled "Sound" in reference designs. Renamed to "Play" to better describe its function across all categories.
*   **Behavior:**
    *   **In FX Mode:** Tapping a slot indicator toggles it as a source for the FX processing chain (same as `SELECT_FX_SOURCE_SLOTS`). Selected slots are visually highlighted.
    *   **In all other modes:** Tapping a slot indicator toggles mute/unmute for that slot (same as `MUTE_SLOT` / `UNMUTE_SLOT`).

---

### **7.6.4. Adjust Tab Layout**

#### **Knob Controls (Standard — Drums, Notes, Bass)**
*   **Layout:** 2 rows × 4 positions
*   **Position:** Top section
*   **Main Knobs:**
    | Row | Col 1 | Col 2 | Col 3 | Col 4 |
    |:---|:---|:---|:---|:---|
    | 1 | Pitch | Length | Tone | Level |
    | 2 | Bounce | Speed | *(empty — reserved for future parameter)* | Reverb |
*   **Additional Reverb Controls:** (displayed when Reverb knob is touched)
    *   Reverb Mix (knob)
    *   Room Size (knob)
    *   Layout: Same 2-row grid pattern below main controls

#### **Knob Controls (Microphone Mode)**
When the active category is **Microphone**, the Adjust tab shows a simplified layout:
*   **Top Section:** 2 knobs only — **Reverb Mix** and **Room Size** (controls the built-in mic reverb effect)
*   **Middle Section:** Monitor toggles — "Monitor until looped" (toggle), "Monitor input" (toggle)
*   **Bottom Section:** Large **Gain** knob (uses `SET_INPUT_GAIN` command)
*   **No pad grid** when in Microphone mode.

#### **Pad Grid**
*   **Structure:** 4×4 grid
*   **Position:** Below knob controls
*   **Content:** Mode-specific pads (same as Mode tab). Not shown in Microphone mode.

---

### **7.6.5. Mixer Tab Layout**

#### **Transport Controls**
*   **Layout:** 2×3 grid
*   **Position:** Top section
*   **Controls:**
    | Row | Col 1 | Col 2 | Col 3 |
    |:---|:---|:---|:---|
    | 1 | **Quantise** (note icon, button) | *(empty)* | **More** (dots icon, button) |
    | 2 | **Metronome** (metronome icon, toggle) | **Tempo** (`120.00`, display/button) | **Key** (`C Minor Pentatonic`, display/button) |
*   **Button Types:**
    *   Toggle: Two-state (on/off with visual feedback)
    *   Display/Button: Shows value, tapping opens editor

#### **Primary Actions**
*   **Layout:** Single large button
*   **Position:** Middle section
*   **Button:**
    1.  **Commit Mix** (checkmark icon) — Light prominent style (primary CTA)
        *   **Action:** Uses `COMMIT_RIFF` command to save the current volume/pan/mute state as a new Riff History entry.
        *   **Visibility:** This button is **only visible when the user has changed mix levels (volume, pan) or toggled a mute** since the last commit or riff load. It is hidden when no mix changes have been made.
        *   **Auto-Commit:** There is no auto-commit on exit or tab switch. If the user exits the jam or leaves the Levels/Mixer mode without committing, uncommitted changes are lost.
        *   **Panic Button:** Long-pressing the Play/Pause button in any tab (or the header) triggers a **Panic** (stops all audio, resets all synth voices). (Uses `STOP` or dedicated `PANIC` command).

#### **Channel Strips**
*   **Layout:** Vertical fader strips (one per active slot)
*   **Arrangement:** Horizontal row, scrollable if > 8 channels
*   **Fader Style:**
    *   Vertical slider bar for volume control (`SET_VOL` command)
    *   **Integrated VU meter:** Real-time level display within the same bar area as the fader — the VU level bounces inside the fader track, showing the slot's audio output level alongside the volume setting
    *   Mute/Solo indicators per strip
*   **Display Info (per channel):**
    *   Instrument/preset name (e.g., "Keymasher")
    *   User attribution (e.g., "bill_tribble")

---

### **7.6.6. Microphone Category View**

#### **Category Selector**
*   Same 2×4 grid as Mode tab (§7.6.2)
*   **Microphone** category highlighted to indicate active state

#### **Monitor Settings**
*   **Layout:** Two toggle switches
*   **Position:** Middle section
*   **Toggles:**
    1.  `"Monitor until looped"` — Default: OFF
    2.  `"Monitor input"` — Default: OFF

#### **Gain Control**
*   **Type:** Large circular knob
*   **Label:** `"Gain"`
*   **Position:** Center bottom
*   **Visual:** Prominent with arc indicator (same style as Mixer faders)

#### **Timeline Display**
*   **Content:** Live recording waveform
*   **Behavior:** Displays audio input waveform in real-time during recording
*   **Position:** Same timeline area as other tabs

---

### **7.6.7. Riff History View**

This is a dedicated full-screen view (not a tab). Accessed by tapping "Expand" in the toolbar or a riff history indicator.

#### **Header**
*   **Back Button:** Top left (returns to main view)
*   **Transport Controls:** Center (Metronome, Play/Pause)

#### **Riff Details (Selected Riff)**
*   **User Info:** Username (e.g., "bill_tribble")
*   **Timestamp:** Relative or absolute (e.g., "Yesterday", "11 Feb 2026")
*   **Metadata:** Time signature and BPM (e.g., "4/4 120.00 BPM"). V1 uses 4/4 time signature exclusively — the "4/4" display is hardcoded. Time signature selection is a potential V2 feature.
*   **Scale:** Key and scale name (e.g., "C Minor Pentatonic")
*   **Avatar:** Circular user image
*   **Riff Icon:** Large oblong layer cake indicator

#### **Actions Row**
*   **Layout:** Horizontal row of buttons
*   **Buttons:**
    1.  `Delete Riff` — *Uses `DELETE_RIFF` command*
    2.  `Export Stems` — *Disabled for V1. Shows "Coming Soon" label.* (Deferred to V2, see §1.3)

#### **History List**
*   **Grouping:** Chronological by date
*   **Date Headers:** e.g., "11 Feb 2026", "7 Feb 2026"
*   **Riff Items:**
    *   **Layout:** Grid (multiple items per row on larger screens)
    *   **Display:** Oblong layer cake with user badge overlay
    *   **User Badge:** Circular badge with user initial
    *   **Selection State:** Outlined border around currently selected riff
    *   **Interaction:** Tap to select and load riff details (does not switch playback)
*   **Load Control:**
    *   **Type:** Button
    *   **Label:** `"Load Older"`
    *   **Position:** Bottom center
    *   **Action:** Loads older riffs from history (pagination)

---

### **7.6.8. More Options / Settings Panel**

Accessed via the "More" button in Mixer tab transport controls. This is the **only** settings access point on all devices. Content is organised into scrollable tabs.

#### **Header**
*   **Title:** `"Settings"`
*   **Close Button:** Top right (`×` icon)

#### **Tab A: Interface (Look & Feel)**
*   **Zoom Level:** Slider [50% - 200%]. (Default: 100%). CSS `html { font-size: X% }` using REM units.
*   **Theme:** Dark | Mid | Light | Match System.
*   **Font Size:** Small | Medium (Default) | Large.
*   **Reduce Motion:** Toggle. Disables canvas visualizer animations and smooth scrolling.
*   **Emoji Skin Tone:** None (Yellow) | Light | Medium-Light | Medium | Medium-Dark | Dark.

#### **Tab B: Audio (Engine Configuration)**
*   **Driver Type:** CoreAudio only for V1.
*   **Input Device:** Dropdown selector for Hardware Input.
*   **Output Device:** Dropdown selector for Hardware Output.
*   **Sample Rate:** Dropdown [44.1kHz | 48kHz | 88.2kHz | 96kHz].
*   **Buffer Size:** Dropdown [16 | 32 | 64 | 128 | 256 | 512 | 1024]. Lower = lower latency, higher CPU.
*   **Input Channels:** Checkbox matrix to enable/disable specific inputs (1-8).
*   **Active Output Channels:** Checkbox list (e.g., `"Speaker 1 + 2"`, `"Left + Right"`).
*   **Test:** Button — plays test tone.

#### **Tab C: MIDI & Sync**
*   **MIDI Inputs:** List of detected ports with "Active" checkboxes.
*   **Clock Source:** Radio button [Internal | External MIDI Clock].
*   **Ableton Link:** Toggle — *Disabled for V1. Shows "Coming Soon" label.* (See §1.3)

#### **Tab D: Library & VST**
*   **VST3 Search Paths:** List of directories. Actions: Add Path, Remove Path.
*   **Scan:** "Rescan All Plugins" button. "Scan on Startup" toggle.
*   **Storage Location:** Path selector for Recordings/Project History.

#### **Section: Quick Toggles** (below tabs, always visible)
*   **Note Names** — Toggle (default: OFF) — Shows note names on pads. Uses `TOGGLE_NOTE_NAMES` command.

#### **Section: User Preferences** (below Quick Toggles, always visible)
*   **Riff Swap Mode** — Radio button [`Instant` | `Swap on Bar`] (Default: `Instant`) — Controls how riff history taps switch playback. Uses `SET_RIFF_SWAP_MODE` command.

---

### **7.6.9. Interaction Patterns**

#### **Tap Behaviors**
*   **Riff History Indicator:** Jump to that riff in history
    *   Playback switch timing: `instant` or `swap_on_bar` (user setting)
*   **Waveform Section:** Set loop length to that section's duration (1/2/4/8 bars)
*   **Category Button:** Switch to that instrument category
*   **Preset Button:** Load that preset
*   **Pad:** Trigger sound (drums) or note (melodic instruments)
*   **Effect Button:** Activate effect (toggle or momentary depending on effect)

#### **Hold Behaviors**
*   **XY Pad:** Crosshair and effect active ONLY while held
    *   Release finger → effect stops, crosshair disappears
*   **Knob/Fader:** Drag vertically or rotationally to adjust value

#### **Selection States**
*   **Selected Category/Preset:** Highlighted with rounded background fill (accent color)
*   **Selected Riff (History View):** Outlined border around item
*   **Active Tab:** Underline or fill in accent color
*   **Toggle ON:** Filled with accent color
*   **Toggle OFF:** Outlined or muted color

#### **Visual Feedback**
*   **Pad Tap:** Brief highlight/glow on press
*   **Button Press:** Scale down slightly (0.95×) + opacity change
*   **Fader/Knob:** Arc indicator fills to current value
*   **Recording State:** Pulsing red indicator on record-enabled slots
*   **Playback State:** Waveform animation (scrolling right-to-left)

---

### **7.6.10. Responsive Breakpoints**

| Breakpoint | Width | Layout Changes |
|:---|:---|:---|
| **Phone** | < 768px | Bottom tab navigation, vertical stack, single-column grids, riff history in drawer |
| **Tablet/Desktop** | ≥ 768px | Top/sidebar navigation, multi-column grids, persistent riff history sidebar |

**Phone Mode Specifics:**
*   Navigation tabs fixed to bottom
*   All grids scale to fill width
*   Riff history collapses to toolbar indicators + expandable drawer
*   Mixer channel strips: Layout scales to fit the screen; no horizontal scrolling required for V1.

**Tablet/Desktop Mode Specifics:**
*   Navigation in header or left sidebar
*   Grids can be multi-column (e.g., preset selector can show 6 columns)
*   Riff history persistent in right sidebar (always visible)
*   Mixer shows all 8+ channels in scrollable grid view

---

## **7.7. Home Screen (Jam Manager)**

The Home screen is the **first screen** the user sees on every app launch. It is the entry point for the application, allowing users to manage their "Jams" (sessions).

**First Launch Behavior:**
*   On the very first launch (no jams exist), the Jam Manager shows a centered **"Create your first jam"** prompt with a prominent "New Jam" button. A note below reads: *"Your jams will appear here."*
*   On subsequent launches, the Jam Manager displays the full jam list. The most recently used jam is highlighted. Tapping it loads the session.

### **7.7.1. Jam List**
*   **Sorting:** chronological, with the **latest jam at the top**.
*   **Search:** Real-time filter by jam name or date.
*   **Jam Item Display:**
    *   **Emoji:** A randomly assigned emoji from the master list (see below).
    *   **Name:** Editable title (defaults to date string).
    *   **Date:** Simple format (e.g., `12 Feb 2026`).
*   **Actions per Jam:**
    *   **Open:** Load the jam into the active session.
    *   **Rename:** Inline editing of the jam name.
    *   **Delete:** Remove the jam and its associated audio history (with "Are you sure?" confirmation).

### **7.7.2. Creating a New Jam**
*   **Trigger:** "New Jam" button.
*   **Initial State:** 
    *   **Emoji:** Randomly selected from the `emojis` array.
    *   **Name:** `Jam [date_simple]` (e.g., "Jam 12 Feb 2026").
    *   **Date Metadata:** Captured as part of the jam's record.

### **7.7.3. Master Emoji Reference**
The following emojis are used for random assignment to new Jams:

```javascript
[
    '😀', '😃', '😄', '😁', '😆', '😅', '😂', '🤣', '😊', '😇',
    '🙂', '🙃', '😉', '😌', '😍', '🥰', '😘', '😗', '😙', '😚',
    '😋', '😛', '😝', '😜', '🤪', '🤨', '🧐', '🤓', '😎', '🤩',
    '🥳', '😏', '😒', '😞', '😔', '😟', '😕', '🙁', '☹️', '😣',
    '😖', '😫', '😩', '🥺', '😢', '😭', '😤', '😠', '😡', '🤬',
    '🤯', '😳', '🥵', '🥶', '😱', '😨', '😰', '😥', '😓', '🤗',
    '🤔', '🤭', '🤫', '🤥', '😶', '😐', '😑', '😬', '🙄', '😯',
    '😦', '😧', '😮', '😲', '🥱', '😴', '🤤', '😪', '😵', '🤐',
    '🥴', '🤢', '🤮', '🤧', '😷', '🤒', '🤕', '🤑', '🤠', '😈',
    '👿', '👹', '👺', '🤡', '💩', '👻', '💀', '☠️', '👽', '👾',
    '🤖', '🎃', '😺', '😸', '😹', '😻', '😼', '😽', '🙀', '😿',
    '😾', '🐶', '🐱', '🐭', '🐹', '🐰', '🦊', '🐻', '🐼', '🐨',
    '🐯', '🦁', '🐮', '🐷', '🐽', '🐸', '🐵', '🙈', '🙉', '🙊',
    '🐒', '🐔', '🐧', '🐦', '🐤', '🐣', '🐥', '🦆', '🦅', '🦉',
    '🦇', '🐺', '🐗', '🐴', '🦄', '🐝', '🐛', '🦋', '🐌', '🐞',
    '🐜', '🦗', '🕷️', '🦂', '🐢', '🐍', '🦎', '🦖', '🦕', '🐙',
    '🦑', '🦐', '🦞', '🦀', '🐡', '🐠', '🐟', '🐬', '🐳', '🐋',
    '🦈', '🐊', '🐇', '🐿️', '🦔', '🌸', '🌺',
    '🌻', '🌷', '🌹', '🥀', '🌼', '🌿', '🍀', '🎋', '🎍', '🌱',
    '🌲', '🌳', '🌴', '🌵', '🌾', '🌽', '🍄', '🌰', '🍞', '🥐',
    '🥖', '🥨', '🥯', '🧀', '🥚'
]
```

## **8. Risk Mitigations & Bead Planning Guide**

> **Note:** These mitigations were derived from an internal risk assessment and are integrated here so that any agent planning or executing beads has the full safety context in one place.

### **8.1. Key Principle**

> **Every bead must produce something testable.** If a bead's output can only be verified by running it together with another bead that doesn't exist yet, the bead is too small or poorly scoped. Merge it with the bead it depends on, or add a stub/mock that makes it independently verifiable.

This is the single most effective way to prevent the "it compiles but doesn't work" failure mode.

---

### **8.2. 🔴 Critical Risk Mitigations**

#### **M1: Dual-Language Schema Sync (Risk: Drift between `schema.ts` and `commands.h`)**

Every command, state field, and error code must be mirrored across C++ and TypeScript. Code compiles on both sides independently but fails at runtime when messages don't deserialize.

**Required bead practices:**
- Every bead that touches commands or state must explicitly name **both** files (`schema.ts` AND `commands.h`) in its scope, even if the bead is "C++ only."
- Create an early **"Schema Foundation" bead** that establishes the full `Command` union and `AppState` interface in both languages — with a verification step that counts type members on each side.
- Consider a code-generation approach: write schema in one language and generate the other. This is extra work but eliminates the class of bugs entirely.

#### **M2: Audio Thread Safety (Risk: Lock-free violations in `processBlock`)**

The audio thread must never allocate memory, lock mutexes, or do I/O. Operations that silently allocate include `std::function`, `juce::String` copy, `std::vector::push_back`, and `DBG()`. These don't crash during development but cause audio glitches under load.

**Required bead practices:**
- Create an explicit **"Audio Thread Contract" bead** early that documents exactly which functions are called on the audio thread and which types/operations are forbidden.
- Every bead targeting `src/engine/` that touches the audio callback path must include a verification step: *"Confirm no heap allocation, no mutex, no I/O in the processBlock path."*
- Add `JUCE_ASSERT_MESSAGE_THREAD` / custom audio-thread assertions to catch violations at debug time. Make this part of the foundation bead.

#### **M3: IPC / Plugin Isolation (Risk: Untestable incrementally)**

The plugin isolation architecture requires a separate binary (`PluginHostApp`), shared memory ring buffers, and a heartbeat protocol. This can't be built incrementally the way UI components can.

**Required bead practices:**
- **Defer plugin isolation to a later phase.** Get the engine working with internal instruments first. This removes the hardest integration risk from the critical path.
- When IPC is built, structure it as a **vertical slice**: one bead that creates the host binary, configures the Projucer Console Application target, establishes shared memory, and successfully passes one audio buffer round-trip. Only then fan out to watchdog/respawn/hot-swap beads.
- The IPC bead must be large and self-contained — not split across multiple agents.

#### **M4: Build System (Resolved: Projucer)**

**Decision:** Projucer (`FlowZone.jucer`) is the authoritative build system. It generates the Xcode project for the main application.

**Required bead practices:**
- The **very first bead must be "Project Skeleton"** — producing a compiling, running JUCE standalone app with an empty `processBlock` and the web view loading a "Hello World" React page. This validates the entire build chain end-to-end.
- Every bead that adds C++ source files must add them to the Projucer project (`FlowZone.jucer`).

---

### **8.3. 🟠 High Risk Mitigations**

#### **M5: WebSocket Protocol (Risk: Not incrementally testable)**

The StateBroadcaster uses JSON Patch (RFC 6902) with complex rules about patches vs snapshots. Both sides must run simultaneously to test.

**Required bead practices:**
- Create a **"Protocol Stub" bead** that implements the WebSocket server sending hardcoded `STATE_FULL` messages and the React client receiving and rendering them. No diffs, no patches — just full state snapshots. This gets both sides talking.
- Add patches as a second bead, with the stub still available as a fallback.
- Include a **"Protocol Conformance Test" bead** that sends canned patch sequences to the React client and validates the resulting state.

#### **M6: Audio Effects & Synth Surface Area (Risk: ~75 implementations accumulating subtle bugs)**

12 core FX, 11 infinite FX, 24 synth presets, 16 drum sounds × 4 kits, plus a sample engine.

**Required bead practices:**
- Group effects/presets by **implementation similarity**, not UI category:
  - *"All filter-based effects"* (Lowpass, Highpass, Comb, Multicomb)
  - *"All delay-based effects"* (Delay, Zap Delay, Dub Delay)
  - *"All noise-based drums"* (hihats, cymbals, snares)
- Each bead must include a **parameter range validation test** — feed min, max, and mid values to every parameter and confirm no NaN, no infinity, no denormals.
- Accept that V1 effects will be simple. The spec already says "simple procedural implementations for V1." Don't over-engineer.

#### **M7: Mobile UI Custom Components (Risk: Hard to verify without engine)**

Custom touch controls (XY pads, vertical faders with VU meters, knobs, waveform timelines) need Canvas rendering and careful touch event handling.

**Required bead practices:**
- Structure UI beads as **component-level** (one bead per major component: XY Pad, Circular Fader, Pad Grid, Riff History Indicator, Waveform Timeline).
- Each bead must produce a **standalone storybook-style demo** that works without the engine backend (using mock state).
- Defer "touch-and-hold" interactions to a polish bead. Get tap-only working first.
- The responsive breakpoint system (§7.6.10) should be a **single foundational bead** that establishes the `ResponsiveContainer` and navigation shell before any view-specific beads.

#### **M8: Shared File Coordination (Risk: Merge conflicts on bottleneck files)**

> **Note:** This risk is significantly reduced for this project — only 1-2 agents will be used (likely one to start). The mitigations below are still good practice for code structure, but are not urgent blockers.

Bottleneck files include `schema.ts`, `commands.h`, `FlowEngine.cpp`, `CommandDispatcher.cpp`, and `AppState`.

**Recommended practices:**
- Serialize schema changes. Schema beads should be completed and pushed before dependent feature beads start.
- Structure `CommandDispatcher` so each command handler is in its **own file** (`handleMuteSlot.cpp`, `handleSetVol.cpp`). The dispatcher just calls them. This eliminates merge conflicts.
- `FlowEngine` should delegate to sub-managers (`TransportService`, `SessionStateManager`). Make this delegation explicit so new code goes into sub-managers, not FlowEngine itself.

---

### **8.4. 🟡 Medium Risk Mitigations**

#### **M9: CrashGuard / Safe Mode (Risk: Rabbit hole)**

The graduated 3-level safe mode is important for production but irrelevant to getting the core loop working.

**Required bead practice:** Make CrashGuard the **last** Phase 1 bead, not the first. Create the sentinel read/write as a stub and fill in the graduated logic later. The engine must be running before crash recovery can be tested.

#### **M10: DiskWriter Tiered Failure (Risk: Requires real disk I/O testing)**

The 4-tier failure strategy (256MB ring buffer, 1GB overflow, partial FLAC flushing) can't be unit-tested meaningfully.

**Required bead practice:** Implement DiskWriter with **Tier 1 (normal writes) only** for the first bead. Add overflow/partial-save tiers as a separate bead with an explicit "simulate slow disk" test.

#### **M11: Binary Visualization Stream (Risk: Separate binary protocol)**

The binary visualization stream is a fully separate protocol from JSON commands, with per-client backpressure.

**Required bead practice:** This is **optional for MVP**. Get JSON state sync working first. Add the binary stream as a Phase 2 enhancement. Waveforms and VU meters can be deferred.

#### **M12: Microtuning Support (Risk: Niche feature with hidden complexity)**

`.scl`/`.kbm` parsing, MTS-ESP integration, and non-12TET frequency mapping add significant complexity.

**Required bead practice:** Implement all synths in **12TET first**. Add microtuning as a Phase 3+ bead that modifies the frequency lookup table. Don't let it block synth beads.

---

### **8.5. 🟢 Lower Risk Notes**

| Item | Mitigation |
|:---|:---|
| **Emoji Skin Tone Preference** | Small but easily forgotten. Make it a chore bead at the end. |
| **Log Rotation + JSONL Logging** | Well-defined but easy to skip. Wrap into a "Production Polish" epic. |
| **HTTP Health Endpoint** | Simple to implement, but agents may forget to wire up live metric values. Bundle with telemetry bead. |

---

### **8.6. Recommended Bead Ordering (Risk-Aware)**

The task breakdown in §9 should follow this risk-aware phasing:

```
PHASE 0: SKELETON (one bead, run first, blocks everything)
├── Projucer project configured (FlowZone.jucer)
├── CivetWeb integrated as source files in libs/civetweb/ (added to Projucer as source group)
├── Empty JUCE app compiles → launches → opens WebBrowserComponent
├── Empty React app loads inside WebBrowserComponent (dev: localhost:5173, prod: bundled resources)
├── WebSocket handshake succeeds (hardcoded "hello")
└── Both build systems verified (C++ and Vite, Catch2 and Vitest)

PHASE 1: CONTRACTS (serial beads, must complete before fanout)
├── schema.ts + commands.h — full type definitions
├── ErrorCodes.h — all error codes registered
├── Audio Thread Contract doc — forbidden operations list
└── AppState round-trip test — C++ serializes, TS deserializes, values match

PHASE 2: ENGINE CORE (can parallelize after contracts)
├── TransportService (independent, testable)
├── CommandQueue + CommandDispatcher (handler-per-file structure)
├── FlowEngine skeleton (empty processBlock, wired to dispatcher)
├── StateBroadcaster (full snapshot only, no patches yet)
└── DiskWriter (Tier 1 only)

PHASE 3: FRONTEND INFRASTRUCTURE (can parallelize with Phase 2)
├── WebSocket client with reconnection
├── ResponsiveContainer + navigation shell
├── Mock state provider (for UI development without engine)
└── Jam Manager (Home screen, first-launch flow)

PHASE 4: INTERNAL AUDIO ENGINES (after Phase 2 engine core works)
├── Filter-based effects (Lowpass, Highpass, Comb, Multicomb)
├── Delay-based effects
├── Distortion/saturation effects
├── Synth presets — 12TET only (Notes, Bass)
└── Drum engine

PHASE 5: UI COMPONENTS (after Phase 3 shell is complete)
├── XY Pad, Pad Grid, Vertical Faders, Knob components
├── Riff History Indicator, Waveform Timeline
├── Slot Indicators (oblong mute/FX source selectors)
└── Assemble views: Dashboard, Mode, FX, Mixer

PHASE 6: INTEGRATION (serial, combines engine + UI)
├── StateBroadcaster patches (RFC 6902)
├── Full command flow: UI → WS → CommandQueue → Dispatcher → Engine → State → UI
├── SessionStateManager (autosave, riff history management)
└── Binary visualization stream (optional for MVP)

PHASE 7: PLUGIN ISOLATION (defer until core works)
├── PluginHostApp binary + Projucer Console Application target (single vertical-slice bead)
├── Shared memory ring buffers
├── Process lifecycle + watchdog
└── Hot-swap + exponential backoff

PHASE 8: POLISH & PRODUCTION
├── CrashGuard + Safe Mode (graduated levels)
├── DiskWriter Tiers 2-4
├── Microtuning support
├── Log rotation + telemetry
├── Settings panel
└── Health endpoint
```

---

### **8.7. Testing Strategy**

FlowZone does not need exhaustive test coverage for V1. Instead, testing is focused on the **dangerous boundaries** — where C++ meets TypeScript, where the audio thread meets the message thread, and where components integrate. Each bead must include its own verification, but the scope is kept practical.

#### **8.7.1. Test Frameworks**

Both frameworks must be configured and verified in the **Phase 0 Skeleton bead** before any feature work begins.

| Side | Framework | Config | Run Command |
|:---|:---|:---|:---|
| **C++ (Engine)** | [Catch2](https://github.com/catchorg/Catch2) v3 | Add as separate Projucer Console Application or Xcode test target. Test binary: `FlowZoneTests` | `./build/Debug/FlowZoneTests` |
| **React (Web Client)** | [Vitest](https://vitest.dev/) | Already Vite-native. Add `vitest` to `devDependencies` | `npm test` (in `src/web_client/`) |

#### **8.7.2. What To Test (By Bead Type)**

| Bead Type | What To Test | Example |
|:---|:---|:---|
| **Schema / Contract** | Round-trip serialization: C++ → JSON → TypeScript → validate all fields match | Task 1.4: `AppState` round-trip |
| **Engine Component** | Construct the component, call its public methods, assert state changes. No audio hardware needed. | `TransportService`: call `setBPM(120)`, assert `getBPM() == 120`. Call `play()`, assert `isPlaying() == true`. |
| **Command Handler** | Push a command through `CommandDispatcher`, assert the correct state mutation occurred. | Push `MUTE_SLOT { slot: 0 }`, assert `slots[0].state == MUTED`. |
| **Audio Effect / Synth** | **Parameter range validation**: feed min, max, and mid values to every parameter. Assert no `NaN`, no `Inf`, no denormals in output buffer. | Process 1024 samples with filter cutoff at 0.0, 0.5, and 1.0. Check output buffer for invalid values. |
| **React Component** | Render with mock state, assert correct DOM output. No engine backend needed. | `<XYPad value={{x: 0.5, y: 0.5}} />` renders a crosshair at center. |
| **WebSocket / Protocol** | Send canned JSON messages, assert client state updates correctly. | Send `STATE_FULL` with 2 slots, assert UI renders 2 slot controls. |
| **Integration** | End-to-end command flow. Requires both engine and UI running. | UI sends `SET_VOL` → verify `StateBroadcaster` emits updated state → UI reflects new volume. |

#### **8.7.3. Test File Locations**

```
/tests
├── /engine                        # C++ unit & integration tests
│   ├── test_transport.cpp         # TransportService tests
│   ├── test_command_dispatch.cpp  # CommandDispatcher + handler tests
│   ├── test_state_roundtrip.cpp   # Schema serialization round-trip
│   ├── test_effects.cpp           # Parameter range validation for FX
│   └── test_synths.cpp            # Parameter range validation for synths
└── /web_client                    # React / TypeScript tests
    ├── components/                # Component render tests
    ├── hooks/                     # WebSocket hook tests
    └── protocol/                  # Protocol conformance tests
```

#### **8.7.4. Rules for Agents**

1. **Every bead must include a verification step.** This can be an automated test *or* a documented manual check (e.g., "Launch app, tap pad, confirm sound plays"). Automated is preferred.
2. **Tests must pass before a bead is marked complete.** Run `ctest` and/or `npm test` as the final step.
3. **Don't write tests for UI aesthetics.** Visual correctness is checked manually or via storybook-style demos.
4. **Don't aim for coverage percentages.** Write tests for logic, boundaries, and parameter ranges. Skip trivial getters/setters.
5. **Audio thread tests are special.** You can't easily unit-test real-time behaviour. Instead, add debug-mode assertions (`JUCE_ASSERT_MESSAGE_THREAD`, custom `isAudioThread()` checks) that fire during manual testing if a violation occurs.

#### **8.7.5. Catch2 Test Example (C++)**

```cpp
// tests/engine/test_transport.cpp
#include <catch2/catch_test_macros.hpp>
#include "engine/transport/TransportService.h"

TEST_CASE("TransportService manages BPM", "[transport]") {
    TransportService transport;
    
    SECTION("Default BPM is 120") {
        REQUIRE(transport.getBPM() == Catch::Approx(120.0));
    }
    
    SECTION("setBPM clamps to valid range") {
        transport.setBPM(300.0);
        REQUIRE(transport.getBPM() == Catch::Approx(300.0));
        
        transport.setBPM(10.0);  // Below minimum (20)
        REQUIRE(transport.getBPM() == Catch::Approx(20.0));
    }
    
    SECTION("Play/Pause toggles state") {
        REQUIRE_FALSE(transport.isPlaying());
        transport.play();
        REQUIRE(transport.isPlaying());
        transport.pause();
        REQUIRE_FALSE(transport.isPlaying());
    }
}
```

#### **8.7.6. Vitest Test Example (TypeScript)**

```typescript
// src/web_client/src/hooks/__tests__/useAppState.test.ts
import { describe, it, expect } from 'vitest';
import { applyStatePatch } from '../useAppState';

describe('applyStatePatch', () => {
  it('applies a volume change patch', () => {
    const state = {
      slots: [{ id: '1', state: 'PLAYING', volume: 0.5 }]
    };
    
    const patch = [
      { op: 'replace', path: '/slots/0/volume', value: 0.8 }
    ];
    
    const result = applyStatePatch(state, patch);
    expect(result.slots[0].volume).toBe(0.8);
  });
  
  it('rejects patches with invalid paths', () => {
    const state = { slots: [] };
    const patch = [
      { op: 'replace', path: '/nonexistent/field', value: 42 }
    ];
    
    expect(() => applyStatePatch(state, patch)).toThrow();
  });
});
```

### **8.8. Human Testing Checkpoints**

Development must pause at these checkpoints for manual verification before proceeding. These are the minimum gates required to catch integration failures early.

#### **Checkpoint 1: After Phase 0** — *"Does it build and run?"*

| Verify | How |
|:---|:---|
| JUCE app compiles and launches | Build in Xcode, run standalone |
| WebBrowserComponent shows React "Hello World" | Visual check |
| WebSocket connection establishes | Check browser DevTools console |
| Catch2 test passes | Run `FlowZoneTests` |
| Vitest test passes | Run `npm test` in `web_client/` |

> **What you should see:** A dark grey window opens (approximately 800×600 pixels). In the center, white text reads "Hello FlowZone" (or similar React placeholder). No crash dialogs appear. In Xcode's console output at the bottom, you should see a line like `[WebSocket] Server listening on port 8765` followed by `[WebSocket] Client connected`. There should be **no red error text** in the Xcode console. If you open Terminal and run the test commands, both should print green "PASSED" or "✓" with zero failures.

**Stop criteria:** Do NOT proceed to Phase 1 until this passes.

#### **Checkpoint 2: After Phase 1 + Task 2.4** — *"Does state flow from C++ to React?"*

| Verify | How |
|:---|:---|
| App launches, React UI appears | Visual check |
| WebSocket messages visible | Browser DevTools → Network → WS |
| `STATE_FULL` contains valid `AppState` | Inspect message payload |
| State changes in C++ reflect in React | Trigger transport play, verify UI updates |
| All Catch2 + Vitest tests pass | `FlowZoneTests` + `npm test` |

> **What you should see:** The app window now shows a basic React UI (may still be unstyled — that's OK). To check state flow: right-click inside the app window → "Inspect Element" (if available) or use a separate browser tab connected to the same WebSocket. In the Network tab → WS section, you should see a single incoming message of type `STATE_FULL` containing a JSON object with fields like `meta`, `session`, `transport`, `slots`. The `transport.bpm` field should show `120` (the default). If you trigger a Play command, you should see the `transport.isPlaying` field change to `true` in a subsequent `STATE_PATCH` message.

**Stop criteria:** Do NOT proceed to UI work until both sides agree on state.

#### **Checkpoint 3: After Phase 3 + Phase 5** — *"Does it look right?"*

| Verify | How |
|:---|:---|
| All 4 tabs visible on desktop | Visual check |
| Resize to phone width → tabs move to bottom | Responsive layout check |
| Custom components render with mock data | XY pad, pad grid, faders displayed |
| Design matches "studio minimalism" aesthetic | Subjective visual check |
| No layout breakage at various widths | Resize browser window |

> **What you should see:** A dark-themed UI reminiscent of a professional music app (think Ableton or a DJ app). At the bottom of the screen, four tabs labeled **Mode**, **Play**, **Adjust**, and **Mixer** with small icons. The currently active tab should be highlighted with an accent color. Tapping each tab should smoothly switch the content area above. You should see placeholder components: a circular XY pad (like a crosshair target), a 4×4 grid of square pads, vertical fader bars with integrated VU meters, and rotary knobs with arc indicators. When you resize the browser window to phone width (~375px), the tabs should stay at the bottom but the content should reflow — no horizontal scrolling, no elements overlapping or clipped. The overall feel should be **dark, clean, and professional** — not like a basic HTML page.

**Stop criteria:** Layout and navigation must be correct before integration.

#### **Checkpoint 4: After Task 6.2** — *"Does the loop work?"*

| Verify | How |
|:---|:---|
| Tap Play → transport starts | Bar phase animates |
| Trigger pad → hear audio | Internal synth produces sound |
| Adjust fader → state updates in real-time | Responsive volume change |
| Open on phone (LAN WebSocket) → same state | Connect from mobile browser |
| Multiple tabs → state syncs | Open 2+ browser tabs |

> **What you should see:** When you tap the **Play** button in the header, the bar phase indicator (a thin animated bar or progress ring) should start moving, cycling from left to right (or 0% to 100%) repeatedly. When you tap a **pad** in the grid, you should **hear a sound** through your speakers/headphones — a synth tone or drum hit. The pad should flash briefly on tap, and a riff history indicator (a small colored oblong) should appear in the toolbar area, confirming audio was captured. When you drag a **fader knob** (circular control in the Mixer tab), the volume should change in real-time — you'll hear the audio get louder or quieter as you drag. To test sync: open the same URL in a second browser tab. Both tabs should show identical state — tapping Play in one tab should show the play state in the other.

**Stop criteria:** Audio must play. Commands must flow. State must sync.

#### **Checkpoint 5: After Phase 8** — *"Is it shippable?"*

| Verify | How |
|:---|:---|
| Full session workflow | Create jam → play → record → commit → load from history |
| Crash recovery | Kill app mid-session → relaunch → verify recovery |
| Settings panel | Change audio device, verify effect |
| Stress test | 8 slots playing → CPU load stays reasonable |
| Overall feel | Premium, responsive, inspires flow |

> **What you should see:** A complete end-to-end workflow: tap Home → Jam Manager shows your sessions → create a New Jam → you're in an empty session. Play pads to make music → see riff indicators appearing as loops are captured → tap Commit → the riff is saved to history. Open the Riff History View (expand button) → you see your riffs grouped by date with colored layer cakes and your avatar badge. Tap one → it loads and plays back. Go to Settings (More button in Mixer tab) → change the audio output device → the sound should switch to the new device immediately (no restart required). For crash recovery: while music is playing, force-quit the app (Cmd+Q). Relaunch it — the session should recover automatically and you should see your most recent riffs intact. The CPU load indicator in the system section should stay under 60% with all 8 slots playing. The overall app should feel **fast, fluid, and premium** — like an instrument, not like a website.

**Stop criteria:** Final sign-off before declaring V1 complete.

---

## **9. Task Breakdown**

> **Important:** This task breakdown should be read in conjunction with the risk mitigations in §8. Each task/bead must follow the applicable mitigation practices listed above.

### **Phase 0: Project Skeleton** *(Blocks everything — complete first)*
1.  **Task 0.1:** Set up Projucer project (`FlowZone.jucer`) with aggressive compiler warnings (`-Wall -Wextra`). Integrate **CivetWeb** as source files in a `libs/civetweb/` directory added to Projucer as a source group (do not use a pre-built library). Configure **Catch2** (as a separate Projucer Console Application or Xcode test target) and **Vitest** (in `web_client/`). Produce a compiling JUCE standalone app with empty `processBlock` + WebBrowserComponent loading a "Hello World" React page (in debug builds, load `http://localhost:5173` from Vite dev server; in release builds, load bundled resources from the app's resource directory or use JUCE 8's `WebBrowserComponent::Options::withResourceProvider()`). WebSocket handshake succeeds. Verify both C++ and Vite build chains end-to-end. Run one trivial Catch2 test and one trivial Vitest test to confirm test infrastructure works.

### **Phase 1: Contracts & Foundations** *(Serial — must complete before fanout)*
2.  **Task 1.1:** **Schema Foundation** — Define full `Command` union and `AppState` interface in both `schema.ts` and `commands.h`. Include a verification step that counts type members on each side.
3.  **Task 1.2:** **Error Codes** — Register all error codes in `ErrorCodes.h`.
4.  **Task 1.3:** **Audio Thread Contract** — Document forbidden operations on the audio thread. Add `JUCE_ASSERT_MESSAGE_THREAD` / custom assertions. Define which functions are audio-thread-callable.
5.  **Task 1.4:** **AppState Round-Trip Test** — C++ serializes `AppState` to JSON, TypeScript deserializes and validates. Confirms both sides agree on the schema.

### **Phase 2: Engine Core** *(Can begin after Phase 1)*
6.  **Task 2.1:** Build `TransportService` with unit tests (independent of audio).
7.  **Task 2.2:** `CommandQueue` + `CommandDispatcher` — structure dispatcher with one handler per file (`handleMuteSlot.cpp`, `handleSetVol.cpp`, etc.).
8.  **Task 2.3:** `FlowEngine` skeleton — empty `processBlock`, wired to dispatcher. Verify no audio-thread violations.
9.  **Task 2.4:** `StateBroadcaster` — full snapshot mode only (`STATE_FULL`). No patches yet.
10. **Task 2.5:** `DiskWriter` — Tier 1 (normal writes) only.

### **Phase 3: Frontend Infrastructure** *(Can parallelize with Phase 2)*
11. **Task 3.1:** Vite + React + TS setup.
12. **Task 3.2:** WebSocket client with full connection lifecycle (reconnection, exponential backoff). Connect to `StateBroadcaster` stub sending hardcoded `STATE_FULL`.
13. **Task 3.3:** `ResponsiveContainer` + navigation shell (breakpoint system from §7.6.10).
14. **Task 3.4:** Mock state provider for UI development without engine backend.

### **Phase 4: Internal Audio Engines** *(After Phase 2 engine core works)*
15. **Task 4.1:** Filter-based effects (Lowpass, Highpass, Comb, Multicomb) — with parameter range validation tests.
16. **Task 4.2:** Delay-based effects (Delay, Zap Delay, Dub Delay) — with parameter range validation tests.
17. **Task 4.3:** Remaining effects (distortion, saturation, modulation, etc.) grouped by implementation similarity.
18. **Task 4.4:** Synth presets (Notes, Bass) — **12TET only**. Microtuning deferred to Phase 8.
19. **Task 4.5:** Drum engine (4 kits, 16 sounds each) — group by synthesis type (noise-based, tonal, etc.).

> **Note:** Sample engine deferred to V2. See §1.3 Future Goals.

### **Phase 5: UI Components** *(After Phase 3 shell is complete)*
20. **Task 5.1:** XY Pad component — standalone demo with mock state. Tap-only first; touch-and-hold deferred.
21. **Task 5.2:** Pad Grid component — standalone demo (with scale-based note mapping for Notes/Bass modes).
22. **Task 5.3:** Vertical Fader / VU Meter / Knob components — standalone demo.
23. **Task 5.4:** Riff History Indicator — standalone demo.
24. **Task 5.5:** Waveform Timeline — standalone demo.
25. **Task 5.6:** Slot Indicator oblongs — standalone demo (FX source selection + mute toggle modes).
26. **Task 5.7:** Assemble views: Dashboard (responsive Grid/Stack), Mode, FX, Mixer, Jam Manager.

### **Phase 6: Integration** *(Serial — combines engine + UI)*
27. **Task 6.1:** `StateBroadcaster` patches (RFC 6902). Protocol Conformance Test bead (canned patch sequences → validate resulting state).
28. **Task 6.2:** Full command flow: UI → WS → CommandQueue → Dispatcher → Engine → State → UI.
29. **Task 6.3:** `SessionStateManager` (autosave, riff history management, crash recovery).

### **Phase 7: Plugin Isolation** *(Defer until core works)*
30. **Task 7.1:** **Vertical Slice** — `PluginHostApp` binary + Projucer Console Application target + shared memory + one audio buffer round-trip. Single self-contained bead.
31. **Task 7.2:** Process lifecycle + watchdog + exponential backoff respawn.
32. **Task 7.3:** Hot-swap + plugin scanning.

### **Phase 8: Polish & Production** *(Final phase)*
33. **Task 8.1:** CrashGuard + Safe Mode (graduated levels). Sentinel stub → full implementation.
34. **Task 8.2:** DiskWriter Tiers 2-4. Include "simulate slow disk" test.
35. **Task 8.3:** Microtuning support (`.scl`/`.kbm` parsing, frequency table modification).
36. **Task 8.4:** Binary visualization stream + Canvas decoder.
37. **Task 8.5:** Settings panel (4 tabs).
38. **Task 8.6:** Safe Mode recovery UI.
39. **Task 8.7:** Health endpoint + developer overlay + telemetry.
40. **Task 8.8:** Log rotation + JSONL verification.
41. **Task 8.9:** Emoji skin tone preference.
42. **Task 8.10:** Error boundary integration + chaos monkey testing.
