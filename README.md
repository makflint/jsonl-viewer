# Claude Code Session Pretty Print

Browser-based pretty viewer for Claude Code JSONL session files.

**Live demo:** https://makflint.github.io/jsonl-viewer/

## Tech Stack

- **C++17** — core parser logic
- **Emscripten** — compile to WebAssembly for browser
- **marked.js** — Markdown rendering in assistant messages
- **Catch2** — test framework (amalgamated, in `third_party/`)
- **CMake** — build system (single `CMakeLists.txt` with two configurations: native for TDD, Emscripten for WASM)

## Build & Test Commands

The same `CMakeLists.txt` serves both builds. When invoked via `emcmake`, it detects Emscripten and builds the WASM target. Otherwise it builds the native Catch2 test runner.

```bash
# Native build (for TDD)
mkdir -p build && cd build && cmake .. && make

# Run C++ tests
./build/tests

# Run JS renderer tests
node tests/renderer_test.js

# WASM build (requires Emscripten + Python 3.10+)
export EMSDK_PYTHON=/usr/bin/python3.10
mkdir -p build_wasm && cd build_wasm
/opt/emsdk/upstream/emscripten/emcmake cmake ..
/opt/emsdk/upstream/emscripten/emmake make
# Output: web/parser.js + web/parser.wasm
# Note: build uses -fexceptions (required for nlohmann/json)

# Run the viewer locally
cd web && python3 -m http.server 8080
# Open http://localhost:8080
```

## Project Goal

Parse and render Claude Code `.jsonl` session files in a browser with:
- Message timeline (user, assistant, tool calls, tool results)
- Markdown rendering in assistant messages
- Collapsible thinking blocks
- Collapsible tool call details with copyable commands
- Tool results grouped under their tool call
- Metadata entries hidden by default (toggle to show)
- File upload / drag-and-drop of `.jsonl` files
- Fully client-side (no server needed)
- Deployed to GitHub Pages via Actions workflow
