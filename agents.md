## Project Context

**FlowZone** is a **JUCE (C++) macOS desktop application**, not a web app.
- **Framework:** JUCE 8.x
- **Language:** C++
- **IDE:** Xcode
- **Platform:** macOS (with potential iOS support)
- **Reference:** See `Spec_FlowZone_Looper.md` for full requirements

## ⚠️ CRITICAL REMINDERS

**DO NOT:**
- ❌ Install npm, Vite, React, Next.js, or any web frameworks
- ❌ Create package.json, tsconfig.json, or web-related config files
- ❌ Use web terminology (hooks, components, JSX, etc.)
- ❌ Suggest browser-based solutions

**DO:**
- ✅ Use JUCE framework and C++ exclusively
- ✅ Work in Xcode for macOS development
- ✅ Use JUCE terminology (Components, AudioProcessor, MessageManager)
- ✅ Focus on native macOS audio application development

## Coding Guidelines (JUCE/C++)
Follow JUCE best practices and modern C++ conventions:
- **JUCE Patterns:** Use JUCE idioms (Component hierarchy, MessageManager, AudioProcessor patterns)
- **Audio Thread Safety:** Never allocate memory or block on the audio thread
- **RAII:** Use smart pointers (`std::unique_ptr`, `juce::OwnedArray`) for resource management
- Iterative Development: Build one component at a time. Verify often.
- No Dead Code: If you write a helper function, use it or delete it.
- Visual Verification: UI must look high spec and premium.
- Keep It Simple (KISS): Prefer simple, readable code over clever abstractions.
- Don't Repeat Yourself (DRY): Extract reusable logic into separate classes/files.
- Modularity:
  - Avoid "God Files". If a file exceeds 200 lines, break it down.
  - Separate audio logic from UI components.
  - One class per file (header + implementation).

## Landing the Plane (Session Completion)

**When ending a work session**, you MUST complete ALL steps below. Work is NOT complete until `git push` succeeds.

**MANDATORY WORKFLOW:**

1. **File issues for remaining work** - Create issues for anything that needs follow-up
2. **Run quality gates** (if code changed) - Tests, linters, builds
3. **Update issue status** - Close finished work, update in-progress items
4. **PUSH TO REMOTE** - This is MANDATORY:
   ```bash
   git pull --rebase
   git push
   git status  # MUST show "up to date with origin"
   ```
5. **Clean up** - Clear stashes, prune remote branches
6. **Verify** - All changes committed AND pushed
7. **Hand off** - Provide context for next session

**CRITICAL RULES:**
- Work is NOT complete until `git push` succeeds
- NEVER stop before pushing - that leaves work stranded locally
- NEVER say "ready to push when you are" - YOU must push
- If push fails, resolve and retry until it succeeds
Use 'bd' for task tracking
