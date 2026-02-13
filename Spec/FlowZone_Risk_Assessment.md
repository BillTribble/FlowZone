# FlowZone — Agentic Build-Out Risk Assessment

**Purpose:** Identify and categorize key risks that will cause problems during the agentic build-out of FlowZone, so that beads can be structured to mitigate them. This document is organized by risk severity and includes concrete mitigation advice for bead planning.

---

## Executive Summary

FlowZone combines **five high-complexity domains** that rarely appear together in a single project:

1. **Real-time audio DSP** (lock-free, zero-allocation audio thread)
2. **IPC process isolation** (shared memory ring buffers, child process management)
3. **Embedded web UI** (React inside a JUCE WebBrowserComponent)
4. **Multi-client WebSocket state sync** (diff/patch protocol)
5. **Procedural audio synthesis** (23+ effects, 24+ synth presets, drum engine)

An AI agent can write each of these individually. The risk is in the **interfaces between them** — where C++ meets TypeScript, where the audio thread meets the message thread, where shared memory meets child processes. A bead that doesn't specify which side of a boundary it owns will produce code that compiles but doesn't integrate.

---

## 🔴 CRITICAL RISKS (Will Almost Certainly Cause Problems)

### RISK 1: Dual-Language Schema Sync Drift

**The problem:** The spec defines a single source of truth in two files — `schema.ts` and `commands.h`. Every command, every state field, every error code must be manually mirrored. An agent implementing a feature will naturally write one side and forget (or subtly mismatch) the other.

**Why it's hard for agents:** An agent working on a C++ bead has no way to validate its work against the TypeScript schema unless the bead explicitly tells it to. The code will compile on both sides independently but fail at runtime when the WebSocket message doesn't deserialize.

**Mitigation for bead planning:**
- Every bead that touches commands or state must explicitly name **both** files (`schema.ts` AND `commands.h`) in its scope, even if the bead is "C++ only."
- Create an early "Schema Foundation" bead that establishes the full `Command` union and `AppState` interface in both languages — with a verification step that counts type members on each side.
- Consider a code-generation approach: write schema in one language and generate the other. This is extra work but eliminates the class of bugs entirely.

---

### RISK 2: Audio Thread Safety Violations

**The problem:** The spec's most sacred rule is "never allocate memory, lock mutexes, or do I/O on the audio thread." But `FlowEngine.processBlock()` must read from `CommandQueue`, write to `DiskWriter`'s ring buffer, update state for `StateBroadcaster`, and interact with IPC shared memory — all lock-free. An agent that adds a `std::string`, a `juce::String`, a `std::vector::push_back`, or even a `DBG()` macro inside `processBlock` will introduce real-time violations that don't crash **during development** but cause audio glitches under load.

**Why it's hard for agents:** Lock-free programming requires understanding what operations allocate. `std::function` allocates. `juce::String` copy allocates. Even `juce::Logger::writeToLog` can block. No compiler error warns you. The agent sees working code and moves on.

**Mitigation for bead planning:**
- Create an explicit "Audio Thread Contract" bead early that documents exactly which functions are called on the audio thread and which types/operations are forbidden.
- Every bead targeting `src/engine/` that touches the audio callback path should include a verification step: "Confirm no heap allocation, no mutex, no I/O in the processBlock path."
- Consider adding a `JUCE_ASSERT_MESSAGE_THREAD` / custom audio-thread assertions to catch violations at debug time. Make this part of the foundation bead.

---

### RISK 3: IPC Shared Memory + Child Process Lifecycle Will Be Hard to Get Right Incrementally

**The problem:** The plugin isolation architecture (§2.2.J-K) requires:
1. A `SharedMemoryManager` that creates ring buffers
2. A `PluginProcessManager` that spawns child processes per manufacturer
3. A separate `PluginHostApp` binary in `src/host/`
4. A heartbeat/watchdog protocol with exponential-backoff respawning
5. Audio data flowing through shared memory between processes

This is an **IPC system with its own build target.** An agent can't build it incrementally the way it can build a UI component. The child process must exist, link against JUCE, compile as a separate executable, and have shared memory mapped at the same addresses — or the entire system is untestable.

**Why it's hard for agents:** An agent given a bead like "Implement PluginProcessManager" will write the manager code but won't know how to set up the second build target, configure CMake for two executables, or test without a runnable host process. The bead will appear "done" but won't work.

**Mitigation for bead planning:**
- **Defer plugin isolation to a later phase.** Get the engine working with internal instruments first. This removes the hardest integration risk from the critical path.
- When you do build IPC, structure it as a **vertical slice**: one bead that creates the host binary, configures the CMake target, establishes shared memory, and successfully passes one audio buffer round-trip. Only then fan out to watchdog/respawn/hot-swap beads.
- The spec's Phase 4 (Tasks 4.1-4.2) is already at the right position, but the bead for it needs to be large and self-contained — not split across multiple agents.

---

### RISK 4: Build System Complexity (CMake + Projucer + Vite)

**The problem:** The spec lists Projucer for project config (§ agents.md) but the directory structure (§4.1) shows `CMakeLists.txt`. The React web client uses Vite. That's **three build systems** in one repo — and an agent needs to know which one to modify for any given change.

**Additionally:** The spec references a `FlowZone.jucer` file AND CMake. These conflict — Projucer generates Xcode projects, CMake also generates Xcode projects. The spec hasn't resolved which is authoritative.

**Why it's hard for agents:** An agent that adds a `.cpp` file to `src/engine/` needs to know: "Do I add it to CMakeLists.txt? To the .jucer file? Both?" If a bead doesn't specify this, the agent will guess wrong, and the project won't compile when the next agent pulls.

**Mitigation for bead planning:**
- Resolve the Projucer vs CMake question **before creating any beads.** Pick one. The spec says Projucer; the directory layout suggests CMake. Make a decision and update the spec.
- The very first bead should be "Project Skeleton" — it should produce a compiling, running JUCE standalone app with nothing but an empty `processBlock` and the web view loading a "Hello World" React page. This validates the entire build chain end-to-end.
- Every bead that adds C++ source files must state: "Add to [build system] as a source file."

---

## 🟠 HIGH RISKS (Likely To Cause Significant Friction)

### RISK 5: WebSocket Protocol Is Specified But Not Incrementally Testable

**The problem:** The StateBroadcaster (§2.2.E) uses JSON Patch (RFC 6902) for incremental state updates, with complex rules about when to send patches vs. snapshots. The React client must apply patches correctly, handle reconnection with stale `revisionId`, and negotiate protocol versions. This is a distributed systems protocol that requires both sides running simultaneously to test.

**Mitigation:**
- Create a "Protocol Stub" bead that implements the WebSocket server sending hardcoded `STATE_FULL` messages, and the React client receiving and rendering them. No diffs, no patches — just full state snapshots. This gets both sides talking.
- Add patches as a second bead, with the stub still available as a fallback.
- Include a "Protocol Conformance Test" bead that sends canned patch sequences to the React client and validates the resulting state.

---

### RISK 6: 23 Audio Effects + 24 Synth Presets = Massive Surface Area

**The problem:** The Audio Engine Spec defines 12 core FX, 11 infinite FX, 12 notes presets, 12 bass presets, 16 drum sounds across 4 kits, plus a sample engine. That's **~75 distinct audio processing implementations.** Each one is small individually, but an agent doing them in sequence will accumulate subtle bugs (wrong parameter ranges, inverted XY mappings, missing frequency clamping).

**Mitigation:**
- Group effects/presets into beads by **implementation similarity**, not by UI category. E.g., "All filter-based effects" (Lowpass, Highpass, Comb, Multicomb), "All delay-based effects" (Delay, Zap Delay, Dub Delay), "All noise-based drums" (hihats, cymbals, snares).
- Each bead should include a **parameter range validation test** — feed min, max, and mid values to every parameter and confirm no NaN, no infinity, no denormals.
- Accept that V1 effects will be simple. The spec already says "simple procedural implementations for V1." Don't let an agent over-engineer.

---

### RISK 7: Mobile-First UI With Complex Interaction Patterns Is Hard to Verify

**The problem:** The mobile layout spec (§7.6) defines touch-and-hold XY pads, riff history layer cakes, waveform timeline sections with tap-to-loop, circular faders with arc indicators, and multiple grid configurations. These are custom UI components that can't be composed from standard React components — they need Canvas rendering and careful touch event handling.

**Mitigation:**
- Structure UI beads as **component-level** (one bead per major component: XY Pad, Circular Fader, Pad Grid, Riff History Indicator, Waveform Timeline).
- Each bead should produce a **standalone storybook-style demo** that works without the engine backend (using mock state).
- Defer "touch-and-hold" interactions to a polish bead. Get tap-only working first.
- The responsive breakpoint system (§7.6.10) should be a single foundational bead that establishes the `ResponsiveContainer` and navigation shell before any view-specific beads.

---

### RISK 8: Multi-Agent Coordination on Shared Files

**The problem:** `agents.md` sets up Agent Mail for file reservations, but certain files are natural bottlenecks:
- `schema.ts` / `commands.h` (every feature touches these)
- `FlowEngine.cpp` (central audio graph)
- `CommandDispatcher.cpp` (routes all commands)
- `AppState` interface (both languages)

If 3 agents work simultaneously, they will inevitably conflict on these files.

**Mitigation:**
- **Serialize schema changes.** Don't have two agents adding commands at the same time. Schema beads should be completed and pushed before dependent feature beads start.
- Structure `CommandDispatcher` so each command handler is in its **own file** (`handleMuteSlot.cpp`, `handleSetVol.cpp`). The dispatcher just calls them. This eliminates merge conflicts.
- `FlowEngine` should delegate to sub-managers (already implied by `TransportService`, `SessionStateManager`). Make this delegation explicit so agents add code to sub-managers, not to FlowEngine itself.

---

## 🟡 MEDIUM RISKS (Will Cause Delays If Ignored)

### RISK 9: CrashGuard + Safe Mode Is a Rabbit Hole

**The problem:** The graduated safe mode (§2.2.F) with 3 levels, a sentinel file, boot loop detection, and a recovery UI is a self-contained subsystem. It's important for production but irrelevant to getting the core loop working.

**Mitigation:** Make CrashGuard the **last** Phase 1 bead, not the first. The spec's startup sequence puts it first, but in development, you need the engine running before you can test crash recovery. Create the sentinel read/write as a stub and fill in the graduated logic later.

---

### RISK 10: DiskWriter Tiered Failure Strategy Requires Real Disk I/O Testing

**The problem:** The 4-tier failure strategy (§4.2) includes a 256MB ring buffer, 1GB overflow, and partial FLAC file flushing. You can't unit-test this meaningfully — you need to actually saturate a disk or simulate slow I/O.

**Mitigation:** Implement DiskWriter with Tier 1 (normal writes) only for the first bead. Add the overflow/partial-save tiers as a separate bead with an explicit "simulate slow disk" test.

---

### RISK 11: Feature Visualization Stream Is a Separate Binary Protocol

**The problem:** The binary visualization stream (§3.7) uses a custom binary format over a separate WebSocket channel with per-client backpressure (ACK-based flow control). This is a fully separate protocol from the JSON command channel.

**Mitigation:** This is optional for MVP. Get the JSON state sync working first. Add the binary visualization stream as a Phase 2 enhancement. If the audio engine works and the UI renders state from JSON, you have a usable product. Waveforms and VU meters can be deferred.

---

### RISK 12: Microtuning Support Is a Niche Feature With Hidden Complexity

**The problem:** `.scl` and `.kbm` file parsing, MTS-ESP integration, and non-12TET frequency mapping add significant complexity to the synth engine for a feature most users won't use immediately.

**Mitigation:** Implement all synths in 12TET first. Add a "Microtuning Support" bead as a Phase 3 task that modifies the frequency lookup table. Don't let it block the synth beads.

---

## 🟢 LOWER RISKS (Worth Noting for Planning)

### RISK 13: Emoji Skin Tone Preference + Random Emoji Assignment
Small but easily forgotten. Make it a chore bead at the end.

### RISK 14: Log Rotation + JSONL Structured Logging
Well-defined in the spec but easy to skip during active development. Wrap it into a "Production Polish" epic.

### RISK 15: HTTP Health Endpoint
Simple to implement (`GET /api/health`), but agents may forget to wire up the live metric values. Bundle with telemetry bead.

---

## Recommended Bead Ordering Strategy

Based on the risks above, here is a suggested ordering that minimizes blocked agents and integration failures:

```
PHASE 0: SKELETON (one bead, run first, blocks everything)
├── CMake/Projucer decision finalized
├── Empty JUCE app compiles → launches → opens WebBrowserComponent
├── Empty React app loads inside WebBrowserComponent
├── WebSocket handshake succeeds (hardcoded "hello")
└── Both build systems verified (C++ and Vite)

PHASE 1: CONTRACTS (serial beads, must complete before fanout)
├── schema.ts + commands.h — full type definitions
├── ErrorCodes.h — all error codes registered
├── Audio Thread Contract doc — forbidden operations list
└── AppState round-trip test — C++ serializes, TS deserializes, values match

PHASE 2: ENGINE CORE (can parallelize after contracts)
├── TransportService (independent, testable)
├── CommandQueue + CommandDispatcher
├── FlowEngine skeleton (empty processBlock, wired to dispatcher)
├── StateBroadcaster (full snapshot only, no patches yet)
└── DiskWriter (Tier 1 only)

PHASE 3: UI SHELL (can parallelize with Phase 2)
├── WebSocket client with reconnection
├── ResponsiveContainer + navigation shell
├── Mock state provider (for UI development without engine)
└── Individual UI components (XY Pad, Pad Grid, Faders, etc.)

PHASE 4: AUDIO ENGINES (after Phase 2 engine core works)
├── Filter-based effects (Lowpass, Highpass, Comb, Multicomb)
├── Delay-based effects
├── Distortion/saturation effects
├── Synth presets (Notes, Bass)
├── Drum engine
└── Sample engine (basic playback)

PHASE 5: INTEGRATION (serial, combines engine + UI)
├── StateBroadcaster patches (RFC 6902)
├── Full command flow: UI → WS → CommandQueue → Dispatcher → Engine → State → UI
├── SessionStateManager (undo/redo, autosave)
└── Binary visualization stream

PHASE 6: PLUGIN ISOLATION (defer until core works)
├── PluginHostApp binary + CMake target
├── Shared memory ring buffers
├── Process lifecycle + watchdog
└── Hot-swap + exponential backoff

PHASE 7: POLISH
├── CrashGuard + Safe Mode
├── DiskWriter Tiers 2-4
├── Microtuning support
├── Log rotation + telemetry
├── Settings panel
└── Health endpoint
```

---

## Key Principle for the Agent Executing Beads

> **Every bead must produce something testable.** If a bead's output can only be verified by running it together with another bead that doesn't exist yet, the bead is too small or poorly scoped. Merge it with the bead it depends on, or add a stub/mock that makes it independently verifiable.

This is the single most effective way to prevent the "it compiles but doesn't work" failure mode that plagues agentic build-outs of complex systems.
