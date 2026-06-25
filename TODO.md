# TODO

Open work for jsonl-viewer. Single source of truth — keep all open tasks here.

- [ ] **Verify committed WASM matches the C++ source.** `web/parser.js` + `web/parser.wasm`
  are checked in so the demo needs no build step, but nothing verifies they're in sync with
  `src/*.hpp` / `src/wasm_bindings.cpp`. A stale artifact could ship a regression silently.
  Add a CI step that rebuilds the WASM and fails if the committed output differs.

- [ ] **Add browser/E2E smoke coverage.** Unit tests cover the parser, schema, and renderer
  logic, but the real WASM → DOM render path is never exercised. Add one real-browser
  walkthrough per render path (Claude-session timeline, and Table + Schema fallback).
