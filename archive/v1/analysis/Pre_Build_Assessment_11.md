# Pre-Build Assessment 11 — Final Spec Integrity Review

**Date:** 14 February 2026  
**Scope:** All documents in the `Spec/` folder only  
**Documents Reviewed:**
- `Spec/Spec_FlowZone_Looper1.6.md` (1840 lines)
- `Spec/Audio_Engine_Specifications.md` (364 lines)
- `Spec/UI_Layout_Reference.md` (375 lines)

**Purpose:** Identify inconsistencies, ambiguities, risks, and gaps that could cause implementation failures for an AI-agent-driven build where the project owner cannot troubleshoot code.

---

## Summary

The spec is mature and well-structured. The architecture is sound, the command protocol is comprehensive, and the phased build plan is solid. However, I found **6 cross-document contradictions**, **8 ambiguities that will stall agents**, **5 missing definitions**, and **7 reliability risks** that should be resolved before bead creation. Each is categorized by severity and includes a recommended resolution.

---

## 1. 🔴 Cross-Document Contradictions (Must Fix)

These are places where two documents directly disagree. Agents will either pick the wrong one or stall asking for clarification.

### C1: DiskWriter Overflow Cap — 512MB vs 1GB

| Location | Value |
|:---|:---|
| [Main Spec §2.2.I](Spec/Spec_FlowZone_Looper1.6.md:179) | "up to **512MB** hard cap on 8GB systems" |
| [Main Spec §4.2](Spec/Spec_FlowZone_Looper1.6.md:671) | "up to **1GB**" |

**Impact:** An agent implementing DiskWriter will use whichever section it reads first. The 512MB figure appears to be system-specific (8GB RAM) while the 1GB figure is unconditional.

**Recommendation:** Unify to a single rule. Suggested: "Overflow cap = 512MB on 8GB systems, scales to 1GB on 16GB+ systems." Or simply pick one value and remove the other. The simpler the rule, the less room for error.

> **Resolution:** **DONE.** Updated spec to use sliding scale (512MB for <16GB RAM, 1GB for >=16GB RAM).

---

### C2: Drum Mode Bounce/Speed Knobs — "Hidden" vs "Removed"

| Location | Statement |
|:---|:---|
| [Main Spec §7.6.4](Spec/Spec_FlowZone_Looper1.6.md:1105) | "**Hidden** (or rendered as disabled/blank)" |
| [Audio Engine Spec §4.3](Spec/Audio_Engine_Specifications.md:265) | "**Removed** from the UI in V1 (not just hidden)" |

**Impact:** "Hidden/disabled" means the grid cell exists but is blank. "Removed" means the grid layout changes. These produce different UI implementations.

**Recommendation:** Pick one. "Hidden/disabled" is simpler (grid stays 2×4 always) and is the safer choice for responsive layout consistency.

> **Resolution:** **DONE.** Drum knobs are **Removed** in V1. Spec updated.

---

### C3: Pan References with No Pan Command

| Location | Statement |
|:---|:---|
| [Main Spec §3.2 COMMIT_RIFF](Spec/Spec_FlowZone_Looper1.6.md:330) | "current mixer state (**volume, pan, mute**)" |
| [Main Spec §3.3 COMMIT_RIFF](Spec/Spec_FlowZone_Looper1.6.md:398) | "'Mix Dirty' hash (**vol + pan + mute**)" |
| [Main Spec §7.6.5](Spec/Spec_FlowZone_Looper1.6.md:1157) | "**No Pan** or Solo controls in V1" |
| [Main Spec §3.4 AppState](Spec/Spec_FlowZone_Looper1.6.md:482) | No `pan` field on slot state |
| [Main Spec §3.2 Command Schema](Spec/Spec_FlowZone_Looper1.6.md:282) | No `SET_PAN` command |

**Impact:** The `COMMIT_RIFF` description references "pan" three times, but there is no `pan` field in the slot state, no `SET_PAN` command, and the Mixer tab explicitly has no pan controls. An agent implementing `COMMIT_RIFF` will be confused about whether to implement pan or not.

**Recommendation:** Remove all references to "pan" from `COMMIT_RIFF` and the Mix Dirty hash. Change to "volume and mute state." Pan can be added in V2 when the control exists.

> **Resolution:** **DONE.** All pan references removed from Spec 1.6.

---

### C4: External MIDI Clock — V1 Feature or V2 Future Goal?

| Location | Statement |
|:---|:---|
| [Main Spec §1.3 Future Goals](Spec/Spec_FlowZone_Looper1.6.md:61) | "**MIDI Clock Sync:** External MIDI Clock as transport source" listed as V2 |
| [Main Spec §7.6.8 Tab C](Spec/Spec_FlowZone_Looper1.6.md:1255) | Clock Source radio: "Internal \| External MIDI Clock" — active, not disabled |
| [Main Spec §3.2 Command Schema](Spec/Spec_FlowZone_Looper1.6.md:348) | `SET_CLOCK_SOURCE` command included (no "V2" note) |
| [Main Spec §3.4 AppState](Spec/Spec_FlowZone_Looper1.6.md:508) | `clockSource: 'internal' | 'external_midi'` in settings |

**Impact:** The settings UI has it as an active control, the command schema supports it, the AppState has the field — but §1.3 says it's V2. An agent will implement it because the command exists. If it's truly V2, the command and UI control should be marked as disabled.

**Recommendation:** Decide. If V2, disable the radio button in §7.6.8 Tab C (like Ableton Link), remove `SET_CLOCK_SOURCE` from the command schema (or mark it V2), and remove `clockSource` from `AppState.settings`. If V1, remove it from §1.3 Future Goals.

> **Resolution:** **DONE.** Defer to V2. Updated command schema and settings to reflect this.

---

### C5: Mixer Channel Strip Layout — Zigzag vs Scrollable

| Location | Statement |
|:---|:---|
| [Main Spec §7.6.5](Spec/Spec_FlowZone_Looper1.6.md:1155) | "Horizontal row, **scrollable if > 8 channels**" |
| [Main Spec §7.6.10 Phone](Spec/Spec_FlowZone_Looper1.6.md:1313) | "**no horizontal scrolling** required for V1" |
| [UI Layout Reference](Spec/UI_Layout_Reference.md:238) | "**zigzag staggered layout** (Odd slots top, Even slots bottom) to fit 8 slots without scrolling" |

**Impact:** Three different descriptions. The zigzag layout from the UI Layout Reference is a specific design decision that isn't mentioned anywhere in the main spec. An agent building the mixer will have to decide which to follow.

**Recommendation:** Add the zigzag layout description to the main spec §7.6.5 explicitly. This is likely the intended design (it solves the phone layout problem) but it needs to be in the authoritative spec, not just the reference JSON.

> **Resolution:** **DONE.** Added Zigzag Layout description to Spec 1.6 §7.6.5.

---

### C6: Drum Pad Table Formatting Error

| Location | Issue |
|:---|:---|
| [Audio Engine Spec §4.1](Spec/Audio_Engine_Specifications.md:227) | Table has MIDI note numbers embedded inside the "Synthesis Method" column |

The table header defines columns: `Pad (Row, Col) | Icon | Sound Type | Synthesis Method` but each row contains an extra value (the MIDI note number) that breaks the column alignment. For example:

```
| (1,1) | double_diamond | Kick 1 | 36 | Sine wave, pitch envelope... |
```

This has 5 pipe-delimited cells but only 4 column headers. The MIDI note number (36, 37, etc.) is hanging between "Sound Type" and "Synthesis Method" without a header.

**Impact:** Agents parsing this table will misread the columns. The MIDI note numbers are critical for the drum pad-to-note mapping.

**Recommendation:** Add a `MIDI Note` column header:
```
| Pad (Row, Col) | Icon | Sound Type | MIDI Note | Synthesis Method |
```

> **Resolution:** **DONE.** Added MIDI Note column to Audio Engine Spec.

---

## 2. 🟠 Ambiguities That Will Stall Agents (Should Fix)

These aren't contradictions, but they're undefined behaviors that will force agents to guess or ask questions.

### A1: What Does the Adjust Tab Show in FX Mode?

[Main Spec §7.6.4](Spec/Spec_FlowZone_Looper1.6.md:1097) defines knob layouts for Drums, Notes, Bass, and Microphone modes. FX Mode is not mentioned. When a user is in FX Mode and taps the Adjust tab, what do they see?

**Options:**
- The XY pad is the only control (Adjust tab is blank or redirects to Play)
- Effect-specific knobs are shown (but which parameters? They vary per effect)
- The knobs from the previously selected instrument mode persist

**Recommendation:** Explicitly state FX Mode behavior in §7.6.4. Suggested: "In FX Mode, the Adjust tab shows effect-specific parameters (e.g., Feedback, Cutoff) mapped to the same 2×4 knob grid. Unmapped positions are blank. Parameter mapping is defined per-effect in Audio_Engine_Specifications.md." Or simpler: "In FX Mode, the Adjust tab is disabled — all FX control happens via the XY pad in the Play tab."

> **Resolution:** **DONE.** Adjust tab shows effect-specific parameters. Added default mappings to Spec 1.6.

---

### A2: SET_LOOP_LENGTH — Does It Change the Global Setting or Just Capture?

[Main Spec §3.2](Spec/Spec_FlowZone_Looper1.6.md:307) describes `SET_LOOP_LENGTH` as "**Dual-purpose:** Sets the loop length AND immediately captures." [AppState](Spec/Spec_FlowZone_Looper1.6.md:458) has `transport.loopLengthBars`.

**Questions:**
1. If the user captures 4 bars into Slot 1, then captures 2 bars into Slot 2 — what is `transport.loopLengthBars`? Is it 2 (last set value)? Does it matter?
2. Each slot has its own `slot.loopLengthBars`. So what does `transport.loopLengthBars` represent? The "last captured length"? The "next capture will be this length"?
3. If all 8 slots have different loop lengths, which determines the transport cycle length for the bar phase animation?

**Recommendation:** Clarify that `transport.loopLengthBars` represents the "most recently set loop length / next capture length." Add a note about how the transport cycle handles mixed-length loops (e.g., "bar phase animation uses the longest active slot's length as the cycle period" or "always cycles in the length of the most recently captured loop").

> **Resolution:** **DONE.** Clarified: "Longest active loop" drives the transport cycle. Shorter loops cycle within.

---

### A3: LOAD_RIFF Behavior Undefined

[Main Spec §3.2](Spec/Spec_FlowZone_Looper1.6.md:331) has `LOAD_RIFF { riffId }` but the behavioral details are missing:

1. Does loading a riff replace ALL 8 slots (clearing those not present in the riff)?
2. Does it respect `riffSwapMode` (instant vs swap_on_bar)?
3. Does the transport need to be playing?
4. What happens to the retrospective buffer — is it cleared?
5. Does it restore the tempo/key from the riff, or keep current settings?

**Recommendation:** Add a "LOAD_RIFF Behavior" subsection under §3.10 or as a note in the error matrix. Suggested: "LOAD_RIFF replaces all 8 slot states with the riff's saved state. Slots not included in the riff are set to EMPTY. The retrospective buffer is NOT cleared. Tempo and key are restored from the riff. Loading respects the `riffSwapMode` setting."

> **Resolution:** **DONE.** Added "LOAD_RIFF Behavior" subsection to Spec 1.6 §3.12.

---

### A4: GoTo Effect — What Playhead Does It Jump?

[Audio Engine Spec §1.1.6](Spec/Audio_Engine_Specifications.md:33) describes "GoTo — Playhead jump effect" with "Jump Probability" and "Quantize Grid." But it doesn't specify *which* playhead it manipulates.

**Options:**
- The transport bar phase position?
- The FX processing read position within the selected slot audio?
- A buffer read position within the effect's internal buffer?

**Impact:** This fundamentally changes the implementation. If it moves the transport position, it affects all slots simultaneously. If it's effect-internal, it only affects the FX-processed audio.

**Recommendation:** Clarify in Audio_Engine_Specifications.md. Suggested: "GoTo jumps the internal read position within the effect's processing buffer (does NOT affect global transport position)."

> **Resolution:** **DONE.** Clarified in Audio Engine Spec.

---

### A5: Auto-Merge Trigger in FX Mode — Contradictory Logic

[Main Spec §7.6.2 Step 7](Spec/Spec_FlowZone_Looper1.6.md:1012):
> "If all 8 slots are full when committing the resampled layer, the standard auto-merge rule applies first"

But Step 6 says the FX commit **first clears the source slots**, then writes the result. If source slots are cleared first, there will always be empty slots available. The auto-merge can only trigger if:
- Only 1 slot is selected as FX source AND the other 7 slots are all full AND the result needs an 8th slot... but clearing the 1 source slot frees 1 slot, so auto-merge still can't trigger.

**Impact:** This paragraph appears to be unreachable code. An agent might waste time implementing a code path that never executes.

**Recommendation:** Re-examine this logic. If it's genuinely unreachable, remove the sentence. If the intent is "source slots are NOT automatically cleared," then the destructive sequence in Step 6 needs to be rewritten.

> **Resolution:** **DONE.** Unreachable code removed. FX Commit takes precedence over Auto-Merge.

---

### A6: Slot Metadata Population — No Explicit Command

`AppState.slots[n]` has fields: `instrumentCategory`, `presetId`, `name`, `userId`. These are read-only from the UI's perspective — no command sets them. They should be auto-populated by the engine when audio is committed to a slot.

**But this is never explicitly stated.** When a `SET_LOOP_LENGTH` fires and audio goes from the retrospective buffer into a slot, the engine must:
1. Set `instrumentCategory` from `activeMode.category`
2. Set `presetId` from `activeMode.presetId`
3. Set `name` from `activeMode.presetName` (or "Slot N"?)
4. Set `userId` to "local"
5. Set `loopLengthBars` from the command parameter
6. Set `originalBpm` from current `transport.bpm`

**Recommendation:** Add a "Slot Population on Capture" subsection to §3.10. List explicitly which fields are set and from where.

> **Resolution:** **DONE.** Added "Slot Population on Capture" subsection to Spec 1.6 §3.11.

---

### A7: Riff History Indicator Colors — Source Not Defined

[Main Spec §3.4](Spec/Spec_FlowZone_Looper1.6.md:500): `riffHistory.colors: string[]` described as "Layer cake colors (source-based)."

But what exactly are these colors? The palette in §7.1 defines:
- Indigo (#5E35B1) for Drums
- Teal (#00897B) for Synths
- Amber (#FFB300) for External Audio
- Vermilion (#E65100) for FX/Combined
- Chartreuse (#7CB342) for Transport & Sync

**Questions:**
1. Is "synths" both Notes and Bass? Or do they get different colors?
2. What color is Microphone input? Amber (external audio)?
3. What about auto-merge results?
4. The palette has 5 colors for 7+ instrument categories.

**Recommendation:** Create an explicit mapping table:

| `instrumentCategory` | Color Name | Hex |
|:---|:---|:---|
| drums | Indigo | #5E35B1 |
| notes | Teal | #00897B |
| bass | Teal (lighter variant) | #26A69A |
| mic | Amber | #FFB300 |
| fx | Vermilion | #E65100 |
| ext_inst | (TBD) | |
| ext_fx | (TBD) | |
| merge | Vermilion (darker) | #BF360C |

> **Resolution:** **DONE.** Added color mapping table to Spec 1.6 §3.4. Defined colors for Ext_Inst (Dark Cyan) and Ext_FX (Deep Orange).

---

### A8: Retrospective Buffer Behavior on Sample Rate Change

[Main Spec §3.10](Spec/Spec_FlowZone_Looper1.6.md:594): Buffer is "pre-allocated at startup" based on session sample rate. But `SET_SAMPLE_RATE` can change the sample rate mid-session.

**Questions:**
1. Is the buffer re-allocated? (This would violate the "never resize" rule.)
2. Is existing buffer content invalidated? (It was captured at the old rate.)
3. Does sample rate change require an engine restart?

**Recommendation:** Add a note: "Changing sample rate requires a brief engine reset. The retrospective buffer is cleared and re-allocated at the new rate. Any uncaptured audio is lost. Transport is paused during the transition."

> **Resolution:** **DONE.** Added note to `SET_SAMPLE_RATE` command in Spec 1.6.

---

## 3. 🟡 Missing Definitions (Should Add)

### M1: No `CLEAR_SLOT` Command (Acknowledged, But Risky)

[Main Spec §3.10 note](Spec/Spec_FlowZone_Looper1.6.md:613) acknowledges "V1 does not include a per-slot clear command."

**Risk for non-engineer user:** If a user fills 5 slots and doesn't like Slot 3, their only option is to load a previous riff (if they committed one). If they didn't commit, there's no recovery at all. Combined with no undo/redo, this could be extremely frustrating.

**Recommendation:** Strongly consider adding `CLEAR_SLOT { slot: number }` as a V1 feature. It's a simple command (set slot state to EMPTY, release audio buffer) and dramatically improves the user experience. The spec already has the infrastructure (slot states, command dispatch) — this is a 1-bead task.

> **Resolution:** **DISMISSED.** Not needed. Philosophy: "User can always go back to previous riff." Added note as general principle.

---

### M2: No PIN Management Commands

[Main Spec §3.2](Spec/Spec_FlowZone_Looper1.6.md:215) describes `AUTH { pin }` for optional PIN auth. But there's no way for the user to:
1. Set or change the PIN
2. Enable/disable PIN requirement
3. View whether PIN is currently enabled

These would be config.json values, but the Settings panel (§7.6.8) has no PIN section.

**Recommendation:** Either: (a) Add a "Security" section to Settings with PIN enable/disable and PIN change controls, or (b) Note that PIN is a manual `config.json` edit only (acceptable for V1, but document it).

> **Resolution:** **DONE.** Removed PIN auth entirely from Spec V1.

---

### M3: No `STOP` or `RESET_TRANSPORT` Command

The spec explicitly says "Play/Pause only — no Stop." But there's also no way to reset the bar phase to zero. If a user pauses and wants to restart from the beginning of the bar, they can't.

**Recommendation:** This is a conscious design choice (noted in the command schema comment). Just ensure agents understand that PAUSE holds position and PLAY resumes. If "restart from bar start" is ever needed, it can be added later.

> **Resolution:** **ACCEPTED.** Design choice confirmed. No Stop/Reset in V1.

---

### M4: Error Surfacing Levels in UI Layout Reference

[UI Layout Reference](Spec/UI_Layout_Reference.md:59) defines three error levels (low/medium/critical) with specific UI treatments, but this doesn't appear in the main spec. The main spec's error matrix (§3.3) defines "Client Behavior" per command but doesn't reference these surfacing levels.

**Recommendation:** Reference the error surfacing levels from the main spec §3.3 or §5.1. Map each error code to a surfacing level (toast/banner/modal).

---

### M5: Session Emoji — No Change/Re-Roll Mechanism

[Main Spec §7.7.2](Spec/Spec_FlowZone_Looper1.6.md:1348): Emoji is "randomly selected" on creation. But `RENAME_JAM` only changes the name — there's no command to change the emoji.

**Recommendation:** Either: (a) Add `emoji` as an optional parameter to `RENAME_JAM`, or (b) Accept the random assignment is permanent (this is fine for V1 — just be explicit about it).

> **Resolution:** **DONE.** Added `emoji` optional parameter to `RENAME_JAM`. User can change it.

---

## 4. 🔴 Reliability Risks for Non-Engineer Owner

These are things that WILL work in development but could cause hard-to-diagnose failures in production use.

### R1: DELETE_JAM Is Permanently Destructive with No Safety Net

[Main Spec §2.2.G](Spec/Spec_FlowZone_Looper1.6.md:167): "DELETE_JAM is a destructive action. It **immediately** removes session metadata, riff history entries, **and all associated audio files** from disk. There is no Undo or Trash folder in V1."

For a non-engineer user, an accidental confirmation dialog tap could permanently destroy hours of creative work. The "Are you sure?" dialog is the only protection.

**Recommendations:**
1. Make the confirmation dialog require **typing the jam name** or a specific word (like GitHub's repo deletion).
2. ~~Consider a 30-day soft-delete~~ (acknowledged as V2, but prioritize it).
3. Ensure the confirmation dialog is a MODAL (blocking) — not a toast or swipeable element. **Verify**: The UI Layout Reference's error_surfacing correctly classifies "Delete Confirmation" as "critical" → "modal_dialog" → "blocking". ✅ Good.

> **Resolution:** **ACCEPTED.** Modal confirmation is sufficient. No change needed.

---

### R2: Varispeed BPM Changes Are Irreversible and Non-Obvious

[Main Spec §2.2.B](Spec/Spec_FlowZone_Looper1.6.md:123): "Changing BPM changes the playback speed **and pitch** of all recorded loops."

This is intentional but highly unusual. Most music apps keep pitch constant when changing tempo. A non-engineer user may change BPM expecting normal behavior and be startled when everything changes pitch.

**Recommendations:**
1. Add a **first-time tooltip** or onboarding hint when the user first changes BPM with active loops: "Changing tempo also changes pitch (varispeed). This is by design for creative flow."
2. Store `originalBpm` per slot (already in spec ✅) so the UI could display "Playing at 130bpm (recorded at 120bpm) — pitch shifted +1.4 semitones."
3. Consider a visual indicator on the tempo display when active loops exist showing the pitch shift amount.

> **Resolution:** **DISMISSED.** "Not a problem." Varispeed behavior is accepted as-is for V1.

---

### R3: CivetWeb Thread Serialization Is Easy to Get Wrong

[Main Spec §2.2.G](Spec/Spec_FlowZone_Looper1.6.md:170): "All outgoing `mg_websocket_write()` calls must be serialized on a **single writer thread**."

This is a subtle threading requirement. If ANY code path (StateBroadcaster, Feature Stream, error responses) writes to the WebSocket from a different thread, it can cause data corruption that only manifests under load.

**Recommendations:**
1. Implement a `WebSocketWriter` singleton class that queues ALL outbound messages and writes from a dedicated thread. No component should ever call `mg_websocket_write()` directly.
2. Add a debug assertion: if `mg_websocket_write()` is called from any thread other than the writer thread, assert in debug mode.
3. Make this a **Phase 0/1 architectural decision** — not something added later.

> **Resolution:** **ACCEPTED.** Implement `WebSocketWriter` singleton (or ensuring serialization) is required.

---

### R4: JSON Patch (RFC 6902) Implementation Is Hard

The spec requires the `StateBroadcaster` to generate JSON Patches (§3.2). RFC 6902 patch generation for complex nested state (8 slots × ~12 fields = 96+ potential patch targets) is a significant implementation challenge. Getting the diff algorithm wrong produces patches that corrupt client state.

**Recommendations:**
1. Use a **well-tested library** for JSON Patch generation (e.g., `nlohmann/json` with a patch extension for C++, `fast-json-patch` for TypeScript verification).

> **Resolution:** **ACCEPTED.** Use a library.
2. The spec wisely defers patches to Phase 6 (Task 6.1). Reinforce that full-snapshot mode (Phase 2) should be the primary mode for all development and testing. Patches are an optimization.
3. Add a **debug mode** that sends both a full snapshot AND the patch for the same state change, allowing the client to verify patch correctness in development.

---

### R5: Lock-Free SPSC Queue — Subtle Correctness Requirements

[Main Spec §2.2.C](Spec/Spec_FlowZone_Looper1.6.md:127): CommandQueue is a "Lock-free SPSC FIFO."

Correct lock-free SPSC requires specific memory ordering (`std::memory_order_acquire`/`release`). An off-by-one error or incorrect fence can cause commands to be read before they're fully written — producing garbage command data that crashes the dispatcher.

**Recommendations:**
1. Use JUCE's `AbstractFifo` (which is a correct lock-free SPSC implementation) rather than rolling a custom one. JUCE has battle-tested this for exactly this use case.

> **Resolution:** **ACCEPTED.** Spec updated to require `juce::AbstractFifo`.
2. If a custom implementation is required, the Phase 1 "Audio Thread Contract" bead (Task 1.3) should include a formal specification of the queue's memory ordering requirements.
3. Unit tests should include a stress test: producer and consumer on separate threads, 1M messages, verify zero corruption.

---

### R6: Binary + JSON on Single WebSocket — Frame Interleaving Risk

[Main Spec §3.7](Spec/Spec_FlowZone_Looper1.6.md:563): "Binary frames on the **same** WebSocket connection used for JSON command/state traffic."

The spec acknowledges "heavy visualization load (30fps) could cause minor jitter." At 30fps, a binary frame is sent every ~33ms. If a JSON `STATE_PATCH` arrives at the same moment, the messages could interleave at the CivetWeb level.

**Recommendations:**
1. The `WebSocketWriter` singleton (from R3) naturally solves this — all outbound messages are serialized.
2. On the client side, ensure the message type check (opcode `0x1` text vs `0x2` binary) is the FIRST thing the handler does, before any JSON parsing.
3. The spec's own note about considering "a dedicated command WebSocket in V2" is wise. For V1, just ensure the serialization is correct.

---

### R7: 96-Second Retrospective Buffer at 20 BPM — Edge Case

[Main Spec §3.10](Spec/Spec_FlowZone_Looper1.6.md:594): Buffer is ~96 seconds to cover "8 bars at 20 BPM." This is exactly 96 seconds with no margin.

If the user is at exactly 20 BPM and captures 8 bars, they're reading the entire buffer with zero slack. If there's ANY processing delay (a few ms of latency between the "capture" command arriving and the actual read), the buffer may have already overwritten the oldest samples.

**Recommendation:** Add a small safety margin: 100 seconds instead of 96, or document that at extreme low tempos, the 8-bar capture may have slight click artifacts at the loop point. Better: Calculate as `96 seconds + (2 × bufferSize/sampleRate)` to account for processing latency.

> **Resolution:** **DONE.** Spec updated to 97 seconds.

---

## 5. 🟢 Minor Issues (Low Priority, Fix If Time Permits)

| # | Issue | Location | Recommendation |
|:---|:---|:---|:---|
| L1 | `quick_toggles.options` is an empty array `[]` | [UI_Layout_Reference.md:360](Spec/UI_Layout_Reference.md:360) | Add the "Note Names" toggle that's described in [main spec §7.6.8](Spec/Spec_FlowZone_Looper1.6.md:1264) |
> **Resolution:** **DONE.** Added "Note Names" toggle.
| L2 | Infinite FX has 11 effects in a 3×4 grid (1 empty slot) | [Audio_Engine_Specifications.md §1.2](Spec/Audio_Engine_Specifications.md:61) | Note that the 12th grid slot is empty. Or add a 12th effect. |
> **Resolution:** **DONE.** Added "Trance Gate" as 12th effect.
| L3 | Riff History View shows "Export Stems" button (disabled V2) | [Main Spec §7.6.7](Spec/Spec_FlowZone_Looper1.6.md:1209) | Consider removing entirely for V1 rather than showing disabled. A disabled button with no context is confusing for non-technical users. |
| L4 | No cancel mechanism for `RESCAN_PLUGINS` | [Main Spec §3.2](Spec/Spec_FlowZone_Looper1.6.md:350) | If a plugin scan hangs, the user has no way to abort. Consider a timeout. |
| L5 | `session.emoji` assigned randomly — collisions likely | [Main Spec §7.7.3](Spec/Spec_FlowZone_Looper1.6.md:1352) | With ~200 emojis, collisions happen after ~15 jams (birthday paradox). Acceptable, but consider tracking used emojis. |
| L6 | UI_Layout_Reference.md `play_tab` mixes FX and instrument layouts | [UI_Layout_Reference.md:136](Spec/UI_Layout_Reference.md:136) | The `play_tab` view definition includes both the instrument pad grid (from `instrument_tab`) and the FX XY pad. Consider splitting more clearly to avoid agent confusion. |
| L7 | `Spec_FlowZone_Looper1.6.md` §2.4 "Shutdown Note" is inside a code block | [Main Spec line 262](Spec/Spec_FlowZone_Looper1.6.md:262) | The shutdown note about DiskWriter blocking is inside the code fence (` ``` `) instead of below it. Move it outside the code block. |

---

## 6. Positive Observations (What's Working Well)

For balance, here's what's strong about the spec:

1. **Complete command schema with error matrix (§3.2-3.3)** — Every command has defined failure modes and client behaviors. This is excellent for agent implementation.

2. **Risk mitigations section (§8)** — The spec includes its own risk assessment with specific bead practices. This is unusually thorough.

3. **Human testing checkpoints (§8.8)** — Five explicit gates with "what you should see" descriptions are invaluable for a non-engineer owner.

4. **Thread priority table (§2.3)** — Clear, unambiguous thread assignments.

5. **Phased build with dependency ordering (§8.6)** — The Phase 0 → Phase 8 ordering correctly defers complex features (plugins, crash guard) until the core works.

6. **CrashGuard graduated safe mode (§2.2.F)** — Well-designed resilience for production use.

7. **Retrospective capture design (§3.10)** — The always-on recording model is well-specified with clear capture source clarification.

8. **Audio Engine Specifications** — Comprehensive parameter ranges, synthesis methods, and XY mappings for every effect/preset. Agents can implement these without guessing.

---

## 7. Recommended Actions Before Bead Creation

### Priority 1 (Must fix — blocks correct implementation)
- [ ] **Fix C1:** Unify DiskWriter overflow cap (512MB vs 1GB)
- [ ] **Fix C2:** Standardize Drum Mode Bounce/Speed knobs (hidden vs removed)
- [ ] **Fix C3:** Remove pan references from COMMIT_RIFF/Mix Dirty hash
- [ ] **Fix C4:** Decide External MIDI Clock V1 vs V2
- [ ] **Fix C6:** Add MIDI Note column to drum pad table
- [ ] **Fix A2:** Clarify `transport.loopLengthBars` meaning with mixed-length loops
- [ ] **Fix A3:** Define LOAD_RIFF fullbehavior

### Priority 2 (Should fix — prevents agent confusion)
- [ ] **Fix C5:** Add zigzag mixer layout to main spec
- [ ] **Fix A1:** Define Adjust tab behavior in FX Mode
- [ ] **Fix A5:** Resolve auto-merge in FX Mode unreachable logic
- [ ] **Fix A6:** Document slot metadata population on capture
- [ ] **Fix A7:** Create instrumentCategory → color mapping table
- [ ] **Fix M1:** Consider adding CLEAR_SLOT as V1 feature

### Priority 3 (Should add — improves reliability)
- [ ] **Fix R1:** Strengthen DELETE_JAM confirmation (type-to-confirm)
- [ ] **Fix R2:** Add varispeed change warning UI hint
- [ ] **Fix R3:** Spec a WebSocketWriter singleton requirement
- [ ] **Fix R7:** Add safety margin to retrospective buffer size

### Priority 4 (Nice to have)
- [ ] Fix the 7 low-priority items (L1-L7)
- [ ] Fix remaining ambiguities (A4, A8, M2-M5)
