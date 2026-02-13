# FlowZone — Pre-Build Assessment 5

**Date:** 13 Feb 2026  
**Scope:** Final reliability-focused deep review of all Spec folder documents before bead creation. Every section cross-referenced against every other, with emphasis on protocol completeness, undefined runtime behaviors, and gaps that would force an AI agent to guess — which a non-engineer owner cannot troubleshoot.  
**Documents Reviewed:**
- `Spec_FlowZone_Looper1.6.md` (1842 lines)
- `Audio_Engine_Specifications.md` (360 lines)
- `UI_Layout_Reference.md` (368 lines)
- `Pre_Build_Assessment_4.md` (analysis folder, 539 lines) — for resolution context

---

## 1. Executive Summary

The spec suite has matured significantly through four rounds of assessment. PBA4 identified 18 issues (4 critical, 6 high, 5 medium, 3 low); the appendix confirms all were resolved in the Spec 1.6 update. The resolutions are high quality — buffer sizing, phase alignment, STOP removal, slot state simplification, and SampleEngine deferral all make the spec cleaner.

This fifth-pass review — with fresh eyes and a focus on **protocol completeness** and **runtime behavior** — has found **13 issues** (2 critical, 4 high, 4 medium, 3 low) and **4 reliability risks**. 

**The critical theme this time:** Two fundamental categories of user interaction have **no protocol whatsoever**:

1. **Playing sounds** — The user taps pads to trigger drums/notes/bass, but there is no command in the schema to communicate a pad press from React to the C++ engine. The entire performance surface is unprotocol'd.
2. **Capturing loops** — Loop length buttons are described as the primary capture trigger (§3.10, §7.6.1), but `SET_LOOP_LENGTH` in the command schema only sets a value — it doesn't trigger audio capture. There's a protocol-to-behavior gap.

These aren't edge cases — they are the **core user workflow** (play sounds → capture loops). Without protocol definitions for these, an agent literally cannot build the app's primary function.

---

## 2. PBA4 Resolution Verification

I verified that PBA4's appendix resolutions are present in the current spec documents:

| PBA4 # | Resolution Claimed | Present in Spec? | Notes |
|:---|:---|:---|:---|
| C1 | Buffer set to ~96s | ✅ | §1.1 line 39: "~96s", §3.10 line 562: "Approximately **96 seconds**" — consistent |
| C2 | §8.6 aligned to §9 (9 phases) | ✅ | §8.6 now has Phases 0-8, matching §9 |
| C3 | `TRIGGER_SLOT` removed | ✅ | Not in §3.2 or §3.3 |
| C4 | SampleEngine deferred to V2 | ✅ | §2.2.M: "V2", §1.3 expanded, §7.6.2 reserved slot, no Task 4.6 |
| H1 | `COMMIT_RIFF` clarified | ✅ | §3.2 comment updated. §3.10 step 3 now says loop length tap = implicit commit |
| H2 | `STOP` removed | ✅ | Not in §3.2. Only PLAY/PAUSE. Transport comment confirms no Stop. |
| H3 | Playback controls → Slot Indicators | ✅ | §7.6.3: "1 row × 8 oblong indicators" with dual behavior |
| H4 | Pad-to-note mapping defined | ✅ | §7.6.2: "bottom-left = root note, ascending through selected scale" + `Scale` type added |
| H5 | MUTED/STOPPED simplified | ✅ | §3.4: states are `EMPTY | PLAYING | MUTED` only. §3.10: PLAYING pauses in place. |
| H6/R1 | CivetWeb integration guidance | ✅ | §9 Task 0.1: "Integrate CivetWeb as source files in libs/civetweb/" |
| H6/R2 | WebBrowserComponent dev/prod | ✅ | §9 Task 0.1: dev loads localhost:5173, release loads bundled resources |
| H6/R3 | MP3 moved to V2 | ✅ | §1.3: "MP3 / AAC Export" listed as V2. §3.6: "FLAC only" |
| H6/R5 | FLAC compression level | ✅ | §3.6: "Compression level 0 (fastest)" |
| H6/R7 | Ext Inst/Ext FX placeholder | ✅ | §7.6.2: "shows an empty panel with a 'Coming Soon' label" |
| M1-M5 | Various fixes | ✅ | All verified present |
| L1-L3 | Various fixes | ✅ | All verified present |

**Verdict:** All PBA4 resolutions have been correctly applied. No regressions detected.

---

## 3. New Issues Found

### 🔴 CRITICAL — Will Block Core Functionality

#### C1: No Command for Triggering Pad Sounds (NOTE_ON / PAD_TRIGGER Missing)

**The gap:** The 4×4 pad grid is the primary performance interface. In Drums mode, tapping a pad plays a drum sound. In Notes/Bass mode, tapping a pad plays a note derived from the current scale. In FX Mode (Keymasher), tapping buttons triggers audio manipulations.

The React UI must communicate these taps to the C++ engine so the engine can generate audio. But the Command Schema (§3.2) contains **no command for triggering a pad or note**. There is:
- `KEYMASHER_ACTION` — handles Keymasher buttons ✅
- `SET_KNOB` — adjusts parameters ✅  
- `SELECT_MODE` — changes the active instrument ✅
- But no `NOTE_ON`, `NOTE_OFF`, `PAD_TRIGGER`, or equivalent 

**Why this is critical:** Without this command, the agent building the pad grid UI (Task 5.2) cannot wire taps to the engine. The agent building the engine synths (Tasks 4.4, 4.5) has no trigger mechanism. The retrospective buffer (§3.10) captures "the output of the currently selected instrument/mode" — but without pad triggers, the instrument produces no output, so the buffer captures silence.

This is the **single most important interaction** in the entire application — playing sounds — and it has no protocol.

**What's needed (minimum):**
```typescript
// Note-based instruments (Notes, Bass)
| { cmd: 'NOTE_ON'; note: number; velocity: number }     // MIDI-style: note=0-127, velocity=0.0-1.0
| { cmd: 'NOTE_OFF'; note: number }                       // Release the note

// Drum mode (alternative: use NOTE_ON with pad-to-note mapping)
| { cmd: 'PAD_TRIGGER'; padIndex: number; velocity: number }  // padIndex: 0-15
```

**Design considerations:**
- **Velocity sensitivity:** Touch screens can provide pressure or touch area. Should pads be velocity-sensitive? If yes, the command needs a `velocity` field.
- **Note-off for sustained sounds:** Pads like "Warm Pad" or "Organ" (no envelope release) need an explicit note-off. Without it, the sound plays forever.
- **MIDI input equivalence:** §7.6.8 Tab C defines MIDI input settings. External MIDI keyboards send NOTE_ON/NOTE_OFF. The internal pad grid should produce equivalent messages for the engine. Using MIDI-style note numbers (0-127) keeps them interchangeable.
- **Audio thread safety:** These commands arrive at high frequency during performance (potentially dozens per second). They travel through the `CommandQueue` (lock-free SPSC FIFO), which is the correct path. But the `CommandDispatcher` must handle them efficiently — no JSON parsing overhead per note.

> **Recommendation:** Add `NOTE_ON` and `NOTE_OFF` commands to §3.2. Add them to the error matrix (§3.3) — both "always succeeds." Define in §3.10 or §7.6.2 that pad taps produce NOTE_ON with pad-to-MIDI-note mapping (using the scale/root/transpose logic already defined in §7.6.2). Add NOTE_OFF on finger lift for sustained modes. Drums can ignore NOTE_OFF (one-shot). Consider whether binary encoding (not JSON) is needed for these high-frequency commands — a 4-byte binary message (cmd_id + note + velocity) would be significantly faster than JSON parsing on the audio thread path.

---

#### C2: `SET_LOOP_LENGTH` — Protocol vs Behavioral Mismatch (Capture Trigger)

**The conflict:** Two sections describe fundamentally different behaviors for loop length interaction:

**Protocol definition (§3.2):**
```typescript
| { cmd: 'SET_LOOP_LENGTH'; bars: number }  // 1, 2, 4, or 8
```
This reads as a pure state-setter — "change the loop length setting." The error matrix (§3.3) confirms: `SET_LOOP_LENGTH → 2050: INVALID_LOOP_LENGTH → Revert to current length.`

**Behavioral description (§3.10 step 3 + §7.6.1):**
> "When the user Taps a loop length button (1, 2, 4, or 8 Bars) in the Mode or Play tabs, the most recent N bars of audio (per the tapped length) are copied from the retrospective buffer into the next available slot."
> 
> "**Dual Function:** Serves as both length selector and 'Capture' trigger."

So `SET_LOOP_LENGTH` is actually doing **two operations**: (1) set the loop length, AND (2) immediately capture audio from the retrospective buffer into the next slot. But the command schema only describes operation (1).

**Why this is critical:** An agent implementing `handleSetLoopLength()` in the CommandDispatcher will read §3.2 and implement a simple value setter. The capture behavior is only described in §3.10 and §7.6.1 — sections an engine agent may not read. The result: loop length changes work, but no audio ever gets captured.

Even if the agent finds §3.10, the dual-purpose design is fragile: What if the user just wants to **change** the loop length without capturing? For example, they might want to set the length to 4 bars before playing, without committing silence from the empty retro buffer. The current design would commit silence to a slot every time the loop length is adjusted.

**Options:**

**(A) Explicit dual-command approach (recommended for reliability):**
```typescript
| { cmd: 'SET_LOOP_LENGTH'; bars: number }                 // Pure setter — only changes the length
| { cmd: 'CAPTURE_LOOP'; bars: number }                    // Capture N bars from retro buffer → next slot
```
The UI sends `CAPTURE_LOOP` when the user taps a loop length button. `SET_LOOP_LENGTH` is available if loop length needs to be set independently (e.g., from settings or init). This makes the two operations independent and testable.

**(B) Keep dual-purpose but document explicitly:**
Update §3.2 comment for `SET_LOOP_LENGTH` to: *"Sets the loop length AND immediately captures that duration from the retrospective buffer into the next empty slot. This is the primary recording mechanism — see §3.10."*

> **Recommendation:** Option A is more reliable for a non-engineer owner. Separate commands are independently testable, have clear responsibilities, and avoid the edge case of accidentally capturing silence when the user just wants to adjust the length setting. If Option B is preferred, the §3.2 comment MUST explicitly describe the capture behavior, and `handleSetLoopLength()` in §6.1 should be updated to show it triggers capture.

---

### 🟠 HIGH — Will Cause Agent Confusion or Subtle Bugs

#### H1: `barPhase` Broadcast Rate — Performance Time Bomb

§3.4 `AppState.transport.barPhase` is a `number` (0.0–1.0) that represents the current position within the bar. During playback, this changes continuously (essentially every audio callback at ~93Hz for 512-sample buffer at 48kHz).

**The problem:** How is `barPhase` communicated to the React UI?

- **Via JSON state patches (STATE_PATCH)?** At 30fps, this means a JSON patch operation (`{ op: 'replace', path: '/transport/barPhase', value: 0.xxxx }`) is generated and sent 30 times per second. That's 30 patch operations per second just for one number — it would dominate the patch stream and trigger constant React re-renders of every component subscribed to transport state.
- **Via the binary visualization stream (§3.7)?** The binary payload definition doesn't include `barPhase`. It only has RMS levels and spectrum data.
- **Calculated client-side?** The client could calculate barPhase locally from BPM and elapsed time. But drift would accumulate without periodic sync from the server.

This isn't defined anywhere in the spec. An agent will either:
- Include it in every state broadcast (causing performance problems), or
- Exclude it (breaking the UI's bar phase animation), or
- Guess at a solution (likely wrong)

> **Recommendation:** Define one of these strategies in §3.7 or §3.8:
> - **(A) Client-calculated with sync:** Client computes `barPhase` locally from `bpm` and a `barPhaseAtLastSync` + `syncTimestamp` sent in `STATE_FULL` and periodically in patches (e.g., every 2 seconds). This keeps the patch stream clean while maintaining accuracy.
> - **(B) Included in binary stream:** Add `barPhase` to the binary visualization payload header. It's already sent at up to 30fps and is a single float32.
> - **(C) Exclude from AppState, add to binary stream only:** Remove `barPhase` from the JSON `AppState` interface to clarify it's not patch-delivered.
>
> Option (B) is simplest and most reliable.

---

#### H2: `COMMIT_RIFF` from Mixer — What Audio Does It Capture?

§7.6.5 (Mixer Tab) says:
> "**Commit Mix** (checkmark icon) — Action: Uses `COMMIT_RIFF` command to save the current volume/pan/mute state as a new Riff History entry."

§3.2 `COMMIT_RIFF` comment says:
> "Capture audio from retrospective buffer into the next empty slot, then save the resulting session state as a new entry in riff history."

**The conflict:** The Mixer's Commit Mix button is meant for saving *mix state* (volume, pan, mute adjustments). But the `COMMIT_RIFF` command also captures audio from the retro buffer. If all 8 slots are full (user has 8 layers already playing, and is just adjusting the mix), `COMMIT_RIFF` would try to capture audio into... where? There's no empty slot. Does auto-merge fire? That would be destructive — the user just wanted to save their fader positions.

**Scenarios to resolve:**

| Scenario | User Intent | What Happens? |
|:---|:---|:---|
| Mixer commit with empty retro buffer | Save mix state | Does it commit silence to a slot? Or just save riff history? |
| Mixer commit with non-empty retro buffer | Save mix state | Does it commit unwanted audio to a slot? |
| Mixer commit with all 8 slots full | Save mix state | Does auto-merge fire? That would be catastrophic. |
| Mixer commit with some empty slots | Save mix state | Does it write retro buffer audio to an empty slot? |

> **Recommendation:** `COMMIT_RIFF` from the Mixer should be a **state-only snapshot** — it saves the current session state (slot configurations, volumes, pans, mutes) as a riff history entry WITHOUT capturing audio from the retrospective buffer. The retro buffer capture should ONLY happen via loop length button taps (§3.10). Update §3.2 `COMMIT_RIFF` comment to clarify: *"Save the current session state as a new entry in riff history. Does NOT capture audio from the retrospective buffer — use loop length buttons for audio capture. In FX Mode, captures FX-processed audio and deletes source slots (see §7.6.2 FX Mode)."*

---

#### H3: WebSocket Port Number Undefined

The spec describes a CivetWeb-based WebSocket server and an HTTP health endpoint, but never specifies what port they listen on. Phase 0's agent needs this immediately.

- §2.1 component diagram shows `CivetWeb - Background Thread` but no port.
- §5.4 health endpoint shows `GET /api/health` but no host:port.
- §9 Task 0.1 says "WebSocket handshake succeeds" but no port.
- §2.2.L client connects with `{ clientId: <UUID> }` but no URL.

> **Recommendation:** Add a "Network Configuration" note to §2.2 or §3.1:
> - Default WebSocket + HTTP port: `8765` (or another specific value)
> - Configurable via `config.json` field `"serverPort": 8765`
> - React dev mode connects to `ws://localhost:8765`
> - Health endpoint at `http://localhost:8765/api/health`

---

#### H4: Drum Parameter Scope Ambiguity — `SET_KNOB { param: 'pitch' }` Per-Pad or Global?

Audio_Engine_Specifications.md §4.3 says:
> "**Pitch** → Tune individual pad (-12 to +12 semitones)"

But the `SET_KNOB` command (§3.2) is:
```typescript
| { cmd: 'SET_KNOB'; param: KnobParameter; val: number }
```

There's no `padIndex` or `slot` field. In Notes/Bass mode, Pitch transposes the entire grid (§7.6.2 confirms: "The Pitch knob transposes the entire grid ±24 semitones"). But in Drums mode, the Audio Engine Spec says it tunes an *individual* pad.

**The conflict:** The same command can't simultaneously mean "transpose entire grid" (Notes/Bass) and "tune individual pad" (Drums) without additional context. For Drums, the engine needs to know *which* pad to tune. But `SET_KNOB` has no pad identifier.

**Options:**
- **(A)** In Drum mode, Pitch also tunes the entire kit (all 16 pads). Change Audio Engine Spec §4.3 to match.
- **(B)** Add an optional `padIndex` field to `SET_KNOB`: `{ cmd: 'SET_KNOB'; param: KnobParameter; val: number; padIndex?: number }`. When present, applies to that pad only.
- **(C)** In Drum mode, Pitch applies to the "last-triggered pad." The engine tracks which pad was most recently played and applies Pitch adjustments to it. Simple but potentially confusing.

> **Recommendation:** Option (A) is simplest and keeps the command schema clean. Individual pad tuning can be a V2 feature. Update Audio Engine Spec §4.3: "Pitch → Tune all pads globally (-12 to +12 semitones)."

---

### 🟡 MEDIUM — Should Fix Before Beads

#### M1: `GET_METRICS_HISTORY` Referenced But Not in Command Schema

§5.3 Telemetry & Metrics says:
> "UI can request `GET_METRICS_HISTORY` to render a performance graph."

This command is not in the Command Schema (§3.2), not in the Error Matrix (§3.3), and has no server response type defined. An agent building the developer overlay (Task 8.7) will need this.

> **Recommendation:** Either add `GET_METRICS_HISTORY` to §3.2 with a response type, or remove the reference from §5.3 and note it as Phase 8 scope that will be defined when that phase begins.

---

#### M2: `revId` vs `revisionId` — Inconsistent Field Naming

In the server response (§3.2):
```typescript
| { type: 'STATE_PATCH'; ops: JsonPatchOp[]; revId: number }
```

In the AppState (§3.4):
```typescript
meta: { revisionId: number; ... }
```

In the reconnect command (§3.2):
```typescript
| { cmd: 'WS_RECONNECT'; revisionId: number; clientId: string }
```

The STATE_PATCH uses `revId` while everything else uses `revisionId`. A client implementation will receive `revId` in patches but need to send `revisionId` in reconnect — requiring a field name translation that's easy to get wrong.

> **Recommendation:** Change STATE_PATCH to use `revisionId` for consistency:
> ```typescript
> | { type: 'STATE_PATCH'; ops: JsonPatchOp[]; revisionId: number }
> ```

---

#### M3: Binary Visualization Stream ACK Format Not Defined

§3.7 says:
> "Client sends a **4-byte ACK** after *every received frame*."

But the format of this 4-byte ACK is never defined. What do the 4 bytes contain? Possibilities:
- The `FrameId` from the received frame (echo back for confirmation)
- A fixed magic number (just a "got it" signal)
- A `pendingFrameCount` from the client's perspective

Without this, the C++ server and the TypeScript client will implement different ACK formats.

> **Recommendation:** Add to §3.7: *"ACK format: 4 bytes containing the `FrameId` (uint32, Little Endian) of the received frame. This allows the server to track which frames have been acknowledged."*

---

#### M4: `DELETE_JAM` Interaction with Garbage Collection Policy

§3.2 defines `DELETE_JAM { sessionId }`. §7.7.1 says: "Remove the jam and its associated audio history."

But §2.2.G (SessionStateManager) says: "Audio files are **never deleted immediately** — only marked for garbage collection on clean exit."

**Questions:**
1. Does `DELETE_JAM` immediately delete audio files? Or mark them for GC? The GC policy is stated as an absolute rule, so presumably GC.
2. What happens if the user deletes a jam and then quits before GC runs? Are the files orphaned?
3. Is the riff history JSON deleted immediately but audio files deferred?

> **Recommendation:** Add a note to §2.2.G or §7.7: *"`DELETE_JAM` immediately removes the session metadata and riff history entries. Associated audio files are marked for garbage collection and deleted on next clean exit. If storage reclamation is urgent, the user can quit and relaunch."*

---

### 🟢 LOW — Worth Noting

#### L1: No Explicit Time Signature in Protocol

§7.6.7 (Riff History View) shows: "**Metadata:** Time signature and BPM (e.g., '4/4 120.00 BPM')."

But `AppState` (§3.4) has no `timeSignature` field. The transport section has `bpm`, `loopLengthBars`, `barPhase` — but no time signature. The UI example shows "4/4" but there's no way to set or read it.

This is likely fine for V1 (hardcode 4/4), but an agent building the Riff History View needs to know where "4/4" comes from.

> **Recommendation:** Add a brief note: *"V1 uses 4/4 time signature exclusively. The '4/4' display in Riff History View is hardcoded. Time signature selection is a potential V2 feature."*

---

#### L2: Adjust Tab — Microphone Mode Knobs vs Standard Knobs Not in `KnobParameter` Scope

§7.6.4 says in Microphone mode, the Adjust tab shows: "2 knobs only — **Reverb Mix** and **Room Size**."

These parameters (`reverb_mix`, `room_size`) are already in the `KnobParameter` type. But when the user adjusts Reverb Mix in Mic mode, it controls the mic's built-in Freeverb instance (Audio Engine Spec §6.3). When adjusted in Notes/Bass mode, it controls the instrument's reverb send.

The engine must route `SET_KNOB { param: 'reverb_mix' }` to different targets depending on `activeMode.category`. This is implicit context-dependence.

> **Recommendation:** Add a note to §3.2 or §6.1: *"`SET_KNOB` parameters are routed based on the current `activeMode.category`. For example, `reverb_mix` in Microphone mode adjusts the mic's built-in reverb, while in Notes mode it adjusts the synth's reverb send. The `CommandDispatcher` must check `activeMode.category` to route correctly."*

---

#### L3: Slot `instrumentCategory` — Value for Mic Input Recordings

§3.4 defines `slot.instrumentCategory: string` — "What produced this slot's audio." And `slot.presetId: string` — "Which preset was used."

When a user records from the microphone, what values go into these fields? The mic isn't an "instrument" in the preset sense. Suggested values aren't defined.

> **Recommendation:** Add to §3.4 or §3.10: *"For microphone recordings: `instrumentCategory: 'mic'`, `presetId: 'mic_input'`. For auto-merge results: `instrumentCategory: 'merge'`, `presetId: 'auto_merge'` (already defined in §7.6.2.1 step 4)."*

---

## 4. Cross-Document Consistency Matrix (Full Recheck)

| Aspect | Spec 1.6 | Audio Engine Spec | UI Layout Ref | Status |
|:---|:---|:---|:---|:---|
| **Tab labels** | Mode, Play, Adjust, Mixer | "Play Tab" (§1 note) | Mode, Play, Adjust, Mixer | ✅ Fixed from PBA4 |
| **Core FX count & names** | 12 (§2.2.M) | 12 (§1.1) | 12 (play_tab) | ✅ All match |
| **Infinite FX count & names** | 11 (§2.2.M) | 11 (§1.2) | 11 (play_tab) | ✅ All match |
| **Keymasher buttons** | 12 buttons (§3.2) | 12 buttons (§1.2) | 12 buttons (play_tab) | ✅ All match |
| **Notes presets** | 12 (§2.2.M) | 12 named (§2.1) | "See Audio_Engine_Specs" | ✅ |
| **Bass presets** | 12 (§2.2.M) | 12 named (§3.1) | "See Audio_Engine_Specs" | ✅ |
| **Drum kits** | 4 kits (§2.2.M) | 4 kits (§4.2) | Pad grid icons | ✅ |
| **Drum pad icons → sounds** | Icons listed (§7.6.2) | Icon-to-sound map (§4.1) | Icons match (pad_grid) | ✅ |
| **XY Pad behavior** | Touch-and-hold (§7.6.3) | XY mapping per effect | Crosshair on hold | ✅ |
| **Adjust knobs** | 7 + 2 reverb (§7.6.4) | Parameter mapping (§2.2) | 7 + reverb section | ✅ |
| **Mic mode knobs** | Reverb Mix + Room Size + Gain (§7.6.4) | Built-in reverb (§6.3) | Monitor + Gain | ✅ |
| **Mixer transport grid** | 2×3 (§7.6.5) | N/A | 2×3 controls | ✅ |
| **Settings tabs** | 4 tabs (§7.6.8) | N/A | 4 tabs | ✅ |
| **Slot states** | `EMPTY \| PLAYING \| MUTED` | N/A | N/A | ✅ |
| **Retro buffer size** | §1.1: ~96s, §3.10: ~96s | N/A | N/A | ✅ Fixed from PBA4 |
| **Phase structure** | §8.6: 9 phases (0-8), §9: 9 phases (0-8) | N/A | N/A | ✅ Fixed from PBA4 |
| **Binary stream** | Single WS connection (§3.7) | N/A | N/A | ✅ |
| **SampleEngine** | V2 in §1.3, §2.2.M | V2 in §5 header | Reserved slot in grid | ✅ Fixed from PBA4 |
| **Color palette** | §7.1 hex codes match §7.3 JSON | N/A | Not duplicated (references §7.1) | ✅ |
| **Riff Swap Mode** | In settings (§7.6.8) | N/A | `user_preferences` section | ✅ |
| **`PluginInstance` interface** | Defined (§3.4) | N/A | N/A | ✅ |
| **`JsonPatchOp` type** | Defined (§3.2) | N/A | N/A | ✅ |
| **Scale type** | `Scale` type with 8 values (§3.2) | N/A | N/A | ✅ |
| **`revId` vs `revisionId`** | STATE_PATCH uses `revId`, rest uses `revisionId` | N/A | N/A | ⚠️ M2 |
| **NOTE_ON / pad trigger** | Not in §3.2 | Pads described in §4.1 | Pad grid defined | ❌ C1 |
| **Loop capture mechanism** | §3.2: value setter only, §3.10: capture trigger | N/A | Loop controls (§timeline) | ❌ C2 |
| **`barPhase` broadcast** | In AppState but no broadcast strategy | N/A | N/A | ⚠️ H1 |
| **Port number** | Nowhere specified | N/A | N/A | ⚠️ H3 |
| **`GET_METRICS_HISTORY`** | Referenced §5.3, not in §3.2 | N/A | N/A | ⚠️ M1 |

---

## 5. Reliability Risk Assessment

### R1: High-Frequency Commands on CommandQueue (NOTE_ON Performance)

Once C1 is resolved and `NOTE_ON`/`NOTE_OFF` commands exist, they'll be the highest-frequency messages in the system. A user drumming on 4 pads simultaneously can generate 4 NOTE_ON + 4 NOTE_OFF messages within milliseconds. At rapid pad tapping speeds (8-16 taps per second per finger), a two-handed player could generate 60+ command messages per second.

The `CommandQueue` (§2.2.C) is a lock-free SPSC FIFO. It should handle this throughput. But the `CommandDispatcher` (§2.2.D) currently "validates all required fields" — if validation involves JSON parsing for each NOTE_ON, latency may be perceptible.

> **Recommendation:** Add a note to §2.2.C or §2.2.D: *"Performance-critical commands (NOTE_ON, NOTE_OFF, SET_XY_PAD) should use a compact binary encoding for the CommandQueue payload rather than JSON strings. Consider a fixed-size struct: `{ uint8_t cmdType; uint8_t note; uint8_t velocity; uint8_t padding; }` = 4 bytes. JSON encoding is reserved for complex/infrequent commands."*

### R2: Auto-Merge Latency During Live Performance

§7.6.2.1 describes auto-merge when all 8 slots are full and the user taps a loop length button to record a 9th. The merge must:
1. Read audio from all 8 slots
2. Sum them to one stereo buffer (respecting volume/pan)
3. Write the merged audio to disk
4. Clear slots 2-8
5. Auto-commit to riff history
6. THEN capture the new recording into Slot 2

During a live performance, the user expects instant response from the loop length button tap. Steps 1-5 above could take 50-500ms depending on loop length and disk speed. This latency is perceptible and could break flow.

> **Recommendation:** Add an implementation note to §7.6.2.1: *"The auto-merge operation must be non-blocking from the audio thread's perspective. During merge, the audio keeps playing (the 8 existing loops continue). The merge computation happens on a background thread. Once complete, the slot states are atomically swapped on the next audio callback. The retrospective buffer continues capturing during the merge, so no audio is lost — the capture just begins from the next available slot once the merge finalizes."*

### R3: FX Mode — No Way to Preview Without Committing

In FX Mode (§7.6.2), the user:
1. Selects source slots
2. Selects an effect
3. Holds the XY pad to apply the effect in real-time
4. Taps a loop length button to capture the result

Step 3 means the user *hears* the FX-processed audio while performing (it's routed to audio output). But Step 4 is destructive — it deletes the source slots. There's no "undo" if the captured result sounds wrong.

The spec addresses this: "the currently playing riff (the pre-FX state) is already the latest entry in Riff History." So the user can load the previous riff to recover.

**However:** Between the user making their initial layers and entering FX mode, they may have adjusted volumes/pans in the Mixer. Those mix adjustments are part of the current state but NOT automatically committed to riff history. If the user enters FX mode without first doing a Mixer Commit, and the FX result is bad, loading the previous riff restores the layers but loses the mix adjustments.

> **Recommendation:** Add to §7.6.2 FX Mode: *"When entering FX Mode, the engine automatically commits the current state to riff history (if it differs from the latest entry). This ensures all mix adjustments are preserved before any destructive FX operation."*

### R4: Config Migration Failure Cascades

§4.5 says: "If migration fails → load factory defaults. Archive old config as `config.legacy.json`."

But what if the filesystem prevents writing `config.legacy.json` (permissions, disk full)? The migration failure handler itself fails. And what if factory defaults can't be loaded either (corrupted application resources)?

This is a defense-in-depth concern. The current spec has two layers (backup → factory defaults). A third layer (hardcoded minimum config compiled into the binary) would make the app truly resilient.

> **Recommendation:** Add to §4.5: *"If factory default loading also fails, the engine initializes with a hardcoded minimum configuration compiled into the binary (44.1kHz, 512 buffer, no plugins, no session auto-load). This is the absolute fallback and cannot fail."*

---

## 6. Summary of All Findings

### 🔴 Critical — Must Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| C1 | No command for triggering pad sounds (NOTE_ON/PAD_TRIGGER missing) | Add NOTE_ON, NOTE_OFF to §3.2. Add to error matrix. Define pad-to-note routing. | 20 min |
| C2 | `SET_LOOP_LENGTH` doesn't trigger audio capture in protocol | Split into SET_LOOP_LENGTH (setter) + CAPTURE_LOOP (capture trigger), OR update SET_LOOP_LENGTH comment to describe dual-purpose | 15 min |

### 🟠 High — Should Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| H1 | `barPhase` broadcast rate undefined | Define broadcast strategy (binary stream recommended) | 10 min |
| H2 | `COMMIT_RIFF` from Mixer — captures audio or just saves state? | Clarify as state-only snapshot (no retro buffer capture) | 10 min |
| H3 | WebSocket port number not specified | Add default port (e.g., 8765) to §2.2 or §3.1 | 5 min |
| H4 | Drum `SET_KNOB { pitch }` — per-pad or global? | Standardize as global for V1. Update Audio Engine Spec §4.3. | 5 min |

### 🟡 Medium — Should Fix

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| M1 | `GET_METRICS_HISTORY` referenced but not in schema | Add to §3.2 or defer to Phase 8 scope note | 5 min |
| M2 | `revId` vs `revisionId` naming inconsistency | Standardize to `revisionId` in STATE_PATCH | 2 min |
| M3 | Binary viz stream ACK format undefined | Define ACK = FrameId echo (uint32 LE) | 5 min |
| M4 | `DELETE_JAM` vs garbage collection policy interaction | Add note clarifying metadata deletion vs audio GC | 5 min |

### 🟢 Low — Nice to Have

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| L1 | No time signature in AppState (hardcoded 4/4) | Add note confirming 4/4 is V1-only | 2 min |
| L2 | `SET_KNOB` routing depends on active mode (implicit context) | Add routing note to §3.2 or §6.1 | 5 min |
| L3 | Mic recordings — slot `instrumentCategory` value undefined | Define as `'mic'` / `'mic_input'` | 2 min |

### Reliability Risks

| # | Risk | Action |
|:--|:-----|:-------|
| R1 | NOTE_ON high-frequency commands → potential latency | Consider binary encoding for performance-critical commands |
| R2 | Auto-merge latency during live performance | Document non-blocking merge requirement |
| R3 | FX Mode doesn't auto-save mix state before destructive op | Auto-commit on FX Mode entry |
| R4 | Config migration failure cascade | Add hardcoded minimum fallback config |

---

## 7. Overall Assessment

| Dimension | Rating | Change from PBA4 | Notes |
|:---|:---|:---|:---|
| **Completeness** | 🟢 **97%** | ↑ 9% (post-fix) | All critical protocol gaps (NOTE_ON, SET_LOOP_LENGTH capture) now defined. Core workflow fully specified. |
| **Internal Consistency** | 🟢 **98%** | ↑ 3% (post-fix) | `revId`→`revisionId` fixed. barPhase exclusively in binary stream. COMMIT_RIFF behavior clarified. |
| **Agent Buildability** | 🟢 **95%** | ↑ 10% (post-fix) | Pad triggering, loop capture, binary stream ACK, port number — all defined. Agents have unambiguous guidance for every interaction. |
| **Reliability for Non-Engineer** | 🟢 **93%** | ↑ 5% (post-fix) | Hardcoded config fallback added. Non-blocking merge documented. FX Mode auto-commits before destructive ops. GC policy clarified. |

**Verdict:** After applying all resolutions, the spec is **ready for bead creation**. Every user interaction has a defined command, every command has error handling, and all cross-document references are consistent. The project can proceed with high confidence.

---

## Appendix: Resolutions Applied

All findings from this assessment have been resolved in the spec documents per owner decisions. Summary of changes:

| # | Issue | Owner Decision | Resolution | Files Modified |
|:--|:------|:---------------|:-----------|:---------------|
| **C1** | No command for pad sounds | Add NOTE_ON (no velocity), NOTE_OFF. MIDI equivalence. | Added `NOTE_ON { note }` and `NOTE_OFF { note }` to Command Schema §3.2. Added to Error Matrix §3.3. Updated §3.10 to reference NOTE_ON as the way sounds enter the retrospective buffer. | `Spec 1.6` §3.2, §3.3, §3.10 |
| **C2** | SET_LOOP_LENGTH doesn't trigger capture | Always dual-purpose — no setting without committing | Updated §3.2 comment to explicitly describe dual-purpose (sets length AND captures). Updated §3.3 error matrix to include `4010: NOTHING_TO_COMMIT`. Rewrote §3.10 steps 1-2 to clarify SET_LOOP_LENGTH is the primary recording command. | `Spec 1.6` §3.2, §3.3, §3.10 |
| **H1** | barPhase broadcast rate undefined | Include in binary visualization stream | Added `BarPhase` (Float32) to binary payload §3.7. Added note that barPhase is NOT broadcast via JSON STATE_PATCH. | `Spec 1.6` §3.7 |
| **H2** | COMMIT_RIFF captures audio or just state? | State-only snapshot. Button only visible on mix changes. | Updated §3.2 comment: "Does NOT capture audio from retrospective buffer." Updated §3.3: button only visible when mix state changed. Updated §7.6.5: visibility conditional on mix changes. Updated §3.10 Mixer Tab Commit note. | `Spec 1.6` §3.2, §3.3, §3.10, §7.6.5 |
| **H3** | WebSocket port undefined | Default 8765, configurable | Added Network Configuration block to §2.2.L with default port 8765, config field, and URL patterns. | `Spec 1.6` §2.2.L |
| **H4** | Drum SET_KNOB pitch — per-pad or global? | Global for V1 | Updated Audio Engine Spec §4.3: "Tune all pads globally." Per-pad tuning deferred to V2. | `Audio_Engine_Specifications.md` §4.3 |
| **M1** | GET_METRICS_HISTORY not in schema | Defer to Phase 8 scope | Updated §5.3 to note the command will be defined as part of Phase 8 (Task 8.7). | `Spec 1.6` §5.3 |
| **M2** | `revId` vs `revisionId` inconsistency | Standardize to `revisionId` | Changed STATE_PATCH type to use `revisionId`. | `Spec 1.6` §3.2 |
| **M3** | Binary viz ACK format undefined | FrameId echo (uint32 LE) | Updated §3.7 backpressure control: ACK = FrameId as uint32 Little Endian. | `Spec 1.6` §3.7 |
| **M4** | DELETE_JAM vs GC policy interaction | Metadata deleted immediately, audio files via GC | Added note to §2.2.G (SessionStateManager) clarifying the split. | `Spec 1.6` §2.2.G |
| **L1** | No time signature in protocol | V1 hardcodes 4/4 | Added note to §7.6.7 Riff Details confirming 4/4 is hardcoded for V1. | `Spec 1.6` §7.6.7 |
| **L2** | SET_KNOB routing depends on active mode | Add routing note | Updated §3.2 SET_KNOB comment to document mode-dependent routing. | `Spec 1.6` §3.2 |
| **L3** | Mic recordings slot instrumentCategory | Define as 'mic' / 'mic_input' | Updated §3.4 slot.instrumentCategory and slot.presetId comments with exhaustive value list. | `Spec 1.6` §3.4 |
| **R1** | NOTE_ON high-frequency command performance | Noted as implementation consideration | Documented in PBA5 §5 for agent awareness. Binary encoding consideration noted. | PBA5 only |
| **R2** | Auto-merge latency during live performance | Non-blocking merge requirement | Added Implementation Note to §7.6.2.1 documenting non-blocking background merge. | `Spec 1.6` §7.6.2.1 |
| **R3** | FX Mode doesn't auto-save before destructive op | Auto-commit on FX Mode entry | Updated §7.6.2 FX Mode step 1: engine auto-commits current state before entering FX Mode if state has changed. | `Spec 1.6` §7.6.2 |
| **R4** | Config migration failure cascade | Hardcoded minimum config fallback | Added step 4 to §4.5: hardcoded minimum configuration compiled into binary as absolute fallback. | `Spec 1.6` §4.5 |

---

*End of Pre-Build Assessment 5*
