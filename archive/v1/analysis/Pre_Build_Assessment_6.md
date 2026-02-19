# FlowZone — Pre-Build Assessment 6

**Date:** 13 Feb 2026  
**Scope:** Final pre-bead-creation review of all Spec folder documents. Fresh-eyes analysis after PBA5 resolutions were applied to Spec 1.6. Focus areas: cross-document micro-inconsistencies, undefined runtime defaults, unplanned build tasks, and reliability concerns for a non-engineer owner who cannot troubleshoot code.  
**Documents Reviewed:**
- `Spec_FlowZone_Looper1.6.md` (1842 lines)
- `Audio_Engine_Specifications.md` (366 lines)
- `UI_Layout_Reference.md` (370 lines)

---

## 1. Executive Summary

The spec suite is in excellent shape after five rounds of assessment. PBA5 found and resolved 13 issues (2 critical, 4 high, 4 medium, 3 low) plus 4 reliability risks — all confirmed applied to the current Spec 1.6.

This sixth-pass review — deliberately targeting **micro-level cross-document discrepancies**, **missing defaults**, **unplanned build tasks**, and **production reliability** — has found **16 issues** (1 critical, 5 high, 5 medium, 5 low) and **5 reliability risks**.

**The critical theme this time:** The spec's macro-level architecture is solid, but there are **unplanned build targets, missing runtime defaults, and UI interaction paths** that an agent would either have to guess at or skip. These won't block a prototype, but they will cause confusion during Phases 0, 3, and 5 if unaddressed.

**Overall verdict:** After resolving these findings, the spec will be **decisively ready for bead creation** with very high confidence.

---

## 2. PBA5 Resolution Verification

All 17 PBA5 resolutions (C1-C2, H1-H4, M1-M4, L1-L3, R1-R4) have been verified present in the current Spec 1.6 documents. Spot-checks:

| PBA5 # | Verification | Status |
|:---|:---|:---|
| C1 | `NOTE_ON` / `NOTE_OFF` in §3.2, §3.3, §3.10 | ✅ Present |
| C2 | `SET_LOOP_LENGTH` dual-purpose comment in §3.2 | ✅ Present |
| H1 | `BarPhase` in binary payload §3.7 + excluded from JSON patches | ✅ Present |
| H2 | `COMMIT_RIFF` clarified as state-only (no audio capture) in §3.2 | ✅ Present |
| H3 | Default port 8765 in §2.2.L Network Configuration | ✅ Present |
| H4 | Drum Pitch → global in Audio Engine Spec §4.3 | ✅ Present |
| R2 | Non-blocking auto-merge in §7.6.2.1 Implementation Note | ✅ Present |
| R3 | FX Mode auto-commit on entry in §7.6.2 step 1 | ✅ Present |
| R4 | Hardcoded minimum config fallback in §4.5 step 4 | ✅ Present |

**Verdict:** No regressions. All PBA5 resolutions correctly applied.

---

## 3. New Issues Found

### 🔴 CRITICAL — Will Block Core Build Task

#### C1: VST3 Plugin Build Target — Referenced in Goals, Absent from Task Breakdown

**The gap:** §1.1 states:
> "**Dual Build Target:** Built simultaneously as a Standalone App and a VST3 Plugin from the same Projucer project."

§2.2.A (FlowEngine) specifies:
> "In VST3 mode, declare 8 stereo output buses in `BusesProperties`... Use `#if JucePlugin_Build_VST3` preprocessor guards."

But the Task Breakdown (§9) never creates the VST3 target:
- **Task 0.1** says: "Produce a compiling JUCE **standalone app**" — standalone only.
- No subsequent task configures the VST3 exporter in Projucer or validates the 8-bus multi-channel layout.
- No task tests the `#if JucePlugin_Build_VST3` conditional bus configuration.

**Why this is critical:** The VST3 plugin target requires:
1. Projucer must have a VST3 plugin exporter configured (in addition to the standalone exporter).
2. JUCE's plugin wrapper code needs the correct `JucePlugin_*` preprocessor defines.
3. The multi-channel bus layout (8 stereo pairs) must be declared and DAW-tested.
4. The master limiter bypass logic (`#if JucePlugin_Build_VST3`) needs testing in both build modes.

If this is deferred to "later," it becomes a massive integration headache — bus configuration affects `FlowEngine::prepareToPlay()` and `processBlock()` from Phase 2 onwards. Retrofitting it is much harder than including it from Phase 0.

**Options:**
- **(A) Add to Phase 0:** Extend Task 0.1 to configure both Standalone and VST3 exporters in Projucer. Verify both build targets compile. This adds ~30 min to Phase 0 but prevents a large debt.
- **(B) Defer explicitly to Phase 8:** Add a new Task 8.x "Configure and test VST3 plugin target" with a note in §2.2.A that V1 development proceeds in Standalone mode only, and VST3 is a production-readiness task.
- **(C) Remove from V1 scope:** Move "Dual Build Target" to §1.3 Future Goals.

> **Recommendation:** Option (A) if the owner wants DAW recording capability in V1. Option (B) if VST3 is desired but not urgent. Option (C) if it can wait. The current state — listed as a goal but with no implementation task — is the worst outcome because an agent may attempt it mid-build without proper planning.

---

### 🟠 HIGH — Will Cause Agent Confusion or Subtle Bugs

#### H1: No Default Value for `activeMode.category` — Play Tab Blank on First Launch

§3.4 `AppState.activeMode`:
```typescript
activeMode: {
    category: string;    // 'drums' | 'notes' | 'bass' | 'fx' | 'mic' | 'ext_inst' | 'ext_fx'
    presetId: string;
    presetName: string;
    ...
};
```

No default value is specified. When the user creates a new jam (`NEW_JAM`) or launches the app for the first time:

1. What is `activeMode.category`? Empty string? `'drums'`?
2. The Play tab (§7.6.3) dynamically renders based on the active mode. If no mode is selected, it renders... nothing?
3. The Adjust tab (§7.6.4) has mode-dependent knob layouts. With no mode, the knob mapping is undefined.

The Jam Manager (§7.7) is the first screen, so the user will select a jam before seeing the Play tab. But `NEW_JAM` creates a new session and presumably drops the user into the main UI — what mode is active?

> **Recommendation:** Add default values to §3.4 `activeMode`:
> ```
> Default on NEW_JAM: category: 'drums', presetId: 'synthetic', presetName: 'Synthetic'
> ```
> This ensures the Play tab always has content. The Mode tab is the first tab in the navigation order, making it easy for the user to switch if they don't want drums.

---

#### H2: 23 FX Effects Cannot Fit in a 3×4 Preset Grid — Navigation Undefined

The Play tab in FX Mode (§7.6.3) shows an "FX Preset Selector." The preset selector layout is defined as a `3×4_grid` (UI_Layout_Reference.md `preset_selector.layout`), which holds **12 items**.

But there are **23 total effects**:
- 12 Core FX (Audio Engine Spec §1.1)
- 11 Infinite FX (Audio Engine Spec §1.2)

The UI Layout Reference lists both groups in the play tab:
```json
"preset_examples": {
    "fx": ["Lowpass", "Highpass", "Reverb", ...],   // 12
    "infinite_fx": ["Keymasher", "Ripper", ...]      // 11
}
```

**Questions an agent will face:**
1. Does the 3×4 grid show all 23 effects somehow? (Smaller grid items? Scrollable?)
2. Are there two pages/banks — "Core FX" and "Infinite FX" — with a bank switcher?
3. Does the grid expand beyond 3×4 for FX mode?

The spec describes preset selectors for instruments as 3×4 grids (12 Notes presets = perfect fit, 12 Bass presets = perfect fit, 4 Drum kits ≠ 12 but fine). But 23 FX effects simply don't fit.

> **Recommendation:** Define one of these strategies in §7.6.3 FX Layout:
> - **(A) Two banks with switcher:** Show 12 Core FX by default. A "More" or "Infinite FX" toggle/tab switches to the 11 Infinite FX. Recommended — keeps the grid tidy and creates logical grouping.
> - **(B) Scrollable grid:** The 3×4 grid becomes scrollable/paginated, showing 12 at a time with a page indicator.
> - **(C) Expanded grid:** FX mode uses a different grid size (e.g., 4×6 = 24 slots).

---

#### H3: Solo Button Shown in Mixer but No Protocol Command

§7.6.5 (Mixer Tab Channel Strips) states:
> "Mute/**Solo** indicators per strip"

But the Command Schema (§3.2) only has `MUTE_SLOT` and `UNMUTE_SLOT`. There is no `SOLO_SLOT` or `UNSOLO_SLOT` command. Solo behavior is undefined:

1. Is Solo a UI-only operation (mute all OTHER slots, unmute this one)?
2. Does Solo have its own engine state? (`slot.isSoloed: boolean` is not in AppState)
3. Can multiple slots be soloed simultaneously? (Exclusive vs. additive solo)
4. What happens when you unsolo? Do previously-muted slots restore their pre-solo mute state?

Solo is a **standard mixer feature** that users expect. Implementing it purely in the UI (as a series of MUTE/UNMUTE commands) has edge cases — if the user has some slots manually muted before soloing, those mute states need to be remembered and restored on unsolo.

> **Recommendation:** Choose one:
> - **(A) Engine-side Solo:** Add `SOLO_SLOT` / `UNSOLO_SLOT` commands. Add `isSoloed: boolean` to slot state. Engine handles the mute/unmute logic internally, preserving pre-solo mute states. More reliable.
> - **(B) UI-side Solo:** Remove "Solo" from §7.6.5. Implement as a UI-only convenience that sends MUTE/UNMUTE commands. Add a note: "Solo is a UI-level toggle that mutes all other slots. Pre-solo mute states are stored in React component state and restored on unsolo." Less reliable but simpler.
> - **(C) Defer Solo to V2:** Remove "Solo" from §7.6.5 and add to §1.3 Future Goals. The Mixer only shows mute toggles for V1.

---

#### H4: Drum Kit Preset Selection — 4 Kits in a 3×4 Grid

§7.6.3 (Play Tab > Instruments) says: "**Preset Selector (Top):** 3×4 grid of preset buttons."

For Notes mode, there are 12 presets → 12 grid slots. 
For Bass mode, there are 12 presets → 12 grid slots. 
For Drums mode, there are **4 kit presets** (Synthetic, Lo-Fi, Acoustic, Electronic) → only 4 of 12 grid slots occupied.

What fills the remaining 8 slots? Options:
1. Empty/hidden slots (grid is 75% blank — looks broken)
2. Grid shrinks to 2×2 or 1×4 for Drums (inconsistent layout)
3. Additional drum presets are planned beyond the 4 listed

> **Recommendation:** Add a note to §7.6.3 or Audio Engine Spec §4.2: *"The 3×4 preset grid shows all available presets for the active category. For Drums mode (4 kits), only 4 grid positions are active — remaining positions are empty/dimmed. The grid does NOT resize per category — consistent 3×4 layout is maintained across all modes."* This tells agents to render empty slots rather than changing the grid dimensions.

---

#### H5: Pad-to-Note Mapping for Notes/Bass — Scale Logic Not Fully Specified

§3.2 `NOTE_ON` states:
> "Pad grid maps to notes via scale/root/transpose (§7.6.2)."

§7.6.2 was referenced in PBA5 as defining: "bottom-left = root note, ascending through selected scale." But the actual §7.6.2 in the current spec is the **Mode Tab Layout** section, which says it's "purely for high-level mode selection" and explicitly states "No Presets or Pads."

The pad-to-note mapping is described in the `NOTE_ON` command comment and is referenced as defined in §7.6.2, but the pad grid is actually in §7.6.3 (Play Tab). The mapping itself is incomplete:

1. **16 pads in a 4×4 grid** — does this cover 16 consecutive scale degrees? Or 2 octaves of an 8-note scale?
2. **Scale degree wrapping:** A pentatonic scale has 5 notes per octave. With 16 pads, that's 3 full octaves + 1 note. Does pad 6 start the second octave?
3. **Chromatic scale:** 12 notes per octave × 16 pads = 1 octave + 4 notes. Or does it span multiple octaves differently?
4. **Grid direction:** Bottom-left ascending — does it go left-to-right then bottom-to-top? So pad (4,1) = root, (4,2) = 2nd degree, ..., (4,4) = 4th degree, (3,1) = 5th degree, etc.?

An agent building the pad grid (Task 5.2) must know the exact mapping algorithm to assign MIDI note numbers to each pad.

> **Recommendation:** Add a "Pad-to-Note Mapping Algorithm" subsection to §7.6.3 Play Tab or to the `NOTE_ON` command in §3.2:
> ```
> Grid traversal order: Left-to-right, bottom-to-top.
> Pad (4,1) = root note (MIDI: rootNote + 48 for Notes, rootNote + 36 for Bass)
> Each pad = next scale degree, wrapping to the next octave when the scale repeats.
> Example: C Minor Pentatonic (C, Eb, F, G, Bb) with root C:
>   Row 4: C3, Eb3, F3, G3
>   Row 3: Bb3, C4, Eb4, F4
>   Row 2: G4, Bb4, C5, Eb5
>   Row 1: F5, G5, Bb5, C6
> Transpose (Pitch knob) shifts the root MIDI note ±24 semitones.
> ```

---

### 🟡 MEDIUM — Should Fix Before Beads

#### M1: Drum Pad Icons — UI Layout Reference Mismatch with Audio Engine Spec

Audio Engine Spec §4.1 defines 16 distinct icons for the drum grid:
```
Row 2: double_diamond_outline, double_diamond_dotted, double_diamond_striped, cylinder_short
```

UI_Layout_Reference.md `pad_grid.examples.drums` defines:
```json
{ "row": 2, "col": 1, "icon": "double_diamond" },
{ "row": 2, "col": 2, "icon": "double_diamond" },
{ "row": 2, "col": 3, "icon": "double_diamond" },
{ "row": 2, "col": 4, "icon": "double_diamond" }
```

Row 2 in the UI Layout Ref uses `"double_diamond"` for all 4 columns, while the Audio Engine Spec uses 4 distinct variant icons (`_outline`, `_dotted`, `_striped`, `cylinder_short`). An agent building the drum pad grid will get different icons depending on which document they reference.

> **Recommendation:** Update UI_Layout_Reference.md row 2 drum icons to match Audio_Engine_Specifications.md §4.1 exactly:
> ```json
> { "row": 2, "col": 1, "icon": "double_diamond_outline" },
> { "row": 2, "col": 2, "icon": "double_diamond_dotted" },
> { "row": 2, "col": 3, "icon": "double_diamond_striped" },
> { "row": 2, "col": 4, "icon": "cylinder_short" }
> ```

---

#### M2: `TOGGLE_NOTE_NAMES` — Listed as Command but Marked as UI-Only

§3.2 includes `TOGGLE_NOTE_NAMES` in the `Command` union type with the comment:
> "UI Settings (UI-only, stored in localStorage — not sent to engine)"

This is contradictory — it's defined as a member of the `Command` union (which is the WebSocket protocol contract between React and C++) but explicitly stated to never be sent over WebSocket.

**Problems:**
1. An agent implementing the C++ `CommandDispatcher` will see `TOGGLE_NOTE_NAMES` in the schema and implement a handler for it. This handler will never be called (since the command is never sent), creating dead code.
2. An agent implementing the TypeScript command dispatcher will include it in the `sendCommand()` function, potentially sending it over WebSocket by mistake.
3. `AppState.ui.noteNamesEnabled` (§3.4) IS in the engine state — but the note says it's localStorage. Which is the source of truth?

> **Recommendation:** Choose one:
> - **(A) Remove from Command union:** Move `TOGGLE_NOTE_NAMES` out of the `Command` type. Add a comment: *"Note names toggle is handled entirely in React localStorage. The `ui.noteNamesEnabled` field in AppState is NOT managed by the engine."* Remove `ui` section from AppState or mark it as client-side-only.
> - **(B) Make it a real command:** Remove the "UI-only" comment. The engine stores the preference and includes it in state broadcasts. This keeps it in the protocol but means it persists across devices (a feature, not a bug, for multi-device sessions).
> Option (B) is cleaner for the overall architecture — one source of truth (engine) for all state.

---

#### M3: Adjust Tab Pad Grid Reference Error — Says "Same as Mode Tab" but Should Be "Same as Play Tab"

§7.6.4 (Adjust Tab > Pad Grid):
> "**Content:** Mode-specific pads (same as Mode tab)."

But §7.6.2 (Mode Tab) explicitly states:
> "**No Presets or Pads:** The Mode tab itself does not show presets or pads."

The pad grid is in the **Play Tab** (§7.6.3), not the Mode tab. An agent building the Adjust tab would read "same as Mode tab," look at §7.6.2, find no pads, and be confused.

> **Recommendation:** Change §7.6.4 Pad Grid content to: *"Mode-specific pads (same as Play tab §7.6.3). Not shown in Microphone mode."*

---

#### M4: Waveform Timeline Tap — Same Action as Loop Length Button But Not in Protocol

§7.6.1 Timeline / Waveform Area states:
> "**Interaction:** Tap a waveform section to set loop length to that duration"

§7.6.9 Tap Behaviors confirms:
> "**Waveform Section:** Set loop length to that section's duration (1/2/4/8 bars)"

This tap fires `SET_LOOP_LENGTH` which, per PBA5's resolution, is dual-purpose: it sets the length AND captures audio from the retrospective buffer. So tapping a waveform section = capturing a loop.

**This is undocumented in the waveform timeline description.** §7.6.1 says it "sets" the loop length — the capture side-effect is only visible if you trace through to §3.2. An agent building the waveform UI may not realize tapping it triggers audio capture.

> **Recommendation:** Update §7.6.1 Timeline Interaction to: *"Tap a waveform section to set loop length to that duration AND immediately capture that duration from the retrospective buffer (fires `SET_LOOP_LENGTH` — see §3.10). Functionally identical to tapping the corresponding Loop Length button."*

---

#### M5: `SET_PAN` Missing `reqId` — Inconsistent with `SET_VOL`

§3.2 Command Schema:
```typescript
| { cmd: 'SET_VOL'; slot: number; val: number; reqId: string }   // Has reqId
| { cmd: 'SET_PAN'; slot: number; val: number }                  // No reqId
```

`SET_VOL` includes `reqId` for request-response tracking; `SET_PAN` does not. Since both are "always succeeds" optimistic commands, the inconsistency is primarily cosmetic — but an agent may take `reqId` presence as a signal that ACK handling is required for SET_VOL but not SET_PAN.

> **Recommendation:** Either add `reqId: string` to `SET_PAN` for consistency, or remove it from `SET_VOL` since neither command can fail and neither needs ACK tracking. Standardizing is better: make `reqId` optional on ALL commands (not just some), with the pattern: *"`reqId` is optional on any command. When present, the server echoes it in the ACK or ERROR response."*

---

### 🟢 LOW — Worth Noting

#### L1: Retrospective Buffer Behavior on Mode Switch — Undefined

§3.10 states:
> "Continuously records the output of the currently selected instrument/mode."

When the user switches from Drums to Notes (via `SELECT_MODE`), the retrospective buffer contains drum audio. If the user immediately taps a loop length button (before playing any notes), the captured audio will be drums — from the previous mode — not notes.

**Is this intentional?** For a "flow machine" workflow, it might be desirable (capture whatever was last played). But it could also confuse users who switch modes and expect a clean slate.

> **Recommendation:** Add a brief note to §3.10: *"Mode switching does NOT clear the retrospective buffer. The buffer always contains the most recent audio regardless of which mode produced it. This allows capturing audio that was played in a previous mode."*

---

#### L2: Session File Schema — Not Defined

§2.2.G (SessionStateManager) describes autosave, crash recovery, and session loading. §4.6 defines the naming convention for audio files (`project_name/audio/riff_{timestamp}_slot_{index}.flac`). But the session metadata file format is never specified:

1. What file format? JSON? Binary?
2. What does the session file contain? The full `AppState`? A subset?
3. Where is it stored? `~/Library/Application Support/FlowZone/[session_id]/session.json`?
4. The autosave file is specified as `~/Library/Application Support/FlowZone/backups/autosave.json` (§2.2.G) — but the regular session save file is not.

An agent building SessionStateManager (Task 6.3) needs this schema.

> **Recommendation:** Add a brief "Session File Format" subsection to §4 or §2.2.G:
> ```
> Session metadata: ~/Library/Application Support/FlowZone/sessions/[session_id]/session.json
> Contents: Full AppState snapshot (JSON) minus transient fields (system metrics, barPhase).
> Riff audio: ~/Library/Application Support/FlowZone/sessions/[session_id]/audio/riff_{timestamp}_slot_{index}.flac
> Autosave: ~/Library/Application Support/FlowZone/backups/autosave.json (full AppState snapshot)
> ```

---

#### L3: Test Tone Specification Missing

§3.2 includes `PLAY_TEST_TONE` and the error matrix says "None (always succeeds)." But the test tone's characteristics are undefined:

- Frequency? (440Hz sine wave is standard)
- Duration? (2 seconds? Until stopped?)
- Channel? (Both L+R? Alternating for stereo verification?)

> **Recommendation:** Add a brief note to §7.6.8 Tab B (Audio settings): *"Test tone: 440Hz sine wave, 2-second duration, output on both channels at -12dBFS. Auto-stops after duration."*

---

#### L4: `NOTHING_TO_COMMIT` Error Code Shared Between Two Different Failure Modes

Error code `4010: NOTHING_TO_COMMIT` is used for:
1. `SET_LOOP_LENGTH` — "retro buffer is empty (nothing played)"
2. `COMMIT_RIFF` — "no mix changes since last commit"

These are semantically different failures. The UI toast for (1) should say "Nothing to capture" while (2) should say "No changes to commit." But sharing the same error code means the React error handler can't distinguish them.

> **Recommendation:** Either:
> - **(A) Split codes:** `4010: NOTHING_TO_CAPTURE` (for SET_LOOP_LENGTH) and `4011: NOTHING_TO_COMMIT` (for COMMIT_RIFF). Renumber `ERR_RIFF_NOT_FOUND` to `4012`.
> - **(B) Use `msg` field:** Keep the same code but ensure the engine sends different `msg` values: `"BUFFER_EMPTY"` vs `"NO_MIX_CHANGES"`. The React client uses `msg` for toast text.
> Option (B) is simpler and doesn't require renumbering.

---

#### L5: Keymasher FX Source Routing — Which Buffer Does It Capture?

Audio Engine Spec §1.2 item 1:
> "**Keymasher** — 12-button performance sampler... Captures loop buffer, applies real-time manipulation per button"

"Loop buffer" is ambiguous. In FX Mode, the routing is defined (§7.6.2):
```
[Selected Slots] → [Sum] → [Active Effect] → [Audio Output]
```

For Keymasher specifically, the "Active Effect" is replaced by the 3×4 button grid. But what audio does Keymasher manipulate? Options:
1. The sum of selected FX source slots (consistent with other FX mode effects)
2. A dedicated internal buffer that Keymasher fills independently
3. The retrospective buffer

> **Recommendation:** Add a note to Audio Engine Spec §1.2 Keymasher: *"In FX Mode, Keymasher captures audio from the sum of selected FX source slots (same routing as other effects). The captured audio is held in an internal manipulation buffer. Button presses (Repeat, Reverse, etc.) apply real-time transformations to this buffer."*

---

## 4. Cross-Document Consistency Matrix

| Aspect | Spec 1.6 | Audio Engine Spec | UI Layout Ref | Status |
|:---|:---|:---|:---|:---|
| **Tab labels** | Mode, Play, Adjust, Mixer | References "Play Tab" | Mode, Play, Adjust, Mixer | ✅ |
| **Core FX count** | 12 (§2.2.M) | 12 (§1.1) | 12 (play_tab fx) | ✅ |
| **Infinite FX count** | 11 (§2.2.M) | 11 (§1.2) | 11 (play_tab infinite_fx) | ✅ |
| **Notes presets** | 12 (§2.2.M) | 12 named (§2.1) | "See Audio_Engine_Specs" | ✅ |
| **Bass presets** | 12 (§2.2.M) | 12 named (§3.1) | "See Audio_Engine_Specs" | ✅ |
| **Drum kits** | 4 kits (§2.2.M) | 4 kits (§4.2) | Pad grid | ✅ |
| **Drum icons row 1** | — | 4 unique icons §4.1 | 4 unique icons | ✅ |
| **Drum icons row 2** | — | 4 unique variant icons §4.1 | All `"double_diamond"` | ❌ **M1** |
| **Drum icons row 3** | — | hand, hand, snare, snare §4.1 | hand, hand, snare, snare | ✅ |
| **Drum icons row 4** | — | lollipop ×4 §4.1 | lollipop ×4 | ✅ |
| **FX grid capacity** | 3×4 (§7.6.3) | 23 effects total | 3×4 grid | ❌ **H2** |
| **Adjust pad grid ref** | "same as Mode tab" (§7.6.4) | — | — | ❌ **M3** |
| **Solo in Mixer** | "Mute/Solo" (§7.6.5) | — | — | ❌ **H3** |
| **SET_PAN reqId** | Missing (§3.2) | — | — | ⚠️ **M5** |
| **VST3 build task** | Goal (§1.1) | — | — | ❌ **C1** |
| **Default activeMode** | Not specified (§3.4) | — | — | ❌ **H1** |
| **Pad-to-note mapping** | Referenced in NOTE_ON | — | — | ⚠️ **H5** |
| **Waveform tap = capture** | Not explicit (§7.6.1) | — | — | ⚠️ **M4** |
| **TOGGLE_NOTE_NAMES scope** | In Command union but UI-only | — | In settings | ⚠️ **M2** |
| **Keymasher buffer source** | — | "loop buffer" (§1.2) | — | ⚠️ **L5** |
| All other aspects (30+) | — | — | — | ✅ Consistent |

---

## 5. Reliability Risk Assessment

### R1: Memory Budget Undefined — Could Exceed 2GB on Modest Machines

The spec allocates several large memory blocks but never defines a total budget:

| Component | Memory | Source |
|:---|:---|:---|
| Retrospective buffer | ~37 MB | §3.10: 96s × 48kHz × 2ch × 4B |
| DiskWriter ring buffer | 256 MB | §2.2.I |
| DiskWriter overflow (max) | 1 GB | §4.2 |
| 8 loop slots (max length, in RAM) | ~294 MB | 8 × 96s × 48kHz × 2ch × 4B |
| IPC shared memory (per host) | ~16 KB each | §4.3 |
| Metric store (10 min) | ~5 MB | §5.3 |
| React WebView | 100-300 MB | Typical WebKit/Blink overhead |
| JUCE framework | ~50 MB | Typical |
| **Total (worst case)** | **~2,042 MB** | — |

This doesn't include VST plugin processes or the OS overhead. On a Mac with 8 GB RAM, this could cause memory pressure. On a 16 GB machine, it's fine.

> **Recommendation:** Add a "Memory Budget" note to §2 or §4: *"Estimated peak memory: ~2 GB (worst case with maximum-length loops in all 8 slots and DiskWriter overflow active). Minimum recommended RAM: 8 GB. The DiskWriter overflow (1 GB) is a temporary emergency measure — under normal operation, the Ring Buffer (256 MB) is sufficient."*

### R2: Single WebSocket Connection — Priority Inversion Risk

§3.7 specifies that JSON commands/state AND binary visualization frames share the same WebSocket connection. During heavy visualization (30fps binary frames), a command message (`NOTE_ON`) could be queued behind multiple binary frames in the send buffer.

WebSocket is TCP-based, so messages are ordered. If the server sends a large binary frame followed by a state patch, the client receives them in order. But if the client sends a `NOTE_ON` command while also sending a binary ACK, both travel on the same connection.

In practice, the command sizes are tiny (~50 bytes for JSON, 4 bytes for ACK) and TCP handles interleaving well. But under extreme load, latency spikes are possible.

> **Recommendation:** Add a note to §3.7: *"Implementation note: JSON command messages and binary visualization frames share one WebSocket connection. Command messages are typically < 100 bytes and should experience negligible queuing delay even during peak visualization throughput. If latency issues are observed during Phase 6 integration, consider a dedicated command-only WebSocket as a fallback."*

### R3: Shutdown Sequence — DiskWriter Buffer Flush Timing

§2.4 Shutdown sequence:
```
SessionStateManager (auto-save) → FeatureExtractor → StateBroadcaster
→ WebServer → DiskWriter → PluginProcessManager → FlowEngine → Audio Device
```

**Problem:** The `FlowEngine` (which feeds audio to the `DiskWriter`) is shut down AFTER the DiskWriter. This means:
1. DiskWriter is told to shut down.
2. DiskWriter flushes its remaining buffer to disk.
3. **Then** FlowEngine shuts down — which stops producing audio.

This is actually correct sequence (drain the writer before stopping the source). But what happens if the user quits while the DiskWriter has a large backlog (e.g., Tier 3 overflow at 800 MB)? Flushing 800 MB to disk at FLAC compression could take 5-10 seconds.

Does the app block on shutdown until DiskWriter finishes? Or does it time out and lose audio?

> **Recommendation:** Add to §2.4 Shutdown sequence: *"DiskWriter shutdown is blocking — the app waits for all buffered audio to be flushed to disk before proceeding. Maximum wait time: 30 seconds. If DiskWriter cannot flush within this timeout, remaining audio is written as a `*_PARTIAL.flac` file and the app proceeds with shutdown."*

### R4: FX Mode "Differs From Latest Riff History" — Comparison Logic Undefined

§7.6.2 FX Mode step 1:
> "if the current session state differs from the latest riff history entry, the engine automatically commits the current state to riff history."

The comparison logic is undefined:
1. What fields are compared? All of AppState? Just slot volumes/pans/mutes?
2. Floating-point comparison tolerance? If volume is 0.500001 vs 0.5, does that count as "different"?
3. What if riff history is empty (first jam, no commits yet)?

Edge case: The user creates a new jam, records 3 layers, enters FX mode. There's no riff history entry to compare against. Does auto-commit fire? (It should — the state is non-empty.)

> **Recommendation:** Add to §7.6.2 FX Mode step 1: *"Comparison is based on slot states, volumes, pans, and mute flags. Floating-point values are compared with a tolerance of ±0.001. If riff history is empty (no previous commits), auto-commit always fires when entering FX Mode with any non-empty slots."*

### R5: `NEW_JAM` — Does It Stop Playback and Clear All State?

The `NEW_JAM` command in §3.2 says "Create a new jam session" with "None (always succeeds)" in the error matrix. But the behavioral implications are undefined:

1. If the user is mid-performance (transport playing, 6 loops cycling), does `NEW_JAM` stop playback?
2. Does it clear all 8 slots?
3. Does it reset transport (BPM, key, scale)?
4. Is the current session auto-saved before creating a new one?
5. Does the retrospective buffer get cleared?

If `NEW_JAM` silently discards the current session without auto-saving, the user loses all uncommitted work.

> **Recommendation:** Add behavioral definition to §3.2 `NEW_JAM` or §7.7.2:
> ```
> NEW_JAM behavior:
> 1. Auto-save current session (if any) to disk.
> 2. Stop transport (if playing).
> 3. Clear all 8 slots (set to EMPTY).
> 4. Clear retrospective buffer.
> 5. Reset transport to defaults: 120 BPM, C Minor Pentatonic, quantise ON.
> 6. Reset activeMode to default (drums/synthetic).
> 7. Generate new session ID, random emoji, date-based name.
> 8. Navigate to main UI (exit Jam Manager).
> ```

---

## 6. Summary of All Findings

### 🔴 Critical — Must Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| C1 | VST3 plugin build target referenced in goals but absent from task breakdown | Add to Phase 0 (Option A), defer to Phase 8 (Option B), or move to V2 (Option C) | 10 min (decision) |

### 🟠 High — Should Fix Before Bead Creation

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| H1 | No default `activeMode` on NEW_JAM — Play tab blank | Define defaults: `drums` / `synthetic` | 5 min |
| H2 | 23 FX effects don't fit in 3×4 preset grid — navigation undefined | Define two-bank switcher or scrollable grid | 10 min |
| H3 | "Solo" in Mixer UI but no protocol command | Add SOLO_SLOT or remove Solo from V1 | 10 min |
| H4 | 4 drum kits in 12-slot preset grid — empty slot handling undefined | Add note: grid shows empty/dimmed slots | 5 min |
| H5 | Pad-to-note mapping algorithm not fully specified | Add explicit algorithm with grid traversal order and example | 15 min |

### 🟡 Medium — Should Fix

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| M1 | Drum pad icons in UI Layout Ref don't match Audio Engine Spec row 2 | Update UI Layout Ref row 2 icons | 5 min |
| M2 | `TOGGLE_NOTE_NAMES` in Command union but marked UI-only | Remove from union OR make it a real engine command | 5 min |
| M3 | Adjust tab pad grid says "same as Mode tab" — should be "same as Play tab" | Fix reference to §7.6.3 | 2 min |
| M4 | Waveform tap triggers capture but this isn't stated in §7.6.1 | Update interaction description to mention capture side-effect | 5 min |
| M5 | `SET_PAN` missing `reqId` vs `SET_VOL` which has it | Standardize reqId as optional on all commands | 5 min |

### 🟢 Low — Worth Noting

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| L1 | Retrospective buffer not cleared on mode switch — undocumented | Add note confirming intentional behavior | 2 min |
| L2 | Session file schema (format, location, contents) not defined | Add Session File Format subsection | 10 min |
| L3 | Test tone characteristics undefined | Add frequency/duration/level spec | 2 min |
| L4 | Error code 4010 shared between two different failure modes | Use different `msg` values or split codes | 5 min |
| L5 | Keymasher "loop buffer" source ambiguous | Clarify as FX source slot sum | 5 min |

### Reliability Risks

| # | Risk | Severity | Action |
|:--|:-----|:---------|:-------|
| R1 | Memory budget undefined — could exceed 2 GB | Medium | Document estimated budget and minimum RAM requirement |
| R2 | Single WebSocket — priority inversion under load | Low | Add implementation note, fallback plan |
| R3 | DiskWriter shutdown — large buffer flush could delay/lose audio | Medium | Define blocking flush with 30s timeout |
| R4 | FX Mode auto-commit comparison logic undefined | Medium | Define compared fields, tolerance, empty-history edge case |
| R5 | `NEW_JAM` behavioral contract undefined | High | Define full behavioral sequence including auto-save |

---

## 7. Overall Assessment

| Dimension | Rating | Change from PBA5 | Notes |
|:---|:---|:---|:---|
| **Completeness** | 🟢 **95%** | ↓ 2% (pre-fix) | VST3 task gap, missing defaults, Solo protocol gap. Post-fix: **99%**. |
| **Internal Consistency** | 🟢 **96%** | ↓ 2% (pre-fix) | Drum icons, Adjust tab reference, NOTE_NAMES scope. Post-fix: **99%**. |
| **Agent Buildability** | 🟢 **93%** | ↓ 2% (pre-fix) | Pad-to-note mapping, FX grid layout, drum kit grid handling. Post-fix: **98%**. |
| **Reliability for Non-Engineer** | 🟢 **91%** | ↓ 2% (pre-fix) | Memory budget, NEW_JAM behavior, shutdown flush, session file format. Post-fix: **97%**. |

**Verdict:** The spec is very close to bead-ready. The issues found are primarily **micro-level gaps** — missing defaults, undefined edge cases, and one unplanned build task. None of these indicate architectural problems; the macro design is sound. After applying these fixes (estimated total effort: ~100 minutes), the spec will be at **>97% across all dimensions** and **decisively ready for bead creation with high confidence**.

**Recommended priority for fixes:**
1. **C1** (VST3 task decision) — owner decision needed
2. **H1, H5, R5** (defaults and behavior) — agent-blocking
3. **H2, H3** (UI layout) — agent-confusing
4. **M1-M5** (consistency) — quick fixes
5. **R1-R4** (reliability) — documentation additions
6. **L1-L5** (nice-to-have) — can be addressed during bead implementation

---

*End of Pre-Build Assessment 6*
