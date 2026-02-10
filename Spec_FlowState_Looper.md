# **Product Requirements Document: FlowState**

**Version:** 0.4

**Status:** Draft

**Platform:** Desktop (macOS) / Potential for iOS

**Framework:** JUCE (C++)
I'll be going from VS Code to Xcode

## **1\. Executive Summary**

FlowState is a "Retrospective Riff Engine" designed for flow-state composition. Inspired by Tim Exile’s “flow machine” and his later app Endlesss. Unlike traditional loopers that require pre-planning (hitting record/stop), FlowState is always listening. It captures audio retroactively based on musical intervals, automatically layering and mixing tracks to prevent "loop paralysis." It features an infinite non-destructive history, allowing users to scroll back through time to retrieve any riff created during a session.

## **2\. Core User Experience Principles**

1. **Never hit record:** The audio buffer is always active.  
2. **No routing fatigue:** The app intelligently finds empty slots for new loops.  
3. **Infinite Recall:** Time is linear; every committed loop is saved to a history database.  
4. **Destructive Creativity, Non-Destructive Storage:** The current "Live" rack merges and flattens audio to keep the workspace clean, but raw multitracks are always preserved in the history.

## **3\. Functional Requirements**

### **3.1. The Audio Engine & Retrospective Looping**

* **Always-On Buffer:** The application maintains a circular RAM buffer continuously recording the last \~60 seconds of audio input.  
* **Commit Triggers:** UI buttons for **1, 2, 4, 8, and 16 bars**.  
* **Commit Logic:**  
  * Upon pressing "4 Bars", the engine calculates the *previous* 4 bars of audio relative to the current tempo grid.  
  * Crossfades (5-10ms) are applied to the start/end to prevent clicks.  
  * This audio is moved from the circular buffer to a **Loop Slot**.  
* **Tempo Detection:**  
  * **Cold Start:** If the transport is stopped and slots are empty, the user can tap "Record" twice. The time between taps sets the tempo and begins the first loop.  
  * **Rolling Start:** User hits "Play," metronome starts, buffering begins.

### **3.2. Slot Architecture & Smart Layering**

* **The Rack:** A fixed set of **8 Stereo Loop Slots**.  
* **One-Shot Assignment:** When a commit is triggered, the engine scans slots 1-8. The audio is placed in the first available (empty) slot.  
* **The "Summing Event" (Auto-Merging):**  
  * **Trigger:** When Slots 1-7 are full.  
  * **Action:** When the user commits the *next* loop (the 8th), the engine performs a bounce.  
  * **The Bounce:** Slots 1-7 are summed (mixed down) into a single stereo file.  
  * **The Result:** \* All Slots 1-7 are cleared.  
    * The "Summed" audio is placed in **Slot 1**.  
    * The *newly* committed loop is placed in **Slot 2**.  
    * *User Perception:* Infinite overdubbing without running out of space.

### **3.3. Riff History & Browser**

* **The "Riff" Definition:** A "Riff" is a snapshot object containing the state of all active slots at a specific moment, plus metadata.  
* **History Stack:** Every time a user commits a loop or modifies a layer, a new Riff state is saved to the timeline.  
* **Navigation:**  
  * Tapping old riffs allows the user to go back in time.  
  * "Instant Recall": Clicking a previous point in history instantly loads those audio files back into the 8 slots.  
* **Browser Filters:**  
  * Date/Time (Today, Last Week, Month).  
  * Key/Scale.

### **3.4. Instrumentation & Color Coding**

Tagged by in-app source

* **Color Logic:**  
  * **Purple:** Drums/Percussion (High transient density).  
  * **Cyan:** Instruments/Synths (Harmonic content).  
  * **Yellow:** External Audio Input (Mic/Line).  
  * **Scarlet:** Combined/Summed tracks (Result of a Summing Event) or FX re-samples.

### **3.5. Internal Synthesis & Microtuning**

* **Built-in Synth:** Simple wavetable or subtractive synth engine integrated into the app for immediate sketching.  
* **Microtuning Support:**  
  * Support for .scl (Scala) or .kbm files.  
  * **Presets:** Just Intonation, Pythagorean, Slendro, Pelog, 12TET (default) (etc. \- choose 5 more key historical ones, with option to upload more via scala or whatever)  
  * **Root Key:** Global setting to anchor the tuning frequencies.

### **3.6. FX Mode & Resampling**

* **FX Chain:** A master FX bus (Reverb, Delay, Bitcrush, Filter).  
* **Resample Workflow:**  
  * User enables "FX Mode."  
  * User can see audio layers and choose what to route to FX (all on by default, add a pref for this)  
  * Play audio through FX.  
  * Hit a Commit Button (e.g., 4 bars).  
  * **Action:** The *output* of the FX bus is captured and placed into a new Slot (colored Scarlet). The original dry slots cleared.

## **4\. Technical Specifications (JUCE Implementation)**

### **4.1. Audio Format**

* **Recording:** FLAC (Lossless) 24-bit / 44.1kHz or 48kHz.  
* **Storage:** \* Primary: FLAC (for disk space efficiency).  
  * or MP3 320kbps (using LAME or system encoder).(User pref)

### **4.2. Data Structure**

* **Project File:** JSON or XML based.  
* **Riff Object:**  
  {  
    "riff\_id": "uuid\_12345",  
    "timestamp": 1715092300,  
    "tempo": 120.0,  
    "root\_key": "C\#",  
    "slots": \[  
      {"slot\_index": 0, "file\_path": "audio/clip\_a.flac", "color": "purple", "gain": 1.0},  
      {"slot\_index": 1, "file\_path": "audio/clip\_b.flac", "color": "cyan", "gain": 0.8}  
    \]  
  }

### **4.3. Critical Classes & Modules**

* juce::AudioTransportSource: Handling playback of the history.  
* juce::AbstractFifo: For the lock-free circular recording buffer.  
* juce::AudioProcessorValueTreeState: Managing parameter state (FX, Synth tuning).  
* juce::dsp::Convolution: For any cabinet/reverb simulation.  
* juce::MidiMessage: For handling microtuning (pitch bend vs. MPE). *Note: MPE is preferred for polyphonic microtuning.*

## **5\. UI/UX Scope**

### 

### **5.3. Settings / Browser (Side Pane)**

* Collapsible pane for Microtuning selection, Audio I/O, and file browsing.

## **6\. Development Phases**

### **Phase 1: The Engine (MVP)**

* Implement circular buffer and retrospective commit logic.  
* Implement 8-slot audio playback.  
* Basic UI for 1/2/4/8/16 bar triggers.  
* tempo changer (starts at 120\)  
* Implement File I/O (saving FLACs to disk).

### **Phase 2: Logic & History**

* Implement "Summing Event" logic (7 slots \-\> 1 slot).  
* Build the Riff History database structure.

### **Phase 3: Creative Suite**

* Add Internal Synth.  
* Implement Microtuning (MTS-ESP support or internal frequency mapping).  
* Add FX processing and Resampling.  
* Color coding logic.

### **Phase 4: Polish**

* UI Skinning (Modern, Dark Mode).  
* MP3 Export.  
* Tempo detection algorithms.

## **7\. Potential Challenges & Risks**

1. **Latency Compensation:** Retrospective looping requires precise sample-counting to offset system input latency. If calculated wrong, loops will drift.  
2. **Disk I/O:** Writing 8 tracks \+ a mixdown simultaneously can block the audio thread. Must use juce::TimeSliceThread or buffering for file writing.  
3. **Microtuning Complexity:** Polyphonic microtuning requires per-note pitch bending or a specific synth architecture. This is complex to implement from scratch.

# **Appendix \- v0.1 comparator for reference**

# **JAM FLOW 0.1 \- Product Requirements Document (PRD)**

## **1\. Executive Summary**

**JAM FLOW** is a high-performance, browser-based live looping workstation and performance instrument. It allows users to build complex, multi-layered compositions using synthesized drums, leads, and basslines, alongside live microphone input. The core philosophy is "Seamless Improvisation," facilitated by a snapshot-based history system and global phase-sync engine.

---

## **2\. Core Features**

### **2.1 Audio Engine & Looping**

* **Global Phase-Sync**: All loops are automatically aligned to a global master clock. When a new loop is triggered or a history state is restored, the playback offset is calculated using currentTime % duration, ensuring perfect phase alignment without stutters.  
* **Quantized Capture**: One-touch recording for 1, 2, 4, or 8 bars.  
* **Multi-Slot Architecture**: Supports up to 8 concurrent loop slots with individual mute and gain control.  
* **Hybrid Input/Resample Engine**:  
  * **Input Mode**: Captures raw live performance (Synth/Mic).  
  * **Resample Mode (FX)**: Captures the Master Output (including all active loops and FX), allowing for complex layer collapsing and "bouncing."  
* 

### **2.2 Instrument Engines**

* **Drums**: Procedural synthesis for Kicks, Snares, Hats, and Percussion with 12 distinct kit styles.  
* **Notes/Bass**: Dual-oscillator subtractive/FM synthesis mapped to musical scales (Major, Minor, Pentatonic, etc.).  
* **Mic**: Real-time passthrough with dedicated monitoring gain and capture capability.

### **2.3 Snapshot History ("Time Travel")**

* **State Snapshots**: Every capture event records the *entire* state of all 8 slots.  
* **Non-Destructive Restoration**: Users can jump back to any previous state in the performance timeline. Switching is seamless and maintains the beat.  
* **Visual Timeline**: A color-coded strip representing the instrument added at each step.

### **2.4 FX Engine**

* **Banked Architecture**:  
  * **Core FX**: Standard mixing and filtering tools.  
  * **Keymasher FX**: Performance-oriented "glitch" and "stutter" effects.  
*   
* **XY Pad Performance**: 12 pads act as touch-sensitive controllers for FX parameters (e.g., Filter Cutoff vs. Resonance).

---

## **3\. Layout Specification (JSON)**

codeJSON

```
{
  "app_identity": {
    "name": "JAM FLOW",
    "theme": "Dark/Slate/OLED",
    "primary_accent": "#f97316 (Orange)"
  },
  "layout_hierarchy": [
    {
      "component": "Header",
      "height": "56px",
      "controls": ["Home", "Play/Pause Toggle", "Reset", "Share", "Add Layer"]
    },
    {
      "component": "MainDisplay",
      "min_height": "100px",
      "variants": {
        "Sound": "Preset Selection Grid (4x3)",
        "FX": "Bank Toggle (Core vs Keymasher)",
        "Mixer": "Scale/Root/Quantize/Shuffle Settings"
      }
    },
    {
      "component": "ModeSelector",
      "height": "56px",
      "buttons": ["Drums", "Notes", "Bass", "FX", "Mic"]
    },
    {
      "component": "TabBar",
      "height": "40px",
      "tabs": ["Sound", "Adjust", "Mixer"]
    },
    {
      "component": "HistoryStrip",
      "height": "40px",
      "type": "HorizontalScroll",
      "item": "SnapshotBubble (Color-coded)"
    },
    {
      "component": "CaptureStrip",
      "height": "80px",
      "slots": [
        {"bars": 1, "visualizer": "Waveform"},
        {"bars": 2, "visualizer": "Waveform"},
        {"bars": 4, "visualizer": "Waveform"},
        {"bars": 8, "visualizer": "Waveform"}
      ]
    },
    {
      "component": "PadGrid",
      "flex": "1",
      "min_height": "200px",
      "type": "4x4 Matrix",
      "modes": ["Synth Playback", "FX XY Performance", "Mic Slot Control"]
    }
  ]
}
```

---

## **4\. Technical Reference Lists**

### **4.1 Instrument Presets**

| Category | Presets |
| :---- | :---- |
| **Drums** | 808 Classic, 909 Punch, Lofi Snap, Trap Heavy, Acoustic, Industrial, Glitch, Synthwave, Jungle, Techno, House, Minimal |
| **Notes** | Sine Pluck, Saw Lead, Square Bell, Soft Pad, Glassy, Retro Lead, Wobble, Chiptune, Flute Synth, Brass, Organ, E-Piano |
| **Bass** | Sub Sine, Reese, Acid 303, Donk, Log Bass, Fuzz Bass, Slap Synth, Deep House, Growl, 8-Bit Bass, FM Bass, Drone |
| **Mic** | Clean, Bright, Warm, Radio, Echo, Room, Hall, Arena, Telephone, Robot, Alien, Demon |

### **4.2 FX Banks**

**Core FX (Bank 1\)**

1. Lowpass  
2. Highpass  
3. Reverb  
4. Stutter  
5. Buzz  
6. GoTo  
7. Saturator  
8. Delay  
9. Comb  
10. Distortion  
11. Smudge  
12. Channel

**Keymasher FX (Bank 2\)**

1. Repeat  
2. Pitch Down  
3. Pitch Rst  
4. Pitch Up  
5. Reverse  
6. Gate  
7. Scratch  
8. Buzz  
9. Stutter  
10. Go To  
11. Go To 2  
12. Buzz slip

### **4.3 Musical Scales**

* Chromatic  
* Major  
* Minor  
* Melodic Minor  
* Pentatonic

