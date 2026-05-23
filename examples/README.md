# Example JSONL files

Drop any of these into the viewer at https://makflint.github.io/jsonl-viewer/ (or `python3 -m http.server` in `web/`) to see how each kind of input renders.

| File | Lines | Schema | What it exercises |
|------|-------|--------|-------------------|
| `claude-session.jsonl` | 22 | Claude Code session log | The original target format: `user`/`assistant` entries, `thinking` blocks, `tool_use` + `tool_result`, `ai-title`, metadata types like `queue-operation` and `file-history-snapshot`. |
| `sample-records.jsonl` | 78 | sample records (sample records) | The raw-fallback path: each line is arbitrary JSON with no Claude session schema, so the parser surfaces them as `raw` entries with pretty-printed JSON and a `line N` header. |

The session file is a sanitised real session about Firefox-extension XPI build artifacts — no credentials, no PII. The sample data is public record content from `example.com`.
