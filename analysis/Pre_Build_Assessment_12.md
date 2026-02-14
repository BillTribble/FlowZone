# Pre-Build Assessment 12 — JUCE-Validated Technical Risk Review

**Date:** 14 February 2026  
**Scope:** All documents in the `Spec/` folder only  
**Documents Reviewed:**
- `Spec/Spec_FlowZone_Looper1.6.md` (1870 lines)
- `Spec/Audio_Engine_Specifications.md` (374 lines)
- `Spec/UI_Layout_Reference.md` (375 lines)

**JUCE Documentation Consulted:**
- `juce::WebBrowserComponent` (juce_gui_extra) — ResourceProvider, NativeFunction, NativeEventListener APIs
- `juce::AbstractFifo` (juce_core) — Lock-free SPSC FIFO
- `juce::AudioProcessor` (juce_audio_processors_headless) — BusesProperties, multi-bus configuration, processBlock
- `juce::dsp::Limiter`, `juce::dsp::DelayLine`, `juce::FlacAudioFormat` — DSP and format validation

**Previous Assessment:** Pre_Build_Assessment_11 (same date). All 6 contradictions, 8 ambiguities, 5 missing definitions, and 7 reliability risks were marked as resolved. This assessment verifies those resolutions and identifies **new** issues found through JUCE API validation and deeper cross-referencing.

**Purpose:** Final pre-build integrity check optimized for a non-engineer project owner who cannot debug code. Every finding is assessed for its probability of causing a build failure or runtime bug that requires engineering intervention.

---

## Executive Summary

The spec is in good shape. Assessment 11's fixes significantly improved consistency. However, I found **4 incomplete resolutions** from Assessment 11 that were marked done but are still present in the spec text, **1 critical architecture question** exposed by JUCE 8 API review, **3 new cross-document inconsistencies**, and **5 implementation risks** that could cause hard-to-diagnose failures. None are showstoppers, but all should be resolved before bead creation to avoid agents stalling or implementing conflicting behaviors.

**Finding Counts:**
| Category | Count | Severity |
|:---|:---|:---|
| Incomplete Resolutions from Assessment 11 | 4 | 🔴 Must fix |
| Architecture Decision Required | 1 | 🔴 Critical |
| New Cross-Document Inconsistencies | 3 | 🟠 Should fix |
| Implementation Risks (JUCE-validated) | 5 | 🟠–🟡 Should address |
| Minor Issues | 4 | 🟢 Low priority |

---

## 1. 🔴 Incomplete Resolutions from Assessment 11

These were marked "DONE" in Assessment 11 but the spec text was **not fully updated**. Agents reading the spec will encounter the old, contradictory content.

### IR1: PIN Auth Still Present in Spec

**Assessment 11 Resolution:** "Removed PIN auth entirely from Spec V1."

**Still in spec:**
| Location | Content |
|:---|:---|
| [Main Spec §2.2.L line 215](Spec/Spec_FlowZone_Looper1.6.md:215) | "Optional PIN auth: If `config.json` has `requirePin: true`, client must send `{ cmd: 'AUTH', pin: '…' }` before any other command is accepted." |
| [Main Spec §3.3 line 405](Spec/Spec_FlowZone_Looper1.6.md:405) | Error matrix row: `AUTH` → `1101: AUTH_FAILED` → "Show Incorrect PIN prompt." |
| [Main Spec §5.1 line 747](Spec/Spec_FlowZone_Looper1.6.md:747) | Error code `1101: ERR_AUTH_FAILED` registered in ErrorCodes.h |

**Impact:** An agent implementing the command schema will see `AUTH` and implement it. An agent building the settings panel will look for a PIN configuration section that doesn't exist.

**Fix Required:** Remove the `AUTH` command from §2.2.L connection lifecycle, the `AUTH` row from §3.3 error matrix, and `1101` from §5.1 error codes. Or, if PIN auth is intended for V1, add it back to the settings panel and command schema properly.

---

### IR2: External MIDI Clock Still in AppState

**Assessment 11 Resolution:** "Defer to V2. Updated command schema and settings to reflect this."

**Still in spec:**
| Location | Content |
|:---|:---|
| [Main Spec §3.4 line 516](Spec/Spec_FlowZone_Looper1.6.md:516) | `clockSource: 'internal' | 'external_midi'` in `AppState.settings` |
| [Main Spec §3.2 line 348](Spec/Spec_FlowZone_Looper1.6.md:348) | `SET_CLOCK_SOURCE` command with `source: 'internal'` — partially updated but still in schema |
| [Main Spec §7.6.8 line 1281](Spec/Spec_FlowZone_Looper1.6.md:1281) | Clock Source radio: "Internal | External MIDI Clock" — not disabled like Ableton Link |

**Impact:** The command exists in the schema, so agents will implement it. The `external_midi` option is in AppState, so the TypeScript types will include it. But the feature doesn't work.

**Decision: Use JUCE native approach.** Remove all external clock infrastructure from V1. When external MIDI clock is implemented in V2, it will use JUCE's native `MidiInput` infrastructure directly — not a WebSocket command.

**Fix Required (all 4 changes):**
1. **Remove** `SET_CLOCK_SOURCE` entirely from the Command union in §3.2 and its row from the §3.3 error matrix.
2. **Remove** `clockSource` from `AppState.settings` in §3.4 — or hardcode it to `'internal'` with a comment `// V2: will add 'external_midi'`.
3. **Disable** the "Clock Source" radio in §7.6.8 Tab C — match the Ableton Link treatment: `"disabled": true, "note": "Coming Soon (V2)"`.
4. **Add** a note to §1.3 Future Goals under "MIDI Clock Sync": *"V2 implementation will use JUCE's native `MidiInput` and `MidiMessageCollector` for clock reception, not a WebSocket command."*

---

### IR3: Shutdown Note Still Inside Code Block

**Assessment 11 L7:** "The shutdown note about DiskWriter blocking is inside the code fence instead of below it."

**Still in spec:** [Main Spec §2.4 line 262](Spec/Spec_FlowZone_Looper1.6.md:262) — The `> **Shutdown Note:**` text is within the ` ``` ` code block. Agents parsing the spec (especially AI agents that respect code blocks as literal content) may miss this critical requirement.

**Fix Required:** Move the shutdown note outside and below the code block. Minor formatting fix.

---

### IR4: Infinite FX Count Mismatch After Trance Gate Addition

**Assessment 11 L2 Resolution:** "Added Trance Gate as 12th effect."

**Partially updated:**
| Location | Count | Effects Listed |
|:---|:---|:---|
| [Main Spec §2.2.M line 229](Spec/Spec_FlowZone_Looper1.6.md:229) | "**Infinite FX (11)**" | Outdated count |
| [Audio Engine Spec §1.2](Spec/Audio_Engine_Specifications.md:61) | 12 items listed (including Trance Gate) | Correct |
| [UI Layout Reference play_tab](Spec/UI_Layout_Reference.md:144) | 11 items in `infinite_fx` array | Missing Trance Gate |

**Impact:** The main spec says 11, the audio engine spec has 12, and the UI reference has 11 names. An agent building the FX preset selector grid won't know whether to show 11 or 12 effects.

**Fix Required:** Update main spec §2.2.M to say "Infinite FX (12)". Add "Trance Gate" to the UI Layout Reference `infinite_fx` preset array.

---

## 2. 🔴 Architecture Decision Required

### AD1: WebSocket (CivetWeb) vs JUCE 8 Native Bridge for Embedded Communication

**This is the single most impactful finding in this assessment.**

The spec's entire communication architecture is built on CivetWeb WebSocket:
- [§2.1](Spec/Spec_FlowZone_Looper1.6.md:72): Component diagram shows `WebServer [CivetWeb - Background Thread]`
- [§2.2.L](Spec/Spec_FlowZone_Looper1.6.md:198): Full WebSocket connection lifecycle
- [§3.7](Spec/Spec_FlowZone_Looper1.6.md:566): Binary visualization stream on same WebSocket
- [§8.6](Spec/Spec_FlowZone_Looper1.6.md:1553): Phase 0 includes "CivetWeb integrated as source files"

**However**, JUCE 8's [`WebBrowserComponent`](https://docs.juce.com/master/classjuce_1_1WebBrowserComponent.html) now provides three features that overlap with CivetWeb's role:

1. **`Options::withNativeFunction(name, callback)`** — Registers a C++ function callable from JavaScript. The JS side calls `window.__JUCE__.backend.getNativeFunction(name)(args)` and receives a Promise. This replaces the "command" direction (React → C++) without WebSocket.

2. **`Options::withNativeEventListener(eventId, callback)`** — C++ can emit events to JS via `emitEventIfBrowserIsVisible(eventId, object)`. JS listens via `window.__JUCE__.backend.addEventListener(eventId, handler)`. This replaces the "state broadcast" direction (C++ → React) without WebSocket.

3. **`Options::withResourceProvider(provider)`** — Serves React build artifacts directly from BinaryData or filesystem. The embedded browser loads from `WebBrowserComponent::getResourceProviderRoot()` instead of `http://localhost:8765`. No HTTP server needed for embedded mode.

**The implications:**

| Concern | WebSocket (CivetWeb) | JUCE Native Bridge |
|:---|:---|:---|
| **Embedded client** | Works but adds complexity (port, connection lifecycle, reconnection) | Simpler — no network layer, no port conflicts, no reconnection logic |
| **Remote client (V2)** | Required — phones/tablets need network access | Cannot work — native bridge is in-process only |
| **Binary visualization** | Supported via binary WebSocket frames | Not directly supported — would need Base64 or custom encoding |
| **Multi-device sync (V2)** | Natural fit — all clients use same WebSocket | Would need a separate WebSocket server for remote clients anyway |
| **Thread safety** | CivetWeb's threading is well-documented but requires serialization (per R3 in Assessment 11) | JUCE native bridge runs on the message thread — no extra serialization needed |
| **Latency** | Network roundtrip (~0.5ms localhost) | Direct function call — near zero |
| **Development effort** | Higher (CivetWeb integration, WebSocket protocol, binary frames, reconnection) | Lower for V1 (but must add WebSocket later for V2) |

**Risk if unaddressed:** If agents build the CivetWeb+WebSocket infrastructure and then discover JUCE's native bridge is simpler for V1, significant rework occurs. Conversely, if agents use the native bridge and V2 remote-client support is needed sooner than expected, the entire communication layer must be rewritten.

**Recommendation — Choose ONE of these strategies and document it explicitly:**

**Option A: CivetWeb WebSocket for everything (current spec).** Keep the current architecture. Accept the extra complexity in V1 as an investment in V2 readiness. Add a note: *"The JUCE `WebBrowserComponent` native bridge (`NativeFunction`/`NativeEventListener`) is deliberately NOT used because WebSocket provides a unified communication path for both embedded and future remote clients."*

**Option B: Hybrid approach.** Use JUCE native bridge for the embedded client in V1. Defer CivetWeb to V2 when remote clients are needed. This simplifies Phase 0 significantly (no CivetWeb integration, no port management, no reconnection logic). Binary visualization would need to be sent as JSON or Base64-encoded data through `emitEventIfBrowserIsVisible`, which may impact 30fps performance.

**Option C: CivetWeb for WebSocket + ResourceProvider for content.** Use CivetWeb only for WebSocket communication (commands + state), but use JUCE's `ResourceProvider` to serve the React build artifacts instead of CivetWeb's HTTP handler. This eliminates the need for CivetWeb to serve static files and simplifies the production deployment (no need to figure out BinaryData serving through CivetWeb).

**My recommendation for a non-engineer owner:** **Option A** is the safest choice. The spec already describes it fully, agents know what to build, and it's future-proof. The extra complexity is manageable within the phased approach. However, add a note acknowledging the native bridge exists and why it's not used, so agents don't second-guess the architecture.

---

## 3. 🟠 New Cross-Document Inconsistencies

### NC1: FX Adjust Tab Knob Mapping Placed in Wrong Section

**Assessment 11 A1 Resolution:** "Adjust tab shows effect-specific parameters. Added default mappings to Spec 1.6."

**Problem:** The FX Mode knob mapping was added to [§7.7.1 "Knob Layout (2×4 Grid)"](Spec/Spec_FlowZone_Looper1.6.md:1366) — which is inside the **Jam Manager / Home Screen** section, not the **Adjust Tab** section (§7.6.4).

Additionally, the mapping says "Parameter mapping is defined per-effect in `Audio_Engine_Specifications.md`" — but the Audio Engine Spec only defines **XY Pad mappings** (X and Y axis), not Adjust tab knob mappings. The Adjust tab has 8 knob positions (2×4 grid), which is a different parameter surface than the 2-axis XY pad.

**Impact:** An agent building the Adjust tab for FX Mode will find no concrete knob assignments. They'll either leave it blank or make arbitrary choices.

**Fix Required:**
1. Move the FX Mode knob mapping description from §7.7.1 to §7.6.4 (Adjust Tab Layout), as a new sub-section alongside the existing Drums/Notes/Bass/Microphone knob layouts.
2. For each effect, define which parameters map to which knob positions. Suggested approach: For effects with 2 parameters (most of them), map Parameter 1 to knob position (1,1) and Parameter 2 to (1,2), leaving remaining positions blank. For effects with 3+ parameters (Gate, Distortion, Compressor, Degrader, Ringmod), map to positions (1,1), (1,2), (1,3), etc.

---

### NC2: TOGGLE_NOTE_NAMES — Command vs localStorage + V1 vs V2

Three separate issues compound on this single toggle:

| # | Location | Statement | Problem |
|:---|:---|:---|:---|
| 1 | [Main Spec §3.2 line 337](Spec/Spec_FlowZone_Looper1.6.md:337) | "UI-only, stored in localStorage — **not sent to engine**" | Says it's client-side only |
| 2 | [UI Layout Reference line 360](Spec/UI_Layout_Reference.md:360) | `"command": "TOGGLE_NOTE_NAMES"` | Tags it as a WebSocket command |
| 3 | [Main Spec §7.6.8 line 1291](Spec/Spec_FlowZone_Looper1.6.md:1291) | "**Note Names** — Toggle (default: OFF, **V2 Feature - Disabled in V1**)" | Says it's disabled in V1 |
| 4 | [Main Spec §3.4 line 519](Spec/Spec_FlowZone_Looper1.6.md:519) | `ui: { noteNamesEnabled: boolean }` | Field exists in AppState (implies engine sends it) |

**Impact:** An agent building the settings panel sees four contradictory signals: it's a command (UI Ref), it's localStorage (main spec §3.2), it's disabled (main spec §7.6.8), and it's in AppState (main spec §3.4). This will cause implementation confusion — should the field exist in the engine state? Should a toggle be rendered? Should it be greyed out?

**Fix Required (all 4 changes):**
1. **Remove** `"command": "TOGGLE_NOTE_NAMES"` from [UI Layout Reference line 360](Spec/UI_Layout_Reference.md:360). Replace with `"storage": "localStorage", "disabled": true, "note": "V2 Feature"`.
2. **Remove** `ui: { noteNamesEnabled: boolean }` from `AppState` in §3.4. Since it's localStorage-only and disabled in V1, it should not be in the engine's state model. Move to a comment: `// V2: ui.noteNamesEnabled will be added when Note Names feature is implemented.`
3. **Keep** the comment in §3.2 command schema as-is — it correctly notes this is UI-only.
4. **Keep** the "V2 Feature - Disabled in V1" note in §7.6.8 — this correctly tells agents to render the toggle as disabled/greyed out.

---

### NC3: Drum Mode Knob Availability — "Removed" vs Parameter Mapping

| Location | Statement |
|:---|:---|
| [Main Spec §7.6.4](Spec/Spec_FlowZone_Looper1.6.md:1132) | "In Drum mode, the **Bounce** and **Speed** knobs are **Hidden** (or rendered as disabled/blank)" |
| [Audio Engine Spec §4.3 lines 267-269](Spec/Audio_Engine_Specifications.md:267) | "**Speed** → Tape speed (pitch/time). **Bounce** → Granular repeat rate. **Note:** In Drum Mode, **Bounce** and **Speed** knobs are **Removed** from the UI in V1 (not just hidden)." |

**Assessment 11 Resolution:** "Drum knobs are **Removed** in V1. Spec updated."

**Problem:** The main spec §7.6.4 still says "Hidden (or rendered as disabled/blank)" — it was NOT updated to say "Removed." Meanwhile, the Audio Engine Spec says "Removed." Additionally, the Audio Engine Spec defines what Bounce and Speed *would* do for drums (`Tape speed`, `Granular repeat rate`), which contradicts the main spec §7.6.4 that says they "are not used by the drum engine in V1."

**Fix Required:** Update main spec §7.6.4 to say "Removed" instead of "Hidden (or rendered as disabled/blank)." Remove the parameter descriptions for Speed/Bounce from Audio Engine Spec §4.3 (or explicitly mark them V2).

---

## 4. 🟠 Implementation Risks (JUCE-Validated)

### R1: Master Limiter Lookahead Adds Latency Not Accounted For

[Main Spec §2.2.A line 117](Spec/Spec_FlowZone_Looper1.6.md:117): "Brickwall limiter... Ceiling: -0.3 dBFS, **lookahead: 1ms**."

At 48kHz, 1ms = 48 samples of latency. JUCE's [`dsp::Limiter`](https://docs.juce.com/master/classjuce_1_1dsp_1_1Limiter.html) implements lookahead by delaying the signal. This means:

1. The `FlowEngine` must call `setLatencySamples(48)` to report this to the host (critical for VST3 mode).
2. In Standalone mode, this adds 1ms of monitoring latency on top of the audio buffer latency. At 512 samples / 48kHz = ~10.7ms buffer latency, this is negligible. But at 64 samples (~1.3ms), it's a 77% increase.
3. The limiter is bypassed in VST3 multi-channel mode, so latency should be 0 in that mode. This means latency reporting changes between Standalone and VST3 — the `#if JucePlugin_Build_VST3` guard needs to control `setLatencySamples()` too.

**Recommendation:** Add a note to §2.2.A: *"The 1ms lookahead introduces 48 samples of latency at 48kHz. `FlowEngine::prepareToPlay` must call `setLatencySamples(48)` in Standalone mode and `setLatencySamples(0)` in VST3 mode."*

---

### R2: FLAC Compression Level 0 Real-Time Feasibility

[Main Spec §3.6 line 561](Spec/Spec_FlowZone_Looper1.6.md:561): "FLAC... **Compression level 0** (fastest) for real-time recording."

JUCE's [`FlacAudioFormat`](https://docs.juce.com/master/classjuce_1_1FlacAudioFormat.html) wraps libFLAC. Compression level 0 is the fastest and should handle real-time 24-bit/48kHz stereo on Apple Silicon without issue. However, the DiskWriter operates on a background thread, so FLAC encoding doesn't need to be truly real-time — it just needs to keep up with the ring buffer drain rate.

**Risk Level:** Low. But the spec should note that FLAC encoding happens on the DiskWriter thread (not the audio thread) to reassure agents that this is safe.

**Recommendation:** Add to §2.2.I (DiskWriter): *"FLAC encoding is performed on the DiskWriter background thread, not the audio thread. The ring buffer decouples real-time capture from encoding latency."*

---

### R3: `AbstractFifo` Maximum Capacity is `bufferSize - 1`

JUCE's [`AbstractFifo`](https://docs.juce.com/master/classjuce_1_1AbstractFifo.html) documentation states: *"The maximum number of items managed by the FIFO is **1 less than the buffer size**."* This is a standard lock-free ring buffer property.

The spec doesn't specify the CommandQueue capacity, but if an agent allocates a buffer of (say) 256 command slots, only 255 are usable. This is unlikely to cause issues but should be documented to prevent off-by-one sizing.

**Recommendation:** Add a note to §2.2.C (CommandQueue): *"Buffer capacity note: `juce::AbstractFifo` manages `bufferSize - 1` items. Allocate one extra slot."*

---

### R4: WebBrowserComponent ResourceProvider for Production Builds

Regardless of whether the native bridge (AD1 Option B) is used, the React app needs to be served to the `WebBrowserComponent`. The spec mentions two modes:
- **Dev:** React dev server at `localhost:5173` — use `goToURL("http://localhost:5173")`
- **Prod:** "bundled resources via BinaryData or local file server" ([§8.6 Phase 0](Spec/Spec_FlowZone_Looper1.6.md:1556))

JUCE 8's `WebBrowserComponent` has `Options::withResourceProvider(provider)` + `getResourceProviderRoot()`. This is the correct way to serve bundled React assets in production. The provider receives URL path strings and returns `Resource` objects (data + MIME type).

**Risk:** The spec doesn't define the production resource serving strategy in detail. Agents need to know:
1. How the Vite build output gets into the JUCE app (BinaryData? Local files?)
2. Whether `ResourceProvider` is used (recommended) or CivetWeb serves static files
3. How the dev/prod modes switch

**Recommendation:** Add to §2.2.L or §8.6 Phase 0:
> *"**Production Content Serving:** Use `WebBrowserComponent::Options::withResourceProvider` to serve the Vite build output. React build artifacts are embedded via JUCE's BinaryData system (add `web_client/dist/` to Projucer as a BinaryData source). The ResourceProvider maps URL paths to BinaryData resources. In development, `goToURL("http://localhost:5173")` is used instead, allowing Vite HMR."*

---

### R5: DELETE_JAM on Active Session Undefined

[Main Spec §3.2 line 336](Spec/Spec_FlowZone_Looper1.6.md:336): `DELETE_JAM { sessionId }` — "Immediately removes session metadata and riff history entries."

**Question:** What happens if `sessionId` matches the **currently active** session? The spec only defines `SESSION_NOT_FOUND` as an error. But deleting the active session should either:
- (a) Be rejected with a new error like `ERR_SESSION_ACTIVE` — "Cannot delete the currently loaded session."
- (b) Trigger `NEW_JAM` first (create a new empty session), then delete the old one.

**Impact for non-engineer user:** If delete-active-session is allowed without creating a new session first, the app enters an undefined state (no session loaded, no slots, no transport).

**Recommendation:** Add to §3.2 `DELETE_JAM`: *"If `sessionId` is the currently active session, the engine first creates and switches to a new empty session (equivalent to `NEW_JAM`), then deletes the old session. This ensures the app is never in a session-less state."* Add this behavior to the error matrix as a note.

---

## 5. 🟢 Minor Issues

| # | Issue | Location | Recommendation |
|:---|:---|:---|:---|
| M1 | `SET_PAN` is still referenced in §3.9 Optimistic UI Pattern | [Main Spec line 594](Spec/Spec_FlowZone_Looper1.6.md:594) | "`SET_VOL`, `SET_PAN`, `SET_TEMPO`" — remove `SET_PAN`. Assessment 11 C3 was supposed to remove all pan references. |
| M2 | CivetWeb version not pinned | [Main Spec §8.6 Phase 0](Spec/Spec_FlowZone_Looper1.6.md:1553) | Add: "Pin CivetWeb to a specific release tag (e.g., v1.16) to prevent API changes between builds." |
| M3 | `Spec_FlowZone_Looper1.6.md` §7.6.5 Line 1180 has garbled text | [Main Spec line 1180](Spec/Spec_FlowZone_Looper1.6.md:1180) | Text reads: "...on narrower ones. This matches the UI Layout Reference.ical slider bar for volume control" — appears to be a merge artifact. Should read: "...on narrower ones. This matches the UI Layout Reference.\n* **Vertical slider bar** for volume control" |
| M4 | Keymasher button grid is 3×4 (12 buttons) but `KeymasherButton` type has 12 values | [Main Spec line 360](Spec/Spec_FlowZone_Looper1.6.md:360) vs [UI Layout Reference line 151](Spec/UI_Layout_Reference.md:151) | Consistent. No issue — just noting for completeness that both sides agree on 12 buttons. ✅ |

---

## 6. Verification of Assessment 11 Resolutions

For completeness, here is the status of every Assessment 11 finding:

| ID | Finding | Resolution Status | Verified? |
|:---|:---|:---|:---|
| C1 | DiskWriter Overflow Cap 512MB vs 1GB | Sliding scale added | ✅ Verified at line 179 |
| C2 | Drum Mode Bounce/Speed knobs | "Removed" in Audio Engine Spec | ⚠️ Main spec not updated (see NC3) |
| C3 | Pan references in COMMIT_RIFF | Removed from COMMIT_RIFF | ⚠️ Still in §3.9 (see M1) |
| C4 | External MIDI Clock V1 vs V2 | Deferred to V2 | ⚠️ Not fully removed (see IR2) |
| C5 | Zigzag mixer layout | Added to main spec | ✅ Verified at line 1180 (but garbled — see M3) |
| C6 | Drum pad MIDI Note column | Added | ✅ Verified in Audio Engine Spec §4.1 |
| A1 | FX Mode Adjust tab | Mapping added | ⚠️ In wrong section (see NC1) |
| A2 | `transport.loopLengthBars` meaning | Clarified | ✅ Verified at line 457 |
| A3 | LOAD_RIFF behavior | §3.12 added | ✅ Verified at lines 635-641 |
| A4 | GoTo effect playhead | Clarified | ✅ Verified in Audio Engine Spec line 34 |
| A5 | Auto-merge in FX Mode unreachable | Removed | ✅ Verified at lines 1037-1039 |
| A6 | Slot metadata population | §3.11 added | ✅ Verified at lines 625-631 |
| A7 | instrumentCategory → color mapping | Table added | ✅ Verified at lines 499-508 |
| A8 | Retro buffer on sample rate change | Note added | ✅ Verified at line 342 |
| M1 | No CLEAR_SLOT command | Dismissed | ✅ Acknowledged |
| M2 | No PIN management commands | PIN removed entirely | ⚠️ Not fully removed (see IR1) |
| M3 | No STOP/RESET_TRANSPORT | Design choice confirmed | ✅ Acknowledged |
| M4 | Error surfacing levels in UI Layout Ref | Not addressed | ⚠️ Still not cross-referenced — low priority |
| M5 | Session emoji change | `emoji` added to RENAME_JAM | ✅ Verified at line 335 |
| R1 | DELETE_JAM destructive | Modal confirmation sufficient | ✅ Acknowledged |
| R2 | Varispeed BPM changes | Dismissed | ✅ Acknowledged |
| R3 | CivetWeb thread serialization | WebSocketWriter singleton required | ✅ Verified mentioned at line 170 |
| R4 | JSON Patch implementation | Use library | ✅ Acknowledged |
| R5 | Lock-free SPSC queue | Use `juce::AbstractFifo` | ✅ Verified at line 127 |
| R6 | Binary + JSON on single WebSocket | Serialization via WebSocketWriter | ✅ Addressed by R3 |
| R7 | 96s retrospective buffer margin | Updated to 97s | ✅ Verified at line 599 |
| L1 | Quick toggles empty | Note Names toggle added | ✅ Verified in UI Layout Reference line 360 |
| L2 | Infinite FX 11 effects | Trance Gate added (12) | ⚠️ Main spec still says 11, UI ref still has 11 (see IR4) |
| L3 | Export Stems disabled button | Not addressed | Still shows disabled button. Low priority. |
| L4 | No cancel for RESCAN_PLUGINS | Not addressed | Low priority. |
| L5 | Emoji collisions | Not addressed | Acceptable. |
| L6 | UI Layout Ref play_tab mixing FX/instrument | Not addressed | Low priority. |
| L7 | Shutdown note inside code block | Not addressed | ⚠️ See IR3 |

**Summary:** 10 of 31 Assessment 11 items have incomplete resolutions (⚠️). 4 of these are consequential enough to be flagged as must-fix (IR1-IR4). The remaining 6 are minor (M1, M4, L3-L6).

---

## 7. Positive Observations (What's Improved Since Assessment 11)

1. **§3.11 Slot Population on Capture** — Excellent addition. Agents now know exactly which fields are set and from where when audio is committed. Zero ambiguity.

2. **§3.12 LOAD_RIFF Behavior** — Fully specified with 5 clear behavioral rules. The `swap_on_bar` mode integration is particularly well-defined.

3. **instrumentCategory → Color Mapping Table** — Direct, unambiguous, includes all 8 categories. Agents can implement this without questions.

4. **Retrospective buffer sample rate change note** — Clean, actionable: "Changing rate requires brief engine reset; retrospective buffer is cleared."

5. **GoTo effect clarification** — "Jumps the internal effector read position, NOT the global transport." Simple, prevents a dangerous misimplementation.

6. **Audio Engine Spec §4.1 MIDI Note column** — Table now parseable with correct column alignment. Critical for drum engine implementation.

---

## 8. Recommended Actions Before Bead Creation

### Priority 1 — Must Fix (blocks correct implementation)

- [ ] **IR1:** Remove PIN auth from §2.2.L connection lifecycle, §3.3 error matrix, and §5.1 error codes
- [ ] **IR2:** Remove `SET_CLOCK_SOURCE` from command schema §3.2 and error matrix §3.3. Remove `clockSource` from `AppState.settings` §3.4. Disable Clock Source radio in §7.6.8 Tab C with "Coming Soon (V2)". Add JUCE native MIDI note to §1.3.
- [ ] **IR4:** Update main spec §2.2.M "Infinite FX (11)" → "(12)" and add Trance Gate to UI Layout Reference
- [ ] **AD1:** ✅ **DECIDED: Use JUCE native bridge** (`NativeFunction`/`NativeEventListener`/`ResourceProvider`) for V1. Update spec architecture (§2.1 component diagram, §2.2.L frontend, §8.6 Phase 0) to replace CivetWeb+WebSocket with JUCE native APIs. Defer CivetWeb/WebSocket to V2 when remote multi-device access is needed.
- [ ] **NC1:** Move FX Mode knob mapping from §7.7.1 to §7.6.4, and define per-effect knob assignments (or state that XY pad parameters are reused for the Adjust knob grid)
- [ ] **R5:** Define DELETE_JAM behavior on active session (auto-switch to new jam first)

### Priority 2 — Should Fix (prevents agent confusion)

- [ ] **NC2:** Fix TOGGLE_NOTE_NAMES (4 changes): Remove `"command"` from UI Layout Reference → replace with `"storage": "localStorage", "disabled": true`. Remove `ui.noteNamesEnabled` from AppState §3.4 (V2). Keep §3.2 comment and §7.6.8 disabled note.
- [ ] **NC3:** Update main spec §7.6.4 drum knob description to say "Removed" instead of "Hidden"
- [ ] **R1:** Add limiter latency note (48 samples at 48kHz) to §2.2.A with `setLatencySamples` requirement
- [ ] **R4:** Define production resource serving strategy (ResourceProvider + BinaryData)
- [ ] **M1:** Remove `SET_PAN` from §3.9 Optimistic UI Pattern
- [ ] **M3:** Fix garbled text at §7.6.5 line 1180

### Priority 3 — Nice to Have

- [ ] **IR3:** Move shutdown note outside code block in §2.4
- [ ] **R2:** Add FLAC encoding thread note to §2.2.I
- [ ] **R3:** Add AbstractFifo capacity note to §2.2.C

---

## 9. Build Confidence Assessment

| Area | Confidence | Notes |
|:---|:---|:---|
| **Command Schema** | 🟢 High | Complete, well-structured, error matrix covers all commands. Minor cleanup needed (PIN, clock source). |
| **Audio Engine Architecture** | 🟢 High | Thread model is sound. AbstractFifo validated. Bus configuration for VST3 is well-specified. |
| **State Management** | 🟢 High | AppState is comprehensive. Slot population rules clear. Riff history behavior defined. |
| **UI Layout** | 🟡 Medium | Good overall structure, but FX Mode Adjust tab undefined, some garbled text, and the note names toggle command conflict. |
| **Communication Layer** | 🟢 High (after AD1 fix) | Decision made: JUCE native bridge for V1. Simpler than WebSocket — no port management, no reconnection logic. ResourceProvider for content serving. Spec needs updating to reflect this. |
| **Error Handling** | 🟢 High | Error codes registered, error matrix complete, failure strategies tiered. |
| **Build System / Phasing** | 🟢 High | Phase 0-8 ordering is risk-aware and well-sequenced. Human checkpoints are excellent. |
| **DSP / Effects** | 🟢 High | Parameter ranges specified. XY mappings defined. Synthesis methods documented. |

**Overall: Ready for bead creation after Priority 1 fixes are applied.** The spec is mature, comprehensive, and well-organized. The remaining issues are all resolvable with targeted text edits — no architectural redesign is needed. The phased build plan with human checkpoints provides strong protection against integration failures.
