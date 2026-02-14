# Pre-Build Assessment 10 — Spec Consistency, Risk, and Reliability Review

**Scope:** This assessment is based solely on the documents in `Spec/`:

- `Spec/Spec_FlowZone_Looper1.6.md`
- `Spec/Audio_Engine_Specifications.md`
- `Spec/UI_Layout_Reference.md`

**Objective:** Identify inconsistencies, ambiguities, and reliability risks that could cause build failures, runtime instability, or unclear implementation choices. Provide mitigations to support a dependable build for a non-engineer owner.

---

## 1) Cross-Spec Consistency Checks

### 1.1 Knob Parameter Set vs Engine Mapping
- **Observation:** The command schema defines a fixed `KnobParameter` set (`pitch`, `length`, `tone`, `level`, `bounce`, `speed`, `reverb`, `reverb_mix`, `room_size`). The audio spec maps Adjust-tab knobs to different per-engine parameters (e.g., drums use `bounce`/`speed` but UI hides them; mic uses only `reverb_mix`/`room_size` + gain). There is no explicit routing table stating which knobs apply to each engine/preset.
- **Risk:** Without an explicit mapping table, UI may send knobs that the engine ignores or misapplies. This can cause “controls do nothing” and perceived instability.
- **Mitigation:** Add a **single authoritative mapping table** in the spec (or in a schema extension) that lists, per `activeMode.category`, which `KnobParameter` values are valid and how they map to engine parameters. Treat unknown knobs as no-ops with explicit `ACK` (not error) to avoid UI desync. Reference: `Spec/Spec_FlowZone_Looper1.6.md`.

### 1.2 FX Mode: Resampling + `COMMIT_RIFF` Overlap
- **Observation:** The spec states `SET_LOOP_LENGTH` is the primary recording mechanism, including FX resampling behavior, while `COMMIT_RIFF` is a state-only snapshot from Mixer. In the FX Mode section, `COMMIT_RIFF` is also described as capturing FX-processed audio and deleting source slots. That overlaps with `SET_LOOP_LENGTH` semantics.
- **Risk:** Conflicting capture behavior could lead to destructive actions when the user expected a state-only snapshot, risking data loss.
- **Mitigation:** Clarify that **only `SET_LOOP_LENGTH` captures audio** and **`COMMIT_RIFF` never touches audio** (or explicitly redefine and update all areas of the spec, protocol, and UI). Add a single “FX capture rule” statement in `Spec/Spec_FlowZone_Looper1.6.md` and ensure the command table matches.

### 1.3 VST3 Output-Only vs Engine Features
- **Observation:** The spec states VST3 target is output-only and plugin hosting is disabled. At the same time, system-wide settings include VST management (scan, paths) without explicitly gating for VST3 mode.
- **Risk:** UI exposing plugin management while in VST3 mode may send invalid commands or imply features that are disabled, causing confusion or errors.
- **Mitigation:** Add explicit `isVstMode` gating in UI and command handling. In VST3 mode, disable UI entry points for plugin hosting and show a “Standalone-only” label. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

### 1.4 Session Deletion vs “Never delete immediately”
- **Observation:** SessionStateManager states audio files are “never deleted immediately” (marked for GC on clean exit). Meanwhile `DELETE_JAM` is described as “immediately removes session metadata and riff history entries.”
- **Risk:** There’s ambiguity about **audio file deletion timing** and whether audio persists after delete. For reliability, storage behavior must be deterministic and clearly communicated to the user.
- **Mitigation:** Add explicit behavior for audio files on `DELETE_JAM`: either (a) mark for GC on clean exit, or (b) immediate deletion. If (a), show “space will be reclaimed on next clean exit” message. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

---

## 2) Reliability & Failure-Mode Risks

### 2.1 DiskWriter Tier 3/4 Memory Pressure
- **Observation:** DiskWriter overflow allows up to 1GB of RAM for temporary buffers, while peak app memory can reach ~2GB. This can cause memory pressure or OOM on 8GB machines.
- **Risk:** In low-memory conditions, the OS may kill the app, leading to user data loss or session corruption.
- **Mitigation:** Add a **hard cap per platform** (e.g., 512MB on 8GB RAM) and proactively show warnings. Add a `memoryBudgetMB` gate (already in AppState) to decide overflow cap dynamically. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

### 2.2 Safe Mode Trigger Window Ambiguity
- **Observation:** CrashGuard increments on boot and clears after 60 seconds of stable runtime. The spec says “plugin crash loop (same manufacturer 3× in 60s)” but no explicit time window definition for plugin crash loops in CrashGuard.
- **Risk:** CrashGuard may under- or over-trigger safe mode if its time window differs from plugin respawn logic (1.5s hang detection + exponential backoff).
- **Mitigation:** Specify CrashGuard time windows for plugin crash counts and align with `PluginProcessManager` timings (e.g., “3 crashes within 60 seconds total engine uptime”). Reference `Spec/Spec_FlowZone_Looper1.6.md`.

### 2.3 WebSocket Thread Safety Ambiguity
- **Observation:** The spec says CivetWeb handlers run on its worker threads, `CommandQueue` is the SPSC queue, and `mg_websocket_write()` is used for outgoing state. However, it also says `StateBroadcaster` queues on message thread.
- **Risk:** If multiple threads write via `mg_websocket_write()`, race conditions or invalid calls could occur depending on CivetWeb thread-safety guarantees.
- **Mitigation:** Define a **single writer thread** for all outgoing WS frames. Either: (a) route all outbound writes through a dedicated WebServer thread queue, or (b) document that `mg_websocket_write()` is thread-safe in this usage and enforce a mutex-free, single-threaded send policy. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

### 2.4 Binary Stream ACK Protocol Without Timeouts
- **Observation:** Binary visualization stream uses ACKs and `pendingFrameCount`, but there’s no timeout or “client dropped” detection beyond silent degradation.
- **Risk:** Silent degradation could mask network failures and make UI feel broken (“frozen meters”) with no diagnostics.
- **Mitigation:** Add a **low-severity log** when a client stops ACKing for N seconds and surface a small UI badge (“visualizers paused”). Keep it non-blocking. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

---

## 3) Protocol & Schema Risks

### 3.1 JSON Patch Size Threshold vs Patch Count
- **Observation:** Patches switch to snapshot when either ops > 20 or payload > 4KB. The spec does not define the calculation method for “4KB” (UTF-8 bytes? compressed?).
- **Risk:** If server and client disagree, patch messages may be inconsistent or truncated; hard-to-debug state divergence can occur.
- **Mitigation:** Define a **deterministic size calculation** on the server (UTF‑8 byte length of serialized patch array). Include this in a protocol note for the client. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

### 3.2 `SET_LOOP_LENGTH` Error Handling vs UI Behavior
- **Observation:** Error matrix includes `ERR_BUFFER_EMPTY` (2051) for `SET_LOOP_LENGTH`, but the UI spec describes visual waveform of “capturable signal only.” It doesn’t define how “empty” is determined (e.g., no signal vs no buffer fill).
- **Risk:** User may see waveform and still receive “Nothing to capture,” undermining trust.
- **Mitigation:** Define “buffer empty” precisely (e.g., “no non-silent samples above -60dB in the last N seconds” or “buffer not yet filled N bars”). Align waveform visualization to the same criteria. Reference `Spec/Spec_FlowZone_Looper1.6.md` and `Spec/UI_Layout_Reference.md`.

### 3.3 `COMMIT_RIFF` gating vs UI visibility
- **Observation:** `COMMIT_RIFF` button is only visible when mix changes have occurred. There is no explicit definition of “mix changes” (volume only, mute only, both?), nor how “last commit” state is persisted.
- **Risk:** Button may appear/disappear unexpectedly, causing user confusion or uncommitted changes.
- **Mitigation:** Define a deterministic “mix dirty” hash (e.g., volume+mutes only) and store it in session metadata so it survives reconnects. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

---

## 4) UI–Engine Alignment Risks

### 4.1 UI “No Horizontal Scroll” vs 8+ Channels
- **Observation:** The UI layout states “no horizontal scrolling required for V1” in Mixer, yet the core spec allows 8 slots and future expansions. Some layouts in `UI_Layout_Reference.md` suggest scaling to fit.
- **Risk:** On small screens, 8 faders may become too narrow to use, impacting usability and perceived reliability.
- **Mitigation:** Set a **minimum fader width** and allow horizontal scrolling when below that threshold (or enable a compact/stacked mixer view). Reference `Spec/UI_Layout_Reference.md`.

### 4.2 V2 Placeholders in UI
- **Observation:** UI references V2 items (Sampler reserved, Ableton Link “Coming Soon,” Note Names toggle disabled). The spec is clear, but UI state/commands for these are not fully specified.
- **Risk:** Placeholder controls might accidentally send commands or appear interactive, leading to bug reports.
- **Mitigation:** Ensure disabled controls have no command bindings and are visually labeled as “Coming Soon.” Include explicit spec line: “Disabled controls must not send any commands.” Reference `Spec/Spec_FlowZone_Looper1.6.md` and `Spec/UI_Layout_Reference.md`.

---

## 5) Audio Engine Implementation Risks

### 5.1 FX Mode: Audio Thread Safety During Auto-Merge
- **Observation:** Auto-merge is intended to run off-thread and then swap states atomically at audio callback boundary.
- **Risk:** If merge writes or allocates on the audio thread, it will cause dropouts. Also, swapping buffers safely without locks is non-trivial.
- **Mitigation:** Specify a **double-buffered slot state** model and a lock-free atomic pointer swap. Provide a short pseudocode snippet in the spec to guide implementation. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

### 5.2 Drum Engine: Bounce/Speed Mapping vs Hidden Knobs
- **Observation:** Drum Engine mapping includes `Speed` and `Bounce` parameters, but UI hides these knobs for Drum mode. This creates a mismatch between engine capabilities and UI.
- **Risk:** Engine may implement parameters that users can’t access, or UI and engine drift.
- **Mitigation:** Decide: either remove `Speed`/`Bounce` for drums in V1 (and update spec), or leave them exposed in “advanced” mode. Currently spec conflicts. Reference `Spec/Audio_Engine_Specifications.md` and `Spec/Spec_FlowZone_Looper1.6.md`.

### 5.3 Microtuning Scope vs Phase Plan
- **Observation:** Microtuning is a V1 requirement in Goals, but risk mitigations recommend deferring to Phase 3+ and implementing 12TET first.
- **Risk:** This is a direct conflict between MVP scope and risk plan. It can cause scheduling failures or incomplete V1.
- **Mitigation:** Choose a single stance: (a) **V1 includes microtuning**, then move it earlier in the plan and define minimal test coverage; or (b) **defer to V2**, and remove from V1 goals. Reference `Spec/Spec_FlowZone_Looper1.6.md` and `Spec/Audio_Engine_Specifications.md`.

---

## 6) Build & Deployment Risks

### 6.1 Dual Target Bus Layout
- **Observation:** Standalone uses 2-channel output; VST3 uses 8 stereo pairs (16 channels). The spec says this “must be configured and tested in Phase 0.”
- **Risk:** Projucer bus configuration changes can easily break builds if done later; also VST3 multi-bus routing requires DAW testing.
- **Mitigation:** Include an explicit **Phase 0 smoke test** in the spec: “Load VST3 in a DAW, verify 16 output channels appear, and confirm signal routed to slot outputs.” Reference `Spec/Spec_FlowZone_Looper1.6.md`.

### 6.2 “Projucer is authoritative” vs React build integration
- **Observation:** Phase 0 includes React “Hello World” in `WebBrowserComponent`, but the spec does not clarify how production builds are bundled (Vite build output location, resource embedding, or local file URLs).
- **Risk:** App may work in dev but fail in production packaging, a common reliability issue.
- **Mitigation:** Add a **production bundling plan**: where Vite outputs to, how JUCE loads local HTML/JS/CSS, and how local WS host is resolved in prod. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

---

## 7) Observability & Supportability Risks (Non-Engineer Owner)

### 7.1 End-User Diagnostics
- **Observation:** Structured logs and a health endpoint are specified but do not define a user-facing “Support Bundle.”
- **Risk:** Non-engineer users cannot supply diagnostic info; reliability issues become hard to triage.
- **Mitigation:** Add a “Generate Support Bundle” command in Settings that zips logs, config, and session metadata (excluding large audio files). Make it a Phase 8 bead. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

### 7.2 UI Error Surfacing
- **Observation:** Errors are defined with codes, but UI behavior only mentions toasts or banners in some cases.
- **Risk:** Critical errors (disk critical, plugin crash loops) may be invisible or unclear.
- **Mitigation:** Define **error severity levels** and map them to UI surfaces: toast vs banner vs modal. Ensure disk-critical and safe-mode entry produce modal dialogs. Reference `Spec/Spec_FlowZone_Looper1.6.md`.

---

## 8) Summary of Highest-Priority Fixes Before Beads

1. **Resolve FX capture semantics** (`SET_LOOP_LENGTH` vs `COMMIT_RIFF`) and update the command spec. 
2. **Define explicit knob routing tables** per mode/category and enforce in both C++ and TS.
3. **Clarify microtuning scope** (V1 must-have vs Phase 3+) to avoid scheduling failure.
4. **Specify production bundling for React** to prevent dev-only success.
5. **Define deterministic “buffer empty” criteria** and align waveform visuals with capture logic.
6. **Lock down WebSocket write-thread policy** to avoid concurrency bugs.
7. **Add support bundle export** to help non-engineer troubleshooting.

---

## 9) Readiness Verdict

The spec is close to bead-ready, but there are **7 critical clarifications** above that should be resolved before bead creation. These are not large rewrites; they are precise rule clarifications that will reduce rework and prevent user-facing failures. Once these are addressed, the plan should be safe to convert into beads.

---

## 10) Suggested Next Action (Non-Engineering Owner)

Approve the top 7 clarifications above and lock them into the spec. This will make the first beads significantly more reliable and reduce the need for troubleshooting later.
