# CLI Reference

## `xtracer`

HTTP/WebSocket render server. Serves the web client UI and the render API.

```
xtracer [options]
```

All flags use double dash (`--`).

### Flags

| Flag | Default | Notes |
|------|---------|-------|
| `--host <ip>` | `127.0.0.1` | Bind address. |
| `--port <n>` | `8080` | TCP port. Must be in range 1–65535. |
| `--scene-dir <path>` | `scene` | Directory scanned for `.scn` scene files. |
| `--gallery-dir <path>` | `gallery` | Directory used to persist completed render gallery entries. |
| `--web-root <path>` | `src/frontend/web-client` | Root directory served as the web client UI. |
| `--max-concurrent-renders <n>` | `999` | Maximum number of renders that may run simultaneously. Must be ≥ 1. |
| `--render-reserve-threads <n>` | `1` | Number of CPU threads to keep free for server tasks. Reduces the thread budget allocated to renders. |
| `--verbose` / `-v` | off | Log every HTTP request. |
| `--help` | — | Print usage and exit. |

### Thread budget

The effective render thread budget is `max(1, capacity − render-reserve-threads)` where `capacity` is `OMP_NUM_THREADS` if OpenMP is available, otherwise the logical core count. Individual render jobs cannot exceed this budget even if `threads` is specified in the render request.

### Web client

The server also serves the built-in web UI at `/`. See `docs/API.md` for the REST and WebSocket API.
