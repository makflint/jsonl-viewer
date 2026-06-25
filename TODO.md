# TODO

Open work for jsonl-viewer. Single source of truth — keep all open tasks here.

- [x] **Verify committed WASM matches the C++ source.** Done via behavioural check rather than
  byte-diff: the `.wasm` binary is not byte-reproducible across builds, so `tests/wasm_test.js`
  loads the committed `web/parser.js` and asserts the parser still behaves correctly across the
  WASM boundary. Run in CI by `.github/workflows/ci.yml`; a stale artifact after a `src/*.hpp`
  change fails the test.

- [ ] **Add browser/E2E smoke coverage.** Unit tests cover the parser, schema, and renderer
  logic, but the real WASM → DOM render path is never exercised. Add one real-browser
  walkthrough per render path (Claude-session timeline, and Table + Schema fallback).
