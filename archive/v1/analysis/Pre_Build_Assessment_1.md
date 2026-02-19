# FlowZone — Final Pre-Build Assessment

**Date:** 13 Feb 2026  
**Scope:** Cross-document review of all spec files for inconsistencies, gaps, missing definitions, and build risks.

---

## Executive Summary

The spec is **substantially well-structured** and the risk mitigations (§8) are excellent. However, this review found **14 concrete inconsistencies and 8 gaps** that would cause real confusion or bugs during agentic build-out. Most are resolvable with small spec edits before bead creation begins.

The issues fall into three categories:
1. **Commands/state missing from the schema** — things the UI needs to do that have no corresponding command defined
2. **Cross-document contradictions** — where two documents say different things
3. **Ambiguities** — where the spec is clear enough for a human to interpret, but not specific enough for an agent to implement without guessing

---

## 🔴 MUST FIX — Will Cause Build Failures

### 1. Missing Commands in Schema (§3.2)

The Command Schema defines the full set of commands, but several features described elsewhere in the spec have **no corresponding command**:

| Feature | Described In | Missing Command |
|:---|:---|:---|
| **FX Mode layer selection** | §7.6.2 (FX Mode) | No `SELECT_FX_SOURCE_SLOTS` command. The spec says "Existing slot selection uses `TRIGGER_SLOT` with a mode-specific flag" — but `TRIGGER_SLOT` has no `mode` or `fxMode` flag in the schema. |
| **XY Pad control** | §7.6.3, activeFX state | No `SET_XY_PAD` command. The `activeFX.xyPosition` state exists but nothing updates it. |
| **Loop length extension** | §7.6.1 (Loop Length Controls) | Buttons say `+ 8 BARS`, `+ 4 BARS` etc. (additive), but the command is `SET_LOOP_LENGTH` (absolute). These are different operations — extending vs. setting. |
| **Tempo editor** | §7.6.5 | Tapping Tempo opens an editor, but only `SET_TEMPO` exists. No `TAP_TEMPO` or inline editing command. |
| **Key/Scale editor** | §7.6.5 | Tapping Key opens an editor, but no modal/editor flow is specified. |
| **Start New** button | §7.6.5 | Button exists in Mixer tab, corresponds to `START_NEW` command — ✅ this one is fine. |
| **Settings changes** | §7.4 | Settings sync to engine "immediately via WebSocket" but no `SET_AUDIO_DEVICE`, `SET_SAMPLE_RATE`, `SET_BUFFER_SIZE` commands exist. |
| **Riff playback swap timing** | §7.6.1 | `instant` vs `swap_on_bar` is a user setting, but no command to set it. |
| **Monitor toggles** | §7.6.6 | `Monitor until looped` and `Monitor input` toggles have no commands. |
| **Input gain control** | §7.6.6, Audio_Engine_Specifications §6.2 | Large gain knob exists but no `SET_INPUT_GAIN` command. |
| **Quantise toggle** | §7.6.5 | Quantise button in Mixer tab transport controls, but no `TOGGLE_QUANTISE` command. |
| **Looper Mode toggle** | §7.6.5 | Looper Mode toggle in Mixer tab, but no command and no explanation of what this mode does. |
| **Note Names toggle** | §7.6.8 | Toggle in More Options, but no command. |

> [!CAUTION]
> **Recommendation:** Before creating beads, add all missing commands to the Command Schema (§3.2), the Error Matrix (§3.3), and make a note for the Schema Foundation bead (Task 1.1) to include them. An agent building the UI will reach for these commands and find nothing.

---

### 2. FX Mode Routing Is Underspecified

§7.6.2 describes FX Mode but leaves critical audio routing questions unanswered:

- **What happens to the normal playback while FX Mode is active?** Do unselected slots continue playing normally, or is the entire output replaced by the FX-processed audio?
- **Can the user play instruments while in FX Mode?** Or is it exclusively a resampling workflow?
- **The "delete source slots after commit" behaviour** — is this the *only* option, or can the user keep the source layers? Destructive-only is risky for user experience.
- **How does FX Mode interact with the XY pad?** §7.6.2 says FX Mode has its own XY pad but also says "FX Mode uses the same main FX chain as the Sound tab." Does this mean the Sound tab's XY pad and FX Mode's XY pad control the same thing?

> [!IMPORTANT]
> **Recommendation:** Add a subsection to §7.6.2 that explicitly defines the audio routing graph for FX Mode, including what the user hears, what gets recorded, and whether source deletion is configurable.

---

### 3. `activeFX.isActive` Has No Toggle Command

The `AppState` (§3.4) includes `activeFX.isActive` described as "Whether effect is currently engaged (finger down)." But the XY Pad interaction model (§7.6.3) says the effect is active **only while the finger is held down**. This means:

- `isActive` is controlled by touch events, not by a command
- The C++ engine needs to know about finger-down/finger-up to apply/bypass the FX
- But there's no `FX_ENGAGE` / `FX_DISENGAGE` command in the schema

This is a real-time audio routing decision that **must** be driven from the engine side, not just the UI side.

> [!WARNING]
> **Recommendation:** Add `FX_ENGAGE` and `FX_DISENGAGE` (or a single `SET_FX_ACTIVE { active: boolean }`) command. The UI sends it on touch-down/touch-up. The engine applies/bypasses the effect accordingly.

---

### 4. `AppState` Missing Fields

The `AppState` interface (§3.4) is missing fields that are referenced elsewhere:

| Missing Field | Referenced By |
|:---|:---|
| `transport.quantiseEnabled` | Quantise button in Mixer tab (§7.6.5) |
| `mic.monitorUntilLooped` | Microphone tab toggle (§7.6.6) |
| `mic.monitorInput` | Microphone tab toggle (§7.6.6) |
| `mic.inputGain` | Gain knob (§7.6.6, Audio_Engine_Specifications §6.2) |
| `settings` (any) | Settings panel (§7.4) — themes, zoom, buffer size, etc. |
| `activeMode.fxMode` | Whether FX resampling mode is active |
| `activeMode.selectedSourceSlots` | Which slots are selected for FX Mode routing |
| `session.noteNamesEnabled` | Note Names toggle (§7.6.8) |

> [!IMPORTANT]
> **Recommendation:** Extend the `AppState` interface to include these fields before the Schema Foundation bead (Task 1.1) is executed. Otherwise the agent will define a partial state that needs patching later.

---

## 🟠 SHOULD FIX — Will Cause Confusion

### 5. Stale CMake References Need Removal

Projucer has been decided as the authoritative build system (§8.2 M4), but several stale CMake references remain across the spec docs that will confuse agents:

- `Spec_FlowZone_Looper1.6.md` §4.1 directory structure still shows `/cmake` as a top-level directory
- `Spec_FlowZone_Looper1.6.md` §8.2 M3, §8.6, and Task 7.1 reference "CMake target" for PluginHostApp
- `FlowZone_Risk_Assessment.md` RISK 4 discusses CMake extensively, and the recommended bead ordering references CMake

The PluginHostApp (headless child process binary) should use a **second Projucer Console Application** target, not CMake.

> **Recommendation:** Search-and-replace all remaining CMake references across the spec docs. Replace "CMake target" with "Projucer Console Application target" where discussing PluginHostApp. Remove `/cmake` from the directory structure.

---

### 6. "More Options" vs "Settings Panel" Overlap

The spec defines two separate settings interfaces:

1. **Settings Panel** (§7.4) — 4 tabs: Interface, Audio, MIDI & Sync, Library & VST
2. **More Options Modal** (§7.6.8) — accessed via "More" button in Mixer tab, contains audio settings, Ableton Link toggle, Note Names toggle

These overlap significantly:
- Both have audio device selection, sample rate, and buffer size controls
- §7.2 says "Settings accessed via 'More' button in Mixer tab (§7.6.8)" for phone mode
- But §7.4 describes a full settings panel with 4 tabs

**Question:** Is the "More Options" modal the *only* way to access settings on phone? If so, it's missing most of the Interface tab (zoom, theme, font size, reduce motion, emoji skin tone) and all of the MIDI & Sync tab. If there's a separate full Settings screen, where is the navigation entry point?

> **Recommendation:** Decide: either (a) the "More" button opens a full Settings screen with all 4 tabs, or (b) the "More" modal is a quick-access subset and there's a separate Settings screen accessed from the Home/Jam Manager. Update the spec to be explicit.

---

### 7. Buffer Size Options Inconsistency

| Location | Values |
|:---|:---|
| §7.4.B (Settings Panel) | 16, 32, 64, 128, 256, 512, 1024 |
| §7.6.8 (More Options) | 64, 128, 256, 512, 1024 |

The Settings Panel includes buffer sizes 16 and 32, which are **unrealistic for most audio interfaces** and will cause xruns on almost all hardware. The More Options modal doesn't include them.

> **Recommendation:** Standardise on 64, 128, 256, 512, 1024. Remove 16 and 32. A 16-sample buffer at 48kHz is 0.33ms — no consumer audio hardware can sustain this.

---

### 8. UI_Layout_Reference.md Has Stale Preset Names

The `UI_Layout_Reference.md` preset examples (Slicer, Razzz, Acrylic, Ting…) don't match the actual preset names defined in `Audio_Engine_Specifications.md` (Sine Bell, Saw Lead, Sub, Growl…).

The UI Layout Reference appears to be sourced from an earlier design iteration and was never reconciled with the final audio engine spec.

> **Recommendation:** Update `UI_Layout_Reference.md` preset examples to use the actual preset names from `Audio_Engine_Specifications.md`, or add a note that UI Layout Reference preset names are illustrative, not authoritative.

---

### 9. Auto-Merge "9th Loop" Trigger Ambiguity

§1.1 says: "The '9th Loop' trigger automatically sums Slots 1-8 into Slot 1."

Questions that an implementing agent will need answers to:
- Does the auto-merge replace Slot 1's audio with the sum, or append?
- Are Slots 2-8 cleared after merge?
- Is the merged result committed to riff history automatically?
- What happens to the slot metadata (instrumentCategory, presetId, userId) after merge?

> **Recommendation:** Add a short subsection (perhaps §7.6.2.1 or a new §3.8) describing the auto-merge algorithm step-by-step, including what state transitions occur.

---

### 10. Ableton Link Listed as V2 Feature But Has Toggle in V1 UI

§1.3 (Future Goals V2) explicitly lists "Ableton Link Integration" as deferred. But:
- §7.6.8 (More Options Modal) has an "Ableton Link" toggle set to default OFF
- `UI_Layout_Reference.md` includes it in the more_options_modal section

For a V1 build, the Ableton Link toggle should either be:
- Removed entirely from the UI
- Present but disabled/greyed out with a "Coming in V2" tooltip

> **Recommendation:** Keep it in the UI spec as a disabled toggle with a "Coming Soon" label. This way the agent doesn't try to implement the functionality but does create the UI element.

---

## 🟡 MINOR — Worth Noting

### 11. "Redo" Button vs No Redo Command

§7.6.5 shows a **Redo** button in the Mixer tab primary actions. But:
- The Command Schema has `UNDO { scope: 'SESSION' }` but no `REDO` command
- `SessionStateManager` (§2.2.G) describes an undo stack but doesn't mention redo
- The Error Matrix has `ERR_NOTHING_TO_UNDO` but no `ERR_NOTHING_TO_REDO`

> **Recommendation:** Add `REDO { scope: 'SESSION' }` to the Command Schema and `ERR_NOTHING_TO_REDO` (code 4002) to ErrorCodes. Or remove the Redo button from the UI if redo is not intended for V1.

---

### 12. Home Button and Help Button Undefined

§7.6.1 Header shows "Home button, Help button" in the left section. But:
- **Home button:** Presumably navigates to the Jam Manager (§7.7), but this navigation flow is not specified
- **Help button:** No help system is defined anywhere in the spec

> **Recommendation:** Define what "Home" does (navigates to Jam Manager) and either spec a minimal help system (link to web docs?) or remove the Help button for V1.

---

### 13. "Share" and "Add" Buttons in Header Undefined

§7.6.1 Header right section has "Share button, Add button." Neither has a defined command or behavior:
- **Share:** Share what? The current session? A riff? As what format?
- **Add:** Add what? A new slot? A new instrument? The Riff History View (§7.6.7) has "Export Video", "Export Stems" — is Share related to these?

> **Recommendation:** Either define the behavior of these buttons or mark them as V2 placeholders.

---

### 14. "Export Video" in Riff History View

§7.6.7 lists "Export Video" as a riff action. Video export is not defined anywhere else in the spec and is a significant feature (requires rendering waveforms/visualizations to video frames, encoding, etc.).

> **Recommendation:** Mark "Export Video" as V2. Keep "Export Stems" and "Delete Riff" for V1.

---

## Architectural Reliability Recommendations

These are not inconsistencies but practical recommendations for build reliability:

### R1: Establish a "Compile Gate" Workflow

Before each bead is marked complete, require:
1. C++ compiles with zero warnings (`-Wall -Wextra -Werror`)
2. Catch2 tests pass
3. Vitest tests pass
4. React app builds without TypeScript errors

Document this as a standard bead completion checklist so every agent follows it.

### R2: Pin All Dependency Versions

The spec mentions JUCE 8.0.x, React 18.3.1, TypeScript 5.x, Vite, Catch2 v3, Vitest. 

**Pin exact versions** in the Phase 0 skeleton bead:
- `package.json` should use exact versions, not ranges (e.g., `"react": "18.3.1"`, not `"^18.3.1"`)
- JUCE version should be specified in the Projucer file
- This prevents different agents getting different dependency versions and introducing incompatibilities

### R3: Define a "Minimum Viable Bead" for Internal Synths

The Audio Engine Spec defines 24 synth presets with detailed parameters. For the first pass, consider implementing a **single generic synth engine** with parameter-driven presets rather than 24 separate implementations. All 24 presets use the same architecture (§2.0: `Oscillator(s) → Amplitude Envelope → Filter → Filter Envelope → Output`). The differences are just parameter values.

This means one bead can deliver all 24 presets by implementing the engine once and defining presets as JSON parameter sets. This is dramatically less code and less bug surface area.

### R4: Add Explicit "What the User Sees" Descriptions to Checkpoints

The Human Testing Checkpoints (§8.8) are excellent but could be more specific for a non-engineer. For example, Checkpoint 1 says "Visual check" — but check for what exactly? Consider adding screenshot mockups or very specific descriptions like "You should see a dark grey window with the text 'Hello FlowZone' centered in white."

---

## Summary Action Items

| # | Priority | Action | Effort |
|:--|:---------|:-------|:-------|
| 1 | 🔴 Must | Add ~12 missing commands to Command Schema | 30 min |
| 2 | 🔴 Must | Fully specify FX Mode audio routing | 30 min |
| 3 | 🔴 Must | Add `FX_ENGAGE`/`FX_DISENGAGE` command | 5 min |
| 4 | 🔴 Must | Add missing `AppState` fields | 15 min |
| 5 | 🟠 Should | Remove stale CMake references across all spec docs | 10 min |
| 6 | 🟠 Should | Resolve Settings Panel vs More Options overlap | 15 min |
| 7 | 🟠 Should | Standardise buffer size options | 5 min |
| 8 | 🟠 Should | Update UI_Layout_Reference preset names | 10 min |
| 9 | 🟠 Should | Define auto-merge algorithm step-by-step | 15 min |
| 10 | 🟠 Should | Clarify Ableton Link toggle in V1 UI | 5 min |
| 11 | 🟡 Minor | Add `REDO` command or remove button | 5 min |
| 12 | 🟡 Minor | Define Home and Help button behavior | 5 min |
| 13 | 🟡 Minor | Define Share and Add button behavior | 5 min |
| 14 | 🟡 Minor | Mark Export Video as V2 | 2 min |

**Total estimated spec update time: ~2.5 hours**

> [!TIP]
> Fixing the 🔴 items (1-4) before bead creation is **essential**. The 🟠 items (5-10) should be fixed but won't block initial beads. The 🟡 items (11-14) can be resolved during build as clarifications.
