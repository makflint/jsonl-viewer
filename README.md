# Claude Code Session Pretty Print

Browser-based pretty viewer for Claude Code JSONL session files.

**Live demos:**
- https://makflint.github.io/jsonl-viewer/ — primary (self-hosted on self-hosted VPS, see [Deployment](#deployment))
- https://makflint.github.io/jsonl-viewer/ — mirror, auto-updated by the GitHub Actions workflow on push to `master`

Drop `examples/sample-records.jsonl` or `examples/claude-session.jsonl` into either to see both render paths.

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

## Deployment

The site is shipped two ways. Both serve the contents of `web/` as static files — there is no backend.

### Primary — self-hosted VPS (`makflint.github.io`)

Manual rsync from your dev machine after rebuilding the WASM. Traefik on the VPS terminates TLS and routes `Host=makflint.github.io` to a docker container `jsonl-pretty` (`nginx:alpine`) that bind-mounts `/srv/app` → `/usr/share/nginx/html`. Push files into that directory and they are live immediately — no container restart needed.

```bash
# 1. Rebuild WASM if parser.hpp / wasm_bindings.cpp changed
EMSDK=/opt/emsdk cmake --build build_wasm

# 2. Push the four web assets to the docroot
rsync -av --checksum \
    web/index.html web/parser.js web/parser.wasm web/renderer.js \
    user@server:/srv/app/

# 3. Smoke-check
curl -s -o /dev/null -w "HTTP %{http_code}\n" https://makflint.github.io/jsonl-viewer/
```

The docroot files are owned by `ubuntu` (mode 644) so no `sudo` is needed.

Traefik routing is configured via container labels (set when the container was created); they survive `docker restart` but not `docker rm`. If the container is ever recreated, use:

```bash
docker run -d --name jsonl-pretty \
    -v /srv/app:/usr/share/nginx/html \
    --label traefik.enable=true \
    --label "traefik.http.routers.jsonl-pretty.rule=Host(\`makflint.github.io\`)" \
    --label traefik.http.routers.jsonl-pretty.entrypoints=websecure \
    --label traefik.http.routers.jsonl-pretty.tls.certresolver=letsencrypt \
    --network <traefik-network> \
    nginx:alpine
```

### Mirror — GitHub Pages

`.github/workflows/pages.yml` uploads `web/` as the Pages artifact on every push to `master`. No manual steps — the workflow handles checkout, configure-pages, upload, and deploy.

```bash
git push origin master
# → Actions workflow "Deploy to GitHub Pages" runs in ~1 min
# → https://makflint.github.io/jsonl-viewer/ updated
```

### Why both?

The self-hosted deploy gives a vanity URL on infrastructure already provisioned for other projects and decouples the live site from GitHub's availability. GitHub Pages stays as an automatic mirror so the public-facing demo is never older than `master`.
