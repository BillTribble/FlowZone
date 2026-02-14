# FlowZone — Pre-Build Assessment 8

**Date:** 14 Feb 2026  
**Scope:** Final reliability-focused review of all Spec folder documents. Focus on runtime logic bugs, data model gaps, and user-facing failure modes that would bite a non-engineer owner. Traces complete user journeys and data flows end-to-end.  
**Documents Reviewed:**
- `Spec_FlowZone_Looper1.6.md` (1758 lines)
- `Audio_Engine_Specifications.md` (366 lines)
- `UI_Layout_Reference.md` (367 lines)

---

## 1. Executive Summary

PBA7 found 13 issues (2 critical, 4 high, 4 medium, 3 low) and 4 reliability risks. **All 13 findings and all 4 reliability risks have been resolved in the current Spec 1.6** — verified in Section 2 below.

PBA8 takes yet another angle: **end-to-end logic tracing**. Rather than reading the spec section-by-section looking for textual inconsistencies (which are now largely resolved), this review traces complete user journeys through the data model, command schema, and engine architecture — step by step — looking for places where the spec's own rules produce unexpected or destructive outcomes at runtime.

**Found: 12 issues** (1 critical, 4 high, 4 medium, 3 low) and **3 reliability risks**.

**The critical theme this time:** The spec's text is now highly consistent and well-structured. The remaining issues are **logic-level** — sequences of operations that individually make sense but combine to produce edge-case bugs (e.g., FX Mode with all 8 slots selected), missing numerical constants (e.g., drum MIDI note mapping, pad base octave), and TypeScript interface errors (duplicate field, missing optional property).

**Overall verdict:** After eight rounds of review, the spec is in excellent shape. The macro architecture, protocol, UI layout, and risk mitigations are solid. The 12 findings below are "implementation landmines" — they won't block initial development but will cause subtle runtime bugs if not addressed. Resolving them before bead creation eliminates the last category of risk.

---

## 2. PBA7 Resolution Verification

All 13 PBA7 findings and 4 reliability risks have been verified as resolved in the current spec:

| PBA7 # | Issue | Resolution | Status |
|:---|:---|:---|:---|
| C1 | No session list in AppState for Jam Manager | `sessions` array added to AppState §3.4 (lines 440-445) | ✅ Resolved |
| C2 | Slot-level loop length not tracked | `loopLengthBars` + `originalBpm` added to slot state §3.4 (lines 489-490) | ✅ Resolved |
| H1 | MIDI input path says "same path" but SPSC is single-producer | Separate `MidiQueue` SPSC defined in §2.2.D CommandDispatcher note (lines 131-135) | ✅ Resolved |
| H2 | Retrospective buffer content ambiguous | "Capture Source (Clarification)" paragraph added to §3.10 (lines 594-595) | ✅ Resolved |
| H3 | `DELETE_JAM` description contradicts GC policy | DELETE_JAM in §3.2 (line 335) now references GC policy explicitly | ✅ Resolved |
| H4 | `protocolVersion` value never formally declared | `PROTOCOL_VERSION = 1` declared in §3.1 (line 278) and §3.4 meta (line 438) | ✅ Resolved |
| M1 | `RiffSnapshot` vs `riffHistory` mapping undefined | Mapping note added to §3.5 (lines 544-546) | ✅ Resolved |
| M2 | `config.json` file path not specified | Path declared in §4.1.1 (line 659) | ✅ Resolved |
| M3 | Auto-merge with mixed loop lengths undefined | Merge output length = longest slot, shorter slots repeat — §7.6.2.1 (line 1039) | ✅ Resolved |
| M4 | Bounce/Speed knobs in Drum mode behavior | §7.6.4 (line 1094): "Hidden (or rendered as disabled/blank)" | ✅ Resolved |
| L1 | Slot count never declared as constant | `MAX_SLOTS = 8` declared in §3.1 (line 276) | ✅ Resolved |
| L2 | `isVstMode` initialization step | Documented in §3.4 meta (line 436): "Set during FlowEngine construction via preprocessor" | ✅ Resolved |
| L3 | CivetWeb threading model | Threading Note added to §2.2.G (line 169) | ✅ Resolved |
| R1 | Audio device hot-unplug undefined | "Audio Device Disconnect" row added to §4.4 (line 691) | ✅ Resolved |
| R2 | Retro buffer at 20 BPM boundary | Confirmed as correct by design — no change needed | ✅ Non-issue |
| R3 | WebSocket reconnection retry limit undefined | Infinite retries + discarded input defined in §2.2.L step 6 (line 212) | ✅ Resolved |
| R4 | First-launch WebSocket race condition | Graceful initial failure handling defined in §2.2.L step 7 (line 213) | ✅ Resolved |

**Verdict:** Clean sweep. All PBA7 resolutions correctly applied, no regressions.

---

## 3. New Issues Found

### 🔴 CRITICAL — Will Cause Data Loss at Runtime

#### C1: FX Mode with All 8 Slots Selected — Destructive Commit Sequence

**The bug:** When all 8 slots are selected as FX sources and the user commits the resampled layer, the spec's operation ordering destroys all audio — including the newly captured FX output.

**Trace through §7.6.2:**

1. User selects all 8 slots as FX sources via `SELECT_FX_SOURCE_SLOTS`
2. User processes audio through effects using XY pad
3. User taps a Timeline Section (e.g., 4 Bars) → fires `SET_LOOP_LENGTH`

**§7.6.2 step 6-7 specifies this sequence:**

> *"the FX-processed audio is captured for that duration and written to the **next empty slot**. The source slots that were selected are then set to EMPTY state."*

> *"Auto-Merge: If all 8 slots are full when committing the resampled layer, the standard auto-merge rule applies first."*

**Step-by-step execution when all 8 are sources:**

1. **Find next empty slot** → None available (all 8 occupied as sources)
2. **Auto-merge fires** → Sums Slots 1-8 into Slot 1, clears Slots 2-8
3. **Write FX audio** → Written to Slot 2 (first empty after merge)
4. **Clear source slots [1-8]** → Clears Slot 1 (the merge result) AND Slot 2 (the just-captured FX audio)
5. **Result: All 8 slots empty. Total audio loss.**

The user loses everything in a single tap — the merged audio, the FX capture, and all original layers. This is the most dangerous edge case in the entire spec because it's a destructive action triggered by a normal user workflow (selecting all slots for FX processing is entirely reasonable).

**Why the spec's safety net doesn't apply:** The spec notes in step 6 that "the currently playing riff (the pre-FX state) is already the latest entry in Riff History — the user committed it when they originally recorded those layers." But this assumes the user committed a riff before entering FX mode. If they didn't (e.g., they just recorded 8 layers and immediately switched to FX), there's no riff history entry to recover from.

**Options:**

- **(A) Reverse the operation order:** Clear source slots *first*, then write FX audio to the first available slot (now Slot 1). Auto-merge is never triggered because slots are freed before the write occurs. Sequence becomes: (1) Clear selected sources → all 8 empty, (2) Write FX audio → Slot 1, (3) Result: Slot 1 = FX output, 2-8 empty. Simple and safe.
- **(B) Exclude the FX output slot from source clearing:** After writing FX audio to a slot, that slot is protected from the "clear source slots" step.
- **(C) Auto-commit a riff before entering FX mode:** The engine automatically commits the current state as a riff history entry when the user selects `category: 'fx'`. This guarantees a recovery point exists.

> **Recommendation:** Option **(A)** is the simplest and most robust. Reverse the order: clear sources first, then capture. Additionally, adopt **(C)** as a secondary safety net — auto-committing a riff on FX mode entry costs nothing and ensures a recovery point always exists. Add to §7.6.2:
> 
> *"On entering FX Mode (SELECT_MODE with category 'fx'), the engine auto-commits the current session state as a riff history entry if any slots are non-empty. This ensures a recovery point exists before destructive FX operations."*
> 
> *"On FX commit: (1) Selected source slots are set to EMPTY. (2) FX-processed audio is written to the first available slot (now freed). Auto-merge is not triggered during FX commit because source slots are cleared before the write."*

---

### 🟠 HIGH — Will Cause Agent Implementation Errors or Runtime Bugs

#### H1: Drum Pad MIDI Note Numbers — Unmapped

**The gap:** The `NOTE_ON` command (§3.2 line 309) sends a MIDI note number (`note: 0-127`). §7.6.3 defines a scale-based note mapping algorithm for Notes/Bass modes. But for Drums, it says: "Drums use fixed pad-to-note mapping (§4.1 Audio Engine Spec)."

Audio Engine Spec §4.1 defines 16 drum sounds by grid position (Row, Col) — but **never assigns MIDI note numbers**. The table maps `(1,1) → Kick 1`, `(3,3) → Snare 1`, etc. — but doesn't say "Kick 1 = MIDI note 36."

**Why this blocks implementation:** The React UI must convert a pad tap at grid position (row, col) into a `NOTE_ON { note: X }`. The C++ engine must decode that `X` back to the correct drum sound. Without a defined mapping, agent A might use `note = (row-1)*4 + (col-1)` (0-15) while agent B uses the General MIDI drum map (note 36-51). The result: pads trigger wrong sounds.

**Options:**

- **(A) Simple index mapping:** `note = (row-1) * 4 + (col-1)` → 0-15. Simple, but doesn't match any MIDI standard.
- **(B) General MIDI drum map offset:** `note = 35 + ((row-1) * 4 + (col-1))` → 36-51. Matches GM mapping (Kick = 36, Snare = 38, etc.) approximately.
- **(C) Explicit lookup table:** Define the exact MIDI note for each pad in Audio Engine §4.1. Most precise and future-proof.

> **Recommendation:** Option **(C)**. Add a `MIDI Note` column to Audio Engine §4.1's pad layout table. Use a simple linear mapping starting at note 36 (C2, matching General MIDI Kick):
> 
> | Pad (Row, Col) | Sound | MIDI Note |
> |:---|:---|:---|
> | (1,1) | Kick 1 | 36 |
> | (1,2) | Tom Low | 37 |
> | ... | ... | ... |
> | (4,4) | Ride | 51 |
> 
> Also add to §7.6.3: *"For Drums mode, pad-to-note mapping is a fixed lookup table (Audio Engine §4.1), not the scale-based algorithm."*

---

#### H2: Notes/Bass Pad Base Octave Not Specified — Risk of Inaudible Notes

**The gap:** §7.6.3 defines the pad-to-note mapping algorithm:

> *"padIndex (0-15) maps to rootNote + scaleDegree(padIndex)"*

`rootNote` is defined as `0-11 (C=0)` in `transport.rootNote` (§3.4 line 459). If `rootNote = 0` (C) and the scale is chromatic, then:
- Pad 0 = note 0 → MIDI note 0 → C-1 (8.18 Hz — **below audible range**)
- Pad 15 = note 15 → MIDI note 15 → D#0 (19.4 Hz — barely audible subsonics)

This produces notes that are physically inaudible through most speakers. The algorithm clearly needs a base octave offset (e.g., `baseNote = 48 + rootNote` for C3, or `baseNote = 60 + rootNote` for middle C), but none is specified.

**Why this matters for reliability:** If an agent implements the algorithm as written (`rootNote + scaleDegree`), every synth preset will produce inaudible or extremely low notes. The user taps a pad and hears nothing — they'll assume the app is broken.

> **Recommendation:** Add to §7.6.3 pad-to-note mapping:
> 
> *"The base MIDI octave for the pad grid is C3 (MIDI note 48). The full mapping formula is: `midiNote = 48 + rootNote + scaleDegreeOffset(padIndex)`, where `scaleDegreeOffset` returns the semitone offset for the Nth degree of the selected scale. With root C (0) and chromatic scale: pad 0 = MIDI 48 (C3), pad 15 = MIDI 63 (D#4). With root C and minor pentatonic: pad 0 = MIDI 48 (C3), pad 4 = MIDI 58 (A#3), pad 5 = MIDI 60 (C4, octave wrap)."*
> 
> **Also add the Pitch knob interaction:** *"The Pitch knob (Adjust tab) transposes the entire grid by ±24 semitones from this baseline. If the user sets Pitch to +12, pad 0 = MIDI 60 (C4)."*

---

#### H3: `COMMIT_RIFF` Description References FX Mode Behavior — Contradicts §7.6.2

**The contradiction:** §3.2 `COMMIT_RIFF` (line 329) says:

> *"In FX Mode, captures FX-processed audio and deletes source slots (see §7.6.2 FX Mode)."*

But §7.6.2 (FX Mode workflow) step 6 explicitly states:

> *"Commit Resampled Layer: When the user taps a **Timeline Section** (1, 2, 4, or 8 Bars), the FX-processed audio is captured..."*

The Timeline Section fires `SET_LOOP_LENGTH`, **not** `COMMIT_RIFF`. The FX capture mechanism is `SET_LOOP_LENGTH` throughout §7.6.2:

> *"SET_LOOP_LENGTH in FX Mode captures the FX-processed audio and triggers the resample-and-replace behavior"*

Meanwhile `COMMIT_RIFF` is defined in §7.6.5 as the Mixer tab's "Commit Mix" button — which saves volume/mute state as a riff history entry. These are two completely different actions.

**Risk:** An implementing agent reads both definitions and wires `COMMIT_RIFF` to do audio capture in FX mode — creating a command that does radically different things depending on mode, making the system harder to test and debug.

> **Recommendation:** Remove the FX Mode sentence from the `COMMIT_RIFF` command definition in §3.2. It should read:
> 
> *"Save the current session state (volumes, mutes) as a new entry in riff history. Does NOT capture audio from the retrospective buffer — use loop length buttons (SET_LOOP_LENGTH) for audio capture. Only available from the Mixer tab, and only enabled when the user has changed mix levels or mute states since the last commit."*
> 
> The FX capture behavior is already correctly documented in §7.6.2 under `SET_LOOP_LENGTH`.

---

#### H4: `TOGGLE_QUANTISE` — What Does It Actually Quantize?

**The gap:** §3.2 (line 300) defines `TOGGLE_QUANTISE` with the comment "Toggle quantise on/off." §3.4 (line 461) defines `quantiseEnabled: boolean` as "Whether input is quantised to grid."

Neither specifies what "quantise" means in this context. For a looping workstation, there are at least four plausible interpretations:

1. **Note input timing** — MIDI notes and pad taps are snapped to the nearest beat/subdivision (like a DAW's input quantize)
2. **Loop capture timing** — `SET_LOOP_LENGTH` commits are aligned to the nearest bar boundary (the retrospective buffer capture window snaps to bar lines)
3. **Transport actions** — Play/Pause snap to beat boundaries
4. **Riff swaps** — Loading riffs from history waits for the next bar (but this is already handled separately by `riffSwapMode`)

For a "flow machine" workstation, interpretation **(2)** is the most likely and useful — it ensures captured loops always start and end on bar boundaries, preventing timing drift when loops are layered. But this is just a guess.

**Why this matters:** Different implementations of quantise will produce fundamentally different user experiences. If an agent implements note-level quantise (option 1) but the designer intended loop-capture quantise (option 2), the feature will feel wrong and be hard to diagnose.

> **Recommendation:** Add a clarification note to `TOGGLE_QUANTISE` in §3.2:
> 
> *"When quantise is enabled, loop capture (SET_LOOP_LENGTH) snaps the retrospective buffer capture window to the nearest bar boundary. This ensures captured loops always align to the transport grid, preventing timing drift when layering. Note input timing is NOT quantized in V1 — all pad/key input is recorded with its natural timing. Transport actions (Play/Pause) are also not affected by quantise."*

---

### 🟡 MEDIUM — Should Fix Before Beads

#### M1: `AppState.meta.protocolVersion` Declared Twice

**The typo:** In §3.4 `AppState.meta` (lines 435-438), `protocolVersion` appears twice:

```typescript
meta: {
    revisionId: number;
    protocolVersion: number;      // Must match client expectation  ← first
    serverTime: number;
    mode: 'NORMAL' | 'SAFE_MODE';
    version: string;
    isVstMode: boolean;
    memoryBudgetMB: number;
    protocolVersion: number;      // V1 protocol version is 1.     ← duplicate
};
```

In TypeScript, the second declaration silently overwrites the first. This won't cause a runtime error, but an agent copying this interface verbatim will have a duplicate property — a linting warning at minimum, and conceptual confusion about which comment is canonical.

> **Recommendation:** Remove the second `protocolVersion` declaration. Merge the comments into a single line:
> 
> ```typescript
> protocolVersion: number;      // V1 protocol version is 1. Must match client expectation.
> ```

---

#### M2: Dark Theme Background Colors Inconsistent Between §7.1 and §7.3

**The inconsistency:**

§7.1 Visual Language (line 838):
> *Dark (Default): "Studio Dark Grey" backgrounds (`#2D2D2D` to `#383838`). Text is off-white (`#DCDCDC`).*

§7.3 JSON Layout (line 883-884):
```json
"bg_app": "#252525",
"bg_panel": "#333333"
```

`#252525` (RGB 37,37,37) is noticeably darker than `#2D2D2D` (RGB 45,45,45) — a 17% brightness reduction. While the visual difference on screen is subtle, an implementing agent must choose one source. If they use §7.1's hex values for the CSS variables but §7.3's values for the Tailwind config, the result is a visible seam between hand-coded and component-framework styles.

> **Recommendation:** Choose one canonical set. The §7.3 JSON is more specific and will be directly referenced by the React theme implementation. Update §7.1 to match:
> 
> *Dark (Default): "Studio Dark Grey" backgrounds (`#252525` to `#333333`). Text is off-white (`#DCDCDC`).*
> 
> Or update §7.3 to match §7.1. Either way, they must agree.

---

#### M3: `PANIC` Command Referenced as `STOP` in Mixer Tab

**The inconsistency:** §7.6.5 Mixer Tab (line 1137):

> *"Panic Button: Long-pressing the Play/Pause button in any tab (or the header) triggers a **Panic** (stops all audio, resets all synth voices). (Uses `STOP` or dedicated `PANIC` command)."*

There is no `STOP` command in the Command schema (§3.2). The correct command is `PANIC { scope: 'ALL' }`, which is defined as: *"ALL: silence + reset all slots + stop transport."*

Additionally, "resets all synth voices" is not explicitly listed in the `PANIC` command definition — it should be, since a panic without voice reset would leave hanging notes.

> **Recommendation:** Update §7.6.5 line 1137:
> 
> *"(Uses `PANIC { scope: 'ALL' }` command — silences all audio output, stops transport, resets synth voices, and resets all slot states.)"*
> 
> Also update the `PANIC` command definition in §3.2 to include voice reset:
> 
> *"ALL: silence audio output + reset all synth/drum voices + reset all slots + stop transport."*

---

#### M4: `reqId` Only Shown on `SET_VOL` — Should Be Documented as Optional on All Commands

**The gap:** In §3.2, only `SET_VOL` includes `reqId: string` in its type definition:

```typescript
| { cmd: 'SET_VOL'; slot: number; val: number; reqId: string }
```

No other command shows `reqId`. But §2.2.D CommandDispatcher says:

> *"Returns `reqId` in error/ack responses **when present** on the command."*

And ServerMessage includes `ACK { reqId: string }` — implying any command can carry a `reqId` for tracking.

An agent implementing the TypeScript client would reasonably conclude that only `SET_VOL` supports request tracking. All other commands would be fire-and-forget with no ACK correlation.

> **Recommendation:** Add a note above the Command union in §3.2:
> 
> *"**requestId convention:** All commands optionally accept a `reqId: string` field for request-response correlation. When `reqId` is present, the server includes it in ACK or ERROR responses. It is not shown on every command definition below for brevity, but is valid on any command."*

---

### 🟢 LOW — Worth Noting

#### L1: Drum Pad Visual Row Ordering Ambiguous

**The ambiguity:** Audio Engine §4.1 defines drum pads as a grid starting at (1,1):

| Pad | Sound |
|:---|:---|
| (1,1) | Kick 1 |
| (4,4) | Ride |

The question: **Is row 1 the top row or the bottom row?** Standard drum machine conventions (MPC, Maschine, SP-404) typically place kicks at the **bottom** and hi-hats at the **top**. The Audio Engine spec places kicks at row 1 and hi-hats at row 4.

If row 1 = top (reading order), kicks are top-left and hi-hats are bottom — the inverse of every major drum machine. If row 1 = bottom, the layout matches convention but contradicts the natural reading order of the table.

Meanwhile, §7.6.3 says the Note/Bass pad traversal is "bottom-to-top" (pad 0 = bottom-left) — but this only applies to melodic note mapping, not drums.

> **Recommendation:** Add a clarifying note to Audio Engine §4.1:
> 
> *"Row 1 is the **top** row of the visual pad grid. This places kicks (Row 1) at the top and hi-hats (Row 4) at the bottom. Note: This differs from some classic drum machines (MPC, Maschine) which place kicks at the bottom. The FlowZone layout follows the Endlesss/Tim Exile convention for intuitive visual scanning."*
> 
> If the intent is actually kicks-at-bottom (matching MPC convention), invert the table so Row 1 = bottom row. Either way, make it explicit.

---

#### L2: Riff History Layer Cake — Color Array Ordering Undefined

**The gap:** §7.6.1 Riff History Indicators describes the visual format:

> *"Oblong layer cake with rounded edges, layers stacked vertically. Each color represents the input source."*

`AppState.riffHistory[n].colors` is `string[]` — an array of color hex values. But the mapping between array index and visual layer position is undefined:

- Is `colors[0]` the **bottom** layer (first recorded) or the **top** layer?
- If 3 colors are in the array, are they rendered bottom-to-top or top-to-bottom?

This is a minor visual concern but agents implementing the Riff History Indicator component (Task 5.4) need to know.

> **Recommendation:** Add to §7.6.1 Riff History Indicators:
> 
> *"`colors[0]` is the bottom layer (oldest/first recorded slot), `colors[n-1]` is the top layer (most recent slot). Colors are derived from the slot's `instrumentCategory` color (§7.1 palette). Empty slots are excluded."*

---

#### L3: Home Button Behavior — Active Session Continues or Stops?

**The gap:** §7.6.1 Header defines:

> *"Home button (navigates to Jam Manager §7.7)"*

The Jam Manager (§7.7) is the first screen on launch and shows all saved sessions. But what happens to the currently active session when the user taps Home?

- Does audio continue playing? (The engine runs independently of UI navigation, so likely yes.)
- Is the current session auto-saved? (§2.2.G auto-save runs every 30 seconds, but the timing might miss recent changes.)
- Are uncommitted mix changes lost? (§7.6.5 says "uncommitted changes are lost" on exit.)

Since there's no `EXIT_JAM` or `LEAVE_SESSION` command in the schema, tapping Home is presumably a **client-side navigation only** — the React app shows the Jam Manager view while the engine continues running the active session in the background. This is actually the correct behavior for a music app (music shouldn't stop when you navigate menus), but it should be documented.

> **Recommendation:** Add to §7.7:
> 
> *"Navigating to the Jam Manager (via the Home button) does NOT stop the active session. Audio continues playing. The engine keeps the current session loaded. The user can return to the active session by tapping it in the Jam list, or start a new session (which stops the current session via the NEW_JAM command). The auto-save timer (30s) continues running while on the Jam Manager screen."*

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
| **Drum icons (all rows)** | — | 16 icons §4.1 | 16 matching icons | ✅ |
| **Pad-to-note mapping (notes/bass)** | §7.6.3 algorithm | — | colored_pads note | ✅ |
| **Drum kit grid display** | §7.6.3 "4 active, 8 dimmed" | — | — | ✅ |
| **Adjust tab pad reference** | "same as Play tab §7.6.3" | — | "instrument_specific_pads" | ✅ |
| **Solo/Pan in Mixer** | "No Pan or Solo in V1" | — | — | ✅ |
| **Default activeMode** | drums/synthetic/Synthetic §3.4 | — | — | ✅ |
| **VU meter style** | §7.6.5 "integrated" | — | "fader_with_integrated_vu_meter" | ✅ |
| **Keymasher buttons** | §3.2 (12 actions) | §1.2 (12 buttons) | play_tab (12 buttons) | ✅ |
| **Keymasher button labels** | snake_case identifiers | Title Case labels | Title Case labels | ✅ |
| **Settings access** | §7.4 "More button in Mixer" | — | "More button in Mixer" | ✅ |
| **Settings tabs** | 4 tabs (§7.6.8) | — | 4 tabs matching | ✅ |
| **WebSocket port** | 8765 (§2.2.L) | — | — | ✅ |
| **BPM range** | 20-300 (§3.2) | — | — | ✅ |
| **Buffer sizes** | 16-1024 (§3.2, §7.6.8) | — | 16-1024 (settings) | ✅ |
| **Sample rates** | 44.1k-96k (§3.2, §7.6.8) | — | 44.1k-96k (settings) | ✅ |
| **Session list in AppState** | `sessions` array (§3.4) | — | — | ✅ |
| **Slot loop length** | `loopLengthBars` in slot | — | — | ✅ |
| **MIDI input path** | Separate MidiQueue §2.2.D | — | — | ✅ |
| **Retro buffer content** | "instrument output only" §3.10 | "wet signal" §6.3 | — | ✅ |
| **DELETE_JAM GC** | Consistent §3.2 + §2.2.G | — | — | ✅ |
| **protocolVersion** | Declared `1` in §3.1 | — | — | ✅ |
| **RiffSnapshot ↔ riffHistory** | Mapping in §3.5 | — | — | ✅ |
| **config.json path** | §4.1.1 defined | — | — | ✅ |
| **Auto-merge mixed lengths** | Longest slot length §7.6.2.1 | — | — | ✅ |
| **Drum mode Bounce/Speed** | "Hidden" §7.6.4 | "Hidden/disabled" §4.3 | — | ✅ |
| **MAX_SLOTS constant** | 8 declared §3.1 | — | — | ✅ |
| **FX Mode commit order** | Sources → capture? | — | — | ❌ **C1** |
| **Drum MIDI note mapping** | "fixed" §7.6.3 | No MIDI notes in §4.1 | Icons only | ❌ **H1** |
| **Pad base octave** | rootNote 0-11 only §3.4 | — | — | ❌ **H2** |
| **COMMIT_RIFF + FX Mode** | §3.2 says FX behavior | §7.6.2 says SET_LOOP_LENGTH | — | ❌ **H3** |
| **Quantise meaning** | "quantised to grid" §3.4 | — | — | ❌ **H4** |
| **protocolVersion duplicate** | Appears twice in meta §3.4 | — | — | ⚠️ **M1** |
| **Dark theme colors** | #2D2D2D-#383838 §7.1 | — | #252525-#333333 §7.3 | ⚠️ **M2** |
| **PANIC vs STOP** | PANIC §3.2 | — | "STOP or PANIC" §7.6.5 | ⚠️ **M3** |
| **reqId on commands** | Only on SET_VOL | — | — | ⚠️ **M4** |
| All other aspects (40+) | — | — | — | ✅ Consistent |

---

## 5. Reliability Risk Assessment

### R1: No Per-Slot Clear + No Undo — Single-Tap Mistakes Are Permanent

**Scenario:** A user is in a 6-slot session that sounds great. They're playing a drum part and hit the wrong pattern. They tap a Timeline Section out of muscle memory → `SET_LOOP_LENGTH` fires → the bad drum take gets committed to Slot 7. Now their mix has a bad layer.

**Recovery paths:**
- **Load a previous riff:** Only works if they committed a riff before the bad take. If they didn't, there's no riff to go back to.
- **Mute Slot 7:** Works as a workaround but the bad audio stays in the slot — they can't reuse that slot for a new take without entering FX mode or loading a previous riff.
- **Per-slot clear:** Explicitly excluded from V1 (§3.10 note on line 611).
- **Undo:** Explicitly deferred to V2 (§1.3).

**Why this matters for a non-engineer owner:** This will be the #1 user frustration point associated with your app. A single mis-tap permanently occupies a slot with bad audio, and with only 8 slots total, that's 12.5% of the available space lost. The workarounds (mute, load previous riff) are non-obvious for new users.

> **Recommendation:** This is a design choice, not a bug — the spec is intentional. But given the owner's concern about reliability, consider adding a per-slot clear command as a **Phase 8+ polish item** rather than deferring it entirely to V2. In the meantime, add a user-facing note to the app's help/onboarding: *"Tip: Commit riffs frequently! If you make a mistake, you can always load a previous riff from history."*

---

### R2: Binary Visualization Frame Size Notation Inconsistent

**Scenario:** An agent implementing the binary visualization stream decoder (§3.7) reads:

> `[BarPhase:4][MasterRMS_L][MasterRMS_R][Slot1_RMS][Slot1_Spec_1...16]...`

`BarPhase:4` uses byte-size notation (4 bytes = Float32). But `MasterRMS_L`, `MasterRMS_R`, `Slot1_RMS`, and the 16-band spectrum values don't include `:4`. The section header says "Raw Float32 Array" — so all values are presumably Float32 (4 bytes each). But an agent speed-reading might assume `MasterRMS_L` is a different size.

**Total frame size calculation (for agent verification):**
- Header: 4 (Magic) + 4 (FrameId) + 8 (Timestamp) = 16 bytes
- Payload: 1 (BarPhase) + 2 (Master L+R) + 8 × (1 RMS + 16 Spectrum) = 139 Float32 values = 556 bytes
- Total: 572 bytes/frame × 30fps = ~17.2 KB/sec

> **Recommendation:** Add byte-size notation to all payload fields in §3.7 for consistency:
> 
> `[BarPhase:4][MasterRMS_L:4][MasterRMS_R:4][Slot1_RMS:4][Slot1_Spec_1:4...Slot1_Spec_16:4]...[Slot8_RMS:4][Slot8_Spec_1:4...Slot8_Spec_16:4]`
> 
> And add: *"Total payload size: 139 × 4 = 556 bytes. Total frame size with header: 572 bytes."*

---

### R3: External Instrument / External FX Categories — Accepted by AppState but Undefined

**Scenario:** `AppState.activeMode.category` (§3.4 line 462) accepts `'ext_inst'` and `'ext_fx'` as valid values. The UI_Layout_Reference.md (lines 70-71) shows these categories in the Mode grid with "Coming Soon" placeholders.

But the Command schema has no handler differentiation for these modes. If an agent implements the command dispatcher and routes `SELECT_MODE { category: 'ext_inst' }`, the engine will set `activeMode.category = 'ext_inst'` — but there's no internal engine, no synth preset, and no audio processing for this mode.

**Risk:** The user taps "Ext Inst" (if the "Coming Soon" overlay fails to prevent it), the mode switches, and every subsequent `NOTE_ON` command sends notes to an engine that doesn't exist for this category. Result: silence with no error.

> **Recommendation:** Add to §2.2.D (CommandDispatcher) or §3.3:
> 
> *"`SELECT_MODE` with `category: 'ext_inst'` or `category: 'ext_fx'` returns `ERROR { code: 2060, msg: 'MODE_NOT_AVAILABLE' }` in V1. These categories are reserved for the Plugin Instrument and Plugin FX modes (Phase 7+). The React UI disables these buttons with a "Coming Soon" overlay — the error response is a server-side safety net in case the overlay fails."*

---

## 6. Summary of All Findings

### 🔴 Critical — Must Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| C1 | FX Mode with all 8 slots selected destroys all audio (auto-merge + source clearing conflict) | Reverse operation order: clear sources first, then write. Auto-commit riff on FX Mode entry. | 20 min |

### 🟠 High — Should Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| H1 | Drum pad MIDI note numbers undefined — no mapping from (row,col) to note value | Add MIDI Note column to Audio Engine §4.1 table (36-51) | 10 min |
| H2 | Notes/Bass pad base octave missing — rootNote 0-11 without offset = inaudible notes | Define base MIDI note 48 (C3) in §7.6.3 mapping algorithm | 5 min |
| H3 | `COMMIT_RIFF` claims FX capture behavior that belongs to `SET_LOOP_LENGTH` | Remove FX sentence from COMMIT_RIFF in §3.2 | 2 min |
| H4 | `TOGGLE_QUANTISE` meaning undefined — note input? loop capture? transport? | Define as loop-capture timing quantise (snap to bar boundaries) | 5 min |

### 🟡 Medium — Should Fix

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| M1 | `protocolVersion` declared twice in `AppState.meta` | Remove duplicate, merge comments | 1 min |
| M2 | Dark theme `bg_app` is `#252525` in §7.3 JSON but `#2D2D2D` in §7.1 text | Align §7.1 text to match §7.3 JSON values | 2 min |
| M3 | Mixer tab references `STOP` command — doesn't exist, should be `PANIC` | Replace "STOP or PANIC" with "PANIC { scope: 'ALL' }" | 2 min |
| M4 | `reqId` only shown on `SET_VOL` but valid on all commands | Add note: "all commands optionally accept reqId" | 2 min |

### 🟢 Low — Worth Noting

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| L1 | Drum pad row 1 = top or bottom visually? | Add note to Audio Engine §4.1 clarifying visual orientation | 2 min |
| L2 | Riff history `colors[]` → layer position mapping undefined | Add note: `colors[0]` = bottom layer | 2 min |
| L3 | Home button → active session behavior undefined | Document that session continues playing, Home is client-side nav | 5 min |

### Reliability Risks

| # | Risk | Severity | Action |
|:--|:-----|:---------|:-------|
| R1 | No per-slot clear + no undo — single-tap mistakes are permanent | Medium | Design choice; consider adding per-slot clear as Phase 8+ polish item |
| R2 | Binary visualization frame size notation inconsistent | Low | Add `:4` byte-size notation to all payload fields |
| R3 | ext_inst / ext_fx modes accepted by AppState but have no engine | Medium | Add error response for unsupported modes in V1 |

---

## 7. Cumulative Resolution Tracking

| Assessment | Issues Found | Resolved in Next Spec | Still Open |
|:---|:---|:---|:---|
| PBA1 | (initial) | — | — |
| PBA2 | (initial) | — | — |
| PBA3 | (initial) | — | — |
| PBA4 | (initial) | — | — |
| PBA5 | (initial) | — | — |
| PBA6 | 21 (1 C, 5 H, 5 M, 5 L, 5 R) | All 21 resolved | 0 |
| PBA7 | 17 (2 C, 4 H, 4 M, 3 L, 4 R) | All 17 resolved | 0 |
| **PBA8** | **15 (1 C, 4 H, 4 M, 3 L, 3 R)** | — | **15** |

---

## 8. Overall Assessment

| Dimension | Rating (Pre-Fix) | Rating (Post-Fix) | Notes |
|:---|:---|:---|:---|
| **Completeness** | 🟢 **96%** | 🟢 **99%** | Drum MIDI mapping and pad octave are the main gaps |
| **Internal Consistency** | 🟢 **97%** | 🟢 **99%** | COMMIT_RIFF FX behavior, theme colors, STOP reference |
| **Agent Buildability** | 🟢 **95%** | 🟢 **99%** | No agent should have to guess numerical constants |
| **Reliability for Non-Engineer** | 🟢 **95%** | 🟢 **98%** | FX Mode data loss edge case is the main concern |
| **Logic Correctness** | 🟢 **94%** | 🟢 **99%** | C1 (FX Mode sequence) was the only logic bug |

**Verdict:** After eight rounds of review, the spec has reached a very high level of quality. The architecture is sound, the protocol is comprehensive, the UI layout is thorough, and the risk mitigations are well-thought-out. The remaining findings fall into three categories:

1. **One logic bug** (C1: FX Mode commit sequence) — a real data-loss edge case that must be fixed
2. **Missing numerical constants** (H1, H2: MIDI note mapping, base octave) — would cause silent implementation errors
3. **Minor text inconsistencies** (M1-M4, L1-L3) — quick editorial fixes

**Estimated total fix effort: ~60 minutes.**

After resolving these 15 findings, the spec will be at **≥98% across all dimensions** and **fully ready for bead creation**. The remaining 2% gap is inherent to any spec of this complexity — edge cases that can only be discovered during implementation.

**Recommended fix priority:**
1. **C1** (FX Mode logic fix) — potential data loss; must fix before any FX-related beads
2. **H1, H2** (MIDI/octave constants) — blocks Phase 4 audio engine beads
3. **H3, H4** (command clarifications) — prevents agent confusion during Phase 2
4. **M1-M4** (editorial) — quick fixes, high clarity payoff
5. **R1, R3** (UX/safety-net) — plan for later phases
6. **L1-L3** (nice-to-have) — can be addressed organically during bead implementation

---

*End of Pre-Build Assessment 8*
