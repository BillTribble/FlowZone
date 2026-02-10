
## Coding Guidelines 
Iterative Development: Build one component at a time. Verify often.
No Dead Code: If you write a helper function, use it or delete it.
Visual Verification: UI must look high spec and premium. 
Error Handling: APIs fail. Always wrap fetch calls in try/catch.
Keep It Simple (KISS): Prefer simple, readable code over clever abstractions.
Don't Repeat Yourself (DRY): Extract reusable logic into hooks or utility files.
Modularity:
Avoid "God Files". If a file exceeds 200 lines, break it down.
Separate Logic (hooks/) from UI (components/).
One component per file.
When tasks are complete, ask the user if they want the new feature added to the extension-design-spec.md

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
