# **Technical Design Doc: FlowZone**

**Version:** 1.1 (Added Settings Panel Specification) **Target Framework:** JUCE 8 (C++20 standard) \+ React 18 **Platform:** macOS (Standalone App) **Repository Structure:** Monorepo (Engine \[C++\] / WebClient \[React/TS\] / Shared)

## **1\. Goals & Non-Goals**

### **1.1. Goals**

* **Identity:** A macOS-first retrospective looping workstation named **FlowZone**.  
  * **Inspiration:** Inspired by the "flow machine" workflow (e.g., Tim Exile’s *Endlesss*), designed to eliminate "loop paralysis."  
  * **Core Principle:** **"Destructive Creativity, Non-Destructive Storage."** The active workspace (the 8 slots) is constantly merged, overwritten, and processed to keep the flow moving ("Destructive" to the current state), but *every* single committed loop is saved to the history database ("Non-Destructive" to the session).  
* **Hybrid Web Architecture:** The application core is a native C++ audio engine, but the User Interface is a **React** web application.  
  * **Local Control:** The macOS app displays the UI using JUCE's `WebBrowserComponent`.  
  * **Remote Control:** The app hosts an internal web server (port 8080), serving the React app to LAN devices.
  * **Collaborative Shared Session:** All connected users (local and remote) control the **same single audio session**. The Audio Engine is the single source of truth; Clients are stateless views.
  * **Unified Interface:** Every connected client loads the full application interface.
* **Strict Decoupling:** The Audio Engine and UI are completely decoupled. The Engine runs regardless of UI state.
* **Performance:** "Feature-Based" Visualization. Instead of streaming raw audio, the engine extracts lightweight features (RMS, peaks) and sends them at substantial frame rates (e.g., 20-30fps) for the client to interpolate, ensuring low-bandwidth WiFi stability.
* **Responsive Design:** The UI must adapt fluidly to the client device's form factor (Phone vs. Tablet/Desktop).  
* **Centralized Audio Processing:** All audio processing happens on the main macOS computer.  
* **Retrospective "Always-On" Capture:** Implement a lock-free circular buffer (\~60s).  
* **Global Phase-Sync (Link-Enabled):**  
  * **Primary:** Ableton Link.  
  * **Cold Start (No Link):** If the transport is stopped and slots are empty, the user can tap "Play" or a "Tap Tempo" trigger twice. The time between taps sets the session tempo and begins the buffering.  
* **Hybrid Sound Engine:** VST3 Hosting, Internal Procedural Instruments, Mic Input Processing.  
* **Microtuning Support:** Internal synths must support microtuning via `.scl` (Scala) and `.kbm` files, with standard presets (Just Intonation, Pythagorean, Slendro, Pelog, 12TET).  
* **"Configure Mode" for VST Parameters:** Users must explicitly "touch" a VST knob to expose it to the React UI.  
* **Smart Layering (Auto-Merge):** The "9th Loop" trigger automatically sums Slots 1-8 into Slot 1\.  
* **FX Mode / Resampling:** Route specific layers through FX and resample into a new layer.

  ### **1.2. Non-Goals**

* **Windows/Linux Support:** macOS only for V1.  
* **Native Widgets:** No use of JUCE `Slider` or `Component`.  
* **Cloud Sync:** Local network sharing only.

  ## **2\. System Architecture**

  ### **2.1. High-Level Components**

  #### **A. Main Process (C++ macOS App)**

*   **`FlowEngine`:** The singleton `juce::AudioProcessor`. Acts as the "Source of Truth" for all state.
*   **`StateBroadcaster`:** Manages WebSocket connections and broadcasts state deltas.
*   **`CommandQueue`:** A lock-free FIFO queue receiving user actions from the WebServer to be processed by the Engine.
*   **`FeatureExtractor`:** Analyzes audio buffers to produce lightweight visualization data (RMS, Spectrum 16-band) for WiFi-safe transmission.
*   **`DiskWriter`:** A background `juce::TimeSliceThread`.
  *   **Responsibility:** Handles all file I/O (FLAC/MP3 writing) to ensure the audio thread is *never* blocked by disk operations.
*   **`WebServer`:** Embedded HTTP/WebSocket server (CivetWeb).
*   **`WebUIContainer`:** `juce::WebBrowserComponent` (Mac wrapper).
*   **`SandboxManager`:** Manages VST3 child processes to prevent engine crashes.
*   **`CrashGuard`:** Startup sentinel that detects consecutive crashes and triggers "Safe Mode" (No VSTs).

  #### **B. Frontend (React Web App)**

* **Framework:** React 18 \+ TypeScript \+ Vite \+ **Tailwind CSS**.  
* **Visuals:** Canvas API / WebGL.

  #### **C. Internal Audio Engines (Native C++)**

* **`InternalSynth`:**  
  * **Engines:** Drums, Bass, Leads.  
  * **Tuning:** Implementation of MTS-ESP or internal frequency mapping for `.scl` support.  
  * **Presets:** Just Intonation, Pythagorean, Slendro, Pelog, 12TET (Default).  
* **`InternalFX`:** Core & Keymasher banks.  
* **`MicProcessor`:** Input chains.

### **2.2. Unidirectional Data Flow & Protocol**

1.  **Action (Client -> Engine):** UI sends specific commands (e.g., `{ "cmd": "set_vol", "slot": 1, "val": 0.5 }`) via WebSocket.
2.  **Processing (Engine):** `CommandQueue` dequeues action on the audio thread/worker thread. Engine updates its internal model.
3.  **State Broadcast (Engine -> Client):**
    *   **Versioned State:** The Engine maintains a `StateRevisionID`.
    *   **Reconciliation:** On connection, Client sends its `last_known_rev`. Engine responds with a full state or a delta.
    *   **Broadcast:** Engine pushes an updated `StateVector` to all connected clients.
4.  **Visualization (Engine -> Client):** `FeatureExtractor` sends a separate, high-frequency binary stream of visualization data (metering, playheads).

* **Configure Mode:** VST Gesture \-\> IPC \-\> C++ Engine \-\> JSON Event \-\> React UI.

  ## **3\. Data Model & Schemas**

  ### **3.1. Riff Snapshot (C++)**

```
struct RiffSnapshot {
    juce::Uuid id;
    int64_t timestamp;
    int64_t stateRevisionId; // For reconciliation
    double tempo;
    int rootKey;
    // ... RigState ...
    std::array<SlotState, 8> slots;
};

```

  ### **3.2. Audio Persistence Strategy**

* **Recording Format:** FLAC (Lossless) 24-bit / 44.1kHz or 48kHz (Default).  
* **Storage Option:** User preference to save as MP3 320kbps (via LAME or system encoder) to save disk space.  
* **Naming Convention:** `project_name/audio/riff_{timestamp}_slot_{index}.flac`

  ## **4\. UI Specification (React)**

  ### **4.1. Visual Language & Design System**

* **Base Aesthetic:** **Functional Studio Minimalism.**  
  * **Panel-Based Grouping:** Avoid harsh high-contrast borders. Use subtle differences in background brightness.  
  * **Soft Geometry:** UI elements should feel precise but not sharp. Use small, consistent border-radii (2px \- 4px).  
  * **Vector Precision:** All meters and graphs use fine, anti-aliased vector lines (1px width).  
  * **Inline Visuals:** Knobs and sliders are "filled" with color to indicate value.  
* **Theme Modes:**  
  * **Dark (Default):** "Studio Dark Grey" backgrounds (`#2D2D2D` to `#383838`). Text is off-white (`#DCDCDC`).  
  * **Mid:** "Classic Grey" backgrounds (`#555555` to `#777777`).  
  * **Light:** "Daylight" backgrounds (`#F2F2F2`).  
  * **Match System:** Auto-switch based on OS.  
* **Typography:** **Inter** (sans-serif) used exclusively.  
* **Color Palette (Tertiary Highlights):**  
  * **Indigo (Blue-Violet):** Drums/Percussion.  
  * **Teal (Blue-Green):** Instruments/Synths.  
  * **Amber (Yellow-Orange):** External Audio Input.  
  * **Vermilion (Red-Orange):** Combined/Summed tracks or FX.  
  * **Chartreuse (Yellow-Green):** Transport & Sync.

  ### **4.2. Navigation & Views**

  #### **Phone Mode (`< 768px`)**

* **Navigation:** Bottom Tab Bar (Dash | Inst | FX | Mixer | **Settings**).  
* **Dashboard:** Vertical Stack.  
* **Mixer:** Scrollable.

  #### **Tablet/Desktop Mode (`>= 768px`)**

* **Navigation:** Top Header / Sidebar.  
* **Dashboard:** Grid Layout.  
* **Mixer:** Full 8-fader view.

  ### **4.3. JSON Layout Specification**

```
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
        "dashboard": { "mobile": ["Stack"], "desktop": ["Grid"] },
        "settings": ["SettingsTabs"]
      }
    }
  ]
}
```

  ### **4.4 Settings Panel Specification**

The settings view is divided into four tabs. Changes made here sync to the Engine immediately via WebSocket.

#### **A. Interface (Look & Feel)**

* **Zoom Level:** Global UI scaling percentage.  
  * *Controls:* Slider \[50% \- 200%\]. (Default: 100%).  
  * *Implementation:* CSS `html { font-size: X% }` using REM units for all layout.  
* **Theme:** Color scheme selector.  
  * *Options:* Dark | Mid | Light | Match System.  
* **Font Size:** Base text scaling independent of layout zoom.  
  * *Options:* Small | Medium (Default) | Large.  
* **Reduce Motion:** Accessibility toggle.  
  * *Action:* Disables canvas visualizer animations and smooth scrolling.

  #### **B. Audio (Engine Configuration)**

* **Driver Type:** (CoreAudio / ASIO / WASAPI) \- *Note: macOS is CoreAudio only for V1.*  
* **Input Device:** Dropdown selector for Hardware Input.  
* **Output Device:** Dropdown selector for Hardware Output.  
* **Sample Rate:** Dropdown \[44.1kHz | 48kHz | 88.2kHz | 96kHz\].  
* **Buffer Size:** Dropdown \[64 | 128 | 256 | 512 | 1024\].  
  * *Note:* Critical for latency management.  
* **Input Channels:** Checkbox matrix to enable/disable specific inputs from the interface (1-8).

  #### **C. MIDI & Sync**

* **MIDI Inputs:** List of detected ports with "Active" checkboxes.  
* **Ableton Link:**  
  * *Toggle:* Enable/Disable Link.  
  * *Status:* "Connected to X Peers".  
  * *Start/Stop Sync:* Option to sync transport start/stop commands.  
* **Clock Source:** Radio button \[Internal | Link | External MIDI Clock\].

  #### **D. Library & VST**

* **VST3 Search Paths:** List of directories.  
  * *Actions:* Add Path, Remove Path.  
* **Scan:**  
  * *Button:* "Rescan All Plugins".  
  * *Toggle:* "Scan on Startup".  
* **Storage Location:** Path selector for where Recordings/Project History are saved.

  ## **5\. Error Handling & "No Silent Fallback"**

* **Network Disconnect:** UI overlay "Reconnecting...". Audio Engine continues running independently. On reconnection, UI requests `FullSync` to match Engine state.
* **Sandbox Crashes:** Toast notification "VST Process Died". Engine attempts to restart worker.
* **Startup Crash Loop (`CrashGuard`):** If app crashes < 10s after boot twice in a row, next boot runs **Safe Mode** (Audio disable, VSTs disabled) and shows a "Recovery" UI to the user to reset settings.
* **Disk I/O Latency:** If `DiskWriter` falls behind, increase RAM buffer size dynamically and warn user. Never stop audio.

  ## **6\. Task Breakdown**

  ### **Phase 1: Core Engine & Server (C++)**

1. **Task 1.1:** Setup JUCE \+ `civetweb`.
2. **Task 1.2:** Implement `FlowEngine` (Buffers, PhaseClock, Slots).
3. **Task 1.3:** Implement `StateBroadcaster` & `CommandQueue` infrastructure.
4. **Task 1.4:** Implement `FeatureExtractor` (RMS/Spectrum logic).
5. **Task 1.5:** Implement `DiskWriter` using `juce::TimeSliceThread`.
6. **Task 1.6:** Implement `CrashGuard` logic.

   ### **Phase 2: Frontend Infrastructure (React)**

5. **Task 2.1:** Setup Vite \+ React \+ TS \+ Tailwind.  
6. **Task 2.2:** Build WebSocket Context.  
7. **Task 2.3:** Implement Responsive Layout Wrapper.

   ### **Phase 3: Internal Engines**

8. **Task 3.1:** Implement `InternalSynth` (with Microtuning/Scala support) & `InternalFX`.  
9. **Task 3.2:** Expose parameters.

   ### **Phase 4: Sandboxing & VST**

10. **Task 4.1:** Implement `HostWorkerApp`.  
11. **Task 4.2:** Implement IPC.

    ### **Phase 5: UI Implementation**

12. **Task 5.1:** Build `/dashboard` (Responsive Grid/Stack).  
13. **Task 5.2:** Build `/instruments`, `/fx`, `/mixer`.  
14. **Task 5.3:** Build `/settings` view with Audio/MIDI/UI configuration tabs.  
15. **Task 5.4:** Implement Routing logic.  
* 

