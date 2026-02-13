# FlowZone — Pre-Build Assessment 4

**Date:** 13 Feb 2026  
**Scope:** Final pre-bead-creation deep cross-reference of all Spec folder documents. Every line of every document checked against every other for internal consistency, undefined behaviors, missing schema elements, and reliability risks — focused on agent-buildability for a non-engineer owner.  
**Documents Reviewed:**
- `Spec_FlowZone_Looper1.6.md` (1805 lines)
- `Audio_Engine_Specifications.md` (360 lines)
- `UI_Layout_Reference.md` (367 lines)
- `Pre_Build_Assessment_3.md` (482 lines) — for context on prior resolutions

---

## 1. Executive Summary

The spec suite is **mature and well-structured**. PBA3 identified 12 issues and 7 reliability risks; all high/medium issues were resolved in the Spec 1.6 update. The task breakdown (§9), risk mitigations (§8), testing strategy (§8.7), and human checkpoints (§8.8) are comprehensive and give agents a clear execution path.

This fourth-pass deep review — reading every line of every document against every other with fresh eyes — has found **18 remaining issues** (4 critical, 6 high, 5 medium, 3 low) and **5 unresolved reliability items** from PBA3 that still need spec-level fixes.

**The key theme this time:** The issues are **undefined operational behaviors** — commands that exist in the schema but whose runtime behavior is never described, missing state/commands for an entire subsystem (SampleEngine), and a structural mismatch between the two copies of the phase plan. These are the kind of gaps that will cause agents to make assumptions, and different agents will make different assumptions.

---

## 2. PBA3 Findings — Unresolved Items Still in Spec

PBA3's Appendix claims resolutions were applied, but several **reliability items were flagged as needing spec changes that are not present in the current documents**:

| PBA3 # | Issue | Current Status | Evidence |
|:---|:---|:---|:---|
| R1 | CivetWeb integration guidance | ❌ **Still missing** | §2.2 and §6 contain no mention of CivetWeb integration method (source inclusion, static lib, etc.) |
| R2 | WebBrowserComponent dev/prod workflow | ❌ **Still missing** | No guidance on dev server URL vs production resource loading |
| R3 | MP3 encoding unavailable via JUCE | ❌ **Still present** | §3.6 still says "MP3 320kbps (via LAME or system encoder)" |
| R5 | FLAC compression level for real-time | ❌ **Not specified** | §3.6 and §4.2 say FLAC 24-bit but no compression level guidance |
| R7 | Ext Inst/Ext FX placeholder behavior | ❌ **Still missing** | §7.6.2 doesn't define what users see when tapping these categories with no plugins |

> **Impact:** These were all flagged as "Add to Spec or Phase 0 Scope" — none were added. An agent executing Phase 0 will hit all five of these immediately.

---

## 3. New Issues Found

### 🔴 CRITICAL — Will Block or Break Implementation

#### C1: Retrospective Buffer Size — 60 Seconds vs 30 Seconds

**The conflict:**
- §1.1 Goals, line 39: *"Implement a lock-free circular buffer (~60s)."*
- §3.10 Retrospective Capture, line 561: *"Approximately **30 seconds** of stereo audio."*

These are in the same document. The pre-allocation math in §3.10 uses 30 seconds (48kHz × 2 × 4 × 30s ≈ 11.5 MB). If the correct value is 60 seconds, the allocation doubles to ~23 MB and the maximum capturable content changes.

**Why this is critical:** The buffer size determines how many bars of audio can be captured retroactively. At 120 BPM, 30 seconds = 60 bars. At 60 BPM, 30 seconds = 30 bars. But at 20 BPM (minimum), 8 bars = 96 seconds — **neither 30 nor 60 seconds is enough for 8 bars at low tempos**. The PBA3 resolution (H3) described a 30-second buffer, but §1.1 still says 60s.

> **Recommendation:** 
> 1. Pick one value and make both sections match.
> 2. Consider that 8 bars at 20 BPM = 96 seconds. If the max capturable is truly 8 bars, the buffer must be at least 96 seconds. Alternatively, document that at very low tempos, the maximum capturable bars will be less than 8 (the buffer holds a fixed time, not a fixed bar count). 
> 3. Update the memory math accordingly.

---

#### C2: Phase Numbering Mismatch Between §8.6 and §9

**The conflict:** The "Recommended Bead Ordering" (§8.6) and the "Task Breakdown" (§9) define different phase structures:

| Phase # | §8.6 (Bead Ordering) | §9 (Task Breakdown) |
|:---|:---|:---|
| 0 | Skeleton | Project Skeleton |
| 1 | Contracts | Contracts & Foundations |
| 2 | Engine Core | Engine Core |
| 3 | **UI Shell** | **Frontend Infrastructure** |
| 4 | **Audio Engines** | **Internal Audio Engines** |
| 5 | **Integration** | **UI Components** ← NEW |
| 6 | **Plugin Isolation** | **Integration** |
| 7 | **Polish** | **Plugin Isolation** |
| 8 | *(none)* | **Polish & Production** |

§8.6 has **8 phases (0-7)**. §9 has **9 phases (0-8)**. The difference: §9 splits UI work into two phases (Phase 3: Frontend Infrastructure + Phase 5: UI Components), while §8.6 doesn't. This cascades — Integration, Plugin Isolation, and Polish are all numbered differently.

The human testing checkpoints (§8.8) reference "Phase 8" (Checkpoint 5), which aligns with §9 but not §8.6.

**Why this is critical:** An agent reading §8.6 to understand phase ordering will get a different plan than an agent reading §9. Since both are in the same spec, this will cause confusion about what "Phase 5" means.

> **Recommendation:** Align §8.6 to match §9 exactly (9 phases, 0-8). §9 is more detailed and should be authoritative — update §8.6's phase tree to mirror it.

---

#### C3: `TRIGGER_SLOT` Command — Behavior Completely Undefined

§3.2 defines: `{ cmd: 'TRIGGER_SLOT'; slot: number; quantized: boolean }`

The error matrix (§3.3) says it can return `2010: SLOT_BUSY`.

But **nowhere in the spec** is `TRIGGER_SLOT`'s actual behavior described. Under the retrospective capture model (§3.10), audio is committed via `COMMIT_RIFF`. So what does `TRIGGER_SLOT` do?

Possible interpretations:
- **(A)** It's a legacy command from a pre-retrospective design and should be removed.
- **(B)** It starts/stops playback of a specific slot (but that's covered by MUTE/UNMUTE).
- **(C)** It arms a specific slot for the next commit (but §3.10 says "next available slot" is automatic).
- **(D)** It triggers a one-shot sample playback in the SampleEngine.
- **(E)** It's used to re-trigger a slot from the beginning of its loop (reset playhead position).

**Why this is critical:** An agent implementing the CommandDispatcher (Task 2.2) must write a handler for `TRIGGER_SLOT`. Without knowing what it does, the handler will either be wrong or empty.

> **Recommendation:** Either define `TRIGGER_SLOT`'s behavior explicitly (what state transition does it cause? what audio operation does it perform?) or remove it from the schema if it's vestigial.

---

#### C4: SampleEngine Has No Commands and No AppState

The Audio Engine Spec §5 defines a complete SampleEngine with:
- 4 playback modes (One-Shot, Gated, Looped, Sliced)
- 6+ parameters (Pitch, Start Offset, Loop Points, ADSR, Filter, Reverse)
- Drag-and-drop / file picker
- 500MB RAM allocation limit
- JSON preset system

The Command Schema (§3.2) has `SELECT_MODE { category: 'sampler' }` to enter sampler mode, but **zero commands** for controlling the sampler:
- No `LOAD_SAMPLE` — how does the user load a sample?
- No `SET_SAMPLE_MODE` — how does the user switch between One-Shot/Gated/Looped/Sliced?
- No `SET_SAMPLE_PARAMETER` — how does the user adjust Start Offset, Loop Points, Reverse?
- No `SET_SAMPLE_PAD` — how does the user assign a sample to a specific pad (Sliced mode)?

The AppState (§3.4) has **no sampler section** — there's no way for the UI to know what sample is loaded, which mode is active, or what the parameter values are.

**Why this is critical:** The SampleEngine is listed as Task 4.6 in the build plan. An agent cannot implement it without commands and state. This is an entire subsystem with no protocol defined.

> **Recommendation:** Either:
> - **(A)** Add sampler commands to §3.2 and sampler state to §3.4. At minimum: `LOAD_SAMPLE { padIndex, filePath }`, `SET_SAMPLER_MODE { mode }`, `SET_SAMPLER_PARAM { param, value }`. Add a `sampler` section to AppState.
> - **(B)** Defer the SampleEngine entirely to V2. Remove Task 4.6 and the "Sampler" category from the Mode selector. Move it to §1.3 Future Goals. This simplifies V1 significantly.

---

### 🟠 HIGH — Will Cause Implementation Confusion

#### H1: `COMMIT_RIFF` Has Dual Purpose — Capture + History Save

§3.2 defines `COMMIT_RIFF` with the comment: *"Save current state to riff history."*

But §3.10 (Retrospective Capture) step 3 says: *"When the user triggers `COMMIT_RIFF`, the most recent N bars of audio... are copied from the retrospective buffer into the next available slot."*

So `COMMIT_RIFF` does **two things**:
1. Copies audio from the retrospective buffer into the next empty slot (creating a new loop layer)
2. Saves the resulting state to riff history

These are semantically very different operations. An agent implementing this must understand that a single command triggers both audio buffer extraction AND state snapshot creation. The comment in §3.2 only describes #2, which will mislead.

Additionally, should every commit to a slot also create a riff history entry? Or are there cases where audio is committed to a slot without creating a history entry? (For example, during auto-merge, step 5 says "auto-commit to Riff History" as a separate action from the merge itself.)

> **Recommendation:** Update the §3.2 comment for `COMMIT_RIFF` to: *"Capture audio from retrospective buffer into the next empty slot, then save the resulting session state as a new entry in riff history."* This makes the dual behavior explicit.

---

#### H2: `STOP` vs `PAUSE` — No Behavioral Distinction Defined

§3.2 defines three transport commands: `PLAY`, `PAUSE`, `STOP`.

The AppState transport section (§3.4) only has `isPlaying: boolean`. There is no `isPaused` or `isStopped` state — both PAUSE and STOP presumably set `isPlaying = false`.

In a traditional DAW:
- PAUSE = stop, keep playhead position
- STOP = stop, reset playhead to beginning

But FlowZone is a loop machine — there's no "beginning" to reset to. All audio loops. So what does STOP do differently from PAUSE? Should STOP reset the bar phase to 0? Should STOP silence all output while PAUSE holds the last sample? Should STOP clear the retrospective buffer?

The error matrix (§3.3) treats both identically: "None (always succeeds)" / "Optimistic update."

> **Recommendation:** Either:
> - **(A)** Define distinct behaviors (e.g., PAUSE = hold position, STOP = reset bar phase to 0.0 + optionally clear retro buffer). Add `transportState: 'playing' | 'paused' | 'stopped'` to AppState to distinguish.
> - **(B)** Remove `STOP` entirely and keep only `PLAY` / `PAUSE`. A loop machine arguably only needs play/pause. This is simpler.

---

#### H3: "Playback Controls" — 8 Circular Buttons Undefined

§7.6.3 Play Tab Layout specifies under both Keymasher and XY Pad layouts:

> **Playback Controls** (Both Layouts): Layout: 2 rows × 4 circular buttons. Position: Below effect control area. Visual: Circular buttons with fill/arc indicators showing state.

That's **8 circular buttons** whose function is never defined. What do they control? Possible interpretations:
- Transport controls (play/stop/record — but those are in the header)
- Per-slot mute/unmute toggles (but there are 8 slots, which would match 8 buttons)
- Loop length selectors
- Effect parameter presets
- Completely unclear

The UI_Layout_Reference.md confirms the layout but adds no information: `"playback_controls": { "layout": "2_rows_of_4_circular_buttons", "position": "below_button_grid", "note": "circular buttons with fill indicators" }`.

> **Recommendation:** Define what these 8 buttons do. If they are slot activity indicators/toggles (showing which slots have content and allowing mute/unmute), say so explicitly. If they're something else, describe the function.

---

#### H4: Pad-to-Note Mapping for Melodic Modes Undefined

When in Notes or Bass mode, the 4×4 pad grid triggers notes. §3.4 transport state includes `rootNote` (0-11) and `scale` (e.g., `'minor_pentatonic'`).

But the spec never defines:
1. **Which notes map to which pads.** A minor pentatonic scale has 5 notes per octave. With 16 pads, does the grid span 3+ octaves? Which pad is the root?
2. **The pad layout orientation.** Is pad (1,1) the lowest note (bottom-left, like a keyboard) or the highest (top-left, like MPC)?
3. **Available scale types.** The command `SET_KEY` accepts a `scale: string` but valid scale names are never enumerated. Only "minor_pentatonic" appears as an example.
4. **How Pitch knob interacts.** The Adjust tab's Pitch knob transposes (-24 to +24 semitones). Does this shift the entire grid or just the last-triggered note?

> **Recommendation:** Add a subsection to §7.6.2 or §3.10 defining:
> - The pad-to-note mapping algorithm (e.g., "Pads are laid out bottom-left = root note, ascending chromatically/scalewise left-to-right, bottom-to-top")
> - A list of valid scale names for the `SET_KEY` command
> - How transpose (Pitch knob) affects the grid

---

#### H5: Slot State Transitions for MUTED Slots During Transport Stop/Start

§3.10 defines transitions:
```
PLAYING → STOPPED (transport stopped)
STOPPED → PLAYING (transport started)
PLAYING → MUTED (user mutes)
MUTED → PLAYING (user unmutes)
```

**Missing transitions:**
- What happens to a **MUTED** slot when transport stops? Does it become STOPPED? Or stay MUTED?
- If MUTED slots stay MUTED during transport stop, what happens when transport restarts? Do they stay MUTED (correct) or switch to PLAYING (incorrect)?

The expected behavior is: MUTED persists regardless of transport state. Only PLAYING ↔ STOPPED responds to transport. But this isn't stated.

> **Recommendation:** Add to §3.10:
> - *"MUTED slots are not affected by transport start/stop. Only PLAYING ↔ STOPPED transitions respond to transport. A MUTED slot remains MUTED regardless of transport state. The user must explicitly UNMUTE."*

---

#### H6: PBA3 Reliability Items Must Be Addressed Before Phase 0

Five PBA3 reliability items (R1, R2, R3, R5, R7 — see §2 above) remain unresolved. These aren't theoretical — they will block the very first bead (Phase 0: Project Skeleton).

Specifically:
- **R1 (CivetWeb):** The Phase 0 agent needs to integrate a WebSocket server. Without guidance, they'll spend time researching options.
- **R2 (WebBrowserComponent):** The Phase 0 agent needs to load the React app. Without dev/prod URL advice, they'll make potentially wrong choices.
- **R3 (MP3):** Less urgent (DiskWriter is Task 2.5), but should be corrected now to avoid confusion later.

> **Recommendation:** Resolve these five items in the spec before bead creation begins. Estimated time: 30 minutes total.

---

### 🟡 MEDIUM — Should Fix Before Beads

#### M1: Audio_Engine_Specifications.md References Old Tab Name "Sound/FX Tab"

Audio_Engine_Specifications.md §1 Implementation Note:

> *"These effects support all UI elements defined in §7.6.3 (Sound/FX Tab) of the main spec."*

But §7.6.3 was renamed from "Sound Tab" to "Play Tab" (as noted in §7.6.3 itself: "This tab was formerly labeled 'Sound' in reference designs. Renamed to 'Play'").

> **Recommendation:** Update the Audio Engine Spec reference to say "§7.6.3 (Play Tab)".

---

#### M2: §7.6.6 Title "Microphone Tab Layout" Is Misleading

§7.6.6 is titled "Microphone Tab Layout" but Microphone is not a tab — it's a **category** within the Mode tab. The content describes what appears when the Microphone category is selected. Calling it a "tab" will confuse agents into thinking there's a 5th navigation tab.

> **Recommendation:** Rename to "Microphone Category View" or "Microphone Mode Layout."

---

#### M3: First Launch / Empty Jam Manager Behavior Undefined

§7.7 describes the Home Screen (Jam Manager) with session listing, search, and CRUD operations. §7.6.1 header has a Home button that navigates there.

But the spec doesn't define:
1. **What happens on first launch?** No jams exist. Does the Jam Manager show with a "Create your first jam" prompt? Or does the app auto-create a jam and skip the Jam Manager entirely?
2. **Is the Jam Manager the first screen?** Or does the app launch directly into the last-used jam (per auto-save in §2.2.G)?
3. **What does the Jam Manager look like with zero entries?** Empty state UI is notoriously forgotten.

> **Recommendation:** Add to §7.7:
> - On first launch: auto-create a new jam and enter it directly (skip Jam Manager).
> - On subsequent launches: load the last-used jam automatically (per SessionStateManager auto-save).
> - Jam Manager is accessed via the Home button. When empty (all jams deleted), show a centered "New Jam" button with an encouraging prompt.

---

#### M4: FX Mode "Delete Source Slots" — What Happens to Audio Files?

§7.6.2 FX Mode step 6: *"The source slots that were selected are then **deleted** (destructive)."*

§2.2.G says: *"Audio files are **never deleted immediately** — only marked for garbage collection on clean exit."*

Questions:
1. Does "deleted" mean setting the slot state to `EMPTY`? (Presumably yes, but should be explicit.)
2. The audio files for the deleted slots remain on disk (per GC policy), but can they still be recovered via Riff History? The spec says the pre-commit state is saved to history (step 5 of auto-merge, referenced in step 7), so presumably yes.
3. If FX Mode doesn't trigger auto-merge (i.e., not all 8 slots are full), is the pre-FX state still auto-committed to riff history before the destructive delete?

> **Recommendation:** Clarify in §7.6.2 FX Mode step 6:
> - *"Source slots are set to EMPTY state. Their audio files remain on disk per the garbage collection policy (§2.2.G)."*
> - *"Before the destructive operation, the current session state is automatically committed to riff history (same as auto-merge step 5), ensuring the pre-FX state can always be recovered."*

---

#### M5: Multi-Channel VST3 Output Bus Configuration Unspecified

§1.1 and §3.4 note say: *"8 stereo pairs (16 channels) for DAW recording via the VST3 plugin target. Slot N → Output Pair N."*

But the spec doesn't define how to declare these bus layouts in JUCE. A `juce::AudioProcessor` needs explicit bus configuration in `BusesProperties` / `isBusesLayoutSupported()`. The agent building Task 0.1 (Projucer setup) or Task 2.3 (FlowEngine skeleton) needs to know:
- How many output buses to declare
- Whether to use a single 16-channel bus or 8 separate stereo buses
- Whether the bus layout differs between Standalone and VST3 targets

> **Recommendation:** Add a brief note to §2.2.A:
> - *"VST3 mode: Declare 8 stereo output buses in `BusesProperties`. Standalone mode: Single stereo output bus (master only). Use `#if JucePlugin_Build_VST3` preprocessor guards for the bus configuration."*

---

### 🟢 LOW — Worth Noting

#### L1: Drum Kit Preset Grid — 4 Presets in 12-Cell Grid

PBA3 marked this "accepted as-is" but it's still potentially confusing for agents. The preset selector is a 3×4 grid (12 cells) for all categories. Drums only have 4 kit presets — leaving 8 cells empty. Notes and Bass each fill all 12.

This isn't a bug, but an agent building the Preset Selector component (Task 5.x) needs to handle the case where fewer than 12 presets exist. The grid should show empty/disabled cells.

> **Recommendation:** Add a one-line note to §7.6.2 Preset Selector: *"Grid cells beyond the available preset count are rendered as empty/disabled."*

---

#### L2: Adjust Tab Row 2 Col 3 "Reserved" — Agent Guidance

§7.6.4 shows `(reserved)` for Row 2, Col 3 of the knob grid. The UI_Layout_Reference has `{ "label": "reserved", "type": "empty" }`.

An agent needs to know: render nothing? A disabled/greyed-out knob? Empty space?

> **Recommendation:** Clarify as: *"Row 2, Col 3: Empty cell (no control rendered). Reserved for future parameter."*

---

#### L3: `SET_KEY` Scale String Values Not Enumerated

The command `SET_KEY { rootNote: number; scale: string }` accepts a `scale` string, but valid values are never listed. Only `'minor_pentatonic'` appears as an example. The error matrix says `2030: INVALID_SCALE` can be returned, implying validation occurs.

> **Recommendation:** Add a list of valid scale values, e.g.:
> ```typescript
> type Scale = 'major' | 'minor' | 'minor_pentatonic' | 'major_pentatonic' 
>            | 'dorian' | 'mixolydian' | 'blues' | 'chromatic';
> ```

---

## 4. Cross-Document Consistency Matrix

Verified alignment across all three spec documents:

| Aspect | Spec 1.6 | Audio Engine Spec | UI Layout Ref | Status |
|:---|:---|:---|:---|:---|
| **Tab labels** | Mode, Play, Adjust, Mixer (§7.6) | References "Sound/FX Tab" ← old name | Mode, Play, Adjust, Mixer | ⚠️ M1 |
| **Core FX list** | 12 effects (§2.2.M) | 12 effects (§1.1), names match | 12 effects (play_tab), names match | ✅ |
| **Infinite FX list** | 11 effects (§2.2.M) | 11 effects (§1.2), names match | 11 effects (play_tab), names match | ✅ |
| **Keymasher buttons** | 12 (§3.2 `KeymasherButton`) | 12 (§1.2 Keymasher), names match | 12 (play_tab), names match | ✅ |
| **Notes presets** | 12 (§2.2.M) | 12 (§2.1), names defined | "See Audio_Engine_Specs" | ✅ |
| **Bass presets** | 12 (§2.2.M) | 12 (§3.1), names defined | "See Audio_Engine_Specs" | ✅ |
| **Drum kits** | 4 kits (§2.2.M) | 4 kits (§4.2) | Pad grid icons defined | ✅ |
| **Drum pad icons** | §7.6.2 icon list | §4.1 icon-to-sound map | Pad grid icons match | ✅ |
| **XY Pad behavior** | Touch-and-hold (§7.6.3) | XY Mapping per effect (§1) | Crosshair on hold | ✅ |
| **Adjust knobs** | 7 + 2 reverb (§7.6.4) | Parameter mapping (§2.2) matches | 7 + reverb section | ✅ |
| **Mic adjust controls** | Reverb Mix + Room Size + Gain (§7.6.4) | Built-in reverb only V1 (§6.3) | Monitor toggles + Gain | ✅ |
| **Mixer transport** | 2×3 grid (§7.6.5) | N/A | 2×3 grid, controls match | ✅ |
| **Settings tabs** | 4 tabs (§7.6.8) | N/A | 4 tabs, controls match | ✅ |
| **Riff Swap Mode** | In settings (§7.6.8) | N/A | In `user_preferences` section | ✅ (fixed from PBA3) |
| **Error codes** | Full list (§5.1) | N/A | N/A | ✅ |
| **`PluginInstance` interface** | Defined in §3.4 | N/A | N/A | ✅ (fixed from PBA3) |
| **`JsonPatchOp` type** | Defined in §3.2 | N/A | N/A | ✅ (fixed from PBA3) |
| **Slot states** | `EMPTY \| PLAYING \| MUTED \| STOPPED` | N/A | N/A | ✅ (RECORDING removed per PBA3) |
| **Retro buffer size** | §1.1: ~60s / §3.10: ~30s | N/A | N/A | ❌ C1 |
| **Phase structure** | §8.6: 8 phases (0-7) | N/A | N/A | ❌ C2 |
| **Phase structure** | §9: 9 phases (0-8) | N/A | N/A | ❌ C2 |
| **Binary stream** | Single WS connection (§3.7) | N/A | N/A | ✅ (fixed from PBA3) |
| **SampleEngine commands** | `SELECT_MODE { 'sampler' }` only | Full subsystem (§5) | N/A | ❌ C4 |
| **TRIGGER_SLOT** | In schema (§3.2) | N/A | N/A | ❌ C3 |
| **Color palette** | §7.1 + §7.3 JSON | N/A | bg/panel/accent colors | ✅ |

---

## 5. Reliability Risk Assessment (New + Unresolved)

### R-NEW-1: `COMMIT_RIFF` in FX Mode vs Normal Mode — Different Behaviors, Same Command

In **Normal Mode**, `COMMIT_RIFF` copies from the retrospective buffer into the next empty slot and saves to riff history.

In **FX Mode**, `COMMIT_RIFF` triggers a completely different workflow: it captures the FX-processed audio for one loop cycle, writes it to the next empty slot, **deletes the source slots**, and saves to history. The pre-delete state is auto-committed first.

This means the `CommandDispatcher` must check `activeMode.isFxMode` to determine which code path to execute. A single command with two radically different behaviors (one of which is destructive) is a prime source of bugs.

> **Recommendation:** Either:
> - **(A)** Add a note to §6.1 (How to Add a Command) or §7.6.2 FX Mode: *"COMMIT_RIFF dispatches to `handleCommitRiff()` which internally checks `isFxMode` to determine the capture-only vs capture-and-delete-sources path."*
> - **(B)** Create a separate command `COMMIT_FX_RESAMPLE` for FX Mode commits, making the two behaviors explicit and independently testable.

Option (B) is more reliable for a non-engineer owner — separate commands are easier to debug than hidden mode-dependent branches.

### R-NEW-2: No Command to Clear/Reset a Single Slot

The slot states include `EMPTY`, and auto-merge/FX Mode can set slots to `EMPTY`. But there's no user-facing command to clear a single slot. If a user records a bad loop into Slot 3, they cannot delete just that slot — they'd need to load a previous riff from history (which replaces ALL slots) or wait for auto-merge.

This is a deliberate design choice ("destructive creativity"), but it may frustrate users in practice. Worth noting as a conscious trade-off.

> **Recommendation:** Add a brief note to §3.10 or §7.6.5: *"V1 does not include a per-slot clear command. Users who want to remove a single layer should load a previous riff from history. A per-slot clear could be added as a V2 feature."*

### R-NEW-3: Scale of Audio Engine Implementation — Reality Check

The Audio Engine Spec defines:
- **23 effects** (12 Core + 11 Infinite), each with 2-4 parameters
- **24 synth presets** (12 Notes + 12 Bass), each with unique oscillator configs
- **64 drum sounds** (16 per kit × 4 kits)
- **1 sample engine** with 4 playback modes

That's approximately **110+ distinct audio processing implementations**. Even as "simple procedural implementations for V1," this is a massive surface area. For context, a single well-tested synth preset with proper ADSR + filter + LFO is ~200-400 lines of C++. At 110 implementations, that's potentially 22,000-44,000 lines of audio code.

This isn't a spec inconsistency — it's a **feasibility concern**. The risk mitigations (§8.3 M6) acknowledge this and recommend grouping by implementation similarity. But the sheer volume should be highlighted:

> **Recommendation:** Consider whether V1 truly needs all 110+ implementations. A lean V1 could ship with:
> - 6 Core FX (instead of 12) — the most distinctive ones
> - 6 Infinite FX (instead of 11) — including Keymasher
> - 6 Notes presets + 6 Bass presets (instead of 12 each)
> - 2 Drum kits (instead of 4) — Synthetic + Electronic
> - SampleEngine deferred to V2
>
> This would cut the audio implementation surface by ~50% while still delivering a fully functional product. The remaining presets/effects can be added incrementally post-launch.
>
> If the full 110+ is desired for V1, the Phase 4 task breakdown should be expanded from 6 tasks to ~12-15, with explicit groupings.

---

## 6. Summary of All Findings

### 🔴 Critical — Must Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| C1 | Retro buffer 60s vs 30s conflict | Pick one value, update both §1.1 and §3.10, recalculate memory math | 10 min |
| C2 | Phase numbering mismatch §8.6 vs §9 | Update §8.6 to match §9 (9 phases, 0-8) | 15 min |
| C3 | `TRIGGER_SLOT` behavior undefined | Define behavior or remove from schema | 10 min |
| C4 | SampleEngine has no commands/state | Add commands + state, OR defer to V2 | 30 min (add) or 10 min (defer) |

### 🟠 High — Should Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| H1 | `COMMIT_RIFF` dual purpose unclear | Update §3.2 comment to describe both operations | 5 min |
| H2 | `STOP` vs `PAUSE` no distinction | Define difference or remove `STOP` | 10 min |
| H3 | "Playback Controls" 8 buttons undefined | Define what the buttons do | 10 min |
| H4 | Pad-to-note mapping undefined | Add scale mapping + valid scales list | 15 min |
| H5 | MUTED slot behavior during transport stop | Add explicit transition note | 5 min |
| H6 | PBA3 reliability items R1/R2/R3/R5/R7 unresolved | Apply the fixes PBA3 recommended | 30 min |

### 🟡 Medium — Should Fix

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| M1 | Audio Engine Spec references old tab name | Change "Sound/FX Tab" → "Play Tab" | 2 min |
| M2 | §7.6.6 titled "Microphone Tab" (it's a category) | Rename section title | 2 min |
| M3 | First launch / empty Jam Manager behavior | Add first-launch + empty-state behavior | 10 min |
| M4 | FX Mode "delete" semantics unclear | Clarify = EMPTY + audio file policy + auto-commit | 10 min |
| M5 | VST3 multi-channel bus config unspecified | Add bus layout guidance to §2.2.A | 5 min |

### 🟢 Low — Nice to Have

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| L1 | Drum kit grid empty cells | Add note about empty cell rendering | 2 min |
| L2 | Adjust tab "reserved" cell rendering | Clarify as empty cell | 2 min |
| L3 | Scale string values not enumerated | Add valid scale type list | 5 min |

### Reliability Items

| # | Risk | Action |
|:--|:-----|:-------|
| R-NEW-1 | `COMMIT_RIFF` mode-dependent behavior | Add explicit note or create separate `COMMIT_FX_RESAMPLE` command |
| R-NEW-2 | No per-slot clear command | Document as conscious V1 trade-off |
| R-NEW-3 | 110+ audio implementations | Consider lean V1 scope or expand task breakdown |

---

## 7. Overall Assessment

| Dimension | Rating | Notes |
|:---|:---|:---|
| **Completeness** | 🟡 **90%** | Excellent overall, but SampleEngine (C4) is a complete subsystem with no protocol, and TRIGGER_SLOT (C3) is undefined. |
| **Internal Consistency** | 🟡 **85%** | Most cross-references align. The buffer size conflict (C1), phase mismatch (C2), and old tab name reference (M1) are clear inconsistencies. |
| **Agent Buildability** | 🟢 **90%** | Phase breakdown, risk mitigations, testing strategy, and human checkpoints are excellent. The undefined behaviors (C3, H2, H3, H4) will cause agents to guess. |
| **Reliability for Non-Engineer** | 🟡 **82%** | CrashGuard, auto-save, graduated safe mode, and tiered disk failure are well-designed. But the unresolved PBA3 items (H6), mode-dependent COMMIT_RIFF (R-NEW-1), and the scale of audio implementation (R-NEW-3) create risk areas a non-engineer can't debug. |

**Verdict:** The spec is very close to bead-ready. Fix the 4 critical items (C1-C4, total ~45-65 min depending on SampleEngine decision) and the high-priority items (H1-H6, total ~75 min), and the project is ready for bead creation with high confidence. The PBA3 unresolved reliability items (H6) should be applied simultaneously.

**Total estimated spec update time for all items: ~2.5 hours**

**Recommended priority order for fixes:**
1. C1 (buffer size — 10 min, trivial but blocks retro buffer implementation)
2. C2 (phase mismatch — 15 min, agents will read both sections)
3. C3 (TRIGGER_SLOT — 10 min, define or remove)
4. C4 (SampleEngine — decision needed: add commands or defer to V2)
5. H6 (PBA3 items — 30 min, blocks Phase 0)
6. H1-H5 (remaining high items — 45 min)
7. M1-M5, L1-L3 (medium/low — 30 min, can be done alongside bead creation if needed)

---

## Appendix: Resolutions Applied

All findings from this assessment have been resolved in the spec documents per owner decisions. Summary of changes:

| # | Issue | Owner Decision | Resolution | Files Modified |
|:--|:------|:---------------|:-----------|:---------------|
| **C1** | Retro buffer 60s vs 30s | Go for 96 seconds | Buffer set to ~96s (enough for 8 bars at 20 BPM). Memory math updated. §1.1 and §3.10 now consistent. | `Spec 1.6` §1.1, §3.10 |
| **C2** | Phase numbering mismatch | Align §8.6 to §9 | §8.6 now has 9 phases (0-8) matching §9 exactly. Names aligned. | `Spec 1.6` §8.6 |
| **C3** | `TRIGGER_SLOT` undefined | Remove — COMMIT_RIFF is the only capture command | Removed from Command Schema and Error Matrix. | `Spec 1.6` §3.2, §3.3 |
| **C4** | SampleEngine has no commands | Defer to V2 | Moved to §1.3 Future Goals. §2.2.M marked V2. Task 4.6 removed from §9. Sampler removed from Mode Tab category grid. | `Spec 1.6` §1.3, §2.2.M, §7.6.2, §9; `UI_Layout_Reference.md` |
| **H1** | `COMMIT_RIFF` dual purpose unclear | Clarify — it captures from buffer AND saves to history. Available from Mixer. | Updated §3.2 comment with full description of dual behavior. | `Spec 1.6` §3.2 |
| **H2** | `STOP` vs `PAUSE` no distinction | Play/Pause only. Remove STOP. | Removed `STOP` from Command Schema and Error Matrix. Transport uses `isPlaying: boolean`. | `Spec 1.6` §3.2, §3.3 |
| **H3** | "Playback Controls" 8 buttons undefined | Oblong slot indicators. FX mode: source selection. Other modes: mute toggles. | Renamed to "Slot Indicators", defined as 1×8 oblongs with dual behavior. | `Spec 1.6` §7.6.3; `UI_Layout_Reference.md` |
| **H4** | Pad-to-note mapping undefined | Add mapping: bottom-left = root, ascending through scale | Added pad-to-note mapping algorithm, octave spanning, and Pitch knob interaction. Added `Scale` type with 8 valid values. | `Spec 1.6` §7.6.2, §3.2 |
| **H5** | MUTED slot during transport stop | Remove STOPPED state entirely. MUTED stays MUTED. PLAYING pauses in place. | Slot states now `EMPTY \| PLAYING \| MUTED`. Transitions simplified. Transport pause/resume does not change slot state. | `Spec 1.6` §3.4, §3.10 |
| **H6** | PBA3 reliability items unresolved | Fix all five | R1: CivetWeb guidance added to Phase 0. R2: Remote WebServer moved to V2. R3: MP3 moved to V2. R5: FLAC compression level 0. R7: Ext Inst/FX "Coming Soon" placeholder. | `Spec 1.6` §1.1, §1.3, §3.6, §7.6.2, §8.6, §9 |
| **M1** | Audio Engine Spec old tab name | Fix | Changed "Sound/FX Tab" → "Play Tab". | `Audio_Engine_Specifications.md` §1 |
| **M2** | §7.6.6 titled "Microphone Tab" | Rename | Changed to "Microphone Category View". | `Spec 1.6` §7.6.6 |
| **M3** | First launch behavior undefined | Jam Manager is first screen. "Create your first jam" prompt. | Added first-launch behavior and empty state UI to §7.7. | `Spec 1.6` §7.7 |
| **M4** | FX Mode delete semantics unclear | Slots set to EMPTY. Audio on disk. Pre-FX state is current riff in history. | Clarified step 6 with explicit state/file/history behavior. | `Spec 1.6` §7.6.2 |
| **M5** | VST3 bus config unspecified | Add guidance | Added bus layout note (8 stereo buses VST3, 1 stereo Standalone). | `Spec 1.6` §2.2.A |
| **L1** | Drum kit grid empty cells | Accept, add note | Added note about empty/disabled cells in preset selector. | `Spec 1.6` §7.6.2 |
| **L2** | Adjust tab reserved cell | Clarify | Updated to "empty — reserved for future parameter". | `Spec 1.6` §7.6.4 |
| **L3** | Scale string values not listed | Add type | Added `Scale` type with 8 valid values. Added to error matrix. | `Spec 1.6` §3.2, §3.3 |
| **R-NEW-2** | No per-slot clear | Document as conscious trade-off | Added note to §3.10 documenting V1 limitation and riff history workaround. | `Spec 1.6` §3.10 |

### Additional Changes (Audio Engine Spec)

| Change | Description | Files Modified |
|:-------|:------------|:---------------|
| **SampleEngine → V2** | Implementation note changed from "Future-Proof Architecture" to "V2 Feature" with reference to V1 procedural-only approach. | `Audio_Engine_Specifications.md` §5 |
| **MicProcessor FX wording** | Clarified that wet signal is what gets recorded. | `Audio_Engine_Specifications.md` §6.3 |
| **Pre-fader terminology** | Already fixed in PBA3. Confirmed present. | `Audio_Engine_Specifications.md` §6.3 |

---

*End of Pre-Build Assessment 4*
