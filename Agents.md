# Claude Code Session Pretty Print

Browser-based pretty viewer for Claude Code JSONL session files.

---

## TDD Principles

1. **Red-Green-Refactor cycle** — Write a failing test first, make it pass with the simplest code, then refactor. Never skip a step.
2. **One test at a time** — Add one small test, make it pass, repeat. No batch implementations.
3. **Test behavior, not implementation** — Tests describe *what* the code does, not *how*. Changing internals should not break tests unless behavior changes.
4. **Triangulation** — Use multiple specific examples to drive toward a general solution. Don't generalize prematurely from a single case.
5. **The simplest thing that works** — Each green step uses the minimum code to pass. Complexity grows only as tests demand it.
6. **Tests are first-class code** — Keep tests readable, maintainable, and free of duplication. Refactor tests too.
7. **Fast feedback** — Tests must run in milliseconds. Slow tests break the cycle.
8. **No production code without a failing test** — Every line of production code exists because a test required it.

---

## Clean Code Principles

1. **Meaningful names** — Variables, functions, and classes reveal intent. No abbreviations, no mental mapping required.
2. **Small functions** — Each function does one thing. If you can extract a block with a meaningful name, do it.
3. **Single Responsibility** — Every module/class has one reason to change.
4. **No duplication (DRY)** — Duplicated logic is a missed abstraction. But don't abstract prematurely — wait for the third occurrence.
5. **Minimal arguments** — Functions take few arguments. More than two is a smell; consider an object.
6. **No side effects** — Functions should do what their name says and nothing else.
7. **Command-Query Separation** — A function either changes state or returns a value, not both.
8. **Fail fast** — Validate at boundaries, throw early, never swallow errors silently.
9. **Boy Scout Rule** — Leave code cleaner than you found it.
10. **YAGNI** — Don't build what isn't needed yet.

---

## Git Discipline

1. **Commit every TDD step** — Each Red, Green, and Refactor gets its own commit. The git history is the story of the project's evolution.
2. **Fine-grained commits** — Small, atomic commits. One logical change per commit. Never bundle unrelated changes.
3. **Descriptive commit messages** — The message explains *why*, not just *what*. Prefix with the TDD phase: `red:`, `green:`, `refactor:`.
4. **Never skip a commit** — Even "trivial" steps get recorded. The history should allow replaying the entire development process step by step.
5. **No squashing** — Preserve the full granular history. Every decision point is visible.

---

## Project Goal

Parse and render Claude Code `.jsonl` session files in a browser with:
- Message timeline (user, assistant, tool calls, tool results)
- Syntax-highlighted code blocks
- Collapsible thinking blocks
- Collapsible tool call details
- File upload / drag-and-drop of `.jsonl` files
- Fully client-side (no server needed)
