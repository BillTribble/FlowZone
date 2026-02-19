# FlowZone V2 Prototype — Technical Spec

**Version:** 2.0-proto  
**Target Framework:** JUCE 8.0.x (C++20)  
**Platform:** macOS (Standalone App, Apple Silicon Native)  
**UI:** JUCE Native (no web UI, no WebSocket, no React)

---

## 1. Goals

### 1.1 Proof of Concept Goal

Get **one element** of the full FlowZone app working end-to-end, done entirely in **JUCE native UI**:

> **Microphone Input** with:
> - Gain control (rotary knob)
> - Level meter (real-time VU)  
> - Monitor button (route mic to output)

### 1.2 Why Start Here

- Mic input is the foundation of the looper — if it doesn't work, nothing else matters
- It validates the audio device pipeline (input → processing → output)
- It's a self-contained proof of concept with no dependencies on transport, slots, or looping
- JUCE native UI eliminates the WebSocket/React complexity that caused v1 failures

### 1.3 Non-Goals (for this prototype)

- ❌ Looping / retrospective buffer
- ❌ Transport (play/pause/BPM)
- ❌ Slots / mixing
- ❌ Effects / synths / drums
- ❌ WebSocket / React UI
- ❌ Session management
- ❌ Plugin hosting
- ❌ Riff history

---

## 2. Architecture

```
┌──────────────────────────────────────┐
│          MainComponent (JUCE)         │
│                                      │
│  ┌──────────┐  ┌──────────────────┐  │
│  │ Gain Knob│  │   Level Meter    │  │
│  │ (Slider) │  │ (Custom Paint)   │  │
│  └──────────┘  └──────────────────┘  │
│                                      │
│  ┌──────────────────────────────────┐│
│  │     Monitor Toggle Button        ││
│  └──────────────────────────────────┘│
│                                      │
│  AudioDeviceManager (mic input)      │
│  AudioCallback: gain → meter → out   │
└──────────────────────────────────────┘
```

### 2.1 Audio Signal Flow

```
Mic Input → Gain Stage → Level Metering → [Monitor ON?] → Output
                              ↓
                       Peak level (atomic)
                              ↓
                       UI repaint timer
```

### 2.2 Thread Model

| Thread | Role |
|---|---|
| **Audio Thread** | Read mic input, apply gain, compute peak, copy to output if monitoring |
| **Message Thread** | JUCE UI — repaint level meter at ~30fps via Timer |

### 2.3 Audio Thread Safety Rules

- Gain value read via `std::atomic<float>`
- Peak level written via `std::atomic<float>`
- Monitor state read via `std::atomic<bool>`
- **No allocation, no locks, no I/O on audio thread**

---

## 3. Components

### 3.1 MainComponent

- Inherits: `juce::AudioAppComponent`
- Owns: `juce::Slider` (gain knob), `LevelMeter`, `juce::TextButton` (monitor toggle)
- Implements: `prepareToPlay()`, `getNextAudioBlock()`, `releaseResources()`
- Timer: repaints level meter at ~30Hz

### 3.2 LevelMeter

- Custom `juce::Component`
- Paints vertical bar based on current peak level
- Green → Yellow → Red gradient
- Peak hold indicator (decays over ~1s)
- Receives peak value via setter (called from Timer on message thread)

### 3.3 Gain Control

- `juce::Slider` in rotary mode
- Range: -60dB to +40dB (stored as linear gain internally)
- Default: 0dB (gain = 1.0)
- Label shows current dB value

### 3.4 Monitor Button

- `juce::TextButton` or `juce::ToggleButton`
- OFF by default (no audio output — avoids feedback)
- When ON: mic input (post-gain) is copied to output buffer
- When OFF: output buffer is cleared (silence)
- Visual state: clear ON/OFF indication

---

## 4. File Structure

```
/flowzone-v2/
├── src/
│   ├── Main.cpp              # JUCE app entry point
│   ├── MainComponent.h       # Main UI + audio 
│   ├── MainComponent.cpp
│   ├── LevelMeter.h          # Custom VU meter component
│   └── LevelMeter.cpp
├── Spec/
│   └── Spec_FlowZone_V2_Prototype.md  (this file)
├── archive/v1/               # All v1 files preserved here
├── libs/JUCE/                 # JUCE framework
├── agents.md
└── .beads/
```

---

## 5. Build Configuration

- **Build System:** Projucer → Xcode (same as v1)
- **Project File:** `FlowZone.jucer` (new, minimal)
- **Source Files:** Only the 5 files in `src/`
- **JUCE Modules:** `juce_core`, `juce_audio_basics`, `juce_audio_devices`, `juce_audio_utils`, `juce_gui_basics`, `juce_gui_extra`
- **Build Command:** `xcodebuild -project Builds/MacOSX/FlowZone.xcodeproj -scheme "FlowZone - All" -configuration Debug`

---

## 6. Future Expansion Path

Once this prototype works reliably:

1. **Phase 2:** Add retrospective circular buffer (always-on capture)
2. **Phase 3:** Add transport (BPM, play/pause) + single loop slot
3. **Phase 4:** Add loop playback + overdub
4. **Phase 5:** Add multiple slots + mixer
5. **Phase 6:** Add effects, instruments, etc.

Each phase builds on the previous one. No phase should break what came before.

---

## 7. Reference

The full v1 spec is preserved at `archive/v1/Spec/` for reference on:
- Audio engine specifications (effects, synths, drums)
- UI layout reference (for eventual full UI rebuild)  
- Full technical design (looping, slots, riff history, etc.)
- MicProcessor design (§6 of Audio_Engine_Specifications.md) — directly relevant to this prototype
