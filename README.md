# Claude Code Session Pretty Print

Browser-based pretty viewer for Claude Code JSONL session files.

## Tech Stack

- **C++17** — core parser logic
- **Emscripten** — compile to WebAssembly for browser
- **Catch2** — test framework (amalgamated, in `third_party/`)
- **CMake** — build system

## Build & Test Commands

```bash
# Native build (for TDD)
mkdir -p build && cd build && cmake .. && make

# Run tests
./build/tests
```

## Project Goal

Parse and render Claude Code `.jsonl` session files in a browser with:
- Message timeline (user, assistant, tool calls, tool results)
- Syntax-highlighted code blocks
- Collapsible thinking blocks
- Collapsible tool call details
- File upload / drag-and-drop of `.jsonl` files
- Fully client-side (no server needed)
