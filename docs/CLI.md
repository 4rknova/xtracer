# CLI Reference

Command-line interface reference for `xtracer_cli` and `xtracer_web`.

---

## `xtracer_cli`

Offline renderer. Renders a single scene file and saves the result as PNG.

```
xtracer_cli [options] <scene.scn>
xtracer_cli version
```

The scene path is a positional argument. All flags use a single dash (`-`).

### Flags

| Flag | Value | Notes |
|------|-------|-------|
| `-renderer <name>` | string | Integrator to use (default: `pathtracer`). See `docs/INTEGRATORS.md` for valid names. |
| `-res <WxH>` | e.g. `1920x1080` | Output resolution. Both dimensions must be ≥ 1. |
| `-rdepth <n>` | uint ≥ 1 | Maximum ray recursion depth (default: `3`). |
| `-samples <n>` | uint ≥ 1 | Samples per pixel. |
| `-aa <n>` | uint ≥ 2 | Antialiasing level (subpixel grid size). |
| `-tile_size <n>` | uint ≥ 1 | Square tile side length in pixels. |
| `-threads <n>` | uint | Number of render threads (`0` = auto). |
| `-cam <name>` | string | Active camera name; overrides `default_camera` in the scene. |
| `-variant <name>` | string | Scene variant to apply before rendering. |
| `-outdir <path>` | path | Output directory for the PNG file (default: current directory). |
| `-mod <modifier>` | string | Apply a scene modifier. Can be repeated for multiple modifiers. |

### Special command

```
xtracer_cli version
```

Prints the version string and exits. This is a bare word (no dash).

### Output

The output file is written to `<outdir>/<scene_basename>_<integrator>.png`. Tonemapping is applied with default settings before saving.

---

## `xtracer_web`

HTTP/WebSocket render server. Serves the web client UI and the render API.

```
xtracer_web [options]
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
