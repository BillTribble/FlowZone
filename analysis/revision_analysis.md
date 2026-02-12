# Revision Analysis: FlowZone Spec 1.3

This document outlines the detailed analysis and rationale for the proposed revisions to `Spec_FlowZone_Looper1.3.md`. The goal is to enhance architecture, reliability, performance, clarity, and operational robustness.

## 1. Architecture

### 1.1. Explicit IPC Mechanism
**Analysis:** The current spec mentions "IPC Shared Memory" but lacks implementation details.
**Proposal:** Define the IPC mechanism as **Lock-Free Ring Buffers over Shared Memory** (using `boost::interprocess` or similar native API). Separate queues for Audio (AudioBlock), MIDI/Events, and Parameters.
**Rationale:** Shared memory is the only viable option for low-latency audio passing between processes. Lock-free structures prevent priority inversion on the audio thread. Separating data types prevents large parameter dumps from blocking audio processing.

### 1.2. Transport Service Decoupling
**Analysis:** Transport logic (BPM, Phase) is currently implied to be part of the `FlowEngine`.
**Proposal:** Extract `TransportService` as a distinct component that `FlowEngine` consumes.
**Rationale:** Allows the transport to be tested in isolation and potentially synchronized with external sources (Link, MIDI Clock) without tangling audio processing logic.

### 1.3. WebServer Isolation
**Analysis:** `CivetWeb` is mentioned in the "Main Process".
**Proposal:** Explicitly state that the WebServer runs on a dedicated background thread, strictly decoupled from the Audio Thread.
**Rationale:** HTTP requests handling (even for localhost) is non-deterministic and must never block audio callbacks.

## 2. Reliability & Failure Handling

### 2.1. Enhanced Heartbeat Monitoring
**Analysis:** The 1s watchdog ping is too slow for real-time audio contexts. A 1s freeze is unacceptable.
**Proposal:** Implement a **High-Frequency Heartbeat (50-100ms)** for plugin hosts.
**Rationale:** Faster detection of hung processes allows the Engine to bypass the dead host faster, minimizing the duration of audio dropouts or silence.

### 2.2. "Safe Mode" Granularity
**Analysis:** "Safe Mode" disables everything.
**Proposal:** Introduce **Graduated Safe Mode**:
1.  *Disable Plugins Only*: If plugin crash detected.
2.  *Disable Audio Device*: If audio driver fails.
3.  *Reset Config*: If config corruption detected.
**Rationale:** A "nuclear option" (reset everything) is frustrating. Targeted disabling preserves as much functionality as possible (e.g., browsing library even if audio is broken).

### 2.3. State Reconciliation & Snapshots
**Analysis:** State diffing is mentioned, but "Rollbacks" are not.
**Proposal:** Implement **Automatic Config Snapshots** (last 3 valid configs). On boot loop detection, revert to `state.backup.json`.
**Rationale:** Prevents the app from becoming unusable due to a bad setting (e.g., selecting an invalid audio device sample rate).

## 3. Performance & Scalability

### 3.1. Thread Priority Specification
**Analysis:** "Audio Thread" is mentioned, but OS-level priorities are crucial.
**Proposal:** Explicitly assign:
*   `Audio Thread`: Realtime Priority (Critical).
*   `Disk Writer`: High Priority.
*   `Feature Extraction`: Low/Idle Priority.
*   `UI/Server`: Normal Priority.
**Rationale:** Ensures the OS scheduler (macOS QoS) respects the realtime constraints of audio processing over UI rendering.

### 3.2. Dynamic Visualization Throttling
**Analysis:** "20-30fps Best Effort" is good, but needs a feedback loop.
**Proposal:** Implement a **Pressure-Based Throttler**. If Audio Callback duration > 70% of buffer size, drop visualization frame rate immediately.
**Rationale:** The #1 priority is audio glitch-free playback. Visuals are a luxury that must be sacrificed under load.

## 4. Clarity & Implementability for Agents

### 4.1. Strict Versioning & Standards
**Analysis:** Generic "JUCE 8" and "React 18".
**Proposal:** Pin versions: `JUCE 8.0.x`, `React 18.3.1`, `TypeScript 5.x`, `Vite 5.x`. Use C++20 standard.
**Rationale:** Removes ambiguity for coding agents setting up the environment.

### 4.2. Standardized Naming Conventions
**Analysis:** Mixed naming styles implied.
**Proposal:** Enforce:
*   **C++:** `PascalCase` classes, `camelCase` methods, `m_member` variables.
*   **TS/JS:** `PascalCase` components, `camelCase` functions/vars.
*   **Files:** `Snake_case` or `PascalCase` consistently (Propose `PascalCase` for C++ files, `kebab-case` for TS files).
**Rationale:** Consistent naming reduces cognitive load and allows standard linting rules.

## 5. Operational Robustness

### 5.1. Structured Error Codes
**Analysis:** Log messages are strings.
**Proposal:** Define a `FlowError` enum and map to integer codes (e.g., `ERR_VST_CRASH = 1001`).
**Rationale:** Allows automated log analysis/alerting and cleaner UI error handling (translation keys).

### 5.2. Telemetry Schema
**Analysis:** JSON logs mentioned.
**Proposal:** Define the exact JSON log schema (Timestamp, Level, Component, Code, Message, Context).
**Rationale:** Ensures logs are machine-readable for debugging tools.
