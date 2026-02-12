# FlowZone Spec v1.4 → v1.5: Change Analysis & Rationale

This document provides a detailed analysis and justification for every change proposed in the v1.5 revision. Changes are grouped into the five review dimensions.

---

## 1. Architecture (6 Changes)

### 1.1 — Introduce an explicit `CommandDispatcher` as a separate, named component

**Problem in v1.4:** The spec names `CommandDispatcher` in section 2.2 but gives it only one line of description ("Routes incoming commands to appropriate handlers. Lock-free SPSC FIFO."). This conflates two distinct responsibilities — the *queue* (a data structure) and the *dispatcher* (a routing / validation layer).

**Proposed change:** Split into two clearly scoped components:
- **`CommandQueue`** — The lock-free SPSC FIFO. Its only job is to move bytes from the WebSocket thread to the audio thread without locking.
- **`CommandDispatcher`** — Reads from the queue on the audio thread, validates command payloads, and routes to the correct handler (`FlowEngine`, `TransportService`, `PluginProcessManager`). This is where unknown-command rejection, rate-limiting, and validation logic live.

**Rationale:** Agents implementing this will otherwise either duplicate validation in every handler or forget it entirely. Making the dispatcher a named class with an explicit contract ("validate, then route") is clearer and more testable.

---

### 1.2 — Add a `SessionStateManager` as a first-class component in the architecture diagram

**Problem in v1.4:** `SessionStateManager` is mentioned only in section 4.3 (Undo/Redo) but doesn't appear in the architecture diagram or the component list. An agent reading the diagram gets no indication that session persistence, undo, and redo are distinct responsibilities.

**Proposed change:** Add `SessionStateManager` to the component diagram and section 2.2 with explicit responsibilities: undo/redo stack management, auto-save, crash-recovery snapshot writing, and session loading.

**Rationale:** If it's important enough to own the undo stack and guard against data loss, it's important enough to appear in the architecture section, not buried in the file system section.

---

### 1.3 — Disambiguate the StateBroadcaster's diff protocol

**Problem in v1.4:** The spec says "JSON Patches (RFC 6902 style)" and "Full Snapshots on connection/re-sync" — but doesn't specify: How big is "small"? Who decides when a patch vs. a full snapshot is sent? What is the ceiling on patch size before it's cheaper to send a snapshot?

**Proposed change:** Specify a concrete threshold: "If the diff contains more than **20 operations** or exceeds **4 KB**, send a full snapshot instead of a patch. The `StateBroadcaster` makes this decision, not the client."

**Rationale:** Without a threshold, each agent or developer invents their own heuristic, leading to bugs where the client receives enormous patches that are slower to apply than a full snapshot. A specific, tunable number makes the behavior deterministic and debuggable.

---

### 1.4 — Add `CrashGuard` as its own component

**Problem in v1.4:** Boot-loop detection ("3 crashes < 60s → Safe Mode") is mentioned in both Goals (1.1) and the failure table (4.2) but is not assigned to any component. Where does this logic live? Who counts the crashes? Where is the crash counter persisted?

**Proposed change:** Introduce `CrashGuard` as a startup-time component:
- On launch, reads a crash counter from `~/Library/Application Support/FlowZone/.crash_sentinel`.
- Increments it immediately, and clears it after 60s of stable runtime.
- If counter ≥ 3 on launch, enters `SAFE_MODE`.
- In `SAFE_MODE`, the engine boots with: all VSTs disabled, default audio device, no auto-load of last session.

**Rationale:** Without a named component and file-based sentinel, agents will either implement this inline in `main()` (making it impossible to unit-test) or forget it entirely. The sentinel file pattern is proven (Chrome, Firefox, and most DAWs use it).

---

### 1.5 — Add connection lifecycle to the React frontend section

**Problem in v1.4:** The frontend section says "Connection Lost → UI shows 'Reconnecting...' overlay. Audio continues." and "Reconnection → UI requests `GET_FULL_STATE` to sync." But there's no mention of: initial connection handshake, authentication (even a simple token to prevent LAN neighbors from accidentally controlling your session), or exponential backoff on reconnect.

**Proposed change:** Define a connection lifecycle:
1. **Connect:** Client →  `WS_CONNECT` with `clientId` (UUID, generated once per browser tab).
2. **Handshake:** Server → sends `STATE_FULL` with current `revisionId`.
3. **Steady state:** Server → sends `STATE_PATCH` ops. Client sends commands.
4. **Disconnect:** Client shows "Reconnecting…" overlay; exponential backoff (100ms, 200ms, 400ms, …, max 5s).
5. **Reconnect:** Client → `WS_RECONNECT` with last known `revisionId`. Server sends diff if possible, or full snapshot if revision is stale.
6. **Optional auth:** If `config.json` has `"requirePin": true`, the client must send `{ cmd: 'AUTH', pin: '…' }` before any other command is accepted. This protects shared-network scenarios.

**Rationale:** Without these details, each developer implements a different reconnection strategy, and there is no protection against accidental LAN access — a real usability risk when performing.

---

### 1.6 — Remove Ableton Link from Goals (was in v1.3, dropped in v1.4 without explanation)

**Problem in v1.4:** v1.0 and v1.3 both listed "Global Phase-Sync: Ableton Link integration" as a goal. v1.4 removed it entirely without replacing it or noting it as a non-goal. An agent reading only v1.4 doesn't know whether Link support was intentionally cut or accidentally lost.

**Proposed change:** Restore "Ableton Link" as a V2 stretch goal in a new **1.3 Future Goals** section. This preserves the intent while making it clear it's not in scope for V1.

**Rationale:** Spec drift across versions creates confusion. Any feature that was once a goal and is now cut should be explicitly documented as cut (and why).

---

## 2. Reliability & Failure Handling (4 Changes)

### 2.1 — Fix the DiskWriter overflow strategy (resolve v1.3 ↔ v1.4 contradiction)

**Problem:** v1.3 says `DiskWriter` **drops packets** when the ring buffer fills. v1.4 changed to a RAM overflow strategy (256MB ring → 1GB overflow → hard stop). Neither version addresses what happens to the *recording session* after a disk-critical event — is the partial recording salvageable?

**Proposed change:** Adopt v1.4's tiered strategy but add:
- Tier 4: On `ERR_DISK_CRITICAL`, the `DiskWriter` flushes whatever remains in the ring buffer and overflow to a file named `*_PARTIAL.flac`, then stops recording. The UI receives a specific `ERR_DISK_CRITICAL` with `partialFilePath` so the user knows where their partial recording is.
- Clarify: Audio playback of already-loaded loops continues unaffected. Only *new recording* stops.

**Rationale:** "Never silently drop samples" is the right principle, but the spec needs to say what happens to the user's work when the worst case hits. A partial file is infinitely better than nothing.

---

### 2.2 — Define maximum retry behavior for plugin respawning

**Problem in v1.4:** "Max 3 retries per session" — per session is too generous. If a plugin crashes immediately on load, the watchdog will respawn it 3 times in rapid succession, causing CPU spikes, IPC churn, and a poor user experience each time.

**Proposed change:** Implement exponential backoff for plugin respawning:
- Retry 1: Immediate.
- Retry 2: After 2 seconds.
- Retry 3: After 10 seconds.
- After retry 3: Mark manufacturer as "Suspended" for this session. User can manually re-enable from the settings panel.
- The "Suspended" state (not "Blacklisted" — less alarming terminology) is shown in the plugin UI panel with a "Retry" button.

**Rationale:** Without backoff, rapid respawning under repeated crashes causes cascading resource contention. Backoff gives transient issues (e.g., a license server being slow) time to resolve.

---

### 2.3 — Add heartbeat timeout justification and tune the value

**Problem in v1.4:** The watchdog pings every 250ms with a hang-detection timeout of >1s. These are aggressive numbers that aren't justified. Why 250ms? Why 1s? On a loaded system, a legitimate audio render might take longer than expected — is a 1s timeout a false positive risk?

**Proposed change:**
- Watchdog ping interval: **500ms** (reduces IPC overhead by 2×, still well within human perceptibility).
- Hang detection: **3 consecutive missed heartbeats (= 1.5s)**. This avoids false positives from a single slow render pass.
- Document the reasoning: "At 48kHz / 512 buffer, a single audio callback is ~10.7ms. A 1.5s timeout allows for ~140 callback periods — far beyond any legitimate processing delay."

**Rationale:** Over-aggressive watchdog timers are a common source of false-positive kills in plugin hosts. The spec should justify its timing choices so agents don't arbitrarily change them.

---

### 2.4 — Add config file validation and migration

**Problem in v1.4:** The failure table mentions "Config Corrupt → Load backup." But there's no mention of config *migration*. When the app updates from v1 to v1.1, what happens to `config.json`? If the schema changes, loading an old config silently could cause undefined behavior.

**Proposed change:** Add a `configVersion` field to `config.json`. On load:
1. Parse JSON. If parse fails → load backup.
2. Check `configVersion`. If older than current → run migration function.
3. If migration fails → load defaults, archive old config as `config.legacy.json`.

**Rationale:** Config migration is one of the most common sources of post-update crashes in desktop apps. Specifying it now prevents a class of bugs that only surface in the field.

---

## 3. Performance & Scalability (3 Changes)

### 3.1 — Specify visualization backpressure with clearer semantics

**Problem in v1.4:** The binary visualization stream section introduces `frames_in_flight` and says "If > 5, server drops frame." But the ACK protocol says "Client sends ACK after every 30 frames." This means the client can be up to *30 frames behind* before an ACK is sent — but the server will start dropping at 5. The numbers don't align.

**Proposed change:** Simplify to a single mechanism:
- Server maintains a per-client `pendingFrameCount` (incremented on send, decremented on ACK).
- Client sends ACK after *every* frame (minimal 4-byte message, negligible overhead).
- If `pendingFrameCount > 3`, server skips the next frame.
- This naturally adapts: fast clients get 30fps; slow clients (phone on WiFi) gracefully degrade to whatever they can handle.

**Rationale:** The "ACK every 30 frames" design creates a large window where the server has no feedback. Per-frame ACK is cheaper than sending a speculative frame that will be dropped anyway.

---

### 3.2 — Specify adaptive degradation thresholds concretely

**Problem in v1.4:** "If Audio Load > 70%, visualization frame rate drops" — but drops *to what*? Is it linear? Stepped? What does "Audio Load" mean precisely (DSP time / buffer time)?

**Proposed change:** Define stepped degradation:

| CPU Load (DSP/Buffer %) | Visualization FPS | Feature Stream |
|:---|:---|:---|
| < 60% | 30 fps | Full (RMS + 16-band spectrum) |
| 60–75% | 15 fps | Reduced (RMS only) |
| 75–90% | 5 fps | RMS only |
| > 90% | 0 fps (paused) | Disabled; UI shows "HIGH LOAD" badge |

- **CPU Load** is defined as: `(DSP processing time per callback) / (audio buffer duration) × 100`.

**Rationale:** Agents can't implement "drops automatically" without knowing the target. Stepped thresholds are simpler to implement and test than continuous curves.

---

### 3.3 — Add SharedMemory ring buffer sizing guidance

**Problem in v1.4:** The IPC layer uses "Shared Memory RingBuffers" but doesn't specify their size or what happens when they fill (on either the engine side or the plugin host side).

**Proposed change:** Specify:
- Ring buffer size = **4 × audio buffer size** (e.g., for 512 samples at 48kHz stereo float32: `4 × 512 × 2 × 4 = 16,384 bytes`).
- If the ring buffer fills (writer catches up to reader), the **writer drops the frame** and increments a `dropCount` metric. The engine treats this as a soft error (no crash, no bypass — just an audio gap).
- A `dropCount > 10 per second` triggers `ERR_PLUGIN_OVERLOAD` and is surfaced in the UI.

**Rationale:** Without sizing guidance, an agent might allocate a single-frame buffer (unbounded glitching) or a 100MB buffer (wasted memory). The 4× heuristic is standard practice in real-time audio (JACK, PortAudio).

---

## 4. Clarity & Implementability for Coding Agents (3 Changes)

### 4.1 — Add `version` field to AppState and command schema version header

**Problem in v1.4:** The `AppState` has `meta.version` (a string), but there's no protocol version in the command schema. If the C++ engine is updated before the React client, commands may be misinterpreted.

**Proposed change:**
- Add `protocolVersion: number` to the `meta` block in `AppState`.
- Add `protocolVersion: number` as a required field on the WebSocket handshake.
- If client and server protocol versions don't match, server responds with `ERROR { code: 1100, msg: 'PROTOCOL_MISMATCH', expected: N, received: M }` and refuses commands.

**Rationale:** Protocol versioning is the single most important guard against subtle desync bugs in client-server apps — and the easiest to forget when iterating fast.

---

### 4.2 — Expand the Agent Implementation Guide with concrete file paths and test expectations

**Problem in v1.4:** The guide says "Update `src/shared/protocol/schema.ts`" and "Add handler in `FlowEngine::handleCommand()`" — but doesn't specify: Where is `handleCommand`? What does a handler's signature look like? How do you test it?

**Proposed change:** Expand the guide into a step-by-step recipe:

1. **Schema (TypeScript):** `src/shared/protocol/schema.ts` — add new variant to the `Command` union type.
2. **Schema (C++):** `src/shared/protocol/commands.h` — add corresponding `enum CommandType` value and `struct` fields. *(This file is the C++ mirror of the TS schema.)*
3. **Dispatcher:** `src/engine/CommandDispatcher.cpp` → `dispatch(const Command&)` — add `case` for the new command type, calling the appropriate handler.
4. **Handler:** `src/engine/FlowEngine.cpp` → add `void handleMyCommand(const MyCommandArgs&)` method.
5. **State update:** Modify `AppState` in both `schema.ts` and the C++ counterpart if the command changes observable state.
6. **UI:** `src/web_client/src/hooks/useCommands.ts` — add typed dispatch helper. Update the relevant React component.
7. **Test:** Add a unit test in `tests/engine/test_commands.cpp` that pushes the command through the `CommandDispatcher` and asserts the state change.

**Rationale:** Agents perform best with file-level specificity. Vague instructions like "update the state" lead to scattered implementations.

---

### 4.3 — Add explicit error response contract for every command

**Problem in v1.4:** The `ServerMessage` union includes `ERROR` with a code and message, but there's no spec for *which commands can produce which errors*. An agent implementing `LOAD_VST` doesn't know whether to handle `ERR_PLUGIN_NOT_FOUND`, `ERR_PLUGIN_CRASH`, or both.

**Proposed change:** Add an error matrix:

| Command | Possible Errors | Client Behavior |
|:---|:---|:---|
| `SET_VOL` | None (always succeeds) | Optimistic update; reconcile on next state patch. |
| `TRIGGER_SLOT` | `2010: SLOT_BUSY` | Show toast "Slot busy, try again." |
| `LOAD_VST` | `3001: PLUGIN_CRASH`, `3010: PLUGIN_NOT_FOUND` | Show error in slot UI. |
| `SET_TEMPO` | `2020: TEMPO_OUT_OF_RANGE` | Clamp UI slider to valid range. |
| `UNDO` | `4001: NOTHING_TO_UNDO` | Disable undo button. |
| `PANIC` | None (always succeeds) | Full UI reset animation. |

**Rationale:** Without this, agents either ignore errors or invent their own error codes, leading to inconsistency between the C++ and React sides.

---

## 5. Operational Robustness (2 Changes)

### 5.1 — Add structured log format specification

**Problem in v1.4:** "Structured JSON logs" is mentioned but the log format is never defined. Agents will invent their own key names, making logs unparseable by any common tool.

**Proposed change:** Define a mandatory log schema:

```json
{
  "ts": "2026-02-12T10:30:00.000Z",
  "level": "INFO|WARN|ERROR|FATAL",
  "component": "FlowEngine|DiskWriter|PluginHost|WebServer|CrashGuard",
  "event": "string_event_id",
  "msg": "Human-readable message",
  "data": { /* arbitrary structured payload */ }
}
```

- Logs are written to `~/Library/Logs/FlowZone/engine.jsonl` (one JSON object per line — JSONL format, not a JSON array).
- Log rotation: Max 10MB per file, keep last 5 files.

**Rationale:** JSONL is grep-friendly, tail-friendly, and ingestible by any log aggregator. Without a schema, "structured logs" is an aspiration, not a spec.

---

### 5.2 — Add health-check endpoint for external monitoring

**Problem in v1.4:** The telemetry section describes a developer overlay inside the app, but there's no way to monitor the app externally (e.g., from a shell script, a status bar widget, or a remote device).

**Proposed change:** Add a simple HTTP health endpoint:
- `GET /api/health` → returns JSON:
```json
{
  "status": "ok|degraded|critical",
  "uptime_s": 3600,
  "cpu_load": 0.45,
  "disk_buffer_pct": 0.12,
  "active_plugin_hosts": 3,
  "safe_mode": false,
  "version": "1.0.0"
}
```
- Status logic: `ok` = normal; `degraded` = CPU > 75% or disk buffer > 80%; `critical` = safe mode or ERR_DISK_CRITICAL active.

**Rationale:** External observability is trivial to implement (CivetWeb already handles HTTP) and enables scripting, dashboards, and automated alerting — all critical for a live performance tool.

---

## Summary of Changes

| # | Dimension | Change | Impact |
|:---|:---|:---|:---|
| 1.1 | Architecture | Split CommandQueue / CommandDispatcher | Testability, clarity |
| 1.2 | Architecture | Promote SessionStateManager to diagram | Discoverability |
| 1.3 | Architecture | Specify patch-vs-snapshot threshold | Determinism |
| 1.4 | Architecture | Add CrashGuard component | Reliability |
| 1.5 | Architecture | Define connection lifecycle | Completeness |
| 1.6 | Architecture | Restore Ableton Link as future goal | Spec hygiene |
| 2.1 | Reliability | Fix DiskWriter overflow → partial save | Data safety |
| 2.2 | Reliability | Exponential backoff for plugin respawn | Stability |
| 2.3 | Reliability | Tune watchdog timing with justification | Fewer false positives |
| 2.4 | Reliability | Config versioning & migration | Update safety |
| 3.1 | Performance | Fix visualization backpressure protocol | Correctness |
| 3.2 | Performance | Define adaptive degradation thresholds | Implementability |
| 3.3 | Performance | Specify IPC ring buffer sizing | Resource control |
| 4.1 | Clarity | Protocol version mismatch detection | Desync prevention |
| 4.2 | Clarity | Expand agent guide with file paths | Agent productivity |
| 4.3 | Clarity | Error matrix per command | Consistency |
| 5.1 | Ops | Structured log schema (JSONL) | Parsability |
| 5.2 | Ops | HTTP health endpoint | External monitoring |
