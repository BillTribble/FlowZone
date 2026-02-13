# FlowZone — Pre-Build Assessment 6

**Date:** 13 Feb 2026  
**Scope:** Final reliability-focused deep review before bead creation. Full cross-reference of all Spec folder documents with emphasis on: runtime ambiguities an agent would need to resolve by guessing, inconsistencies between documents, missing state fields, and edge-case behaviors that would cause subtle bugs a non-engineer cannot troubleshoot.  
**Documents Reviewed:**
- `Spec_FlowZone_Looper1.6.md` (1842 lines)
- `Audio_Engine_Specifications.md` (360 lines)
- `UI_Layout_Reference.md` (368 lines)
- `Pre_Build_Assessment_5.md` (analysis folder, 499 lines) — for resolution context

---

## 1. Executive Summary

PBA5 found 13 issues and 4 reliability risks. The appendix confirms all were resolved in the current Spec 1.6. Those resolutions are solid — NOTE_ON/NOTE_OFF commands, SET_LOOP_LENGTH dual-purpose clarification, barPhase in binary stream, COMMIT_RIFF as state-only snapshot, port number defined, etc.

This sixth-pass review — conducted with fresh eyes and a focus on **runtime edge cases, missing state, and agent-confusing ambiguities** — has found **19 issues** (2 critical, 5 high, 7 medium, 5 low) and **3 reliability risks**.

**The critical themes this time:**

1. **Play Tab content gap** — The Play tab is described as a "sub-view of Mode" showing presets+pads for the selected category, but the spec only defines FX content (effect presets + XY pad) for it. When the user selects Drums/Notes/Bass, what does the Play tab actually show? This is the tab where users spend most of their performance time, and its non-FX content is unspecified.

2. **Missing AppState fields for Settings UI** — The Settings panel (§7.6.8) displays current audio device, input/output channels, and MIDI port states, but `AppState` (§3.4) has no fields for these. The Settings UI cannot render current selections without state to read from.

---

## 2. PBA5 Resolution Verification

| PBA5 # | Resolution Claimed | Present in Spec? | Notes |
|:---|:---|:---|:---|
| C1 | NOTE_ON / NOTE_OFF added to §3.2 | ✅ | Lines 300-301: `NOTE_ON { note }`, `NOTE_OFF { note }`. Error matrix updated. §3.10 references NOTE_ON. |
| C2 | SET_LOOP_LENGTH dual-purpose documented | ✅ | §3.2 comment says "Sets the loop length AND immediately captures..." §3.3 includes `4010: NOTHING_TO_COMMIT` |
| H1 | barPhase in binary stream | ✅ | §3.7 payload includes `[BarPhase:4]`. Note says not in JSON STATE_PATCH. |
| H2 | COMMIT_RIFF = state-only snapshot | ✅ | §3.2 comment: "Does NOT capture audio from the retrospective buffer." §7.6.5: button only visible on mix changes. |
| H3 | Port 8765 defined | ✅ | §2.2.L: default port 8765, configurable via config.json. |
| H4 | Drum pitch = global | ✅ | Audio_Engine_Spec §4.3: "Tune all pads globally." Per-pad deferred to V2. |
| M1 | GET_METRICS_HISTORY deferred to Phase 8 | ✅ | §5.3: "will be defined as part of Phase 8 (Task 8.7)" |
| M2 | revId → revisionId | ✅ | §3.2 STATE_PATCH now uses `revisionId` |
| M3 | Binary ACK = FrameId uint32 LE | ✅ | §3.7: "Client sends a **4-byte ACK** (the `FrameId`...)" |
| M4 | DELETE_JAM vs GC clarified | ✅ | §2.2.G: "DELETE_JAM immediately removes session metadata... audio files marked for GC" |
| L1 | 4/4 hardcoded for V1 | ✅ | §7.6.7: "V1 uses 4/4 time signature exclusively" |
| L2 | SET_KNOB routing note | ✅ | §3.2: "Routed based on current activeMode.category" |
| L3 | Mic slot instrumentCategory defined | ✅ | §3.4: instrumentCategory values listed including 'mic', 'merge' |
| R1 | NOTE_ON performance concern | ✅ | Documented in PBA5 for agent awareness |
| R2 | Non-blocking auto-merge | ✅ | §7.6.2.1 Implementation Note added |
| R3 | FX Mode auto-commit on entry | ✅ | §7.6.2 step 1: "the engine automatically commits the current state" |
| R4 | Hardcoded minimum config fallback | ✅ | §4.5 step 4: "hardcoded minimum configuration compiled into the binary" |

**Verdict:** All PBA5 resolutions are correctly applied. No regressions detected.

---

## 3. New Issues Found

### 🔴 CRITICAL — Will Block Core Functionality

#### C1: Play Tab Content Undefined for Non-FX Modes (Drums, Notes, Bass, Mic)

**The gap:** §7.6.3 states:

> "The Play tab is a **sub-view of Mode**. When the user selects a category (Drums, Notes, Bass, etc.) from the Mode tab, the app automatically switches to the Play tab to show the detail view for that category: presets, pads, and (in FX Mode only) the XY pad."

But §7.6.3's actual content only defines FX-specific elements:
- **Preset Selector:** Lists only FX presets (Core FX: Lowpass, Highpass... and Infinite FX: Keymasher, Ripper...).
- **Effect Control Area:** Only defines Keymasher button grid and XY pad — both FX-exclusive components.
- **Slot Indicators:** Defined below the effect control area.

There is **no definition** of what the Play tab shows for Drums, Notes, Bass, or Microphone modes. The Mode tab (§7.6.2) defines the category selector, preset selector, and pad grid — but the Play tab is described as the "sub-view" where the user actually performs. What layout does the Play tab use when the user has selected Drums?

`UI_Layout_Reference.md` has the same gap — `play_tab` only contains FX preset examples and the effect control areas.

**Why this is critical:** The Play tab is where the user spends the majority of their time performing (tapping pads, playing notes). An agent building Tasks 5.1-5.2 (XY Pad, Pad Grid) needs to know which tab they live in. An agent assembling views (Task 5.7) must know the Play tab's layout for each mode.

**What's needed:**

The Play tab should show the following for non-FX modes:
- **Drums:** Preset selector (4 kits in 3×4 grid) + 4×4 pad grid (drum icons)
- **Notes/Bass:** Preset selector (12 presets in 3×4 grid) + 4×4 pad grid (colored scale pads)
- **Microphone:** No pads, no presets — gain knob + waveform display + monitor toggles
- **Ext Inst/Ext FX:** "Coming Soon" placeholder

Alternatively, if the Mode tab already shows presets + pads (as described in §7.6.2), and the Play tab is just the FX view — then the spec should clarify that the Play tab is exclusively FX mode, and pads/presets always live in the Mode tab.

**User Decision (C1):**
- **Play Tab content:**
  - **Drums/Notes/Bass:** Shows **Preset Selector** (3x4 grid) + **Pad Grid** (4x4) + Slot Indicators.
  - **Microphone:** Shows **Audio Input Selector** (allows selection of one active input) + Gain/Monitor controls.
- **Microphone Mode:** "Show the audio inputs here and allow the user to select one of them, one only for audio input."

> **Recommendation:** Clarify the relationship between Mode and Play tabs. Two options:
> 
> **(A) Play tab shows instrument-specific content per mode (recommended):**
> Add sub-sections to §7.6.3 defining Play tab layout for each category:
> - Drums: kit preset selector + 4×4 drum pad grid + slot indicators
> - Notes/Bass: preset selector + 4×4 scale pad grid + slot indicators
> - Microphone: gain controls + waveform (reference §7.6.6)
> - FX: FX preset selector + XY pad or Keymasher grid + slot indicators (already defined)
> 
> **(B) Play tab is FX-only; Mode tab is the main performance tab:**
> Rename "Play" to "FX" in the tab bar. Remove the "sub-view of Mode" description. State explicitly that the pad grid and preset selector always live in the Mode tab, and the Play/FX tab is only relevant when FX mode is active. Otherwise, tapping Mode or Play shows Mode.

**Action Taken:** Implemented User Decision (Play tab shows presets + pads; Mic shows input selector).


---

#### C2: Missing AppState Fields for Settings UI

The Settings panel (§7.6.8) displays current audio configuration, MIDI ports, and VST paths — but `AppState` (§3.4) lacks the state fields needed to render these.

**Missing fields:**

| Settings UI Element | Needs to Read | Present in AppState? |
|:---|:---|:---|
| Input Device dropdown (current selection) | `audioInputDevice` | ❌ Not in AppState |
| Output Device dropdown (current selection) | `audioOutputDevice` | ❌ Not in AppState |
| Input Channels checkbox matrix | `inputChannels` | ❌ Not in AppState |
| Output Channels checkbox list | `outputChannels` | ❌ Not in AppState |
| Available input/output devices list | `availableAudioDevices` | ❌ Not in AppState |
| MIDI Inputs list with "Active" checkboxes | `midiInputs` (list + active state) | ❌ Not in AppState |
| Clock Source radio | `clockSource` | ✅ In `settings.clockSource` |
| VST3 Search Paths list | `vstSearchPaths` | ❌ Not in AppState |
| Storage Location path | `storageLocation` | ✅ In `settings.storageLocation` |
| Available plugins list | `availablePlugins` | ❌ Not in AppState |

Commands exist to *set* these values (`SET_AUDIO_DEVICE`, `SET_INPUT_CHANNELS`, `SET_MIDI_INPUT_ACTIVE`, `SET_VST_SEARCH_PATHS`), but the React UI has no state to *read* current values from.

**Why this is critical:** Without these state fields, the Settings panel cannot show current selections. The agent building Task 8.5 (Settings panel) would have to either invent state fields (schema drift risk) or build a settings UI that can't display current values (broken UX).

> **Recommendation:** Add a `devices` block and expand `settings` in `AppState` (§3.4):
> ```typescript
> devices: {
>   audioInputs: Array<{ id: string; name: string }>;
>   audioOutputs: Array<{ id: string; name: string }>;
>   activeInputDevice: string;   // id
>   activeOutputDevice: string;  // id
>   inputChannels: number[];     // enabled channel indices
>   outputChannels: number[];    // enabled channel indices
>   midiInputs: Array<{ portId: string; name: string; active: boolean }>;
>   availablePlugins: Array<{ pluginId: string; name: string; manufacturer: string }>;
> };
> settings: {
>   // ... existing fields ...
>   vstSearchPaths: string[];
> };
> ```
> Also update `schema.ts` and `commands.h` in the Schema Foundation task (Task 1.1).

---

### 🟠 HIGH — Will Cause Agent Confusion or Subtle Bugs

#### H1: Retrospective Buffer Behaviour During Transport Pause — Undefined

§3.10 says: "The engine maintains the circular buffer, always recording."

When transport is paused:
- Can the user still play pads (NOTE_ON/NOTE_OFF)? Pads are a live instrument — they should work regardless of transport state.
- If pads are active during pause, the retrospective buffer captures pad audio.
- But `SET_LOOP_LENGTH` captures "the most recent N bars of audio" — what's a "bar" when transport is paused? There's no advancing bar phase.

**Scenarios an agent must handle:**

| State | User Action | Expected Behavior | Currently Defined? |
|:---|:---|:---|:---|
| Transport paused, no slots filled | User plays pads | Sound plays. Retro buffer captures. | ❌ Unclear |
| Transport paused, played some pads | User taps "2 BARS" | Capture 2 bars of audio from retro buffer? What duration is "2 bars" at current BPM with paused transport? | ❌ Unclear |
| Transport paused, some slots playing | User taps pad | Can user play over paused loops? | ❌ Unclear |

> **User Decision (H1):** "If user captures bars then we restart the transport."
> 
> **Revised Recommendation:** Add to §3.10 defining transport-pause behavior:
> - **Transport Restart on Capture:** When `SET_LOOP_LENGTH` is triggered (defining the loop), the transport **restarts** (resets to start and plays) to ensure the loop plays back locally in sync from the beginning.
> - **Pads during Pause:** Pads are always active.
> - **Retro Buffer:** Always captures.

> **Recommendation:** Add a section to §3.10 defining transport-pause behaviour:
> - *"Pads (NOTE_ON/NOTE_OFF) are always active regardless of transport state. The user can play sounds at any time."*
> - *"The retrospective buffer always captures audio, including during transport pause."*
> - *"When transport is paused, `SET_LOOP_LENGTH` still captures from the retrospective buffer. The 'N bars' duration is calculated using the current BPM setting (e.g., 2 bars at 120 BPM = 4 seconds of audio), even though bars are not actively advancing. This allows the user to jam over a paused session and capture a loop before starting transport."*
> - *"When the captured loop is committed to a slot and transport is paused, that slot enters PLAYING state but does not produce audio until transport resumes."*


---

#### H2: Play Tab Preset Selector + Mode Tab Preset Selector — Duplicated or Distinct?

Both Mode (§7.6.2) and Play (§7.6.3) tabs define a "Preset Selector" as a 3×4 grid. When the user selects a category in Mode and auto-switches to Play:

- Does the Mode tab still show its own preset selector?
- Does the Play tab show the same presets as Mode, or FX presets only?
- If the user manually navigates back to Mode tab, does it show the category grid + presets + pads all at once?

`UI_Layout_Reference.md` defines `instrument_tab` with a `preset_selector` AND `play_tab` with its own `preset_selector` — but the preset content examples in `play_tab` are FX-only.

**Why this matters:** An agent building the navigation system (Task 3.3) and assembling views (Task 5.7) needs definitive knowledge of what each tab contains. If both tabs show presets, the user sees a duplicated UI. If only one shows presets, the other needs to be clearly scoped.

**User Decision (H2):**
- **Mode Tab:** Shows **Modes (Categories) ONLY**. No presets.
- **Play Tab:** Shows **Presets + Pads**.
- **Behavior:** When user selects a mode in Mode Tab, app **instantly switches to Play Tab** and shows presets/controls for that mode.
- **FX Presets:** Only show if Effects mode is selected.

> **Recommendation:** Define the division explicitly:
> - **Mode tab** = Category selector (2×4 grid) + preset selector (3×4) + pad grid (4×4). This is the "configuration" view — pick your instrument, pick your preset, see your pads.
> - **Play tab** = This tab changes its layout based on current mode:
>   - *FX mode:* FX preset selector + XY pad/Keymasher grid + slot indicators (already defined)
>   - *Non-FX modes:* Pad grid only (full-screen, optimized for performance). No category selector, no preset selector — those are in Mode tab. Slot indicators below the pads.
> - This makes Mode = "set up" and Play = "perform". The auto-switch from Mode → Play puts the user directly into performance view after selecting a preset.


---

#### H3: `TOGGLE_NOTE_NAMES` — Schema Contradiction (UI-Only vs Engine State)

§3.2 Command Schema lists `TOGGLE_NOTE_NAMES` with the comment:
> "UI Settings (UI-only, stored in localStorage — not sent to engine)"

But it's defined as a `Command` type within the `Command` union that agents will implement handlers for in `CommandDispatcher`. The `ui.noteNamesEnabled` field also exists in `AppState` (§3.4) — which is engine state broadcast to all clients.

**The contradiction:** If it's "not sent to engine," it shouldn't be in the `Command` union or `AppState`. If it IS in `AppState` (for multi-client state sync), it needs to be sent to the engine. It can't be both.

**User Decision (H3):** "Happy to take your recommendation here."

> **Explanation of Issue:** The spec currently says `TOGGLE_NOTE_NAMES` is "UI-only" (doesn't go to the engine), but assumes it's handled like other engine commands. This is contradictory. The recommendation (Option B) treats it as a real engine command so note names stay improved across sessions and devices.

> **Recommendation:** Pick one:
> **(A)** Remove from Command union and AppState. Keep as React-local state in localStorage. Simple, but note names won't sync across multiple connected clients.
> **(B)** Keep in Command union and AppState. Remove the "UI-only" comment. Add to Error Matrix (always succeeds). The engine stores this as part of session state and broadcasts to all clients. This is better for multi-device consistency (V2).
>
> **Recommended:** Option (B) for future-proofing. Update the §3.2 comment to remove the "UI-only" note.


---

#### H4: Zap Delay Feedback > 1.0 — Self-Oscillation Without Safety Net in VST3 Mode

Audio_Engine_Specifications.md §1.2 Zap Delay: `Feedback (0.0 - 1.2)`.

Feedback > 1.0 means the delay output grows exponentially — intentional for creative use (self-oscillation). The main spec §2.2.A specifies a master limiter (brickwall, -0.3 dBFS) in Standalone mode, which catches this.

**However:** The same section says the master limiter is "**bypassed in VST3 multi-channel mode** so that dynamics are preserved for DAW recording."

In VST3 mode, a user holding the Zap Delay XY pad at feedback > 1.0 for several seconds will produce audio that grows without bound. The DAW may have its own limiter, but FlowZone's output will be dangerously loud. This is the exact kind of problem a non-engineer user would not anticipate.

**User Decision (H4):** "This is not a problem let's ignore this for now." (Issue Dismissed).

> **Recommendation:** Add a per-effect output clipper or soft limiter to all effects with feedback > 1.0 (Zap Delay, Dub Delay at 0.99). This is independent of the master limiter and protects the output even in VST3 mode. Add a note to §2.2.A: *"Effects with feedback parameters exceeding 1.0 must include their own internal soft-clipper to prevent runaway gain, independent of the master limiter."*


---

#### H5: FX Mode Retro Buffer Content — What Is Being Captured?

§3.10: "Continuously records the output of the currently selected instrument/mode."
§7.6.2 FX Mode step 6: "When the user taps a Loop Length Button, the FX-processed audio is captured for that duration and written to the next empty slot."

**Question:** Where does the FX-processed audio come from for capture?

- **Option A:** The retro buffer switches to recording the FX output bus when in FX Mode. The `SET_LOOP_LENGTH` command then captures from the retro buffer as usual.
- **Option B:** FX Mode uses a separate recording mechanism — it records from the FX output bus into a dedicated buffer, bypassing the retro buffer entirely.

This matters because:
- If Option A, the retro buffer must change its input source when entering/exiting FX Mode.
- If Option B, the retro buffer content is stale (it was recording the pre-FX instrument output), and `SET_LOOP_LENGTH` needs mode-aware routing logic to know which buffer to capture from.

§7.6.2 also says "The user cannot play instruments while in FX Mode (V1)" — so the instrument output is silent, meaning the retro buffer would be recording silence unless it's re-routed to the FX output.

**User Decision (H5):**
- **Option A confirmed:** In FX Mode, FX output feeds into the retro buffer.
- Note: See M2 for drum parameter updates.

> **Recommendation:** Add to §3.10 or §7.6.2: *"When FX Mode is active, the retrospective buffer's input is re-routed to the FX output bus (the sum of selected slots processed through the active effect). This ensures `SET_LOOP_LENGTH` captures the FX-processed audio from the same retro buffer mechanism used in all other modes. When FX Mode is exited, the retro buffer input reverts to the instrument output."*


---

### 🟡 MEDIUM — Should Fix Before Beads

#### M1: Audio Engine Spec — Stale "Sound/FX Tab" Reference

Audio_Engine_Specifications.md §1 (line 9):
> "These effects support all UI elements defined in §7.6.3 (Sound/FX Tab)"

The main spec renamed this to "Play Tab" in PBA4. The section number §7.6.3 is still correct, but the label "Sound/FX Tab" is stale and may confuse an agent searching for a "Sound" tab that doesn't exist.

> **Recommendation:** Update Audio_Engine_Specifications.md §1 to reference "§7.6.3 (Play Tab)".

---

#### M2: Drum Adjust Tab — Bounce, Speed, Reverb Undefined for Drums

The Adjust tab (§7.6.4) shows 7 knobs: Pitch, Length, Tone, Level, Bounce, Speed, Reverb.

Audio_Engine_Specifications.md §4.3 only defines drum parameter mappings for 3 of them:
- **Pitch** → Tune all pads globally ✅
- **Tone** → Filter cutoff globally ✅
- **Level** → Output gain globally ✅

Missing drum mappings:
- **Length** → ?
- **Bounce** → ?
- **Speed** → ?
- **Reverb** → ?

The synth parameter mapping (Audio_Engine_Spec §2.2) defines these: Length = Decay/Release multiplier, Bounce = Attack time, Speed = LFO rate, Reverb = reverb send. Some of these translate to drums (Length could control decay time, Reverb makes sense as a send), but others are ambiguous (what does Speed/LFO rate do for drums? They have no LFO by default).

An agent building Task 4.5 (Drum engine) needs to know what these knobs do.

**User Decision (M2):** "Do not add these extra parameters to the drums, we only need perhaps length and reverb."
- **Keep:** Length, Reverb.
- **Ignore/Remove:** Bounce, Speed (for drums).

> **Recommendation:** Expand Audio_Engine_Specifications.md §4.3 with full drum parameter mappings:
> - **Length** → Decay time multiplier (0.1x - 5.0x) — makes drum hits shorter or longer
> - **Reverb** → Reverb send level (0.0 - 1.0) — same as synth
> - **Bounce/Speed** → Not used for drums (per user decision).

---

#### M3: `NOTHING_TO_COMMIT` (4010) — Dual Meaning for Different Commands

§3.3 Error Matrix uses error code `4010: NOTHING_TO_COMMIT` for two different failure conditions:

- **`SET_LOOP_LENGTH`** → 4010 means "retrospective buffer is empty (nothing was played)"
- **`COMMIT_RIFF`** → 4010 means "no mix changes have been made since last commit"

These are semantically different failures. A client showing an error toast based on code 4010 would need to know which command triggered it to display the right message ("Nothing to capture" vs "No changes to commit").

> **Recommendation:** Either:
> **(A)** Keep 4010 for both but add a `detail` field to the ERROR response: `{ type: 'ERROR'; code: 4010; msg: 'NOTHING_TO_COMMIT'; detail?: string; reqId?: string }` — where `detail` is `'empty_buffer'` or `'no_mix_changes'`.
> **(B)** Split into two codes: `4010: ERR_EMPTY_BUFFER` (for SET_LOOP_LENGTH) and `4011: ERR_NO_MIX_CHANGES` (for COMMIT_RIFF). Note: 4011 is currently used for `ERR_RIFF_NOT_FOUND` — renumber if splitting.
>
> **Recommended:** Option (A) is less disruptive since error codes are already registered.

---

#### M4: Riff History `colors` Array — instrumentCategory-to-Color Mapping Not Defined

`AppState.riffHistory[].colors: string[]` is described as "Layer cake colors (source-based)." §7.1 defines a color palette:
- Indigo (#5E35B1) → Drums/Percussion
- Teal (#00897B) → Instruments/Synths
- Amber (#FFB300) → External Audio Input
- Vermilion (#E65100) → Combined/Summed tracks or FX
- Chartreuse (#7CB342) → Transport & Sync

`slot.instrumentCategory` values are: `'drums'`, `'notes'`, `'bass'`, `'fx'`, `'mic'`, `'ext_inst'`, `'ext_fx'`, `'merge'`.

**Undefined mappings:**
- `'notes'` → Teal? (Instruments/Synths)
- `'bass'` → Also Teal? Both are synths, but the layer cake must differentiate same-color layers with "±30% brightness" per §7.6.1.
- `'mic'` → Amber? (External Audio Input) — mic is audio input, so yes?
- `'ext_inst'` → Teal or Amber? External instrument input is audio from outside, but it's VST-hosted...
- `'ext_fx'` → Vermilion? (FX)
- `'merge'` → Vermilion? (Combined/Summed)

> **Recommendation:** Add an explicit mapping table to §7.1 or §7.6.1:
> ```
> drums   → #5E35B1 (Indigo)
> notes   → #00897B (Teal)
> bass    → #00897B (Teal, differentiated via ±30% brightness)
> fx      → #E65100 (Vermilion)
> mic     → #FFB300 (Amber)
> ext_inst → #FFB300 (Amber, differentiated via ±30% brightness)
> ext_fx  → #E65100 (Vermilion, differentiated via ±30% brightness)
> merge   → #E65100 (Vermilion)
> ```

---

#### M5: Slot Playback Source — RAM or Disk?

When `SET_LOOP_LENGTH` captures audio from the retrospective buffer:
1. Raw PCM float32 data is copied from the circular buffer (RAM).
2. The audio must be written to disk as FLAC (§3.6).
3. The slot must play the audio immediately.

**Question:** Does the slot play from the RAM copy or from the FLAC file on disk?

For instant playback (zero-latency capture-to-play, which is the core UX promise), the slot must play from RAM. The FLAC write can happen asynchronously on the DiskWriter thread.

But when loading a riff from history (`LOAD_RIFF`), the audio must be loaded from disk (FLAC → decoded → RAM).

This two-path system (new recordings play from RAM; loaded riffs play from disk) is a common pattern but needs to be specified so the agent builds the correct data flow.

> **Recommendation:** Add to §3.10 or §2.2.I: *"When audio is captured from the retrospective buffer, the raw PCM data is copied into an in-memory playback buffer owned by the slot. Playback starts immediately from RAM. Simultaneously, the DiskWriter thread encodes and writes the audio as FLAC to disk. When loading a riff from history, the FLAC file is decoded into the slot's in-memory playback buffer before playback begins."*

---

#### M6: Mixer Channel Strips — "Scrollable if > 8 Channels" Is Misleading

§7.6.5: "Vertical fader strips (one per active slot). Arrangement: Horizontal row, scrollable if > 8 channels."

There are exactly 8 fixed slots (the 9th triggers auto-merge). There will never be more than 8 channels. "Scrollable if > 8" is misleading — it was likely meant for phone screens where 8 strips don't fit in the viewport width.

**User Decision (M6):** "No need for scrolling, we make them fit on phone screen."

> **Recommendation:** Change to: *"Horizontal row. On phone screens, strips are sized/scaled to fit continuously within the viewport. No scrolling required."*


---

#### M7: VST3 Build Target — Not in Task Breakdown

§1.1 Goals: "**Dual Build Target:** Built simultaneously as a Standalone App and a VST3 Plugin."
§2.2.A: "In VST3 mode, declare 8 stereo output buses... Use `#if JucePlugin_Build_VST3` preprocessor guards."

But the Task Breakdown (§9):
- Task 0.1 says "produce a compiling JUCE standalone app" — no mention of VST3.
- No task explicitly sets up the VST3 target in Projucer.

The VST3 target requires specific Projucer configuration (plugin characteristics, bus layouts, manufacturer code, etc.). If it's not part of Task 0.1, when does it happen?

**User Decision (M7):** Option B (Phase 8 for VST3).

> **Recommendation:** Option (B) — keep Phase 0 minimal (Standalone only). VST3 target is additive and doesn't affect the core looping workflow.

---

### 🟢 LOW — Worth Noting

#### L1: Drum Pad Icons — 5 Pads Share `double_diamond` Icon

Audio_Engine_Specifications.md §4.1 maps pad icons to sounds. Row 2 uses `double_diamond` for pads (2,1) Kick 2, (2,2) Kick 3, (2,3) Kick 4, AND (2,4) Tom High. Plus pad (1,1) Kick 1 is also `double_diamond`.

That's 5 of 16 pads sharing the same icon. Users won't be able to visually distinguish between these sounds. This mimics some hardware drum machines (where kick variations look similar), so it may be intentional — but worth confirming.

**User Decision (L1):** "Let's find new icons for these kicks."

> **Recommendation:** **Action:** Assign unique/distinct icons to each kick variation to ensure visual differentiation.

---

#### L2: FX Mode Entry Auto-Commit + Mixer "Dirty State" Tracking

§7.6.2 FX Mode step 1: "the engine automatically commits the current state to riff history [on FX entry]."
§7.6.5: COMMIT_RIFF button is "only visible when the user has changed mix levels... since the last commit."

If the user adjusts volumes (dirty state), then enters FX mode (auto-commit clears dirty state), then returns to Mixer — the Commit button should be hidden because the auto-commit already saved their changes.

This requires the auto-commit to update whatever "dirty state" flag the Mixer UI tracks. Since the auto-commit happens engine-side and the dirty state is tracked UI-side, the engine must broadcast that a commit occurred (via STATE_PATCH to `riffHistory`). The UI should detect the new riff in history and clear its dirty flag.

**User Decision (L2):** "If user leaves levels mode without commiting, no auto commit happens. They lose these changes if they exit the jam, this is OK. Auto commit is not a thing here."
- **Clarification:** Mixer "dirty state" is not auto-committed on mode switch. Changes are lost if not explicitly committed.

> **Recommendation:** Remove "auto-commit on FX Mode Check" logic if it conflicts with this principle, or clarify that "Mixer" changes specifically are exempt from auto-commit.

---

#### L3: `PANIC` Command — All vs Engine Scope Ambiguity

§3.2: `{ cmd: 'PANIC'; scope: 'ALL' | 'ENGINE' }`
- `ALL`: "silence + reset all slots + stop transport"
- `ENGINE`: "silence audio output only, preserve state"

§3.3 Error Matrix: both "always succeeds."

**Question:** Where is the PANIC button in the UI? It's not in any tab layout (§7.6.1-§7.6.8). There's no button defined for triggering PANIC. Is it:
- A hidden keyboard shortcut?
- Accessible from the Settings panel?
- A gesture (e.g., long-press Play/Pause)?
- Only triggered via the HTTP health endpoint or external tool?

For a non-engineer user, a panic button should be easily accessible during a live performance.

**User Decision (L3):** "Long press Play/Pause for panic."

> **Recommendation:** Add PANIC to the UI: **Long-press Play/Pause (2s) = PANIC (All Scope).**

---

#### L4: No Per-Slot Clear — UX Friction for Non-Engineer

§3.10 note: "V1 does not include a per-slot clear command."

If a user records a bad layer and wants to clear just that slot:
1. They must open Riff History.
2. Find and load the previous riff.
3. This restores the pre-recording state — but any volume/pan adjustments made after the last commit are lost.

The FX Mode auto-commit protects against this in FX scenarios, but regular recording (non-FX) has no such protection. A user who adjusts volumes, records a bad layer, and wants to undo just the recording will lose their mix changes.

**User Decision (L4):** "Not a problem they just use riff history... clear slot is not a thing." (Issue Dismissed).

> **Recommendation:** Accept current behavior.

---

#### L5: Riff History View — No "Load Riff" Action Button

§7.6.7 Riff History View defines:
- **Actions Row:** "Delete Riff" and "Export Stems (disabled)."
- **Interaction:** "Tap to select and load riff details (does not switch playback)."

But there's no explicit "Load" button to apply/play the selected riff. Tap-to-select only shows riff details — it "does not switch playback." How does the user actually load a riff into the active session?

Is it:
- Double-tap a riff?
- Tap the riff indicator in the toolbar (§7.6.1 says "Tap: Jumps backwards in riff history")?
- A "Load Riff" action button that's missing from the Actions Row?

The Riff History indicators in the toolbar (§7.6.1) allow tap-to-jump, but the full Riff History View (§7.6.7) doesn't clearly define how to apply a riff.

**User Decision (L5):** "Selecting a riff plays it. Just tap and it plays. We show transport controls at top of screen."

> **Recommendation:** **Behavior:** Tapping a riff in history **immediately loads AND plays it**. Transport controls remain visible at top of screen.

---

## 4. Cross-Document Consistency Matrix (Full Recheck)

| Aspect | Spec 1.6 | Audio Engine Spec | UI Layout Ref | Status |
|:---|:---|:---|:---|:---|
| **Tab labels** | Mode, Play, Adjust, Mixer | "Play Tab" (but also "Sound/FX Tab" §1) | Mode, Play, Adjust, Mixer | ⚠️ M1 (stale ref) |
| **Core FX count & names** | 12 (§2.2.M) | 12 (§1.1) | 12 (play_tab) | ✅ |
| **Infinite FX count & names** | 11 (§2.2.M) | 11 (§1.2) | 11 (play_tab) | ✅ |
| **Keymasher buttons** | 12 buttons (§3.2) | 12 buttons (§1.2) | 12 buttons (play_tab) | ✅ |
| **Notes presets** | 12 (§2.2.M) | 12 named (§2.1) | "See Audio_Engine_Specs" | ✅ |
| **Bass presets** | 12 (§2.2.M) | 12 named (§3.1) | "See Audio_Engine_Specs" | ✅ |
| **Drum kits** | 4 kits (§2.2.M) | 4 kits (§4.2) | Pad grid icons | ✅ |
| **Drum pad icons → sounds** | Icons listed (§7.6.2) | Icon-to-sound map (§4.1) | Icons match | ✅ |
| **Drum pad icons duplicated** | 5 pads = `double_diamond` | Same (§4.1) | Same | ⚠️ L1 |
| **XY Pad behavior** | Touch-and-hold (§7.6.3) | XY mapping per effect | Crosshair on hold | ✅ |
| **Adjust knobs** | 7 + 2 reverb (§7.6.4) | Parameter mapping (§2.2) — Notes/Bass only | 7 + reverb section | ⚠️ M2 (drums incomplete) |
| **Drum parameters** | 7 knobs shown in Adjust | Only 3 mapped in §4.3 | Same 7 knobs | ⚠️ M2 |
| **Mic mode knobs** | Reverb Mix + Room Size + Gain (§7.6.4) | Built-in reverb (§6.3) | Monitor + Gain | ✅ |
| **SET_KNOB routing** | Mode-dependent (§3.2 note) | Different param maps per mode | N/A | ✅ Fixed PBA5 |
| **Mixer transport grid** | 2×3 (§7.6.5) | N/A | 2×3 controls | ✅ |
| **Settings tabs** | 4 tabs (§7.6.8) | N/A | 4 tabs | ✅ |
| **Settings state fields** | AppState missing device fields | N/A | Settings needs current values | ❌ C2 |
| **Slot states** | `EMPTY \| PLAYING \| MUTED` | N/A | N/A | ✅ |
| **Play tab non-FX content** | "Sub-view of Mode" for all categories | N/A | Only FX content defined | ❌ C1 |
| **Play tab FX content** | Defined (§7.6.3) | Effect lists (§1.1, §1.2) | Defined (play_tab) | ✅ |
| **NOTE_ON / NOTE_OFF** | In §3.2 schema | N/A | N/A | ✅ Fixed PBA5 |
| **TOGGLE_NOTE_NAMES** | "UI-only" comment + in Command union + in AppState | N/A | In settings quick_toggles | ⚠️ H3 |
| **barPhase delivery** | Binary stream (§3.7) | N/A | N/A | ✅ Fixed PBA5 |
| **Port number** | 8765 (§2.2.L) | N/A | N/A | ✅ Fixed PBA5 |
| **revisionId naming** | Consistent throughout | N/A | N/A | ✅ Fixed PBA5 |
| **Binary ACK format** | FrameId uint32 LE (§3.7) | N/A | N/A | ✅ Fixed PBA5 |
| **PANIC button location** | Command defined (§3.2), no UI placement | N/A | Not in any layout | ⚠️ L3 |
| **Riff History "Load" action** | Not in actions row (§7.6.7) | N/A | Not defined | ⚠️ L5 |
| **VST3 target in task breakdown** | Goals mention dual target | N/A | N/A | ⚠️ M7 |
| **Color-to-category mapping** | Palette defined (§7.1), categories defined (§3.4) | N/A | Format described | ⚠️ M4 (mapping absent) |
| **Transport pause + pads** | Not specified | N/A | N/A | ⚠️ H1 |
| **FX Mode retro buffer source** | Not specified | N/A | N/A | ⚠️ H5 |
| **Slot playback source (RAM vs disk)** | Not specified | N/A | N/A | ⚠️ M5 |
| **Error 4010 dual meaning** | Same code, different conditions | N/A | N/A | ⚠️ M3 |
| **Zap Delay feedback > 1.0** | N/A | 0.0-1.2 (§1.2) | N/A | ⚠️ H4 |

---

## 5. Reliability Risk Assessment

### R1: Schema Foundation Task Must Include Device State

The Schema Foundation bead (Task 1.1) must define `AppState` with ALL the fields the UI will eventually need — including device/MIDI/plugin state (C2 above). If device state is added later (Task 8.5 timeline), it requires schema changes that break the "contracts complete before fanout" principle from §8.6.

**Mitigation:** Define device/settings state fields in Task 1.1 even if the Settings UI isn't built until Task 8.5. The engine can populate them with defaults initially.

### R2: Retrospective Buffer ↔ FX Mode ↔ Transport Pause — Three-Way Interaction

The retrospective buffer's behavior changes based on two independent variables: (1) whether FX Mode is active, and (2) whether transport is playing or paused. This creates 4 states, of which only 1 (instrument mode + transport playing) is fully defined. The remaining 3 need explicit documentation:

| State | Retro Buffer Input | SET_LOOP_LENGTH Behavior |
|:---|:---|:---|
| Instrument mode + Playing | Instrument output | Capture N bars from retro buffer ✅ Defined |
| Instrument mode + Paused | Instrument output (pads active?) | ❌ H1 |
| FX Mode + Playing | FX output bus? | ❌ H5 |
| FX Mode + Paused | FX output bus? Silence? | ❌ Not addressed |

**Mitigation:** Resolve H1 and H5 before beads. The "FX Mode + Paused" state should be clarified too — likely "FX Mode cannot be entered while transport is paused" or "FX Mode requires transport to be playing."

### R3: Mode → Play Auto-Switch — Navigation Edge Cases

§7.6.3 says selecting a category in Mode "automatically switches to the Play tab." But:
- What if the user is on the Adjust tab and changes the mode via some other path? Do they get auto-switched?
- What if the user is in FX Mode on the Play tab, then switches to Drums on the Mode tab — does the Play tab instantly reload with Drums content (which is undefined per C1)?
- What if the user navigates back to Mode tab manually while in FX Mode — do they see FX selected in the category grid?

These navigation edge cases need clear routing rules.

**Mitigation:** Define the auto-switch as: "Tapping a category in the Mode tab's category selector always navigates to the Play tab AND changes `activeMode.category`. No other action triggers auto-switch. The user can manually navigate to any tab at any time."

---

## 6. Summary of All Findings

### 🔴 Critical — Must Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| C1 | Play Tab content undefined for Drums/Notes/Bass/Mic modes | Define Play tab layout per mode or clarify it's FX-only | 20 min |
| C2 | Missing AppState fields for Settings UI (audio devices, MIDI, VST) | Add `devices` block to AppState §3.4 | 15 min |

### 🟠 High — Should Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| H1 | Retrospective buffer behavior during transport pause | Define pad availability + capture calculation during pause | 10 min |
| H2 | Play tab and Mode tab both define Preset Selector — unclear division | Define Mode = setup, Play = perform. Clarify content split. | 10 min |
| H3 | `TOGGLE_NOTE_NAMES` — UI-only comment contradicts presence in Command union + AppState | Remove "UI-only" comment or remove from Command/AppState | 5 min |
| H4 | Zap Delay feedback > 1.0 has no safety net in VST3 mode | Add per-effect soft-clipper requirement | 5 min |
| H5 | FX Mode retro buffer input source — what's being captured? | Define retro buffer re-routing in FX Mode | 10 min |

### 🟡 Medium — Should Fix

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| M1 | Audio Engine Spec references "Sound/FX Tab" (stale name) | Update to "Play Tab" | 2 min |
| M2 | Drum Adjust Tab knobs (Bounce, Speed, Reverb, Length) undefined for drums | Expand Audio_Engine_Spec §4.3 | 10 min |
| M3 | Error 4010 dual meaning (empty buffer vs no mix changes) | Add `detail` field or split codes | 5 min |
| M4 | instrumentCategory → riff color mapping not defined | Add mapping table to §7.1 or §7.6.1 | 5 min |
| M5 | Slot playback source (RAM vs FLAC) not specified | Add implementation note to §3.10 | 5 min |
| M6 | Mixer "scrollable if > 8 channels" — misleading since max is 8 | Reword for responsive context | 2 min |
| M7 | VST3 build target not in task breakdown | Add to Phase 0 or Phase 8 | 5 min |

### 🟢 Low — Nice to Have

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| L1 | 5 drum pads share `double_diamond` icon | Document as intentional or differentiate | 5 min |
| L2 | FX Mode auto-commit + Mixer dirty state tracking | Add implementation note | 5 min |
| L3 | PANIC command — no UI button defined | Add long-press or icon trigger | 5 min |
| L4 | No per-slot clear — UX friction for bad recordings | Document workaround for V1 | 5 min |
| L5 | Riff History View — no "Load Riff" action button | Add Load button to actions row | 5 min |

### Reliability Risks

| # | Risk | Action |
|:--|:-----|:-------|
| R1 | Schema Foundation must include device state fields | Include in Task 1.1 scope |
| R2 | Retro buffer behavior depends on FX Mode × Transport state (4 combos, only 1 defined) | Resolve H1 + H5 before beads |
| R3 | Mode → Play auto-switch has navigation edge cases | Define clear routing rules |

---

## 7. Overall Assessment

| Dimension | Rating (pre-fix) | Rating (post-fix) | Notes |
|:---|:---|:---|:---|
| **Completeness** | 🟡 **92%** | 🟢 **98%** | Play tab non-FX content and device state fields are the biggest gaps. Once defined, coverage is near-complete. |
| **Internal Consistency** | 🟡 **94%** | 🟢 **99%** | Stale naming, duplicate preset selectors, and TOGGLE_NOTE_NAMES contradiction are minor fixes. |
| **Agent Buildability** | 🟡 **90%** | 🟢 **97%** | The biggest agent-confusion risk is C1 (what does Play tab show for Drums?). Once resolved, agents have deterministic guidance for every view and interaction. |
| **Reliability for Non-Engineer** | 🟡 **91%** | 🟢 **96%** | Zap Delay safety, transport-pause behavior, and slot playback path are important for preventing user-visible glitches. |

**Verdict:** The spec suite has reached a high maturity level through 5 rounds of assessment. This final pass found one structural gap (Play tab content), one state completeness gap (device/MIDI state in AppState), and a set of runtime edge cases that would cause subtle bugs if left to agent guesswork. None of these are as severe as earlier rounds (missing NOTE_ON, missing port number, etc.) — these are refinement-level issues.

**After resolving all items, the spec is ready for bead creation.** The 19 issues require approximately **2 hours total** of specification work. I recommend resolving C1+C2+H1-H5 as a minimum gate before starting beads, with M1-M7 and L1-L5 addressed in a follow-up pass or as part of the relevant bead's pre-work.

---

*End of Pre-Build Assessment 6*
