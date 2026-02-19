# Pre-Build Assessment 9 — Spec Folder Consistency & Risk Review

Scope: Full review of the current Spec folder only. Sources read: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md), [`Spec/Audio_Engine_Specifications.md`](Spec/Audio_Engine_Specifications.md), [`Spec/UI_Layout_Reference.md`](Spec/UI_Layout_Reference.md).

## Executive Summary

Overall architecture is coherent and unusually well risk-mitigated; however, there are several **spec inconsistencies and gaps** that could lead to implementation drift and user-facing reliability issues. The highest-risk items are **command/schema drift**, **UI command references that are not in the command schema**, and **ambiguous error handling for destructive actions**. These should be resolved before beads creation to avoid rework and untestable beads.

## Key Findings (Inconsistencies & Risks)

### 1) **Command schema mismatch: `STOP` vs `PANIC`**

**Spec conflict**: The Mixer tab section references a “Panic” action using `STOP` or `PANIC`, but the command schema includes only `PANIC` and **no** `STOP` command.

- Evidence: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md) lines 1141–1146 and 353–355.

**Risk**: Agents could implement a `STOP` command or UI behavior that is not in the schema; C++/TS drift and test failures.

**Recommendation**: Remove `STOP` references; specify **only** `PANIC` with scope `ALL|ENGINE` as the long-press behavior. Update the mixer section to explicitly state `PANIC`.

---

### 2) **UI references a command not in the schema: `TOGGLE_NOTE_NAMES`**

**Spec conflict**: UI Settings mention `TOGGLE_NOTE_NAMES` as a command, but the command schema explicitly states “UI-only, stored in localStorage — not sent to engine.”

- Evidence: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md) lines 336–338 and 1260–1263; [`Spec/UI_Layout_Reference.md`](Spec/UI_Layout_Reference.md) lines 349–355.

**Risk**: Either the UI sends a command the engine never handles or developers add a command that shouldn’t exist, violating “stateless view” and adding unnecessary protocol surface.

**Recommendation**: Align the UI Settings spec with the schema. If it must stay UI-only, remove the “command” notation and clarify it is **localStorage only**. If it must be engine-stateful, add it to the schema and AppState (`ui.noteNamesEnabled`) as a **UI-only field** (kept in state snapshots but not affecting audio).

---

### 3) **Duplicate `protocolVersion` field in `AppState.meta`**

**Spec conflict**: `protocolVersion` appears twice in the `AppState.meta` definition.

- Evidence: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md) lines 430–439.

**Risk**: Schema implementation ambiguity and potential serialization bugs; codegen/round-trip tests will fail or require hacks.

**Recommendation**: Remove the duplicate field in the spec. Keep a single `protocolVersion` entry.

---

### 4) **`SET_LOOP_LENGTH` errors: mismatch between error matrix and narrative**

**Spec conflict**: Error matrix lists `4010: NOTHING_TO_COMMIT` for `SET_LOOP_LENGTH`, while the descriptive text says `msg: "BUFFER_EMPTY"` when the retro buffer is empty.

- Evidence: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md) lines 390–391.

**Risk**: Inconsistent UI behavior and incorrect error mapping in tests; unit tests may encode wrong error codes.

**Recommendation**: Decide **one error code** for “buffer empty” (prefer `2050` or a new dedicated `ERR_BUFFER_EMPTY` under 2000–2999). Align matrix and narrative. Reserve `4010` for `COMMIT_RIFF` only.

---

### 5) **VST3 vs Standalone scope ambiguity**

**Spec conflict**: The project is described as “Standalone App + VST3 Plugin,” but also states “VST3 hosting (Phase 7) only available in Standalone.” That implies a separate plugin **target** that only exposes multi-channel output, not hosting.

- Evidence: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md) lines 38–39 and 117–119.

**Risk**: Confusion over plugin responsibilities and tests for DAW usage; risk of wasted effort implementing hosting in plugin mode.

**Recommendation**: Add a one-paragraph clarification under System Architecture: **VST3 target = output-only “recorder” mode**, no internal plugin hosting; Standalone app = full host + instruments. Also clarify UI differences in VST mode (`isVstMode`).

---

### 6) **Retrospective buffer capture source vs UI expectations**

**Spec risk**: Buffer captures only **live instrument output** (not existing slots), except in FX Mode. This is correct for avoiding doubling, but UI flow suggests “timeline waveform” always represents what the user just heard (including layers). That’s not true outside FX Mode.

- Evidence: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md) lines 592–596; UI timeline in [`Spec/UI_Layout_Reference.md`](Spec/UI_Layout_Reference.md) lines 41–53.

**Risk**: User confusion and QA defects (“timeline doesn’t match playback”).

**Recommendation**: Add a UI note: timeline visualizes **capturable signal**, not the full mix. If a “mix timeline” is desired, specify a second visualization (future goal) to avoid ambiguity.

---

### 7) **Session deletion policy conflicts with “safe capture” principle**

**Spec tension**: “Destructive Creativity, Safe Capture” is core, but `DELETE_JAM` removes session metadata immediately and audio on next clean exit. No trash in V1 (deferred to V2).

- Evidence: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md) lines 16–17, 166–168, 1340, 64–65.

**Risk**: Users can permanently lose data with a single deletion and no recovery, violating the product’s reliability promise.

**Recommendation**: Add a **soft-delete** staging area even in V1 (minimal implementation: “Recently Deleted” folder with time-limited retention, or require explicit “type to confirm”). If not, document a stronger warning and explicit confirmation flow.

---

### 8) **Audio thread safety vs command validation in `CommandDispatcher`**

**Spec risk**: `CommandDispatcher` validates payloads on the audio thread. Complex JSON validation may allocate or parse. This is safe only if decoding happens **before** the audio thread.

- Evidence: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md) lines 129–135 and 24–25.

**Risk**: Unintended allocations or parsing on the audio thread leading to dropouts.

**Recommendation**: Explicitly state that **JSON parsing and validation happens off audio thread**, and the audio thread receives pre-decoded, fixed-size structs via `CommandQueue`. If that’s already intended, add it as a non-negotiable rule in the “Audio Thread Contract.”

---

### 9) **State mutation ambiguity: slot `name` and `userId`**

**Spec gap**: Slots include `name` and `userId` but the spec doesn’t define how they are set or updated in V1 (single user).

- Evidence: [`Spec/Spec_FlowZone_Looper1.6.md`](Spec/Spec_FlowZone_Looper1.6.md) lines 480–488.

**Risk**: UI must display these fields; if not defined, tests will assert defaults differently across teams.

**Recommendation**: Define defaults in V1 (e.g., `userId = "local"`, `name = presetName + timestamp`), and ensure UI can handle empty values gracefully.

---

### 10) **Sample engine design conflicts with “no Node in engine” guideline**

**Spec risk**: The SampleEngine uses JSON presets stored under `/assets/presets/sampler`, which implies build pipeline changes in the web client or bundling in JUCE.

- Evidence: [`Spec/Audio_Engine_Specifications.md`](Spec/Audio_Engine_Specifications.md) lines 296–317.

**Risk**: If this gets implemented early, it can pull non-essential file system complexity into the engine, and blurred boundary between UI assets and engine assets.

**Recommendation**: For V1, explicitly tag SampleEngine paths as **future-only** and remove from engine tasks to avoid early entanglement. If needed later, specify a JUCE-safe asset packaging approach (no node in engine).

---

## Reliability-Focused Recommendations (Pre-Beads)

1) **Create a “Spec Corrections” bead** before any implementation. It should:
   - Remove `STOP` references, standardize on `PANIC`.
   - Resolve `TOGGLE_NOTE_NAMES` semantics (UI-only vs engine command).
   - Remove duplicate `protocolVersion`.
   - Align `SET_LOOP_LENGTH` error codes.
   - Clarify VST3 vs Standalone responsibilities.

2) **Add explicit “Audio Thread Contract” section updates** to assert decoding/validation is off-audio-thread (use fixed-size structs). This is essential to prevent hidden allocations.

3) **Improve deletion safety**: If no V1 trash is allowed, require a strong confirmation and include a clear warning in the spec so UI implements it consistently.

4) **Define default values** for `slot.name`, `slot.userId`, `session.emoji` when single-user. This avoids undefined UI state and repeatable tests.

5) **Clarify timeline waveform meaning** (capturable signal vs mix). Add a UI hint or an alternate visualization for the mix in a later phase.

6) **Explicitly mark V2-only features** in the UI layout reference so agents don’t wire disabled controls to commands.

## Risk Register (Condensed)

| Risk | Severity | Likelihood | Mitigation |
|---|---|---|---|
| Command schema drift (`STOP`, `TOGGLE_NOTE_NAMES`) | High | High | Correct spec + enforce schema checks in Phase 1 |
| Audio thread allocations (validation on audio thread) | High | Medium | Pre-decode commands; add assertions |
| Destructive deletes violate “safe capture” promise | High | Medium | Add soft-delete or stronger confirmation |
| Error code mismatch for `SET_LOOP_LENGTH` | Medium | High | Define single error code and update matrix |
| VST3 target scope ambiguous | Medium | Medium | Add explicit VST behavior section |
| Timeline waveform confusion | Medium | Medium | Clarify visualization meaning |

## Readiness Assessment

After correcting the inconsistencies above, the spec is ready for bead creation. Current risk profile is manageable, but the **schema mismatches and destructive UX gaps** should be fixed first to avoid brittle tests and user data loss.
