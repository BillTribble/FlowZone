# FlowZone — Pre-Build Assessment 6

**Date:** 13 Feb 2026  
**Scope:** Final-pass reliability review with fresh eyes, focused on cross-document inconsistencies, undefined runtime behaviors, missing defaults, resource budgets, and anything that would force an AI coding agent to guess — which a non-engineer owner cannot troubleshoot.  
**Documents Reviewed (Spec folder only):**
- `Spec_FlowZone_Looper1.6.md` (1842 lines)
- `Audio_Engine_Specifications.md` (366 lines)
- `UI_Layout_Reference.md` (370 lines)

**Prior Art:** PBA5 found 13 issues + 4 reliability risks; all resolved and incorporated into current Spec 1.6. This review starts from the *updated* spec, looking for what PBA5 missed or introduced.

---

## 1. Executive Summary

The spec suite is in excellent shape after five rounds of assessment. PBA5's critical issues (NOTE_ON/NOTE_OFF, SET_LOOP_LENGTH dual-purpose, barPhase broadcast, port number, etc.) have all been cleanly resolved in the current documents.

This sixth-pass review — with a focus on **agent-tripping ambiguities**, **missing defaults/initial values**, **resource budgets**, and **cross-document micro-inconsistencies** — has found **16 issues** (0 critical, 4 high, 7 medium, 5 low) and **5 reliability risks**.

**The dominant theme this time:** No show-stoppers remain, but there are several "agent will guess wrong" items — places where an agent has to make a decision without guidance. For a non-engineer owner, these guesses are the most dangerous kind of bug: the code compiles, the app runs, but a behavior is subtly wrong and hard to diagnose.

**Bottom line:** After resolving the H-level items below, this spec is **ready for bead creation**.

---

## 2. PBA5 Resolution Verification

I verified that all PBA5 resolutions are present in the current spec documents:

| PBA5 # | Resolution Claimed | Present in Spec 1.6? | Notes |
|:---|:---|:---|:---|
| C1 | NOTE_ON / NOTE_OFF added | ✅ | §3.2 lines 300-301, §3.3 "always succeeds", §3.10 step 1 references NOTE_ON |
| C2 | SET_LOOP_LENGTH dual-purpose documented | ✅ | §3.2 comment: "Sets the loop length AND immediately captures" |
| H1 | barPhase in binary stream | ✅ | §3.7 payload includes `[BarPhase:4]`, §3.7 note says exclusively binary |
| H2 | COMMIT_RIFF = state-only snapshot | ✅ | §3.2 comment: "Does NOT capture audio from the retrospective buffer" |
| H3 | WebSocket port 8765 | ✅ | §2.2.L Network Configuration block |
| H4 | Drum pitch = global | ✅ | Audio Engine Spec §4.3: "Tune all pads globally" |
| M1 | GET_METRICS_HISTORY deferred | ✅ | §5.3: "defined as part of Phase 8 (Task 8.7)" |
| M2 | revisionId standardized | ✅ | §3.2 STATE_PATCH uses `revisionId` |
| M3 | Binary ACK = FrameId echo | ✅ | §3.7: "4-byte ACK (the FrameId...as uint32 Little Endian)" |
| M4 | DELETE_JAM + GC clarified | ✅ | §2.2.G: metadata deleted immediately, audio files via GC |
| L1 | Time signature V1 = 4/4 | ✅ | §7.6.7: "V1 uses 4/4 time signature exclusively" |
| L2 | SET_KNOB mode-dependent routing | ✅ | §3.2 SET_KNOB comment documents routing |
| L3 | Mic slot values defined | ✅ | §3.4 instrumentCategory comment lists 'mic', 'mic_input', 'merge', 'auto_merge' |
| R1 | NOTE_ON performance consideration | ✅ | Documented as agent awareness item |
| R2 | Non-blocking auto-merge | ✅ | §7.6.2.1 Implementation Note added |
| R3 | FX Mode auto-commit on entry | ✅ | §7.6.2 step 1: "engine automatically commits the current state" |
| R4 | Hardcoded minimum config fallback | ✅ | §4.5 step 4: hardcoded minimum configuration |

**Verdict:** All PBA5 resolutions correctly applied. No regressions detected.

---

## 3. New Issues Found

### 🟠 HIGH — Should Fix Before Bead Creation

#### H1: Drum Pad Icons Mismatch Between UI_Layout_Reference.md and Audio_Engine_Specifications.md

**The gap:** Audio_Engine_Specifications.md §4.1 defines precise, differentiated icons for all 16 drum pads:

| Pad | Audio Engine Spec Icon | UI Layout Reference Icon |
|:---|:---|:---|
| (2,1) | `double_diamond_outline` | `double_diamond` |
| (2,2) | `double_diamond_dotted` | `double_diamond` |
| (2,3) | `double_diamond_striped` | `double_diamond` |
| (2,4) | `cylinder_short` | `double_diamond` |

Row 2 in the UI Layout Reference uses `double_diamond` for all four pads, but the Audio Engine Spec has four distinct icon variants (`_outline`, `_dotted`, `_striped`, `cylinder_short`). An agent building the pad grid (Task 5.2) will read one document and get different icon names than an agent building the drum engine (Task 4.5).

**Why this matters:** This is a visual-only issue (won't crash), but for a non-engineer: (a) the pads will look wrong and be confusing since four different kicks all have the same icon, and (b) debugging "why do all my kick pads look the same?" requires understanding which spec file the agent followed. The Audio Engine Spec has the clearly correct, more detailed version.

> **Recommendation:** Update `UI_Layout_Reference.md` pad grid → drums → samples to match Audio_Engine_Specifications.md §4.1 exactly. The Audio Engine Spec is the authoritative source for icon-to-sound mapping.

---

#### H2: Solo Mentioned in Mixer But Not in Protocol

§7.6.5 Channel Strips states:
> "Mute/Solo indicators per strip"

The command schema (§3.2) defines `MUTE_SLOT` and `UNMUTE_SLOT`, but there is no `SOLO_SLOT` command. The error matrix (§3.3) doesn't mention solo. The AppState (§3.4) `slots` array has no `solo` field.

**Why this matters:** An agent building the Mixer tab (Task 5.7 / Task 6.2) will see "Mute/Solo indicators" and need to implement Solo. Without a protocol definition, the agent must guess:

- **Option A (UI-only Solo):** Solo is implemented client-side — it mutes all other slots and unmutes the selected one, sending individual `MUTE_SLOT`/`UNMUTE_SLOT` commands for each. No engine support needed, but it floods the command queue with 7+ commands.
- **Option B (Engine Solo):** Add a `SOLO_SLOT` command that the engine handles atomically.
- **Option C (Remove Solo for V1):** Simplify to mute-only; add Solo in V2.

> **Recommendation:** For V1 reliability, Option C is safest — remove "Solo" from §7.6.5 and simplify to "Mute indicators per strip." If Solo is desired, define it as Option A (UI-only) with a clear note: *"Solo is implemented client-side: tapping Solo on a channel mutes all other non-empty slots and unmutes the selected one. There is no engine-level SOLO command."*

---

#### H3: FX Preset Display — 23 Effects Don't Fit a 3×4 Grid

The Play tab (§7.6.3) shows FX presets in the preset selector area. The UI Layout Reference defines the preset selector as a `3x4_grid` (12 positions). But there are:

- **Core FX Bank:** 12 effects (§1.1 of Audio Engine Spec)
- **Infinite FX Bank:** 11 effects (§1.2 of Audio Engine Spec)
- **Total:** 23 effects

23 effects cannot fit into a 12-position grid. The UI Layout Reference `play_tab.preset_selector.preset_examples` lists them as two separate groups (`fx` and `infinite_fx`), but there's no UI mechanism defined for switching between the two groups.

**Questions an agent must answer:**
1. Are there two sub-tabs or pages within the FX preset area (Core / Infinite)?
2. Is the grid scrollable (6×4 = 24 positions)?
3. Is there a toggle/filter to switch between banks?

> **Recommendation:** Define the FX preset navigation. Simplest option: two rows of presets labeled "Core" and "Infinite" in a single scrollable 6×4 grid (24 positions, one empty). Or: a toggle switch above the grid that flips between the two banks. Add this to §7.6.3 under "Layout: FX (Resampling)" and update UI_Layout_Reference.md accordingly.

---

#### H4: No Default Value for `activeMode.category` — What Shows on First Launch?

§3.4 defines `activeMode.category` as a string with values `'drums' | 'notes' | 'bass' | 'fx' | 'mic' | 'ext_inst' | 'ext_fx'`. But no default is specified.

On **first launch** (or NEW_JAM), the user arrives at the Jam Manager (§7.7), creates a jam, and enters the dashboard. The Navigation tabs show Mode, Play, Adjust, Mixer. If the user taps **Play** or **Adjust** before selecting a mode:

- The Play tab dynamically displays content "based on the active mode" (§7.6.3). With no active mode, what renders?
- The Adjust tab shows knobs that vary by mode (§7.6.4). With no mode, which knobs appear?
- The retrospective buffer records "the output of the currently selected instrument/mode" (§3.10). With no mode, what audio is captured?

This is a classic "undefined initial state" bug. An agent will either crash on null state, show an empty screen, or pick an arbitrary default — none of which are desirable for a non-engineer to troubleshoot.

> **Recommendation:** Add a default value to §3.4: `activeMode.category` defaults to `'drums'` on NEW_JAM. Add a note: *"On session creation, the default active mode is Drums. The app opens to the Mode tab, allowing the user to select their preferred category before playing."* Also specify that the initial tab on entering a new jam is the **Mode tab** (not Play or Adjust).

---

### 🟡 MEDIUM — Should Fix Before Beads

#### M1: Adjust Tab Pad Grid Says "Same as Mode Tab" But Mode Tab Has No Pads

§7.6.4 Pad Grid states:
> "**Content:** Mode-specific pads (same as Mode tab). Not shown in Microphone mode."

But §7.6.2 explicitly says:
> "**No Presets or Pads:** The Mode tab itself does not show presets or pads (V1 decision). It is purely for high-level mode selection."

The pads are in the **Play tab** (§7.6.3), not the Mode tab. The Adjust tab reference should say "same as Play tab."

> **Recommendation:** Update §7.6.4: *"Content: Mode-specific pads (same as Play tab). Not shown in Microphone mode."*

---

#### M2: `TOGGLE_NOTE_NAMES` Listed as Command But Described as "UI-Only"

§3.2 includes `TOGGLE_NOTE_NAMES` in the `Command` union type with the comment:
> "UI Settings (UI-only, stored in localStorage — not sent to engine)"

This is contradictory — it's in the Command type (implying it's a WebSocket message) but described as UI-only (implying it's localStorage). An agent implementing the WebSocket command handler will either:
- Include it in the C++ `CommandDispatcher` (unnecessary engine code), or
- Skip it in C++ but an agent building `commands.h` will add it to the enum (schema sync violation)

The `AppState` (§3.4) has `ui.noteNamesEnabled` — this suggests the engine tracks it. But the comment says localStorage.

> **Recommendation:** Decide one:
> - **(A) Engine-managed:** Remove the "UI-only" comment. The engine tracks `ui.noteNamesEnabled` in AppState. Commands flow through WebSocket like all others. Simple, consistent.
> - **(B) Client-only:** Remove from the `Command` union type entirely. Add a comment in §3.4 that `ui.noteNamesEnabled` is client-local state not sent from the engine. Don't include it in `commands.h`.
>
> Option (A) is cleanest for agent consistency — treat it like every other command.

---

#### M3: `SET_PAN` Missing `reqId` While `SET_VOL` Has It

§3.2 command definitions:
```typescript
| { cmd: 'SET_VOL'; slot: number; val: number; reqId: string }
| { cmd: 'SET_PAN'; slot: number; val: number }
```

`SET_VOL` has `reqId` but `SET_PAN` doesn't. Both are "always succeeds" commands with optimistic UI updates. The inconsistency means:
- An agent implementing `useCommands.ts` will wonder if `SET_PAN` is intentionally simpler or if `reqId` was accidentally omitted.
- The `CommandDispatcher` (§2.2.D) says "Returns reqId in error/ack responses when present on the command" — this works for both, but the schema inconsistency is misleading.

> **Recommendation:** Either add `reqId` to `SET_PAN` for schema consistency, or remove `reqId` from `SET_VOL` since both are "always succeeds" and don't need request tracking. The simplest choice: make `reqId` consistently optional on all commands (`reqId?: string`), as §2.2.D already handles its optional presence.

---

#### M4: Waveform Timeline Tap = Loop Capture? Interaction Not in §7.6.9

§7.6.1 Timeline / Waveform Area says:
> "**Interaction:** Tap a waveform section to set loop length to that duration"

§7.6.9 Interaction Patterns confirms:
> "**Waveform Section:** Set loop length to that section's duration (1/2/4/8 bars)"

Since `SET_LOOP_LENGTH` is dual-purpose (sets length AND captures audio from the retrospective buffer), tapping a waveform section would trigger a capture — identical to tapping a loop length button. This is probably intentional (the waveform is a visual alternative to the buttons), but it's not explicitly stated.

**Risk:** If an agent interprets "set loop length" literally (just the setter, not the capture), the waveform tap would change the length display but not capture audio, while the buttons do both. The user sees two UI elements that look like they do the same thing but behave differently.

> **Recommendation:** Add a brief note to §7.6.1 Timeline interaction: *"Tapping a waveform section fires the same `SET_LOOP_LENGTH` command as the corresponding loop length button — this includes capturing audio from the retrospective buffer (see §3.10). The waveform sections are a visual alternative to the loop length buttons."*

---

#### M5: Keymasher — What Audio Buffer Does It Operate On?

Audio_Engine_Specifications.md §1.2 item 1 (Keymasher) says:
> "Implementation: Captures loop buffer, applies real-time manipulation per button"

"Loop buffer" is not a defined term in the spec. Possible interpretations:
- The **retrospective buffer** (always-on capture of instrument output)
- The **audio from selected FX source slots** (since Keymasher is in the Infinite FX Bank and FX Mode routes selected slots → effect)
- The **current master output** mix
- A **dedicated Keymasher capture buffer** (internal to the effect)

In FX Mode, the routing is defined (§7.6.2): selected slots → sum → active effect → output. But Keymasher doesn't use the XY pad — it uses a 3×4 button grid. Does Keymasher still receive audio from the FX source slot routing? Or does it have its own capture mechanism?

> **Recommendation:** Clarify in Audio_Engine_Specifications.md §1.2 item 1: *"Keymasher receives audio from the same FX source routing as other effects — the summed output of selected source slots (see Spec §7.6.2). It maintains an internal circular capture buffer (~2 bars at current tempo) that is continuously filled from this source. Button presses manipulate playback position and processing of this internal buffer in real-time."*

---

#### M6: VST3 Plugin Build Target — No Task Covers Setup

§1.1 Goals states:
> "**Dual Build Target:** Built simultaneously as a Standalone App and a VST3 Plugin from the same Projucer project."

§2.2.A specifies:
> "**Bus Configuration:** In VST3 mode, declare 8 stereo output buses... Use `#if JucePlugin_Build_VST3` preprocessor guards."

But the Task Breakdown (§9) Task 0.1 only mentions: "Produce a compiling JUCE standalone app." No task covers:
1. Enabling the VST3 plugin target in Projucer
2. Configuring the 8 stereo output buses
3. Testing VST3 loading in a DAW
4. Implementing the preprocessor guards for bus configuration

The VST3 target is a stated V1 goal but has no planned work item.

> **Recommendation:** Either add a dedicated task (e.g., Task 2.6 or a Phase 6 integration task): *"Configure VST3 plugin target in Projucer. Set up bus configuration with `#if JucePlugin_Build_VST3` guards. Verify plugin loads in a DAW and outputs 8 stereo pairs."* Or defer VST3 to a post-V1 milestone and update §1.1 to reflect this.

---

#### M7: `4010: NOTHING_TO_COMMIT` — Shared Error Code, Different Failure Modes

Error code `4010` is used for two different situations:

| Command | 4010 Meaning | User Action |
|:---|:---|:---|
| `SET_LOOP_LENGTH` | Retrospective buffer is empty (user hasn't played anything) | Toast: "Nothing to capture" |
| `COMMIT_RIFF` | No mix changes since last commit | Button is hidden, but "if somehow triggered..." show toast |

The same error code produces different user-facing messages depending on which command triggered it. An agent building the error handler in the React UI needs to know the source command to display the right message. The `ERROR` server response (§3.2) includes `code` and `msg` but not the originating command — the agent would need to correlate by `reqId` or source context.

> **Recommendation:** Either:
> - **(A)** Split into two codes: `4010: ERR_RETRO_BUFFER_EMPTY` (for SET_LOOP_LENGTH) and `4011: ERR_NO_MIX_CHANGES` (for COMMIT_RIFF). Shift current `4011: ERR_RIFF_NOT_FOUND` to `4012`. Register in §5.1.
> - **(B)** Keep shared code, but use different `msg` strings: the server sends `"RETRO_BUFFER_EMPTY"` vs `"NO_MIX_CHANGES"` in the `msg` field, allowing the UI to display contextual toasts.
>
> Option (B) is lower-effort and probably sufficient.

---

### 🟢 LOW — Worth Noting

#### L1: `PLAY_TEST_TONE` — No Parameters Defined

§3.2 defines `{ cmd: 'PLAY_TEST_TONE' }` with no parameters. An agent implementing this needs to know:

- **Frequency:** 440Hz (A4)? 1kHz (standard test)?
- **Duration:** 1 second? Until stopped?
- **Channel:** Left only? Right only? Both? Alternating?
- **Volume:** Full scale? -12dBFS?

> **Recommendation:** Add a brief note to §3.2 or §7.6.8: *"Test tone: 1kHz sine wave at -12dBFS, 2 seconds duration, both channels simultaneously. No user-configurable parameters."*

---

#### L2: Session File Format Not Defined

§2.2.G (SessionStateManager) handles autosave, crash-recovery, session loading, and riff history. §4.1 defines the directory structure and audio file naming (`riff_{timestamp}_slot_{index}.flac`). But the **session metadata file format** is never specified:

- Is the session state saved as JSON? What's the schema?
- Where exactly is the session metadata file stored?
- What's the autosave file path? (§2.2.G says `~/Library/Application Support/FlowZone/backups/autosave.json` — this defines the backup path but not the primary session file.)
- How is riff history stored? Each riff as a separate JSON file? One big array?

An agent building SessionStateManager (Task 6.3) will need to invent a file format. This is likely fine for V1 — the agent can design it — but if a second agent later needs to read session files, they'll have no schema.

> **Recommendation:** Add a brief note to §2.2.G or §4.1: *"Session metadata is stored as JSON at `~/Library/Application Support/FlowZone/sessions/{sessionId}/session.json`. The file contains the `AppState` structure (§3.4) at the time of save, plus a `riffHistory` array. The autosave backup uses the same format at `~/Library/Application Support/FlowZone/backups/autosave.json`. Exact schema will be finalized during Task 6.3 implementation."*

---

#### L3: Ext Inst / Ext FX Category Behavior When Running as VST3

§7.6.2 shows Ext Inst and Ext FX categories in the 2×4 grid with `"placeholder": "Coming Soon"` from the UI Layout Reference. §1.1 says "VST3 hosting (Phase 7) is only available in Standalone mode."

