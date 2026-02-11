# Plan: FlowZone JUCE Application

This plan outlines the development of **FlowZone**, a macOS desktop "Retrospective Riff Engine" built with JUCE (C++).

> **Note:** This plan follows the [Original Specification](./Spec_FlowZone_Looper.md) for a native C++/JUCE implementation targeting macOS (with potential iOS support later).

## 1. Technical Stack

*   **Framework:** JUCE 8.x (C++)
*   **IDE:** Xcode (macOS)
*   **Build System:** CMake (via Projucer or manual setup)
*   **Audio Format:** FLAC (lossless, 24-bit/48kHz) + optional MP3 export
*   **Data Storage:** JSON-based project files + audio file references
*   **Version Control:** Git + GitHub

## 2. Core Architecture

### 2.1 Audio Engine Components
*   **Circular Buffer:** `juce::AbstractFifo` for lock-free 60-second ring buffer
*   **Audio I/O:** `juce::AudioDeviceManager` for input/output
*   **Transport:** `juce::AudioTransportSource` for playback synchronization
*   **DSP Chain:** `juce::dsp` modules for FX (Reverb, Delay, Filters)
*   **File I/O:** `juce::FlacAudioFormat` for lossless recording

### 2.2 Data Model
*   **Slot Manager:** Manages 8 stereo loop slots
*   **Riff History:** Timeline-based snapshot system (JSON metadata + audio files)
*   **Project Structure:**
    ```
    FlowZoneProject/
    ├── project.json          # Metadata, tempo, key, riff history
    └── audio/
        ├── riff_001_slot_0.flac
        ├── riff_001_slot_1.flac
        └── ...
    ```

### 2.3 UI Architecture (JUCE Components)
*   **Main Window:** `juce::DocumentWindow`
*   **Slot Rack:** Custom `juce::Component` with 8 waveform displays
*   **Transport Controls:** Play/Stop, Tempo, Commit buttons (1/2/4/8/16 bars)
*   **History Browser:** `juce::ListBox` with timeline navigation
*   **Settings Panel:** Audio I/O, microtuning, FX routing

## 3. Development Phases

### Phase 1: Project Setup & Audio I/O
1.  **JUCE Installation:**
    *   Download JUCE from [juce.com](https://juce.com)
    *   Install Projucer
    *   Create new Audio Application project
2.  **Basic Audio Setup:**
    *   Configure `AudioDeviceManager`
    *   Implement basic audio input passthrough
    *   Add VU meter for input monitoring
3.  **Git Integration:**
    *   Commit initial Projucer project
    *   Setup `.gitignore` for JUCE projects (ignore `Builds/`, `JuceLibraryCode/`)

### Phase 2: Circular Buffer & Retrospective Capture
1.  **Implement Ring Buffer:**
    *   Create `CircularAudioBuffer` class using `juce::AbstractFifo`
    *   Continuously write input to buffer (60s capacity)
2.  **Commit Logic:**
    *   Calculate bar lengths from BPM (default 120)
    *   Extract N bars from buffer on button press
    *   Apply crossfades (5-10ms) to prevent clicks
3.  **Tempo System:**
    *   Implement tap tempo for cold start
    *   Add tempo slider/input field

### Phase 3: 8-Slot Playback System
1.  **Slot Architecture:**
    *   Create `LoopSlot` class (holds `AudioBuffer`, gain, color metadata)
    *   Implement "find first empty" slot assignment
2.  **Playback Engine:**
    *   Sync all slots to global transport
    *   Implement looping with phase-locked playback
3.  **UI for Slots:**
    *   8 waveform displays (using `juce::AudioThumbnail`)
    *   Mute/Solo/Clear buttons per slot
    *   Color coding (Purple/Cyan/Yellow/Scarlet)

### Phase 4: Summing Event (Auto-Merge)
1.  **Bounce Logic:**
    *   Detect when slots 1-7 are full
    *   On 8th commit, render slots 1-7 to single stereo file
    *   Clear slots 1-7, place bounce in slot 1, new loop in slot 2
2.  **Offline Rendering:**
    *   Use `juce::AudioFormatWriter` to write bounced audio
    *   Ensure no audio thread blocking

### Phase 5: Riff History & Persistence
1.  **Riff Snapshot System:**
    *   Create `RiffSnapshot` class (timestamp, BPM, slot states)
    *   Save snapshots to JSON on every commit
2.  **History Browser:**
    *   Timeline UI with clickable riff entries
    *   Instant recall: load audio files back into slots
3.  **File Management:**
    *   Save/Load project files
    *   FLAC encoding/decoding

### Phase 6: Internal Synth & Microtuning
1.  **Synth Engine:**
    *   Implement basic wavetable/subtractive synth
    *   MIDI input support
2.  **Microtuning:**
    *   Load .scl/.kbm files (Scala format)
    *   Implement frequency remapping
    *   Presets: 12TET, Just Intonation, Pythagorean, etc.

### Phase 7: FX Chain & Resampling
1.  **Master FX Bus:**
    *   Reverb, Delay, Bitcrusher, Filters
    *   Use `juce::dsp` modules
2.  **FX Mode:**
    *   Route selected slots through FX
    *   Capture FX output as new loop (Scarlet color)

### Phase 8: Polish & Export
1.  **UI Refinement:**
    *   Dark mode theme
    *   Custom look-and-feel
2.  **Export Features:**
    *   MP3 export (using LAME or system encoder)
    *   Stems export (individual slot files)
3.  **Performance:**
    *   Optimize audio thread
    *   Latency compensation

## 4. Immediate Next Steps

1.  **Install JUCE:**
    *   Download from [juce.com/get-juce](https://juce.com/get-juce)
    *   Run Projucer
2.  **Create Project:**
    *   New Project → Audio Application
    *   Name: FlowZone
    *   Target: macOS (Xcode)
3.  **Initial Commit:**
    *   Add JUCE project files to git
    *   Update `.gitignore` for JUCE-specific files

## 5. JUCE-Specific .gitignore

```gitignore
# JUCE Build artifacts
Builds/
JuceLibraryCode/

# Xcode
*.xcodeproj/xcuserdata/
*.xcworkspace/xcuserdata/
DerivedData/

# macOS
.DS_Store
```

## 6. Resources

*   **JUCE Docs:** [docs.juce.com](https://docs.juce.com)
*   **JUCE Forum:** [forum.juce.com](https://forum.juce.com)
*   **JUCE Tutorials:** Focus on AudioDeviceManager, AbstractFifo, AudioTransportSource
