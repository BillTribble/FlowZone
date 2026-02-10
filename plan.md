# Plan: FlowState Web Application

This plan outlines the development of **FlowState**, a web-based "Retrospective Riff Engine" inspired by the original C++/JUCE specification. It leverages modern web technologies to deliver a desktop-class audio experience in the browser.

> **Note:** This plan adapts the [Original Specification](./Spec_FlowState_Looper.md) for a web environment. All functional requirements (logic, behavior) are derived from the spec, while technical implementation details are adjusted for the Web Platform.

## 1. Technical Stack (Web Adaptation)

*   **Framework:** Next.js (React) or Vite (React) - *Recommendation: Vite for lower latency dev server, though Next.js works fine.*
*   **Language:** TypeScript
*   **Styling:** Tailwind CSS (for "Premium/High Spec" UI as per `agents.md`)
*   **Audio Engine:** **Web Audio API** (Raw) + **Tone.js** (for scheduling/synths/effects).
*   **State Management:** **Zustand** (for efficient, transient audio state updates) or **Jotai**.
*   **Storage (Persistence):** **IndexedDB** (via `idb` or `localforage`) to store large audio blobs (FLAC/WAV) locally. *LocalStorage is insufficient for audio.*
*   **Component Library:** Radix UI (for accessible primitives) + Lucide React (icons).

## 2. Core Architecture

### 2.1 Audio Engine (The "Always-On" Buffer)
*   **Circular Buffer Implementation:**
    *   Use `AudioWorklet` for low-latency processing off the main thread.
    *   Maintain a ~60s ring buffer (Float32Array) in the worklet.
    *   *Reference Spec §3.1: "Always-On Buffer"*
*   **Commit Logic:**
    *   Slice the buffer from `Active Time - Loop Length` when "Commit" is triggered.
    *   Crossfade edges (5-10ms) in the AudioWorklet or main thread post-processing.
    *   *Reference Spec §3.1: "Commit Logic"*

### 2.2 Slot System
*   **State:** Array of `Slot` objects (BufferSource nodes).
*   **Merging (Summing Event):**
    *   OfflineAudioContext to render multiple buffers into one new buffer (The "Bounce").
    *   *Reference Spec §3.2: "The Summing Event"*

## 3. Development Phases

### Phase 1: Foundation & "Hello Sound"
1.  **Project Initialization:**
    *   Setup Vite/Next.js with TypeScript & Tailwind.
    *   Configure ESLint/Prettier (AirBnb or standard).
    *   Setup `agents.md` guidelines enforcement (e.g. husky hooks).
2.  **Audio Context Setup:**
    *   Initialize `Tone.Context` or raw `AudioContext`.
    *   Create a basic microphone input stream (`navigator.mediaDevices.getUserMedia`).
    *   Visual feedback: Simple VU meter to confirm input.

### Phase 2: The Retrospective Engine (Core Logic)
1.  **Circular Buffer Worklet:**
    *   Write `circular-buffer.worklet.ts` processor.
    *   Implement `AudioWorkletNode` to communicate with UI.
2.  **Commit Triggers:**
    *   UI Buttons: 1, 2, 4, 8 Bars.
    *   Logic to calculate sample length based on **BPM** (default 120).
    *   Extraction: Pull data from the worklet and create an `AudioBuffer`.

### Phase 3: The Rack & Playback
1.  **Slot UI:**
    *   Create 8 visual slots.
    *   Waveform visualization (Canvas or SVG) for slot content.
2.  **Playback Engine:**
    *   Trigger all active slots in sync with the Transport (Global Transport).
    *   Implement "Play/Stop" and "Loop" functionality.
3.  **Slot Management:**
    *   "Find First Empty" logic (*Spec §3.2*).
    *   Mute/Solo/Clear controls per slot.

### Phase 4: History & Persistence
1.  **IndexedDB Layer:**
    *   Schema for `Riff`: `{ id, timestamp, bpm, slots: [blobs] }`.
    *   Save `Riff` on every "Commit" event (*Spec §3.3*).
2.  **History Browser:**
    *   Sidebar to list previous Riffs.
    *   "Instant Recall": Load blobs from IDB into Audio Buffers.

### Phase 5: Synthesis & Effects (Creative Suite)
1.  **Internal Synth:**
    *   Tone.js `PolySynth` or `FMSynth` integration.
    *   On-screen keyboard or Web MIDI support.
2.  **FX Chains:**
    *   Master Bus effects (Reverb, Delay) using Tone.js nodes.
    *   Resampling workflow (*Spec §3.6*).

### Phase 6: Polish & Aesthetics
1.  **Glassmorphism UI:**
    *   Implement the "Premium" look inspired by the spec description.
    *   Micro-interactions and animations (Framer Motion).
2.  **Optimization:**
    *   Ensure UI doesn't block Audio Thread.
    *   Memory management (revoke object URLs, clean up buffers).

## 4. Immediate Next Steps

1.  **Initialize Git Repo:**
    *   `git init`
    *   `git add .`
    *   `git commit -m "Initial commit"`
2.  **Scaffold Application:**
    *   Run `npm create vite@latest . -- --template react-ts` (or Next.js).
3.  **Install Core Deps:**
    *   `npm install tone zustand idb framer-motion lucide-react`
