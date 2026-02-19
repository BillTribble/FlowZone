# FlowZone — Pre-Build Assessment 7

**Date:** 13 Feb 2026  
**Scope:** Final deep-dive review of all Spec folder documents before bead creation. Line-by-line cross-reference focusing on architectural gaps, undefined runtime behaviors, missing data flows, and reliability concerns for a non-engineer owner.  
**Documents Reviewed:**
- `Spec_FlowZone_Looper1.6.md` (1803 lines)
- `Audio_Engine_Specifications.md` (366 lines)
- `UI_Layout_Reference.md` (367 lines)

---

## 1. Executive Summary

PBA6 found 16 issues (1 critical, 5 high, 5 medium, 5 low) and 5 reliability risks. **All 21 findings have been resolved in the current Spec 1.6** — verified in Section 2 below.

This seventh-pass review takes a different angle: rather than hunting for textual inconsistencies (which are now largely eliminated), PBA7 examines **data flow completeness, architectural blind spots, and runtime edge cases** that would cause an implementing agent to stop and ask questions, or worse, make silent assumptions that break at runtime.

**Found: 13 issues** (2 critical, 4 high, 4 medium, 3 low) and **4 reliability risks**.

**The critical theme this time:** The spec's text is internally consistent and well-written. The remaining gaps are **structural** — missing data flows between components (e.g., how does the Jam Manager get a session list?), undefined relationships between data models (e.g., `RiffSnapshot` vs `riffHistory`), and unspecified runtime behaviors for common user actions (e.g., what happens when an audio interface is unplugged mid-session?).

**Overall verdict:** These are the "last mile" issues. Resolving them will make the spec genuinely agent-proof — an agent can read the spec top-to-bottom and implement without needing to ask clarifying questions.

---

## 2. PBA6 Resolution Verification

All 21 PBA6 findings have been verified as resolved in the current spec:

| PBA6 # | Issue | Resolution | Status |
|:---|:---|:---|:---|
| C1 | VST3 build target missing from task breakdown | Task 0.2 now configures dual build targets | ✅ Resolved |
| H1 | No default `activeMode` on NEW_JAM | Inline defaults: `drums` / `synthetic` / `Synthetic` in §3.4 | ✅ Resolved |
| H2 | 23 FX effects don't fit 3×4 grid | §7.6.3 now specifies Core FX grid + "Infinite FX" bank toggle | ✅ Resolved |
| H3 | Solo in Mixer but no protocol command | §7.6.5 now reads "No Pan or Solo controls in V1" | ✅ Resolved |
| H4 | 4 drum kits in 12-slot grid undefined | §7.6.3 note: "only 4 grid slots active, remaining empty/dimmed" | ✅ Resolved |
| H5 | Pad-to-note mapping incomplete | §7.6.3 now has traversal, algorithm, octave coloring, wrap rules | ✅ Resolved |
| M1 | Drum icons row 2 mismatch between docs | UI_Layout_Reference.md row 2 now matches Audio Engine Spec §4.1 | ✅ Resolved |
| M2 | `TOGGLE_NOTE_NAMES` in Command union but UI-only | Removed from Command union; now a comment-only reference | ✅ Resolved |
| M3 | Adjust tab says "same as Mode tab" | §7.6.4 now references "same as Play tab §7.6.3" | ✅ Resolved |
| M4 | Waveform tap doesn't mention capture | §7.6.1 now explicitly says "Immediately captures" | ✅ Resolved |
| M5 | `SET_PAN` missing `reqId` | Pan removed from V1 entirely (no `SET_PAN` command) | ✅ Resolved |
| L1 | Retrospective buffer on mode switch undefined | §3.10 now explicitly states buffer not cleared on mode switch | ✅ Resolved |
| L2 | Session file schema not defined | §4.1.1 "Session File Format" added | ✅ Resolved |
| L3 | Test tone spec missing | §7.6.8 Tab B: "440Hz sine, -12dBFS, 2s, both channels" | ✅ Resolved |
| L4 | Error code 4010 shared between failure modes | §3.3 now uses `msg: "BUFFER_EMPTY"` vs `msg: "NO_MIX_CHANGES"` | ✅ Resolved |
| L5 | Keymasher buffer source ambiguous | Audio Engine Spec §1.2: "captures from sum of selected FX source slots" | ✅ Resolved |
| R1 | Memory budget undefined | §3.4 `memoryBudgetMB` + note: "under 2GB, minimum 8GB RAM" | ✅ Resolved |
| R2 | Single WebSocket priority inversion | §3.7 Priority Note added with V2 fallback plan | ✅ Resolved |
| R3 | DiskWriter shutdown flush undefined | §2.4 Shutdown Note: 30s blocking flush, then PARTIAL.flac | ✅ Resolved |
| R4 | FX Mode auto-commit comparison logic | **Non-issue per owner direction** — latest riff in history already represents pre-FX state; no auto-commit needed | ✅ Non-issue |
| R5 | `NEW_JAM` behavioral contract undefined | §3.2 now specifies: stops playback, clears slots, resets defaults, saves previous session | ✅ Resolved |

**Verdict:** Clean sweep. All PBA6 resolutions correctly applied, no regressions.

---

## 3. New Issues Found

### 🔴 CRITICAL — Will Block Core Build Tasks

#### C1: Jam Manager Has No Way to Get the Session List

**The gap:** The Jam Manager (§7.7) displays a list of all saved sessions with emoji, name, and date. Users can open, rename, or delete any session. But the data flow for populating this list is completely undefined.

**What the spec says:**
- `AppState.session` (§3.4) contains a **single** session: `{ id, name, emoji, createdAt }`.
- `LOAD_JAM` (§3.2) takes a `sessionId` — implying the engine knows about multiple sessions.
- `DELETE_JAM` and `RENAME_JAM` also take `sessionId`.
- `NEW_JAM` creates a new session.

**What's missing:**
1. **No session list in AppState.** The client has no way to discover which sessions exist. `AppState.session` is singular.
2. **No `GET_JAM_LIST` command.** No command requests a session list from the engine.
3. **No session list in `STATE_FULL`.** When the Jam Manager screen loads, it needs all sessions to render. The initial `STATE_FULL` handshake only sends `AppState`, which has one session.

**An agent building Task 3.4 (Jam Manager) or Task 5.7 (Assemble views)** will immediately hit this wall: where does the session list come from?

**Options:**
- **(A) Add `sessions` array to AppState:** `sessions: Array<{ id: string; name: string; emoji: string; createdAt: number }>` at the top level. Engine populates it by scanning `~/Library/Application Support/FlowZone/sessions/`. This is the cleanest approach — one source of truth, sessions update automatically via `STATE_PATCH` when jams are created/deleted/renamed.
- **(B) Add a `GET_JAM_LIST` command + response:** Client requests the list, engine responds with a one-time message (not part of AppState). This is more RESTful but breaks the "engine state is the single source of truth" pattern.

> **Recommendation:** Option (A). Add a `sessions` array to AppState (§3.4) and update the session list whenever `NEW_JAM`, `RENAME_JAM`, or `DELETE_JAM` fires. This keeps the "engine = source of truth" architecture intact.

---

#### C2: Slot-Level Loop Length Not Tracked — Breaks Playback and Auto-Merge

**The gap:** Each slot is committed with a specific loop length (1, 2, 4, or 8 bars via `SET_LOOP_LENGTH`). But `AppState.slots[n]` (§3.4) has no `loopLengthBars` field. There is only a global `transport.loopLengthBars`.

**Why this breaks things:**

1. **Playback of mixed-length slots:** If Slot 1 was committed at 2 bars and Slot 3 at 8 bars, the engine must loop Slot 1 four times for every one play of Slot 3. Without per-slot loop length metadata, the engine can only infer length from the audio file's sample count and the BPM at time of recording — but BPM could have changed since recording.

2. **Auto-Merge (§7.6.2.1):** The merge algorithm "mixes audio from Slots 1-8 into a single stereo buffer at current playback levels." If slots have different loop lengths, the merge output length and the looping behavior of shorter slots is undefined.

3. **Riff loading:** `LOAD_RIFF` restores slot states. Without per-slot loop length, restored slots would need to re-derive their loop length from the audio file, which requires knowing the original BPM.

**Options:**
- **(A) Add `loopLengthBars` to each slot:** `loopLengthBars: number` (1, 2, 4, or 8) added to the slot state in §3.4. This is the minimal, correct fix.
- **(B) Store loop length in audio file metadata:** Use FLAC metadata tags. More complex and requires parsing at load time.
- **(C) Store sample count + original BPM per slot:** Derivable but fragile if BPM changes.

> **Recommendation:** Option (A). Add `loopLengthBars: number` to each slot in AppState. Also add `originalBpm: number` to each slot so the engine knows the tempo context of the recording (needed if the user changes BPM after recording — does the loop time-stretch or just play at the new loop boundary?). Add a note to §7.6.2.1 Auto-Merge: *"Merge output length equals the longest slot's loop length. Shorter slots are repeated to fill the merge duration."*

---

### 🟠 HIGH — Will Cause Agent Confusion or Runtime Bugs

#### H1: External MIDI Input Path — Bypasses `CommandQueue` But Spec Says "Same Path"

**The gap:** §3.10 states:
> "External MIDI input also triggers NOTE_ON/OFF through the same path."

But `NOTE_ON`/`NOTE_OFF` are WebSocket commands (§3.2) that travel: React UI → WebSocket → `CommandQueue` (SPSC FIFO) → `CommandDispatcher` → Engine.

External MIDI arrives through JUCE's `MidiInput::Listener::handleIncomingMidiMessage()` callback, which runs on the message thread or a MIDI thread — not the WebSocket thread. It cannot go through the same `CommandQueue` SPSC FIFO because SPSC (Single Producer, Single Consumer) only supports one writer. Adding MIDI as a second producer would require either:

1. **A separate MIDI→Engine queue** (MPSC or second SPSC), or
2. **Marshaling MIDI to the WebSocket thread** (adds latency), or
3. **Processing MIDI directly on the audio thread** (lowest latency but different path from UI commands)

An agent implementing MIDI input (mentioned in §7.6.8 Tab C "MIDI & Sync") will need to know which approach to use.

> **Recommendation:** Add a note to §2.2.C (CommandQueue) or §3.10: *"External MIDI input does NOT pass through the WebSocket CommandQueue. MIDI events from JUCE's `MidiInput` callback are queued on a **separate** lock-free SPSC FIFO (`MidiQueue`) dedicated to MIDI input. The `CommandDispatcher` drains both queues (CommandQueue and MidiQueue) on each audio callback. MIDI note messages are converted to the same internal `NoteOn`/`NoteOff` event format used by UI commands, ensuring identical sound generation regardless of input source."*

---

#### H2: Retrospective Buffer Content — What Exactly Is Being Captured?

**The gap:** §3.10 says:
> "Continuously records the output of the currently selected instrument/mode."

§6.3 (MicProcessor) adds:
> "FX are applied to the input signal **before** it enters the retrospective capture buffer."
> "When FX Mode is active, the retrospective buffer input is re-routed to the FX output bus."

This gives us three modes of capture:
- **Mic mode:** Wet mic signal (mic input + reverb)
- **FX mode:** FX output bus (sum of selected slots + active effect)
- **Instrument modes (drums/notes/bass):** "Output of the currently selected instrument"

**The ambiguity for instrument modes:** When the user is in Drums mode and 5 existing loops are playing, what does the retrospective buffer capture?

1. **Only the live drum performance** (what the user is actively playing via pads) — makes sense for "I want to capture what I just played"
2. **The full mix output** (loops + live performance) — would mean each captured slot contains the entire mix, causing doubling when layered
3. **The instrument output only** (what the internal synth is generating, before it's mixed with existing loops) — same as (1) but explicit

Option 2 would be a critical design flaw (captured audio includes already-looping audio, leading to exponential buildup when layered). The intent is clearly option 1/3, but the spec needs to be explicit.

> **Recommendation:** Add to §3.10: *"The retrospective buffer captures **only** the live instrument output (the audio generated by the active mode's engine in response to note/pad input). It does NOT capture playback audio from existing filled slots. This prevents audio doubling when captured loops are layered. In FX Mode, the buffer captures the FX output bus (sum of selected slots processed through the active effect) — this is the one exception where existing slot audio enters the capture path, by design."*

---

#### H3: `DELETE_JAM` vs Garbage Collection Policy — Contradictory

**The gap:** §2.2.G (SessionStateManager) states:
> "Audio files are **never deleted immediately** — only marked for garbage collection on clean exit."

But `DELETE_JAM` (§3.2) says:
> "Delete a jam session and its audio"

And §2.2.G also says:
> "`DELETE_JAM` immediately removes session metadata and riff history entries; associated audio files are marked for GC and deleted on next clean exit."

This last line resolves the contradiction — but only partially. The behavior is spread across two parts of §2.2.G. An agent might read only the DELETE_JAM command definition ("delete a jam session **and its audio**") and implement immediate file deletion, violating the GC policy.

> **Recommendation:** Update the `DELETE_JAM` description in §3.2 to: *"Delete a jam session. Immediately removes session metadata and riff history entries. Audio files are marked for garbage collection and deleted on next clean exit (per §2.2.G GC policy)."* This makes the command description self-contained.

---

#### H4: `protocolVersion` Value Never Declared

**The gap:** The protocol version is used in three places:
- §2.2.L Connection Lifecycle: "If `protocolVersion` doesn't match, server sends `ERROR { code: 1100, msg: 'PROTOCOL_MISMATCH' }` and closes."
- §3.4 `AppState.meta.protocolVersion: number`
- §5.4 Health endpoint returns `"protocol_version": 1`

The health endpoint example shows `1`, but this is an example JSON response, not a normative declaration. Nowhere does the spec state: "V1 protocol version is `1`."

An agent implementing the handshake check needs the exact value. Hardcoding `1` based on an example is fragile.

> **Recommendation:** Add to §3.1 (Single Source of Truth) or §3.4 meta: *"`protocolVersion`: V1 protocol version is `1`. This value is incremented when breaking changes are made to the command schema or state shape. The server and client must agree on this value during the WebSocket handshake."*

---

### 🟡 MEDIUM — Should Fix Before Beads

#### M1: `RiffSnapshot` (§3.5) vs `AppState.riffHistory` — Relationship Undefined

§3.5 defines the C++ internal representation:
```cpp
struct RiffSnapshot {
    juce::Uuid id;
    int64_t timestamp;
    int64_t stateRevisionId;
    double tempo;
    int rootKey;
    std::array<SlotState, 8> slots;
};
```

§3.4 defines the client-facing representation:
```typescript
riffHistory: Array<{
    id: string;
    timestamp: number;
    name: string;
    layers: number;       // Number of active slots when committed
    colors: string[];     // Layer cake colors (source-based)
    userId: string;
}>;
```

These are clearly different structures representing the same concept at different levels. But:

1. `RiffSnapshot` has `tempo`, `rootKey`, `slots[8]` — none of which appear in `riffHistory`.
2. `riffHistory` has `name`, `layers`, `colors`, `userId` — none of which appear in `RiffSnapshot`.
3. Which one is persisted to disk? Both? One derived from the other?
4. When `LOAD_RIFF` fires, does the engine read the full `RiffSnapshot` (with slot audio references) and reconstruct the session from it?

An agent building SessionStateManager (Task 6.3) needs to know the mapping.

> **Recommendation:** Add a note to §3.5: *"`RiffSnapshot` is the full internal representation stored on disk (as JSON + audio file references). `AppState.riffHistory` is a lightweight summary derived from `RiffSnapshot` for UI display. The mapping: `riffHistory.layers` = count of slots with `state != EMPTY`; `riffHistory.colors` = array of source-category colors from slot `instrumentCategory`; `riffHistory.name` = auto-generated from timestamp. `LOAD_RIFF` reads the full `RiffSnapshot` from disk and restores all slot states, audio, volumes, and transport settings."*

---

#### M2: Config File Location Not Specified

§2.2.L references `config.json` for server port (`"serverPort": 8765`) and PIN auth (`"requirePin": true`).  
§4.5 references `config.json` with `configVersion`.  
§4.4 references `config.json` in the failure handling table ("Config Corrupt" → "JSON parse failure").

But the file path is never specified. Options:
- `~/Library/Application Support/FlowZone/config.json` (standard macOS app support)
- `~/Library/Preferences/FlowZone/config.json` (macOS preference convention)
- Adjacent to the binary
- Within the session directory (would make it per-session, which doesn't match the described behavior)

> **Recommendation:** Add to §4 or §4.1: *"`config.json` is located at `~/Library/Application Support/FlowZone/config.json`. This is the application-level configuration file — not per-session. It stores server port, PIN authentication, audio device preferences, VST search paths, and interface settings. The `configVersion` field enables migrations across app updates."*

---

#### M3: Auto-Merge (§7.6.2.1) — Behavior with Mixed Loop Lengths Undefined

Directly related to C2 but worth calling out as a separate implementation concern:

§7.6.2.1 says: "Mix the audio from Slots 1-8 into a single stereo buffer at current playback levels."

If Slot 1 is 2 bars and Slot 7 is 8 bars:
- Is the merge output 2 bars (truncating longer loops)?
- Is the merge output 8 bars (repeating shorter loops)?
- Is it the global `transport.loopLengthBars` value?

> **Recommendation:** Add to §7.6.2.1: *"The merge output length equals the longest active slot's loop duration. Shorter slots are repeated (looped) to fill the merge length. The resulting merged audio in Slot 1 has `loopLengthBars` equal to the longest source slot."*

---

#### M4: Bounce and Speed Knobs — Behavior in Drum Mode Undefined

Audio Engine Spec §4.3:
> "**Bounce/Speed** → NOT USED for Drum Mode in V1."

The Adjust Tab (§7.6.4) renders a fixed 2×4 knob grid with Bounce at (2,1) and Speed at (2,2) for all instrument modes (except Microphone which has its own layout). When the user is in Drums mode:

1. Are Bounce and Speed knobs **hidden**?
2. Are they **visible but disabled** (greyed out, non-interactive)?
3. Are they **visible and interactive but have no effect**?

Option 3 is a poor user experience. Option 1 changes the grid layout per mode. Option 2 is the most professional approach.

> **Recommendation:** Add to §7.6.4 or Audio Engine Spec §4.3: *"In Drum mode, Bounce and Speed knobs are rendered in their grid positions but displayed as disabled (greyed out, non-interactive). This maintains the consistent 2×4 grid layout across all modes while indicating these parameters are not available for drums."*

---

### 🟢 LOW — Worth Noting

#### L1: Slot Count — Never Formally Declared as a Constant

The spec references "8 slots" in 15+ locations (§2.2.A multi-channel output, §3.4 AppState slots array, §7.6.2.1 auto-merge "Slots 1-8", §7.6.5 mixer "8 channels"). But:

- `AppState.slots` is typed as `Array<{...}>` — dynamic length
- `RiffSnapshot` uses `std::array<SlotState, 8>` — fixed at 8
- There's no `const MAX_SLOTS = 8` declaration

An agent could implement a dynamic slot count on the React side but fixed-8 on the C++ side.

> **Recommendation:** Add to §3.1 or §3.4: *"The slot count is a compile-time constant: 8. `AppState.slots` always contains exactly 8 entries. Empty slots have `state: 'EMPTY'`. This constant should be defined as `MAX_SLOTS = 8` in both `commands.h` and `schema.ts`."*

---

#### L2: `isVstMode` in AppState — How Is It Set?

§3.4 `AppState.meta.isVstMode: boolean` is described as "True when running as VST3 plugin (read-only)." This is set at compile time via `#if JucePlugin_Build_VST3`. But AppState is runtime state that gets serialized to JSON. The value should be set during engine initialization based on the build target, but this init step isn't documented.

> **Recommendation:** Add to §2.4 Component Lifecycle startup sequence or §2.2.A: *"`isVstMode` is set once during `FlowEngine` construction based on `#if JucePlugin_Build_VST3` preprocessor guard. It is immutable for the session lifetime and included in every `STATE_FULL` snapshot."*

---

#### L3: CivetWeb Thread Model — Not Documented

§2.1 labels the WebSocket server as "CivetWeb - Background Thread." Task 0.1 specifies integrating CivetWeb as source files. But the spec doesn't document:

1. CivetWeb runs its own thread pool for handling HTTP/WebSocket connections.
2. WebSocket message callbacks fire on CivetWeb's worker threads, not JUCE's message thread.
3. The `CommandQueue` SPSC FIFO is the thread-safe bridge between CivetWeb's callback thread (producer) and the audio thread (consumer).
4. `StateBroadcaster` must push state updates to CivetWeb's WebSocket from the message thread.

An agent unfamiliar with CivetWeb's threading model may inadvertently create race conditions.

> **Recommendation:** Add to §2.2 or alongside the Thread Priority Table (§2.3): *"CivetWeb runs its own internal thread pool. WebSocket message handlers (`on_message`) execute on CivetWeb worker threads — not the JUCE message thread. Thread safety is ensured by: (1) incoming commands are written to the lock-free `CommandQueue` SPSC FIFO on the CivetWeb thread and consumed on the audio thread; (2) outgoing state broadcasts are queued by `StateBroadcaster` on the message thread and sent via CivetWeb's `mg_websocket_write()` which is thread-safe."*

---

## 4. Cross-Document Consistency Matrix (Updated)

| Aspect | Spec 1.6 | Audio Engine Spec | UI Layout Ref | Status |
|:---|:---|:---|:---|:---|
| **Tab labels** | Mode, Play, Adjust, Mixer | References "Play Tab" | Mode, Play, Adjust, Mixer | ✅ |
| **Core FX count** | 12 (§2.2.M) | 12 (§1.1) | 12 (play_tab fx) | ✅ |
| **Infinite FX count** | 11 (§2.2.M) | 11 (§1.2) | 11 (play_tab infinite_fx) | ✅ |
| **FX bank switching** | §7.6.3 "toggle switch" | — | Two arrays in preset_examples | ✅ |
| **Notes presets** | 12 (§2.2.M) | 12 named (§2.1) | "See Audio_Engine_Specs" | ✅ |
| **Bass presets** | 12 (§2.2.M) | 12 named (§3.1) | "See Audio_Engine_Specs" | ✅ |
| **Drum kits** | 4 kits (§2.2.M) | 4 kits (§4.2) | Pad grid (16 cells) | ✅ |
| **Drum icons all rows** | — | 16 unique icons §4.1 | 16 matching icons | ✅ |
| **Pad-to-note mapping** | §7.6.3 algorithm | — | colored_pads note | ✅ |
| **Drum kit grid** | §7.6.3 "4 active, 8 dimmed" | — | — | ✅ |
| **Adjust tab pad reference** | "same as Play tab §7.6.3" | — | "instrument_specific_pads" | ✅ |
| **Solo in Mixer** | "No Pan or Solo in V1" | — | — | ✅ |
| **Default activeMode** | drums/synthetic/Synthetic §3.4 | — | — | ✅ |
| **VU meter style** | §7.6.5 "integrated" | — | "fader_with_integrated_vu_meter" | ✅ |
| **Keymasher buttons** | §3.2 KeymasherButton type (12) | §1.2 item 1 (12 buttons) | play_tab keymasher (12 buttons) | ✅ |
| **Keymasher button labels** | repeat, pitch_down, etc. | Repeat, Pitch Down, etc. | Repeat, Pitch Down, etc. | ✅ |
| **Settings access** | §7.4 "More button in Mixer" | — | "More button in Mixer" | ✅ |
| **Settings tabs** | Interface, Audio, MIDI & Sync, Library & VST | — | Interface, Audio, MIDI & Sync, Library & VST | ✅ |
| **WebSocket port** | 8765 (§2.2.L) | — | — | ✅ |
| **BPM range** | 20-300 (§3.2) | — | — | ✅ |
| **Buffer sizes** | 16-1024 (§3.2, §7.6.8) | — | 16-1024 (settings) | ✅ |
| **Sample rates** | 44.1k-96k (§3.2, §7.6.8) | — | 44.1k-96k (settings) | ✅ |
| **Session list in AppState** | **Missing** | — | — | ❌ **C1** |
| **Slot loop length** | **Missing from slot state** | — | — | ❌ **C2** |
| **MIDI input path** | "same path" §3.10 | — | — | ⚠️ **H1** |
| **Retro buffer content** | "instrument output" §3.10 | "wet signal" §6.3 | — | ⚠️ **H2** |
| **DELETE_JAM GC behavior** | Contradictory §3.2 vs §2.2.G | — | — | ⚠️ **H3** |
| **protocolVersion value** | Health example only | — | — | ⚠️ **H4** |
| **RiffSnapshot↔riffHistory** | Both defined, mapping missing | — | — | ⚠️ **M1** |
| **config.json path** | Not specified | — | — | ⚠️ **M2** |
| **Auto-merge mixed lengths** | Undefined §7.6.2.1 | — | — | ⚠️ **M3** |
| **Drum mode Bounce/Speed** | "NOT USED" (Audio Spec §4.3) | disabled? hidden? | — | ⚠️ **M4** |
| All other aspects (40+) | — | — | — | ✅ Consistent |

---

## 5. Reliability Risk Assessment

### R1: Audio Device Hot-Unplug — Behavior Undefined

**Scenario:** The user is mid-performance with an external audio interface. They accidentally unplug the USB cable (or the interface crashes).

**What happens:**
- JUCE fires `AudioDeviceManager::audioDeviceError()` or the device disappears from the device list.
- The audio callback stops (no hardware to drive it).
- All playing loops freeze.
- The WebSocket and React UI remain alive.

**What the spec covers:**
- CrashGuard Level 2 (§2.2.F): "Audio driver failure **at boot**" — not runtime.
- §4.4 General Failure Handling: No row for "audio device disconnect at runtime."

**Why this matters for a non-engineer:** This is not an edge case — USB audio interfaces can disconnect due to cable issues, hub power problems, or macOS sleep/wake cycles. Without defined behavior, the app may hang, crash, or enter an unrecoverable state.

> **Recommendation:** Add to §4.4 Failure Handling table:
> 
> | Scenario | Severity | Detection | Response | Recovery |
> |:---|:---|:---|:---|:---|
> | **Audio Device Disconnect** | High | `AudioDeviceManager` error callback | Pause transport. UI banner: "Audio device disconnected." | Auto-reconnect when device reappears. If user switches device in Settings, resume. |

---

### R2: Retrospective Buffer at Maximum Capacity — Edge Case

**Scenario:** User is at 20 BPM (minimum tempo). 8 bars at 20 BPM = exactly 96 seconds — the full buffer capacity. The user plays continuously for longer than 96 seconds before tapping a loop length button.

**What happens:** The circular buffer overwrites the oldest audio. If the user then taps "8 Bars," they get the most recent 96 seconds — which is correct. But if they've been playing for exactly 96.001 seconds, the very first note is overwritten. At exactly 20 BPM, there is zero margin.

**Why this matters:** The design is correct — circular buffers overwrite by design. But the user experience at the boundary is worth understanding: at 20 BPM with 8 bars, the user gets *exactly* what they played with no safety margin. At 19.9 BPM (below the stated minimum, but if the user types it), 8 bars would exceed the buffer.

> **Recommendation:** This is actually well-handled by the spec — the BPM minimum of 20 was specifically chosen to match the 96-second buffer (noted in §3.10). No spec change needed. Note for agents: *the BPM clamping in TransportService must be enforced before the retrospective buffer duration math is computed.*

---

### R3: WebSocket Reconnection During Active Performance

**Scenario:** The embedded `WebBrowserComponent` reloads (e.g., due to a React error boundary triggering a full remount), breaking the WebSocket connection. The user is mid-performance with 6 loops playing.

**What the spec covers:**
- §2.2.L Connection Lifecycle step 5: "Disconnect: Client shows 'Reconnecting…' overlay. Audio continues unaffected. Reconnect uses exponential backoff: 100ms → 200ms → 400ms → … → max 5s."
- Step 6: "Client → `WS_RECONNECT` with last known `revisionId`. Server sends diff if revision is recent, or full snapshot if stale."

**What's not covered:**
- During reconnection, any user pad taps or knob turns are lost (no WebSocket = no command path).
- If the reconnect takes > 5 seconds (max backoff), does the client give up? How many retries?
- Can the user still hear audio during reconnect? (Yes — engine runs independently. Good.)

> **Recommendation:** Add to §2.2.L step 5: *"Reconnection retries indefinitely (no maximum retry count). During reconnection, user input is discarded — audio continues unaffected but the UI is non-interactive. The 'Reconnecting…' overlay should include a countdown to next retry attempt."*

---

### R4: First-Launch Race Condition — WebSocket Before React Mounts

**Scenario:** On first launch, the JUCE app starts CivetWeb (WebSocket server) and then loads the React app in `WebBrowserComponent`. If the React app tries to establish the WebSocket connection before CivetWeb is fully initialized, the connection fails silently.

The startup sequence (§2.4) shows WebServer starting before SessionStateManager. But `WebBrowserComponent` (which loads React) is part of the JUCE Component hierarchy — it starts rendering when the main window opens, which could be before the WebServer is ready.

> **Recommendation:** Add to §2.4 or §2.2.L: *"The React client must handle initial WebSocket connection failure gracefully. On first connection attempt, if the server is not yet ready, the client retries using the same exponential backoff strategy as reconnection. The 'Connecting…' overlay is shown until the first successful `STATE_FULL` is received."*

---

## 6. Summary of All Findings

### 🔴 Critical — Must Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| C1 | Jam Manager has no way to get the session list — no `sessions` array in AppState, no `GET_JAM_LIST` command | Add `sessions` array to AppState; engine populates by scanning session directory | 15 min |
| C2 | Slot-level loop length not tracked — breaks playback of mixed-length slots and auto-merge | Add `loopLengthBars` (and optionally `originalBpm`) to each slot in AppState | 10 min |

### 🟠 High — Should Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| H1 | MIDI input path says "same path" but SPSC queue can't have two producers | Define separate MidiQueue SPSC; dispatcher drains both | 10 min |
| H2 | Retrospective buffer content ambiguous — does it capture live instrument only or full mix? | Clarify: captures live instrument output only (not existing loop playback) | 5 min |
| H3 | `DELETE_JAM` description says "delete audio" but GC policy says "never delete immediately" | Update DELETE_JAM in §3.2 to reference GC policy explicitly | 5 min |
| H4 | `protocolVersion` value never formally declared (only in a health endpoint example) | Declare `protocolVersion = 1` in §3.1 or §3.4 | 2 min |

### 🟡 Medium — Should Fix

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| M1 | `RiffSnapshot` (C++) vs `riffHistory` (TS) — relationship and mapping undefined | Document mapping: RiffSnapshot = full disk format, riffHistory = UI summary | 10 min |
| M2 | `config.json` file path never specified | Declare path: `~/Library/Application Support/FlowZone/config.json` | 2 min |
| M3 | Auto-merge with mixed loop lengths — output length undefined | Define: merge length = longest slot, shorter slots repeat | 5 min |
| M4 | Bounce/Speed knobs in Drum mode — shown but "NOT USED" without UI guidance | Define as "visible but disabled/greyed out" | 2 min |

### 🟢 Low — Worth Noting

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| L1 | Slot count (8) never formally declared as a constant | Add `MAX_SLOTS = 8` constant declaration to §3.1 | 2 min |
| L2 | `isVstMode` initialization step not documented | Document as set once during FlowEngine construction | 2 min |
| L3 | CivetWeb threading model not documented | Add thread model note to §2.2 or §2.3 | 5 min |

### Reliability Risks

| # | Risk | Severity | Action |
|:--|:-----|:---------|:-------|
| R1 | Audio device hot-unplug at runtime undefined | High | Add to §4.4 failure handling table |
| R2 | Retrospective buffer at exact 20 BPM boundary | Low | No change needed — design is correct, BPM clamping prevents overflow |
| R3 | WebSocket reconnection during performance — retry limit and input loss undefined | Medium | Define infinite retry + discarded input behavior |
| R4 | First-launch race condition — React connects before CivetWeb is ready | Medium | Document graceful initial connection failure handling |

---

## 7. Overall Assessment

| Dimension | Rating (Pre-Fix) | Rating (Post-Fix) | Notes |
|:---|:---|:---|:---|
| **Completeness** | 🟢 **94%** | 🟢 **99%** | Session list and slot loop length are the two structural gaps |
| **Internal Consistency** | 🟢 **97%** | 🟢 **99%** | DELETE_JAM wording and MIDI path are the only contradictions |
| **Agent Buildability** | 🟢 **92%** | 🟢 **98%** | C1 blocks Jam Manager entirely; C2 blocks mixed-length playback |
| **Reliability for Non-Engineer** | 🟢 **93%** | 🟢 **98%** | Audio device disconnect is the main gap for real-world resilience |

**Verdict:** The spec has matured enormously through seven assessment rounds. The macro architecture is sound, the protocol is well-defined, the UI layout is comprehensive, and the risk mitigations are thorough. The remaining issues are **structural data-flow gaps** (session list, slot loop length) and **runtime edge cases** (MIDI threading, device disconnect). These are the type of issues that only emerge from tracing data through the system end-to-end, which is exactly what this review did.

After resolving these 13 findings (estimated total effort: ~75 minutes), the spec will be at **≥98% across all dimensions** and **ready for bead creation with very high confidence**.

**Recommended fix priority:**
1. **C1, C2** (structural gaps) — these block Phase 2 and Phase 3 tasks
2. **H1, H2** (architectural clarity) — these prevent silent runtime bugs
3. **H3, H4, M1-M4** (documentation) — quick fixes, high clarity payoff
4. **R1, R3, R4** (reliability) — important for real-world resilience
5. **L1-L3** (nice-to-have) — can be addressed during bead implementation

---

*End of Pre-Build Assessment 7*
