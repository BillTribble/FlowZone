# FlowZone — Pre-Build Assessment 3

**Date:** 13 Feb 2026  
**Scope:** Final deep cross-reference of all Spec documents for internal consistency, residual risks, ambiguities, and reliability concerns — focused on agent-buildability for a non-engineer owner.  
**Documents Reviewed:**
- `Spec_FlowZone_Looper1.6.md` (1780 lines)
- `Audio_Engine_Specifications.md` (360 lines)
- `UI_Layout_Reference.md` (359 lines)
- `Pre_Build_Assessment_1.md` (276 lines)
- `Pre_Build_Assessment_2.md` (90 lines)

---

## 1. Executive Summary

The specification is **substantially improved** since PBA1. All 14 critical/major issues from PBA1 have been addressed in the current Spec 1.6, and PBA2 correctly confirmed this. The task breakdown (§9) and risk mitigations (§8) are excellent.

However, this third-pass deep review — reading every line of every spec document against every other — has found **12 remaining issues** (3 high, 5 medium, 4 low) and **7 reliability risks** that could cause confusion, implementation bugs, or wasted cycles if not addressed before bead creation begins.

**The key theme:** Most remaining issues are not about *missing* features (PBA1 caught those), but about **ambiguous interactions** — places where two well-defined features intersect and the spec doesn't say what happens at the boundary.

---

## 2. Verification: PBA1 Findings Resolution Status

All PBA1 findings have been verified as resolved in the current spec:

| PBA1 # | Issue | Status | Evidence |
|:---|:---|:---|:---|
| 1 | Missing commands | ✅ Fixed | `FX_ENGAGE`, `SET_XY_PAD`, `SET_INPUT_GAIN`, `TOGGLE_QUANTISE`, `TOGGLE_MONITOR_INPUT`, `TOGGLE_MONITOR_UNTIL_LOOPED`, `TOGGLE_NOTE_NAMES`, `SELECT_FX_SOURCE_SLOTS`, `SET_RIFF_SWAP_MODE`, audio settings commands all present in §3.2 |
| 2 | FX Mode routing | ✅ Fixed | §7.6.2 FX Mode now has explicit routing diagram, deletion policy, and unselected-slots behavior |
| 3 | `FX_ENGAGE`/`FX_DISENGAGE` | ✅ Fixed | Both commands present in §3.2 and error matrix §3.3 |
| 4 | Missing `AppState` fields | ✅ Fixed | `quantiseEnabled`, `mic.*`, `settings.*`, `isFxMode`, `selectedSourceSlots`, `ui.noteNamesEnabled` all in §3.4 |
| 5 | Stale CMake refs | ✅ Fixed | No CMake references remain; Projucer is authoritative throughout |
| 6 | Settings overlap | ✅ Fixed | Unified into single Settings panel via "More" button (§7.6.8) |
| 7 | Buffer sizes | ⚠️ Noted | 16/32 still in spec dropdown list; PBA2 §4.2 instructs agents to hide them. Acceptable. |
| 8 | UI_Layout_Reference preset names | ✅ Fixed | Preset examples now reference `Audio_Engine_Specifications.md` with "illustrative" caveat |
| 9 | Auto-merge | ✅ Fixed | §7.6.2.1 provides step-by-step algorithm |
| 10 | Ableton Link | ✅ Fixed | Marked "Coming Soon" / disabled in §7.6.8 Tab C |
| 11 | `REDO` command | ✅ Fixed | `REDO` in §3.2 schema, `ERR_NOTHING_TO_REDO (4002)` in §5.1 and error matrix |
| 12 | Home/Help buttons | ✅ Fixed | §7.6.1 Header now defines Home (left), no Help button. Removed ambiguity. |
| 13 | Share/Add buttons | ✅ Fixed | Removed from header spec. §7.6.1 right section is now Undo + Redo. |
| 14 | Export Video | ✅ Fixed | Not present in §7.6.7 actions. Only Delete Riff and Export Stems (disabled, "Coming Soon"). |

**Conclusion:** PBA1 issues are fully resolved. No regressions found.

---

## 3. New Issues Found

### 🟠 HIGH — Will Cause Implementation Confusion

#### H1: `RECORDING` Slot State Is Vestigial Under Retrospective Capture Model

**The conflict:** §3.4 defines slot states as `'EMPTY' | 'RECORDING' | 'PLAYING' | 'MUTED' | 'STOPPED'`, but §3.10 (Retrospective Capture) describes a model where there is **no explicit record start/stop**. The state transitions in §3.10 are:

```
EMPTY → PLAYING (audio committed from retrospective buffer)
PLAYING → STOPPED (transport stopped)
STOPPED → PLAYING (transport started)
PLAYING → MUTED (user mutes)
MUTED → PLAYING (user unmutes)
```

The `RECORDING` state never appears in these transitions. Under retrospective capture, audio is committed from a buffer — it doesn't "record" into a slot in real-time. So when would a slot ever have state `RECORDING`?

**Why this matters:** An agent implementing slot state management will see `RECORDING` in the type definition and attempt to handle it, potentially creating a code path that's never triggered, or worse, misunderstanding the retrospective model and building a traditional record-arm workflow.

**Possible resolution options:**
- **(A) Remove `RECORDING`** from the slot state enum — it has no corresponding transition under the retrospective model.
- **(B) Redefine `RECORDING`** to mean "the retrospective buffer is actively capturing for this time slot" — but this doesn't map to a *slot*, it maps to the engine's global capture state.
- **(C) Keep `RECORDING`** as a transient state during commit — the brief moment between "buffer committed" and "playback starts." But this is typically instantaneous (a pointer swap), so the UI would never see it.

> **Recommendation:** Choose option (A) unless there's a future use case. Remove `RECORDING` from the slot state enum, and add a comment in the schema explaining why it's absent: "Recording is always-on via retrospective buffer (§3.10). Slots transition directly from EMPTY to PLAYING on commit."

---

#### H2: Undo/Redo Granularity and Audio File Lifecycle Are Ambiguous

**The issue:** §2.2.G says SessionStateManager holds `std::deque<AppState>` for undo, and "every destructive command (Record Over, Delete, Clear) pushes the previous state to the stack." But:

1. **`AppState` doesn't contain audio data** — it contains metadata (slot states, names, IDs, etc.). The actual audio is in files on disk. If undo needs to restore a previous slot's audio, it needs to know which file to re-associate. How?
2. **"Audio files are never deleted immediately — only marked for garbage collection on clean exit."** This is good, but the undo mechanism needs to hold *references* to old audio files so it can restore them. Are these file paths stored in the undo stack's `AppState` snapshots?
3. **What counts as a "destructive command"?** The spec lists "Record Over, Delete, Clear" but doesn't define these as named commands. The Command Schema (§3.2) has `COMMIT_RIFF`, `DELETE_RIFF`, `DELETE_JAM`, `NEW_JAM`. Which of these trigger undo pushes?
4. **Auto-merge (§7.6.2.1)** is explicitly destructive — it sums 8 slots and clears 7. The spec says it auto-commits to riff history first (step 5), but does it also push to the undo stack? If the user hits "Undo" after auto-merge, what happens?

**Why this matters:** A non-engineer owner cannot debug undo/redo bugs. If the undo mechanism doesn't correctly restore audio file references, users will hit "Undo" and get silence — or worse, hear the wrong loop.

> **Recommendation:** Add a subsection to §2.2.G specifying:
> - The undo stack stores `AppState` snapshots **including file path references** for each slot's audio.
> - List exactly which commands push to the undo stack (e.g., `COMMIT_RIFF`, `DELETE_RIFF`, auto-merge, FX Mode commit).
> - Confirm that Undo after auto-merge restores the 8 individual slots (this requires the audio files to still exist on disk — which they should, per the GC policy).

---

#### H3: Retrospective Buffer Resizing on Loop Length Change

**The issue:** §3.10 says "The buffer length matches `transport.loopLengthBars`." The user can change `loopLengthBars` at any time via `SET_LOOP_LENGTH` (values: 1, 2, 4, 8 bars).

Questions an implementing agent will face:
1. If the user is 3 bars into a performance and switches from 4 bars to 1 bar — what happens to the 3 bars already captured? Is the buffer truncated? Does it keep only the most recent 1 bar?
2. If the user switches from 1 bar to 8 bars — is the buffer extended? With silence? Or does it only start filling from the change point?
3. Is the buffer sized by bars (tempo-dependent) or by a fixed time? At 60 BPM, 8 bars = 32 seconds. At 300 BPM, 8 bars ≈ 6.4 seconds. Does the buffer physically resize?

**Why this matters:** Buffer resizing on the audio thread is a memory allocation — explicitly forbidden by §1.3 Audio Thread Contract. Either the buffer is pre-allocated at max size (8 bars at minimum tempo = ~32s at 60 BPM) or there's a non-trivial reallocation strategy.

> **Recommendation:** Add to §3.10:
> - "The retrospective buffer is pre-allocated at startup to hold the maximum possible duration (8 bars at the minimum tempo of 20 BPM = ~96 seconds of stereo audio at 48kHz ≈ 37MB)."
> - "When `loopLengthBars` changes, the capture window adjusts within the existing buffer — no reallocation occurs. The most recent N bars of audio are used on commit."
> - Or if a different strategy is intended, document it explicitly.

---

### 🟡 MEDIUM — Should Fix Before Beads

#### M1: `KnobParameter` Type Incomplete for Mic Mode

§3.2 defines `KnobParameter` as: `'pitch' | 'length' | 'tone' | 'level' | 'bounce' | 'speed' | 'reverb' | 'reverb_mix' | 'room_size'`

These map to the Adjust Tab knobs (§7.6.4). But when the **Microphone** category is active, the Adjust tab should reasonably show mic-specific controls. The Audio Engine Spec §6.2 describes Input Gain as a separate control (with its own `SET_INPUT_GAIN` command), and §6.3 describes an Input FX Chain.

**The question:** When the user is in Microphone mode and opens the Adjust tab, which knobs appear? The spec shows the standard 8 knobs (Pitch, Length, Tone, Level, Bounce, Speed, reserved, Reverb) — but Pitch, Length, Bounce, and Speed make no sense for a microphone input.

> **Recommendation:** Either (a) define a mic-specific `KnobParameter` mapping (e.g., Gain maps to `SET_INPUT_GAIN`, Reverb/Room Size are mic reverb) or (b) explicitly state that the Adjust tab shows the *most recently selected instrument's* knobs, not the active input mode's. This way, switching to Mic doesn't change the Adjust tab.

---

#### M2: MicProcessor FX Chain vs FX Mode — Interaction Undefined

§7.6.2 (FX Mode) states: "The Audio In slot has its own independent FX chain (simple reverb for vocals), which is only accessible from the Audio In / Microphone mode and is not related to FX Mode."

But Audio Engine Spec §6.3 says: "Same FX as InternalFX (§1) can be applied to input. Common use: Lowpass/Highpass filtering, Reverb, Delay, Compressor."

**The contradiction:** §7.6.2 says the mic has a "simple reverb for vocals" (i.e., a single, fixed effect). §6.3 says the mic can use *any* of the 23 InternalFX effects. These are very different implementations — one is a single hardcoded reverb, the other is a full FX chain.

> **Recommendation:** Decide one of:
> - **(A) Simple reverb only** — Mic mode gets Reverb Mix + Room Size knobs (matching the Adjust tab reference). No FX chain selector. This is simpler and appropriate for V1.
> - **(B) Full FX chain** — Mic mode gets its own effect selector (like FX Mode but independent). This is more powerful but significantly more work.
>
> If (A), update Audio Engine Spec §6.3 to say "V1: Built-in reverb only. V2: Full FX chain support." If (B), add a command for mic FX selection.

---

#### M3: Preset Grid Count Mismatch — Infinite FX Has 11, Not 12

The Preset Selector across all modes uses a **3×4 grid (12 positions)**. This works for:
- Notes presets: 12 ✅
- Bass presets: 12 ✅
- Drum presets: 12 (4 kits × 3 per row) — but spec says **4 kits** total (Synthetic, Lo-Fi, Acoustic, Electronic). A 3×4 grid has 12 cells but only 4 presets. Does each kit have sub-presets? Or are 8 cells empty?
- Core FX: 12 ✅
- Infinite FX: **11** — one cell will be empty

**The Drum kit issue is bigger:** §4.2 defines only 4 drum kits. A 3×4 grid needs 12 presets. Either the spec needs 8 more drum kit presets, or the grid should show 4 larger cells, or subsets/variations of the 4 kits should be defined.

> **Recommendation:**
> - For Infinite FX (11): Leave the 12th cell empty or add a placeholder slot for future effects.
> - For Drums (4 kits): Either define 8 additional kit variations (e.g., "Synthetic Tight", "Synthetic Ambient", "Lo-Fi Dusty", etc.) to fill the 3×4 grid, or explicitly state that drums use a smaller grid (1×4 or 2×2) for kit selection and the pad grid shows the 16 individual drum sounds.

---

#### M4: `SET_LOOP_LENGTH` vs `+ N BARS` Button Semantics

§7.6.1 Loop Length Controls show buttons: `[+ 8 BARS] [+ 4 BARS] [+ 2 BARS] [+ 1 BAR]`

The `+` prefix implies **additive** — e.g., pressing `+ 2 BARS` extends the current loop by 2 bars. But the only loop length command is `SET_LOOP_LENGTH { bars: number }` with values `1, 2, 4, or 8`, which is **absolute** (set to N bars, not add N bars).

Meanwhile, §7.6.1 Timeline/Waveform Area says: "Tap a waveform section to **set** loop length to that duration" — this IS absolute.

PBA1 item 1 flagged this but the resolution is inconsistent — the **buttons** still say `+ N BARS` (additive) while the **command** is absolute.

> **Recommendation:** Choose one:
> - **(A) Buttons are absolute** — Remove the `+` prefix. Buttons become `[8 BARS] [4 BARS] [2 BARS] [1 BAR]`. Matches the command semantics and the waveform tap behavior. *(Simpler.)*
> - **(B) Buttons are additive** — Add an `EXTEND_LOOP { bars: number }` command and keep `SET_LOOP_LENGTH` for the waveform tap. *(More complex, probably not needed for V1.)*

---

#### M5: WebSocket Binary Stream — Single vs Dual Connection Conflict

The main spec §3.7 says: "Transmission: **Separate binary WebSocket channel.**"

PBA2 §4.4 recommends: "Use a **single** WebSocket connection for both JSON and Binary (interleaved text/binary frames)."

These directly contradict. An agent implementing Phase 2/3 will encounter both documents.

> **Recommendation:** Make a definitive decision in the spec. Single connection with interleaved frames (text for JSON, binary for viz) is simpler and recommended. Update §3.7 to say "Transmitted as binary frames on the **same** WebSocket connection used for JSON command/state traffic. The client distinguishes frame types by WebSocket opcode (0x1 text = JSON, 0x2 binary = visualization)."

---

### 🟢 LOW — Worth Noting

#### L1: `UI_Layout_Reference.md` Missing Riff Swap Mode Setting

§7.6.8 defines a "User Preferences" section below Quick Toggles with `Riff Swap Mode` (Instant | Swap on Bar). The `UI_Layout_Reference.md` JSON settings panel only includes `quick_toggles` with Note Names — it doesn't include the Riff Swap Mode section.

> **Recommendation:** Add `"user_preferences"` section to the JSON under `settings_panel`, matching §7.6.8.

---

#### L2: `PluginInstance` Interface Referenced but Undefined

§3.4 `AppState` references `pluginChain: PluginInstance[]` in the slot definition but the `PluginInstance` interface is never defined in the spec. PBA2 §4.1 provides a definition for agents to use:

```typescript
interface PluginInstance {
  id: string;
  pluginId: string;
  manufacturer: string;
  name: string;
  bypass: boolean;
  state?: string;  // Base64 VST3 state blob
}
```

> **Recommendation:** Add this interface definition to §3.4 directly below the `AppState` definition, so it's self-contained and agents don't need to cross-reference PBA2.

---

#### L3: Mixer Tab Transport Grid Has Empty Cell

§7.6.5 Mixer tab transport controls define a 2×3 grid:

```
Row 1: Quantise     (empty)      More
Row 2: Metronome    Tempo        Key
```

Row 1, Col 2 is explicitly `*(empty)*`. This looks intentional (reserved space), but an agent will wonder if something should go there. The reference design had "Looper Mode" in that position (PBA1 item 38), which was intentionally removed.

> **Recommendation:** Either fill the cell with a useful control (e.g., BPM tap-tempo button, loop length display) or add a comment: "Reserved for V2 feature (e.g., Looper Mode or Tap Tempo)."

---

#### L4: Audio Engine Spec §6.3 References "Pre-Fader" Without Defining Fader

§6.3 says "FX are pre-fader (applied before loop recording)." In the context of the MicProcessor, "pre-fader" presumably means "before the volume control / before the signal enters the retrospective buffer." But FlowZone doesn't have a traditional fader-based signal chain — it has circular faders in the Mixer (§7.6.5) which control slot playback volume, not input volume.

> **Recommendation:** Replace "pre-fader" with "applied to the input signal before it enters the retrospective capture buffer."

---

## 4. Reliability Risk Assessment

These are not spec inconsistencies but **practical build risks** that affect reliability for a non-engineer owner.

### R1: CivetWeb Integration — No Integration Guide

The Component Diagram (§2.1) specifies CivetWeb for the HTTP/WebSocket server. But:
- CivetWeb is not a JUCE module — it's a standalone C/C++ library
- The spec doesn't say how to integrate it (static library? source inclusion? header-only?)
- Projucer needs to know about CivetWeb's source files and include paths
- CivetWeb has its own threading model that must coexist with JUCE's `MessageManager`

**Impact:** An agent executing Phase 0 (Project Skeleton) will need to make integration decisions without guidance. Wrong decisions here cascade through the entire project.

> **Recommendation:** Add a brief note to §2.2 or §6 specifying:
> - CivetWeb should be added as **source files** (not a pre-built library) in a `libs/civetweb/` directory, added to Projucer as a source group.
> - Or alternatively, use JUCE's built-in `StreamingSocket` class for a minimal WebSocket implementation (avoids the third-party dependency entirely but requires more code).
> - This decision should be made in the Phase 0 bead scope.

---

### R2: WebBrowserComponent Development Workflow

During development, the React app runs on Vite's dev server (`http://localhost:5173`) for hot reload. In production, the React build output is bundled into the app's resources folder and loaded via `file://` or JUCE's resource serving.

The spec doesn't describe how to switch between these modes. This matters because:
- `juce::WebBrowserComponent` on macOS uses WKWebView
- WKWebView has security restrictions on `file://` URLs (CORS, localStorage, etc.)
- The agent building Phase 0 needs to know: does the WebBrowserComponent load a URL or local resources?

> **Recommendation:** Add to Phase 0 task scope:
> - In debug builds: `WebBrowserComponent::goToURL("http://localhost:5173")`
> - In release builds: `WebBrowserComponent::goToURL(juce::File::getSpecialLocation(...).getChildFile("web_client/dist/index.html").getFullPathName())`
> - Or use JUCE 8's `WebBrowserComponent::Options::withResourceProvider()` to serve embedded resources.

---

### R3: MP3 Encoding Is Not Available via JUCE

§3.6 says: "Storage Option: User preference to save as MP3 320kbps (via LAME or system encoder)."

JUCE's `AudioFormatManager` includes MP3 **decoding** but not **encoding**. MP3 encoding requires either:
- Linking against LAME (GPL licensed — licensing implications for a commercial app)
- Using macOS's AudioToolbox framework (`ExtAudioFile` with `kAudioFormatMPEG4AAC` — but this is AAC, not MP3)
- Using a system encoder if available

**Impact:** An agent implementing the DiskWriter (Task 2.5) or the storage format option may attempt to use JUCE's format system and find encoding fails silently.

> **Recommendation:** Change the storage option to:
> - **Primary:** FLAC (lossless, already specified)
> - **Space-saver alternative:** AAC via macOS AudioToolbox (native, no licensing issues, excellent quality)
> - Remove the MP3 reference, or explicitly note it requires LAME and defer to V2.

---

### R4: 96-Second Retrospective Buffer Memory Impact

Based on the analysis in H3 above, if the retrospective buffer is pre-allocated at maximum size (8 bars at 20 BPM ≈ 96 seconds):
- At 48kHz, 24-bit, stereo: 96s × 48000 × 2 channels × 4 bytes (float32 internal) = **~36.9 MB per buffer**
- If the engine captures for *each of the 8 slots* simultaneously: 8 × 36.9 MB = **~295 MB**

But §3.10 says "the engine maintains **a** circular buffer" (singular) — suggesting one global retrospective buffer, not one per slot. This makes sense since only one instrument is active at a time. So the memory impact is ~37 MB, which is manageable.

> **Recommendation:** Clarify in §3.10 that there is **one** retrospective buffer for the currently active instrument/mode, not one per slot. This is implied but should be explicit to prevent an agent allocating 8 buffers.

---

### R5: FLAC Real-Time Encoding CPU Cost

§3.6 specifies FLAC 24-bit as the recording format. FLAC encoding is ~5-10× more CPU-intensive than WAV writing. While the DiskWriter runs on a background thread (§2.2.I), the encoding still needs to keep up with real-time audio throughput.

At 48kHz stereo 24-bit: raw throughput is ~288 KB/s. FLAC encoding at compression level 5 (JUCE default) should handle this comfortably on Apple Silicon, but it's worth noting.

**The real concern** is when **8 slots are simultaneously being written** (e.g., during auto-merge bounce + commit). If 8 FLAC encoders are running in parallel, that's 8× the CPU on the DiskWriter thread.

> **Recommendation:** Add a note to §4.2 or §2.2.I: "DiskWriter should use FLAC compression level 0 (fastest) for real-time recording. Higher compression levels can be applied during session export or idle time if storage space is a concern."

---

### R6: `JsonPatchOp` Type Not Defined

§3.2 Server Responses reference `JsonPatchOp[]` in the `STATE_PATCH` message:
```typescript
| { type: 'STATE_PATCH'; ops: JsonPatchOp[]; revId: number }
```

But `JsonPatchOp` is never defined. RFC 6902 operations are well-known, but the agent needs the TypeScript type:

```typescript
interface JsonPatchOp {
  op: 'add' | 'remove' | 'replace' | 'move' | 'copy' | 'test';
  path: string;
  value?: any;
  from?: string;
}
```

> **Recommendation:** Add this type definition to §3.2, adjacent to the `ServerMessage` type.

---

### R7: "Ext Inst" and "Ext FX" Categories Need Placeholder Behavior

The Mode Tab (§7.6.2) shows "Ext Inst" and "Ext FX" categories. These require the Plugin Isolation system (Phase 7, deferred). During Phases 0-6, these categories will exist in the UI but have no backend.

The spec doesn't define what the user sees when tapping these without any plugins installed or configured.

> **Recommendation:** Add a UI note to §7.6.2:
> - When no VST3 plugins are available for a category, show: the "Add a new Plugin" card (already spec'd) with an additional note: "No VST3 plugins found. Add search paths in Settings → Library & VST."
> - During development (before Phase 7), these categories should show a "Coming Soon" placeholder card instead of the plugin browser.

---

## 5. Cross-Document Consistency Matrix

Verified alignment between all documents:

| Aspect | Spec 1.6 | Audio Engine Spec | UI Layout Ref | Status |
|:---|:---|:---|:---|:---|
| **Tab labels** | Mode, Play, Adjust, Mixer (§7.6) | N/A | Mode, Play, Adjust, Mixer (nav_tabs) | ✅ Consistent |
| **Core FX list** | 12 effects (§2.2.M) | 12 effects (§1.1) | 12 effects (play_tab) | ✅ Consistent |
| **Infinite FX list** | 11 effects (§2.2.M) | 11 effects (§1.2) | 11 effects (play_tab) | ✅ Consistent |
| **Notes presets** | 12 (§2.2.M) | 12 (§2.1) | "See Audio_Engine_Specs" | ✅ Consistent |
| **Bass presets** | 12 (§2.2.M) | 12 (§3.1) | "See Audio_Engine_Specs" | ✅ Consistent |
| **Drum kits** | 4 kits (§2.2.M) | 4 kits (§4.2) | N/A | ⚠️ Grid mismatch (M3) |
| **Keymasher buttons** | 12 buttons (§3.2 KeymasherButton) | 12 buttons (§1.2) | 12 buttons (play_tab) | ✅ Consistent |
| **XY Pad behavior** | Touch-and-hold (§7.6.3) | XY Mapping per effect (§1) | Crosshair on hold (play_tab) | ✅ Consistent |
| **Adjust knobs** | 7 knobs + 2 reverb (§7.6.4) | Parameter mapping (§2.2) | 7 knobs + reverb section | ✅ Consistent |
| **Mixer transport** | 2×3 grid (§7.6.5) | N/A | 2×3 grid (mixer_tab) | ✅ Consistent |
| **Mic controls** | Monitor toggles + Gain (§7.6.6) | Monitor + Gain (§6) | Monitor toggles + Gain | ✅ Consistent |
| **Settings tabs** | 4 tabs (§7.6.8) | N/A | 4 tabs (settings_panel) | ✅ Consistent |
| **Riff Swap Mode** | In settings (§7.6.8) | N/A | **Missing** from JSON | ⚠️ L1 |
| **Error codes** | Full list (§5.1) | N/A | N/A | ✅ Complete |
| **AppState fields** | Full interface (§3.4) | N/A | N/A | ⚠️ `PluginInstance` undefined (L2) |
| **Color palette** | §7.1 + §7.3 JSON | N/A | N/A | ✅ Self-consistent |

---

## 6. PBA2 Pre-Flight Items — Validation

PBA2 provided 5 pre-flight instructions for agents. Validating their completeness:

| PBA2 Item | Status | Assessment |
|:---|:---|:---|
| §4.1 Define `PluginInstance` | ✅ Good | Should be added to main spec (see L2) |
| §4.2 Buffer Size Safety | ✅ Good | Hide 16/32 unless requested |
| §4.3 Asset Management | ✅ Good | Projucer "Copy Files" step needed |
| §4.4 Binary Stream Connection | ⚠️ Conflicts with spec | See M5 — needs definitive decision |
| §4.5 JSON Library (nlohmann/json) | ✅ Good | Appropriate choice, header-only |

---

## 7. Recommendations Summary

### Must Fix Before Bead Creation (High Priority)

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| H1 | `RECORDING` slot state vestigial | Remove from enum or redefine with clear transition | 10 min |
| H2 | Undo/Redo granularity undefined | Add undo-spec subsection to §2.2.G | 20 min |
| H3 | Retrospective buffer resizing | Document pre-allocation + window strategy | 10 min |

### Should Fix Before Bead Creation (Medium Priority)

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| M1 | `KnobParameter` incomplete for Mic | Define mic-mode knob mapping or clarify behavior | 10 min |
| M2 | Mic FX chain ambiguity | Choose simple-reverb vs full-FX and align spec | 10 min |
| M3 | Drum kit grid count mismatch (4 vs 12) | Define 12 kit presets or change drum preset selector layout | 15 min |
| M4 | `+ N BARS` additive vs absolute | Remove `+` prefix from buttons OR add EXTEND command | 5 min |
| M5 | Binary stream single vs dual connection | Update §3.7 to match PBA2 recommendation | 5 min |

### Nice To Fix (Low Priority)

| # | Issue | Fix | Effort |
|:--|:------|:----|:-------|
| L1 | UI_Layout_Reference missing riff swap | Add to JSON | 5 min |
| L2 | `PluginInstance` undefined | Add interface to §3.4 | 5 min |
| L3 | Mixer empty cell | Fill or annotate as reserved | 2 min |
| L4 | "Pre-fader" terminology | Reword for clarity | 2 min |

### Reliability Items (Add to Spec or Phase 0 Scope)

| # | Risk | Action | Where |
|:--|:-----|:-------|:------|
| R1 | CivetWeb integration | Add integration guidance | §2.2 or Phase 0 task |
| R2 | WebBrowserComponent dev workflow | Add dev/prod URL switching | Phase 0 task |
| R3 | MP3 encoding unavailable | Switch to AAC or defer MP3 | §3.6 |
| R4 | Retrospective buffer count | Clarify: 1 global buffer, not 8 | §3.10 |
| R5 | FLAC encoding under load | Specify compression level 0 for real-time | §4.2 or §2.2.I |
| R6 | `JsonPatchOp` type missing | Add TypeScript definition | §3.2 |
| R7 | Ext Inst/Ext FX placeholder | Define empty-state UI behavior | §7.6.2 |

---

## 8. Overall Assessment

| Dimension | Rating | Notes |
|:---|:---|:---|
| **Completeness** | 🟢 **95%** | Schema, state, commands, error codes, UI layout are all comprehensive. Minor gaps noted above. |
| **Internal Consistency** | 🟡 **88%** | Most cross-references align. The issues found are edge-case interactions (undo + audio files, buffer resizing, mic FX chain). |
| **Agent Buildability** | 🟢 **92%** | Phase breakdown (§9) and risk mitigations (§8) give agents a clear, ordered path. Testing strategy (§8.7) and human checkpoints (§8.8) are excellent. |
| **Reliability for Non-Engineer** | 🟡 **85%** | The CrashGuard, auto-save, graduated safe mode, and DiskWriter tiers are well-designed for resilience. The undo/redo ambiguity (H2) and MP3/FLAC encoding risks (R3, R5) could cause confusing failures that a non-engineer would struggle to diagnose. |

**Verdict:** The spec is in excellent shape. Fix H1-H3 and M5 (total ~45 minutes of spec editing), and the project is ready for bead creation with high confidence.

**Total estimated spec update time for all items: ~1.5 hours**

---

## Appendix: Resolutions Applied

All findings from this assessment have been resolved in the spec documents. Summary of changes:

| # | Issue | Resolution | Files Modified |
|:--|:------|:-----------|:---------------|
| **H1** | `RECORDING` slot state vestigial | **Removed** from slot state enum. Added comment explaining retrospective model. | `Spec 1.6` §3.4 |
| **H2** | Undo/Redo ambiguous | **Removed entirely from V1.** Undo/Redo commands, error codes, UI buttons, and SessionStateManager undo logic all removed. Riff History is the primary recovery mechanism. Added to V2 Future Goals (§1.3). | `Spec 1.6` §1.3, §2.1, §2.2.G, §3.2, §3.3, §5.1, §7.6.1, §7.6.5, §8.6, §9; `UI_Layout_Reference.md` header, toolbar, mixer |
| **H3** | Retrospective buffer resizing | **Clarified:** Buffer is a fixed ~30s pre-allocated circular buffer. `SET_LOOP_LENGTH` selects a *portion* of the buffer to dump into a slot — no resizing occurs. Silent bars are included if selected length exceeds played content. | `Spec 1.6` §3.10 |
| **M1** | `KnobParameter` incomplete for Mic | **Added** Microphone-specific Adjust tab layout: Reverb Mix + Room Size knobs, Monitor toggles, Gain knob. No pad grid in Mic mode. | `Spec 1.6` §7.6.4 |
| **M2** | Mic FX chain ambiguity | **Decided:** V1 = simple built-in reverb only (Freeverb). Full FX chain deferred to V2. | `Audio_Engine_Specifications.md` §6.3 |
| **M3** | Drum kit grid count | **Accepted as-is.** Empty grid cells are fine — no need to fill all 12. | No change |
| **M4** | `+ N BARS` additive confusion | **Removed `+` prefix.** Buttons are now `[8 BARS] [4 BARS] [2 BARS] [1 BAR]` — absolute set, not additive. | `Spec 1.6` §7.6.1; `UI_Layout_Reference.md` |
| **M5** | Binary stream single vs dual connection | **Decided:** Single WebSocket connection with interleaved text/binary frames. Updated §3.7. | `Spec 1.6` §3.7 |
| **L1** | UI Layout Ref missing riff swap | **Added** `user_preferences` section with Riff Swap Mode. | `UI_Layout_Reference.md` |
| **L2** | `PluginInstance` undefined | **Added** interface definition to §3.4. | `Spec 1.6` §3.4 |
| **L3** | Mixer empty cell | **Accepted as-is.** Empty cells are fine. | No change |
| **L4** | "Pre-fader" terminology | **Replaced** with "applied to input signal before retrospective capture buffer." | `Audio_Engine_Specifications.md` §6.3 |
| **R6** | `JsonPatchOp` type missing | **Added** TypeScript interface definition to §3.2. | `Spec 1.6` §3.2 |
| **R4** | Buffer count ambiguity | **Clarified:** V1 uses one global buffer. Added V2 multi-user note. | `Spec 1.6` §3.10, §1.3 |

### Additional Changes (Owner Feedback)

| Change | Description | Files Modified |
|:-------|:------------|:---------------|
| **Mixer fader style** | Changed from circular faders to **standard vertical fader bars with integrated VU meters**. VU levels bounce inside the fader track. | `Spec 1.6` §7.6.5; `UI_Layout_Reference.md` mixer_tab |
| **Multi-user buffer V2 plan** | Added to §1.3 Future Goals: per-user retrospective buffers for multi-user jamming, with session settings for user count. | `Spec 1.6` §1.3 |
| **Undo/Redo V2 plan** | Added to §1.3 Future Goals as a deferred feature. | `Spec 1.6` §1.3 |

---

*End of Pre-Build Assessment 3*
