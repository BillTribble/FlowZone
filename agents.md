# AGENTS.md — FlowZone Agentic Coding Guide v1.1

## RULE 0 — THE FUNDAMENTAL OVERRIDE PREROGATIVE

If I tell you to do something, even if it goes against what follows below, YOU MUST LISTEN TO ME. I AM IN CHARGE, NOT YOU.

---

## RULE 1 — ABSOLUTE (DO NOT EVER VIOLATE THIS)

You may NOT delete any file or directory unless I explicitly give the exact command **in this session**.

- This includes files you just created (tests, tmp files, scripts, etc.).
- You do not get to decide that something is "safe" to remove.
- If you think something should be removed, stop and ask. You must receive clear written approval **before** any deletion command is even proposed.

Treat "never delete files without permission" as a hard invariant.

---

## Irreversible Git & Filesystem Actions

Absolutely forbidden unless I give the **exact command and explicit approval** in the same message:

- `git reset --hard`
- `git clean -fd`
- `rm -rf`
- Any command that can delete or overwrite code/data

Rules:

1. If you are not 100% sure what a command will delete, do not propose or run it. Ask first.
2. Prefer safe tools: `git status`, `git diff`, `git stash`, copying to backups, etc.
3. After approval, restate the command verbatim, list what it will affect, and wait for confirmation.
4. When a destructive command is run, record in your response:
   - The exact user text authorizing it
   - The command run
   - When you ran it

If that audit trail is missing, then you must act as if the operation never happened.

---

## Project Context

**FlowZone** is a **macOS-first retrospective looping workstation** built as a native desktop app.

- **Framework:** JUCE 8.0.x
- **Language:** C++20
- **Project Config:** CMake (generates Xcode project in `build/`)
- **IDE:** Xcode (builds & debugging)
  - **Xcode.app location:** `/Applications/Xcode.app`
  - **xcodebuild:** `/Applications/Xcode.app/Contents/Developer/usr/bin/xcodebuild`
  - **Developer directory:** `/Applications/Xcode.app/Contents/Developer` (configured via `xcode-select`)
  - **Build command:** `cmake --build build --config Debug` (or `xcodebuild -project build/FlowZone.xcodeproj -scheme FlowZone -configuration Debug`)
- **Code Authoring:** Antigravity (AI-assisted coding → passed to Xcode for compilation)
- **Platform:** macOS (Apple Silicon Native), Standalone App
- **Frontend:** React 18.3.1 (TypeScript 5.x) — embedded via `juce::WebBrowserComponent`
- **Reference:** See `Spec_FlowZone_Looper1.6.md` for the full technical design
- **Log Files:** `~/FlowZone_Logs/` — separate files per category:
  - `startup.log` — App lifecycle (startup/shutdown)
  - `audio_flow.log` — Engine peak levels (sampled, audio thread safe)
  - `state_broadcast.log` — State broadcast to UI (sampled ~1/sec)
  - `ws_server.log` — WebSocket server events (connections, messages)

### Architecture Summary

| Component | Location | Stack |
|:---|:---|:---|
| **Audio Engine** | `src/engine/` | C++20, JUCE `AudioProcessor`, real-time thread |
| **Transport Service** | `src/engine/transport/` | C++20, Ableton Link |
| **Plugin Host (IPC)** | `src/host/` | C++20, Shared Memory RingBuffers |
| **Shared Types** | `src/shared/` | C++ primitives (no JUCE deps) |
| **Web Client (UI)** | `src/web_client/` | React 18.3.1, TypeScript, Vite, Tailwind CSS v4 (base utilities + custom design tokens per Spec §7.1) |
| **Project Config** | `CMakeLists.txt` | CMake → Xcode |

### ⚠️ CRITICAL PLATFORM RULES

**DO NOT:**
- ❌ Install npm, yarn, pnpm, or any Node package managers for the engine
- ❌ Confuse the React *web client* (embedded UI) with the JUCE *engine* — they are separate concerns
- ❌ Use web terminology (hooks, components, JSX) when discussing the C++ engine
- ❌ Suggest browser-based or Electron-based solutions
- ❌ Allocate memory or block on the audio thread

**DO:**
- ✅ Use JUCE framework and modern C++20 for the engine
- ✅ Use React/TypeScript for the embedded web UI only
- ✅ Respect the architecture: C++ Engine = Source of Truth, React = Stateless View
- ✅ Use JUCE idioms (Component hierarchy, MessageManager, AudioProcessor patterns)
- ✅ Use CMake for project config; build & debug in Xcode

---

## Coding Guidelines

### C++ / JUCE Engine

- **Documentation:** Always consult the JUCE documentation via the `juce-docs` MCP server when implementing or modifying C++ engine code. This is mandatory for all JUCE-related work.
- **Audio Thread Safety:** The audio thread is sacred. **Never** allocate memory, lock mutexes, or perform I/O on the audio thread.
- **RAII:** Use smart pointers (`std::unique_ptr`, `juce::OwnedArray`) for resource management.
- **Lock-Free Queues:** Use SPSC FIFOs for cross-thread communication (audio ↔ message thread).
- **Compiler Warnings:** Enable `-Wall -Wextra` in Projucer exporter settings. Treat warnings as errors.
- **One Class Per File:** Header (`.h`) + Implementation (`.cpp`). No god files.
- **Max 200 Lines:** If a file exceeds 200 lines, break it into smaller, focused units.
- **Separation of Concerns:** Audio logic and UI components live in separate directories and never directly reference each other.

### React / TypeScript Web Client

- **Stateless View:** The React UI only renders state received from the engine via WebSocket. It does **not** own application state.
- **Strictly Typed Protocol:** All commands sent to the engine use the `Command` union type. All state received uses the `AppState` interface. See Spec §3.1.
- **Canvas for Meters:** Use Canvas API for performance-sensitive visualizations (VU meters, waveforms).
- **Error Boundaries:** Wrap major UI sections in `ErrorBoundary` components. UI crashes must never affect the audio engine.

### General

- **KISS:** Prefer simple, readable code over clever abstractions.
- **DRY:** Extract reusable logic into separate classes/files.
- **No Dead Code:** If you write a helper function, use it or delete it.
- **Visual Verification:** UI must look high-spec and premium.
- **Iterative Development:** Build one component at a time. Verify often.

---

## Code Editing Discipline

- Do **not** run scripts that bulk-modify code (codemods, one-off scripts, giant `sed`/regex refactors).
- Large mechanical changes: break into smaller, explicit edits and review diffs.
- Subtle/complex changes: edit by hand, file-by-file, with careful reasoning.

---

## Backwards Compatibility & File Sprawl

We optimize for a clean architecture now, not backwards compatibility.

- No "compat shims" or "v2" file clones.
- When changing behavior, migrate callers and remove old code.
- New files are only for genuinely new domains that don't fit existing modules.
- The bar for adding files is very high.

---

## Issue Tracking with br (Beads)

All issue tracking goes through **Beads**. No other TODO systems.

### Tool Installation Paths

- `br` lives at `/Users/billt/.local/bin/br`
- `bv` lives at `/Users/billt/.local/bin/bv`

Ensure `/Users/billt/.local/bin` is present in your `$PATH` (the default shell is `/bin/zsh`). If another agent cannot invoke the tools directly, have them add the following line to their shell profile and reload it:

```bash
export PATH="/Users/billt/.local/bin:$PATH"
```

Key invariants:

- `.beads/` is authoritative state and **must always be committed** with code changes.
- Do not edit `.beads/*.jsonl` directly; only via `br`.

### Basics

Check ready work:

```bash
br ready --json
```

Create issues:

```bash
br create "Issue title" -t bug|feature|task -p 0-4 --json
br create "Issue title" -p 1 --deps discovered-from:br-123 --json
```

Update:

```bash
br update br-42 --status in_progress --json
br update br-42 --priority 1 --json
```

### Full Command List

| Command | Description |
|:---|:---|
| `init` | Initialize a beads workspace |
| `create` | Create a new issue |
| `q` | Quick capture (create issue, print ID only) |
| `list` | List issues |
| `show` | Show issue details |
| `update` | Update an issue |
| `close` | Close an issue |
| `reopen` | Reopen an issue |
| `delete` | Delete an issue (creates tombstone) |
| `ready` | List ready issues (unblocked, not deferred) |
| `blocked` | List blocked issues |
| `search` | Search issues |
| `dep` | Manage dependencies |
| `label` | Manage labels |
| `epic` | Epic management commands |
| `comments` | Manage comments |
| `stats`/`status` | Show project statistics |
| `count` | Count issues with optional grouping |
| `stale` | List stale issues |
| `lint` | Check issues for missing template sections |
| `defer`/`undefer`| Defer/Undefer issues (schedule for later) |
| `config` | Configuration management |
| `sync` | Sync database with JSONL file |
| `doctor`/`info` | Diagnostics and workspace metadata |
| `schema` | Emit JSON Schemas for tool integration |
| `where` | Show the active .beads directory |
| `audit` | Record and label agent interactions |
| `changelog` | Generate changelog from closed issues |
| `graph` | Visualize dependency graph |
| `agents` | Manage AGENTS.md workflow instructions |

Complete:

```bash
br close br-42 --reason "Completed" --json
```

Types:

- `bug`, `feature`, `task`, `epic`, `chore`

Priorities:

- `0` critical (security, data loss, broken builds)
- `1` high
- `2` medium (default)
- `3` low
- `4` backlog

### Agent Workflow

1. `br ready` to find unblocked work.
2. Claim: `br update <id> --status in_progress`.
3. Implement + test.
4. If you discover new work, create a new bead with `discovered-from:<parent-id>`.
5. Close when done.
6. Commit `.beads/` in the same commit as code changes.

### Sync

- Run `br sync --flush-only` to export to `.beads/issues.jsonl` without git operations.
- Then run `git add .beads/ && git commit -m "Update beads"` to commit changes.

### Never

- Use markdown TODO lists.
- Use other trackers.
- Duplicate tracking.

---

## Using bv as an AI Sidecar

`bv` is a graph-aware triage engine for Beads projects (`.beads/beads.jsonl`). Instead of parsing JSONL or hallucinating graph traversal, use robot flags for deterministic, dependency-aware outputs.

**⚠️ CRITICAL: Use ONLY `--robot-*` flags. Bare `bv` launches an interactive TUI that blocks your session.**

### The Workflow: Start With Triage

**`bv --robot-triage` is your single entry point.** It returns everything you need in one call:
- `quick_ref`: at-a-glance counts + top 3 picks
- `recommendations`: ranked actionable items with scores, reasons, unblock info
- `quick_wins`: low-effort high-impact items
- `blockers_to_clear`: items that unblock the most downstream work
- `project_health`: status/type/priority distributions, graph metrics
- `commands`: copy-paste shell commands for next steps

```bash
# 1) Start with triage (single-call mega-command)
bv --robot-triage

# 2) Minimal mode: just the top pick + claim command
bv --robot-next

# 3) Token-optimized output (TOON)
bv --robot-triage --format toon
export BV_OUTPUT_FORMAT=toon

# 4) Full robot help
bv --robot-help
```

### Other Useful bv Commands

| Command | Returns |
|---------|---------|
| `--robot-plan` | Parallel execution tracks with `unblocks` lists |
| `--robot-priority` | Priority misalignment detection |
| `--robot-insights` | Full metrics: PageRank, betweenness, critical path, cycles |
| `--robot-alerts` | Stale issues, blocking cascades, priority mismatches |
| `--robot-suggest` | Hygiene: duplicates, missing deps, label suggestions |

### Scoping & Filtering

```bash
bv --robot-plan --label engine                # Scope to label's subgraph
bv --recipe actionable --robot-plan           # Pre-filter: ready to work (no blockers)
bv --recipe high-impact --robot-triage        # Pre-filter: top PageRank scores
bv --robot-triage --robot-triage-by-track     # Group by parallel work streams
bv --robot-triage --robot-triage-by-label     # Group by domain
```

### jq Quick Reference

```bash
bv --robot-triage | jq '.quick_ref'                        # At-a-glance summary
bv --robot-triage | jq '.recommendations[0]'               # Top recommendation
bv --robot-plan | jq '.plan.summary.highest_impact'        # Best unblock target
bv --robot-insights | jq '.Cycles'                         # Circular deps (must fix!)
```

Use `bv` instead of parsing `beads.jsonl` — it computes PageRank, critical paths, cycles, and parallel tracks deterministically.

---

## MCP Agent Mail — Multi-Agent Coordination

Agent Mail is available as an MCP server for coordinating work across agents.

What Agent Mail gives:
- Identities, inbox/outbox, searchable threads.
- Advisory file reservations (leases) to avoid agents clobbering each other.
- Persistent artifacts in git (human-auditable).

### Core Patterns

1. **Same repo**
   - Register identity:
     - `ensure_project` then `register_agent` with the repo's absolute path as `project_key`.
   - Reserve files before editing:
     - `file_reservation_paths(project_key, agent_name, ["src/**"], ttl_seconds=3600, exclusive=true)`.
   - Communicate:
     - `send_message(..., thread_id="FEAT-123")`.
     - `fetch_inbox`, then `acknowledge_message`.
   - Fast reads:
     - `resource://inbox/{Agent}?project=<abs-path>&limit=20`.
     - `resource://thread/{id}?project=<abs-path>&include_bodies=true`.

2. **Macros vs granular:**
   - Prefer macros when speed is more important than fine-grained control:
     - `macro_start_session`, `macro_prepare_thread`, `macro_file_reservation_cycle`, `macro_contact_handshake`.
   - Use granular tools when you need explicit behavior.

### Common Pitfalls

- "from_agent not registered" → call `register_agent` with correct `project_key`.
- `FILE_RESERVATION_CONFLICT` → adjust patterns, wait for expiry, or use non-exclusive reservation.

### Starting Agent Mail

If Agent Mail isn't available as an MCP server, flag to the user. They may need to start it using the `am` alias or by running:
```bash
cd "<agent_mail_install_dir>/mcp_agent_mail" && bash scripts/run_server_with_token.sh
```

---

## Handling Changes from Other Agents

If you see unexpected changes in the working tree (modified files you didn't touch), these are changes created by other agents working on the project concurrently. This is normal and happens frequently.

**The rule is simple:** You NEVER, under ANY CIRCUMSTANCE, stash, revert, overwrite, or otherwise disturb the work of other agents. Treat those changes exactly as if you made them yourself.

---

## Landing the Plane (Session Completion)

**When ending a work session**, you MUST complete ALL steps below. Work is NOT complete until `git push` succeeds.

**MANDATORY WORKFLOW:**

1. **File issues for remaining work** — Create beads (`br create ...`) for anything that needs follow-up
2. **Run quality gates** (if code changed) — Build, tests, linters
3. **Update issue status** — Close finished work (`br close ...`), update in-progress items
4. **PUSH TO REMOTE** — This is MANDATORY:
   ```bash
   git pull --rebase
   br sync --flush-only
   git add .beads/
   git commit -m "Update beads"
   git push
   git status  # MUST show "up to date with origin"
   ```
5. **Clean up** — Clear stashes, prune remote branches
6. **Verify** — All changes committed AND pushed
7. **Hand off** — Provide context for next session

**CRITICAL RULES:**
- Work is NOT complete until `git push` succeeds
- NEVER stop before pushing — that leaves work stranded locally
- NEVER say "ready to push when you are" — YOU must push
- If push fails, resolve and retry until it succeeds

<!-- bv-agent-instructions-v1 -->

---

## Beads Workflow Integration

This project uses [beads_viewer](https://github.com/Dicklesworthstone/beads_viewer) for issue tracking. Issues are stored in `.beads/` and tracked in git.

### Essential Commands

```bash
# View issues (launches TUI - avoid in automated sessions)
bv

# CLI commands for agents (use these instead)
br ready              # Show issues ready to work (no blockers)
br list --status=open # All open issues
br show <id>          # Full issue details with dependencies
br create --title="..." --type=task --priority=2
br update <id> --status=in_progress
br close <id> --reason="Completed"
br close <id1> <id2>  # Close multiple issues at once
br sync               # Commit and push changes
```

### Workflow Pattern

1. **Start**: Run `br ready` to find actionable work
2. **Claim**: Use `br update <id> --status=in_progress`
3. **Work**: Implement the task
4. **Complete**: Use `br close <id>`
5. **Sync**: Always run `br sync` at session end

### Key Concepts

- **Dependencies**: Issues can block other issues. `br ready` shows only unblocked work.
- **Priority**: P0=critical, P1=high, P2=medium, P3=low, P4=backlog (use numbers, not words)
- **Types**: task, bug, feature, epic, question, docs
- **Blocking**: `br dep add <issue> <depends-on>` to add dependencies

### Session Protocol

**Before ending any session, run this checklist:**

```bash
git status              # Check what changed
git add <files>         # Stage code changes
br sync                 # Commit beads changes
git commit -m "..."     # Commit code
br sync                 # Commit any new beads changes
git push                # Push to remote
```

### Best Practices

- Check `br ready` at session start to find available work
- Update status as you work (in_progress → closed)
- Create new issues with `br create` when you discover tasks
- Use descriptive titles and set appropriate priority/type
- Always `br sync` before ending session

<!-- end-bv-agent-instructions -->
