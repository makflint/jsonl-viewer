# JSONL Viewer

Browser-based pretty viewer for **JSONL and JSON** files. The parsing core is written in
**C++23** and compiled to **WebAssembly**, so everything runs 100% client-side — your file
is never uploaded and never leaves the browser.

**▶ Live demo: https://makflint.github.io/jsonl-viewer/**

Drop a `.jsonl` / `.json` file (or one of the bundled files in [`examples/`](examples/)) and the
viewer picks one of two render paths automatically:

- **Claude Code session** → a readable message timeline (user / assistant / tool calls) with
  Markdown, collapsible *thinking* blocks, and collapsible tool-call details.
  Try [`examples/claude-session.jsonl`](examples/claude-session.jsonl).
- **Any other JSONL/JSON** → records with no session schema fall back to pretty-printed rows,
  plus a **Table view** with grouped headers and a **Schema** tab showing per-field presence,
  types, numeric ranges, array sizes, and top values.
  Try [`examples/products.jsonl`](examples/products.jsonl).

## Features

- Message timeline: user, assistant, tool calls, tool results
- Markdown rendering in assistant messages (sanitised with DOMPurify)
- Collapsible *thinking* blocks
- Collapsible tool-call details with copyable commands; results nested under their call
- Metadata entries hidden by default (one-click toggle)
- Table + Schema views for arbitrary JSONL/JSON, with right-click column hide
- Syntax highlighting (highlight.js, Catppuccin theme)
- File picker **and** drag-and-drop
- Fully client-side — no backend, no upload

## How it works

```
.jsonl / .json ──▶ C++23 parser (WASM) ──▶ Session model ──▶ JS renderer ──▶ DOM
                   src/parser.hpp                            web/renderer.js
                   src/schema.hpp
```

The C++ core ([`src/parser.hpp`](src/parser.hpp), [`src/schema.hpp`](src/schema.hpp)) parses each
record and analyses the schema; [`src/wasm_bindings.cpp`](src/wasm_bindings.cpp) exposes it to the
page via Emscripten. The renderer ([`web/renderer.js`](web/renderer.js)) is plain JavaScript that
turns the parsed model into HTML. A record that doesn't match the Claude-session shape becomes a
`raw` row, which is what drives the Table and Schema views.

The compiled artifacts (`web/parser.js`, `web/parser.wasm`) are committed, so the demo runs with no
build step — you only need the toolchain below if you change the C++.

## Tech stack

- **C++23** — core parser and schema analysis
- **Emscripten** — compile the core to WebAssembly
- **marked.js** + **DOMPurify** — Markdown rendering
- **highlight.js** (Catppuccin) — syntax highlighting
- **Catch2** — C++ test framework (amalgamated, in `third_party/`)
- **CMake** — one `CMakeLists.txt`, two targets: native (for TDD) or Emscripten (for WASM)

## Build & test

The same `CMakeLists.txt` serves both builds: invoked via `emcmake` it builds the WASM target,
otherwise it builds the native Catch2 test runner.

```bash
# Native build + C++ tests (TDD loop)
cmake -B build && cmake --build build && ./build/tests

# JS renderer tests
node tests/renderer_test.js

# Rebuild the WASM (only if you changed the C++ core; requires Emscripten + Python 3.10+)
# Point EMSDK at your emsdk root, e.g. export EMSDK=/path/to/emsdk
cmake -B build_wasm -DCMAKE_TOOLCHAIN_FILE=cmake/emscripten_toolchain.cmake .
cmake --build build_wasm          # → web/parser.js + web/parser.wasm

# Run the viewer locally (a static server is required — .wasm can't load over file://)
cd web && python3 -m http.server 8080
# open http://localhost:8080
```

## Deployment

The whole app is static — there is no backend. [`.github/workflows/pages.yml`](.github/workflows/pages.yml)
publishes the contents of `web/` to GitHub Pages on every push to `main`; GitHub serves `.wasm`
with the correct `application/wasm` type, so it works out of the box.

```bash
git push origin main
# → "Deploy to GitHub Pages" runs (~1 min) → https://makflint.github.io/jsonl-viewer/
```

## Development

Built test-first. The TDD, Clean Code, and commit conventions used throughout live in
[`Agents.md`](Agents.md).
