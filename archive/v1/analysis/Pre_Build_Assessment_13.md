# Pre-Build Assessment 13 — Final Integrity & Risk Review

**Date:** 14 February 2026  
**Scope:** All documents in the `Spec/` folder only  
**Documents Reviewed:**
- `Spec/Spec_FlowZone_Looper1.6.md` (1880 lines)
- `Spec/Audio_Engine_Specifications.md` (373 lines)
- `Spec/UI_Layout_Reference.md` (375 lines)

**JUCE Documentation Consulted:**
- `juce::WebBrowserComponent` — NativeFunction, NativeEventListener, ResourceProvider, emitEventIfBrowserIsVisible, getResourceProviderRoot
- `juce::AudioProcessor` — BusesProperties, processBlock, setLatencySamples, WrapperType

**Previous Assessment:** Pre_Build_Assessment_12. This assessment verifies which of Assessment 12's recommendations have been addressed and identifies any remaining or newly discovered issues.

**Purpose:** Final pre-build integrity check. Designed for a non-engineer project owner — every finding is assessed for its impact on build reliability and whether it could cause an agent to stall, implement incorrect behaviour, or produce a runtime failure.

---

## Executive Summary

Assessment 12's findings have been **substantially addressed**. Of the 17 tracked items from Assessment 12, **13 are fully resolved**, **1 is partially resolved**, and **3 remain unaddressed** (all minor text issues). The architecture decision (CivetWeb + WebSocket) is now explicitly documented with rationale. The overall spec quality is high.

This assessment found **2 new substantive issues** and **4 residual text defects** from Assessment 12. None are architectural — all are fixable with targeted text edits.

**Finding Counts:**

| Category | Count | Severity |
|:---|:---|:---|
| Unresolved from Assessment 12 | 3 | 🟡 Minor text |
| Partially resolved from Assessment 12 | 1 | 🟠 Should fix |
| New cross-document inconsistency | 1 | 🟠 Should fix |
| New risk (non-engineer safety) | 1 | 🟡 Medium |
| New minor issues | 2 | 🟢 Low priority |

---

## 1. Verification of Assessment 12 Resolutions

### ✅ Fully Resolved (13 of 17)

| ID | Finding | Evidence of Resolution |
|:---|:---|:---|
| **IR1** | PIN Auth remnants | Removed from connection lifecycle (§2.2.L lines 210-218), error codes (§5.1 lines 740-769), and command schema. No trace of `AUTH` or `1101`. |
| **IR2** | External MIDI Clock in schema/state | `SET_CLOCK_SOURCE` has removal comment (line 351). `clockSource` removed from `AppState.settings` (line 517). Clock Source radio in §7.6.8 Tab C now shows `"disabled": true, "note": "Coming Soon (V2)"` (line 1294). |
| **IR4** | Infinite FX count 11→12 | Main spec §2.2.M (line 233) now says "Infinite FX (12)" and lists all 12 including Trance Gate. UI Layout Reference `infinite_fx` array (line 144) now includes "Trance Gate". |
| **AD1** | CivetWeb vs JUCE native bridge | Architecture decision explicitly documented at §2.2.L (line 206): *"We deliberately use CivetWeb + WebSockets (instead of JUCE 8's native bridge) to ensure future V2 compatibility with remote clients..."* This is Option A from Assessment 12. JUCE docs confirm `emitEventIfBrowserIsVisible()` only works when the browser component is visible — another argument supporting the WebSocket choice. |
| **NC1** | FX Mode Adjust tab knob mapping | New section §7.6.4a (lines 1138-1150) defines per-parameter knob assignments for FX Mode with 2-param and >2-param rules. Correctly placed after §7.6.4. |
| **NC2** | TOGGLE_NOTE_NAMES conflicts | All 4 changes made: Command removed from UI Ref (line 360 now has `"storage": "localStorage", "disabled": true`). `ui.noteNamesEnabled` removed from AppState (lines 519-520 show comment only). §3.2 comment retained. §7.6.8 disabled note retained. |
| **NC3** | Drum Mode knobs "Hidden" vs "Removed" | Main spec §7.6.4 (line 1132) now says "**Removed** (blank)" — matches Audio Engine Spec §4.3 (line 269). |
| **R2** | FLAC encoding thread note | Added to §2.2.I DiskWriter (line 182): *"FLAC encoding is performed on this background thread, decoupled from the audio thread via the RingBuffer."* |
| **R3** | AbstractFifo capacity note | Added to §2.2.C CommandQueue (line 128): *"Capacity Note: `juce::AbstractFifo` manages `bufferSize - 1` items. Allocate one extra slot."* |
| **R1** | Master limiter latency | N/A — The spec changed to "No Master Limiter" (line 117): *"no master dynamics processing (limiter/compressor) is applied to the output."* The latency concern is eliminated by this design change. See New Finding NR1 below for related risk. |
| **M2** | CivetWeb version not pinned | Phase 0 (line 1563) now includes "Pin to v1.16". |
| **M4** | Keymasher button grid count | Confirmed consistent: 12 buttons in both `KeymasherButton` type (line 363) and UI Layout Reference grid (lines 151-163). |
| **M1-orig** | SET_PAN references in COMMIT_RIFF | Removed from COMMIT_RIFF command description. |

### ⚠️ Partially Resolved (1 of 17)

#### PR1: Production Content Serving Strategy (Assessment 12 R4)

**Assessment 12's recommendation:** Define exactly how production React assets are served.

**Current state:** Line 218 says: *"In production builds, the React app artifacts (HTML/JS/CSS) should be served by CivetWeb from the local filesystem or via JUCE's `ResourceProvider` if preferred for simplification — but the Command/State communication MUST remain over WebSocket."*

**Issue:** This gives two options but doesn't choose one. Since the architecture decision (AD1) commits to CivetWeb for all communication, the natural and simplest approach is:
- **Development:** React dev server at `localhost:5173` (Vite HMR)
- **Production:** CivetWeb serves the Vite build output (`web_client/dist/`) as static files from the local filesystem

Using JUCE's `ResourceProvider` alongside CivetWeb would mean maintaining two separate serving mechanisms, which adds unnecessary complexity.

**Recommendation:** Replace the hedging language with a clear decision:
> *"**Production Content Serving:** CivetWeb serves the Vite build output from the local filesystem (`~/Library/Application Support/FlowZone/web_client/` or bundled alongside the app). In development, `WebBrowserComponent::goToURL("http://localhost:5173")` enables Vite HMR. Command/State communication always uses WebSocket regardless of content serving method."*

**Risk if unaddressed:** An agent may choose `ResourceProvider` for production serving, which is incompatible with the CivetWeb architecture since `ResourceProvider` intercepts all URL requests within the WebView, potentially breaking the WebSocket connection path. Medium risk.

---

### ❌ Unresolved from Assessment 12 (3 of 17)

These are all minor text defects that won't cause build failures but reduce spec clarity.

#### U1: Shutdown Note Still Inside Code Block (Assessment 12 IR3)

**Location:** [Main Spec §2.4 line 265](Spec/Spec_FlowZone_Looper1.6.md:265)

The `> **Shutdown Note:** DiskWriter shutdown is blocking...` text remains inside the ` ``` ` code block (between lines 260-266). AI agents parsing the spec may treat code blocks as literal content and miss this critical requirement.

**Fix:** Move the shutdown note outside and below the closing ` ``` ` on line 266. Formatting-only change.

---

#### U2: SET_PAN Still in §3.9 Optimistic UI Pattern (Assessment 12 M1)

**Location:** [Main Spec §3.9 line 595](Spec/Spec_FlowZone_Looper1.6.md:595)

Text reads: *"For low-latency commands (`SET_VOL`, `SET_PAN`, `SET_TEMPO`)..."* — `SET_PAN` does not exist in the V1 command schema (pan was removed). An agent reading this section may search for a `SET_PAN` command that doesn't exist.

**Fix:** Change to: *"For low-latency commands (`SET_VOL`, `SET_TEMPO`)..."*

---

#### U3: Garbled Text at §7.6.5 Mixer Tab (Assessment 12 M3)

**Location:** [Main Spec §7.6.5 line 1193](Spec/Spec_FlowZone_Looper1.6.md:1193)

Text reads: *"...on narrower ones. This matches the UI Layout Reference.ical slider bar for volume control (`SET_VOL` command)"*

This is a merge artifact. The `.ical slider bar` fragment appears to be the tail end of "Vert**ical** slider bar" that got incorrectly concatenated.

**Fix:** Replace with:
> *"...on narrower ones. This matches the UI Layout Reference.*
> *   **Vertical slider bar** for volume control (`SET_VOL` command)"*

---

## 2. 🟠 New Cross-Document Inconsistency

### NC4: Infinite FX Bank Section Header in Audio Engine Spec Still Says "(11 Effects)"

| Location | Text |
|:---|:---|
| [Audio Engine Spec §1.2 line 61](Spec/Audio_Engine_Specifications.md:61) | `### **1.2. Infinite FX Bank (11 Effects)**` |
| Same section, lines 63-111 | 12 effects listed (including Trance Gate at item 12) |
| [Main Spec §2.2.M line 233](Spec/Spec_FlowZone_Looper1.6.md:233) | `**Infinite FX (12):**` — Correct |
| [UI Layout Reference line 144](Spec/UI_Layout_Reference.md:144) | 12 items in `infinite_fx` array — Correct |

**Impact:** The heading count mismatch in the Audio Engine Spec was partially fixed in Assessment 12 (main spec and UI ref were updated) but the Audio Engine Spec's section header was missed.

**Fix:** Change [Audio Engine Spec §1.2 line 61](Spec/Audio_Engine_Specifications.md:61) from `(11 Effects)` to `(12 Effects)`.

---

## 3. 🟡 New Risk: No Output Limiter for Non-Engineer User

### NR1: Digital Clipping Without Master Limiter

**Relevant spec text:** [Main Spec §2.2.A line 117](Spec/Spec_FlowZone_Looper1.6.md:117): *"no master dynamics processing (limiter/compressor) is applied to the output. Users should manage gain staging within the mix."*

**Context:** This design decision prioritizes minimum monitoring latency. It was a deliberate replacement for the earlier brickwall limiter specification (removed in response to Assessment 12 R1 about lookahead latency).

**Risk for non-engineer user:** With 8 slots playing simultaneously, individual slot volumes at 1.0, and no master limiter, the summed output can easily exceed 0 dBFS. Digital clipping:
- Sounds harsh and unpleasant
- Could damage speakers or headphones in extreme cases
- Is invisible to a non-engineer who doesn't understand gain staging

**This is NOT a build-blocking issue**, but it's a usability risk for your specific profile (non-engineer user).

**Recommendation — choose one:**

1. **Accept the risk (current spec).** The "flow machine" philosophy prioritizes immediacy. Users will learn to manage gain by ear. Add a visual peak indicator (VU meter with red zone) to make clipping visible.

2. **Add a zero-latency soft clipper** instead of a lookahead limiter. A waveshaper-style soft clipper (e.g., `tanh()` or similar) adds **zero latency** (no lookahead needed) and prevents harsh digital clipping. It won't be transparent — it will colour the sound at high levels — but it's significantly better than hard clipping. This could be added as a single line in `processBlock` after the mix sum.

3. **Add a simple auto-gain stage.** After summing all slots, divide by the number of active slots (or apply a fixed headroom offset like -6dB). This doesn't prevent clipping but reduces its likelihood significantly.

If you choose Option 2 or 3, add a note to §2.2.A. If you choose Option 1, consider adding a UI element (like a "CLIP" indicator on the header bar) so the user knows when the output is clipping.

---

## 4. 🟢 New Minor Issues

### M5: `GENERATE_SUPPORT_BUNDLE` Missing from Error Matrix

**Location:** [Main Spec §3.2 line 359](Spec/Spec_FlowZone_Looper1.6.md:359) defines the command. [Main Spec §3.3](Spec/Spec_FlowZone_Looper1.6.md:381) (Error Matrix) has no entry for it.

**Fix:** Add to §3.3:

| Command | Possible Errors | Client Behavior |
|:---|:---|:---|
| `GENERATE_SUPPORT_BUNDLE` | `1001: ERR_DISK_FULL` | Show error toast. On success, show path to generated zip. |

---

### M6: `ui: {}` Empty Object Remaining in AppState

**Location:** [Main Spec §3.4 lines 519-521](Spec/Spec_FlowZone_Looper1.6.md:519)

```typescript
ui: {
    // noteNamesEnabled removed from state (client-side localStorage only)
};
```

This empty object serves no purpose in V1 and will be serialized/deserialized for every state snapshot. While harmless, it adds noise. An agent may wonder if they should populate it.

**Recommendation:** Either remove the `ui` field entirely with a comment `// V2: ui section will be added`, or leave as-is with the existing comment (current approach is acceptable). Low priority.

---

## 5. DELETE_JAM on Active Session (Assessment 12 R5 Follow-Up)

**Assessment 12 recommended:** Define what happens when `DELETE_JAM` targets the currently active session.

**Current state:** [Main spec §3.2 line 339](Spec/Spec_FlowZone_Looper1.6.md:339) describes `DELETE_JAM` as: *"Immediately removes session metadata and riff history entries. **User must confirm via irreversible warning dialog.**"*

The spec does NOT explicitly state what happens if `sessionId` matches the active session. However, the confirmation dialog provides a user-facing gate, and the intended UX flow (Home button → Jam Manager → select jam → delete) means the user would typically navigate away from the session first.

**Residual risk:** If the user somehow issues `DELETE_JAM` on the active session (e.g., via Jam Manager without navigating away first), the app could enter an undefined state with no loaded session.

**Recommendation:** Add to §3.2 `DELETE_JAM` comment:
> *"If `sessionId` is the currently active session, the engine automatically creates a new empty session (equivalent to `NEW_JAM`) before deleting the old one. This ensures the app is never in a session-less state."*

This is a **should-fix** before beads but not a blocker.

---

## 6. Full Assessment 12 Verification Table

| ID | Finding | Status | Notes |
|:---|:---|:---|:---|
| IR1 | PIN Auth remnants | ✅ Resolved | Fully removed from all locations |
| IR2 | External MIDI Clock | ✅ Resolved | Properly deferred with V2 notes |
| IR3 | Shutdown note in code block | ❌ Unresolved | See U1 |
| IR4 | Infinite FX count 11→12 | ✅ Resolved | Main spec + UI ref updated. Audio Engine Spec header missed (see NC4) |
| AD1 | CivetWeb vs native bridge | ✅ Resolved | Explicitly documented with rationale |
| NC1 | FX Adjust tab knob mapping | ✅ Resolved | New §7.6.4a section added |
| NC2 | TOGGLE_NOTE_NAMES conflicts | ✅ Resolved | All 4 changes made |
| NC3 | Drum knobs "Hidden" vs "Removed" | ✅ Resolved | Both specs now say "Removed" |
| R1 | Master limiter latency | ✅ Resolved | Limiter removed entirely (see NR1 for safety note) |
| R2 | FLAC encoding thread note | ✅ Resolved | Note added to §2.2.I |
| R3 | AbstractFifo capacity note | ✅ Resolved | Note added to §2.2.C |
| R4 | Production resource serving | ⚠️ Partial | Strategy mentioned but not committed. See PR1. |
| R5 | DELETE_JAM on active session | ⚠️ Not addressed | See §5 above |
| M1 | SET_PAN in §3.9 | ❌ Unresolved | See U2 |
| M2 | CivetWeb version pin | ✅ Resolved | "Pin to v1.16" added |
| M3 | Garbled text §7.6.5 | ❌ Unresolved | See U3 |
| M4 | Keymasher button count | ✅ Consistent | 12 buttons confirmed on both sides |

---

## 7. JUCE API Validation Summary

### WebBrowserComponent (Architecture Decision Validation)

The JUCE 8 `WebBrowserComponent` provides:
- **`NativeFunction`** — JS→C++ function call (returns Promise)
- **`emitEventIfBrowserIsVisible()`** — C++ → JS events ⚠️ *Only works when visible*
- **`ResourceProvider`** — Serves bundled content without HTTP server
- **`getResourceProviderRoot()`** — Returns platform-specific root URL

The spec's choice of CivetWeb+WebSocket over the native bridge is **validated** by:
1. `emitEventIfBrowserIsVisible()` only works while the WebView is visible — unreliable for audio state
2. Native bridge cannot serve remote clients (V2 requirement)
3. Binary WebSocket frames for 30fps visualization cannot be efficiently sent via `emitEvent` (would need Base64 encoding)

### AudioProcessor (Bus Configuration Validation)

JUCE's `AudioProcessor` supports multi-bus configuration via `BusesProperties` in the constructor. The spec's approach of using `#if JucePlugin_Build_VST3` preprocessor guards (line 118) to configure 8 stereo output buses (VST3) vs. 1 stereo bus (Standalone) is the correct JUCE pattern.

**Validated:** `setLatencySamples()` is available (confirmed in API). Since no master limiter is used, latency = 0 in both modes. No conditional latency reporting needed.

### AbstractFifo (CommandQueue Validation)

Confirmed: `juce::AbstractFifo` is in `juce_core/containers`. The spec correctly notes the `bufferSize - 1` usable items property (line 128). Usage for SPSC lock-free command passing is the standard JUCE pattern.

---

## 8. Build Confidence Assessment

| Area | Confidence | Change from Assessment 12 |
|:---|:---|:---|
| **Command Schema** | 🟢 High | ⬆ PIN auth and clock source fully cleaned up |
| **Audio Engine Architecture** | 🟢 High | No change — solid |
| **State Management** | 🟢 High | No change — comprehensive |
| **UI Layout** | 🟢 High | ⬆ FX Mode knob mapping resolved, note names conflict resolved |
| **Communication Layer** | 🟢 High | ⬆ Architecture decision documented with clear rationale |
| **Error Handling** | 🟢 High | Minor: GENERATE_SUPPORT_BUNDLE missing from matrix |
| **Build System / Phasing** | 🟢 High | No change — well-sequenced |
| **DSP / Effects** | 🟢 High | Minor: Audio Engine Spec header count |
| **Cross-Document Consistency** | 🟡 Medium-High | 3 minor text defects remain |

---

## 9. Recommended Actions Before Bead Creation

### Priority 1 — Should Fix (prevents agent confusion)

- [ ] **PR1:** Commit to CivetWeb static file serving for production. Remove the "or via `ResourceProvider`" hedging from §2.2.L line 218.
- [ ] **R5:** Add DELETE_JAM active-session handling to §3.2 (auto-create new session first).
- [ ] **NC4:** Update Audio Engine Spec §1.2 header from "(11 Effects)" to "(12 Effects)".

### Priority 2 — Minor Text Cleanup

- [ ] **U1:** Move shutdown note outside the code block in §2.4.
- [ ] **U2:** Remove `SET_PAN` from §3.9 Optimistic UI Pattern.
- [ ] **U3:** Fix garbled text at §7.6.5 line 1193.
- [ ] **M5:** Add `GENERATE_SUPPORT_BUNDLE` to §3.3 Error Matrix.

### Priority 3 — Design Decision (Optional)

- [ ] **NR1:** Decide on output clipping protection strategy (accept risk / soft clipper / auto-gain). If accepting the risk, consider adding a "CLIP" visual indicator to the header bar.

---

## 10. Final Verdict

**The spec is ready for bead creation.** The Priority 1 items above are recommended but not blocking — they are unlikely to cause agent stalls or incorrect implementations by themselves. The spec is comprehensive, internally consistent (with the noted minor exceptions), and well-structured for agent consumption.

The phased build plan (§8.6), human checkpoints (§8.8), and testing strategy (§8.7) provide strong guardrails against integration failures. The architecture is sound and the JUCE API choices have been validated.

**Remaining effort to reach "zero open issues":** ~30 minutes of targeted text edits across 3 files.
