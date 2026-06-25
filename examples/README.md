# Example files

Drop any of these into the viewer (https://makflint.github.io/jsonl-viewer/, or `python3 -m http.server`
in `web/`) to see how each kind of input renders.

| File | Records | Kind | What it exercises |
|------|---------|------|-------------------|
| `claude-session.jsonl` | 8 | Claude Code session | The message timeline: `user`/`assistant` entries, a `thinking` block, Markdown with a code block, a `tool_use` (with a copyable command) and its nested `tool_result`, an `ai-title`, and metadata types (`queue-operation`, `file-history-snapshot`) hidden behind the toggle. |
| `products.jsonl` | 24 | Raw JSONL records | The raw-fallback path: arbitrary JSON with no session schema → pretty-printed rows **plus** a Table view with grouped headers (nested `dimensions`, `supplier`) and a Schema tab (presence %, types, numeric ranges, array sizes, top values). |

Both files are synthetic and contain no real personal data.
