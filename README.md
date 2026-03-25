# Claude Code Session Pretty Print

Browser-based pretty viewer for Claude Code JSONL session files.

## Tech Stack

- **C++17** — core parser logic
- **Emscripten** — compile to WebAssembly for browser
- **Catch2** — test framework (amalgamated, in `third_party/`)
- **CMake** — build system (single `CMakeLists.txt` with two configurations: native for TDD, Emscripten for WASM)

## Build & Test Commands

The same `CMakeLists.txt` serves both builds. When invoked via `emcmake`, it detects Emscripten and builds the WASM target. Otherwise it builds the native Catch2 test runner.

```bash
# Native build (for TDD)
mkdir -p build && cd build && cmake .. && make

# Run tests
./build/tests

# WASM build (requires Emscripten + Python 3.10+)
export EMSDK_PYTHON=/usr/bin/python3.10
mkdir -p build_wasm && cd build_wasm
emcmake cmake .. && emmake make
# Output: web/parser.js + web/parser.wasm
```

## Project Goal

Parse and render Claude Code `.jsonl` session files in a browser with:
- Message timeline (user, assistant, tool calls, tool results)
- Syntax-highlighted code blocks
- Collapsible thinking blocks
- Collapsible tool call details
- File upload / drag-and-drop of `.jsonl` files
- Fully client-side (no server needed)
