# Claude Code Session Pretty Print

Browser-based pretty viewer for Claude Code JSONL session files.

**Live demo:** https://makflint.github.io/jsonl-viewer/

## Tech Stack

- **C++23** — core parser logic
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
# Set EMSDK to your emsdk root, e.g.: export EMSDK=/opt/emsdk
# First time only — cmake/emscripten_toolchain.cmake auto-locates Python 3.10+
cmake -B build_wasm -DCMAKE_TOOLCHAIN_FILE=cmake/emscripten_toolchain.cmake .
# Subsequent builds
make -C build_wasm
# Output: web/parser.js + web/parser.wasm

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
