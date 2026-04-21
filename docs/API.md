# Web API Reference

REST and WebSocket API served by `xtracer_web`. All API paths are prefixed with `/api/`. Parameters are accepted as URL query strings or POST body (`application/x-www-form-urlencoded`). Responses are JSON with `Content-Type: application/json` unless noted.

Source: `src/frontend/web-server/routes.cc`.

---

## Health & Info

### `GET /api/health`

Returns `{"ok":true}`. Use as a liveness probe.

### `GET /api/about`

Returns server version, hardware info, and third-party licenses.

```json
{
  "name": "XTRACER WEB",
  "version": "...",
  "logical_cores": 8,
  "openmp_max_threads": 8,
  "render_reserve_threads": 1,
  "render_auto_threads": 7,
  "max_concurrent_renders": 999,
  "active_renders": 0,
  "third_party_licenses": [...]
}
```

---

## Workspaces

A workspace is a per-client render session that tracks the active scene, job, and quality settings. Clients identify themselves with an opaque `client_id` string (max 96 chars, alphanumeric + `_-.`).

### `GET /api/workspaces`

List all workspaces. Pass `client_id` to filter visibility fields.

**Response fields per workspace:**

| Field | Notes |
|-------|-------|
| `id` | Workspace UUID |
| `name` | Human-readable name |
| `is_owned_by_client` | True if `client_id` created this workspace |
| `is_active_for_client` | True if this is the caller's active workspace |
| `active_scene` | Last scene opened in this workspace |
| `active_job_id` | Currently running job ID |
| `last_job_id` | Most recent job ID |
| `quality_samples`, `quality_aa`, `quality_rdepth` | Persisted quality settings |
| `quality_sample_distribution` | `grid` or `random` |
| `updated_ms` | Last-modified timestamp (ms since epoch) |

### `POST /api/workspaces`

Create a new workspace. Returns `{"id":"...","name":"..."}` (HTTP 201).

| Parameter | Notes |
|-----------|-------|
| `client_id` | Optional. Automatically sets this workspace as active for the client. |
| `name` | Human-readable name |

Returns HTTP 409 if the workspace limit is reached.

### `POST /api/workspaces/active`

Set the active workspace for a client.

| Parameter | Required | Notes |
|-----------|----------|-------|
| `client_id` | Yes | |
| `workspace_id` | Yes | |

### `POST /api/workspaces/delete`

Delete a workspace. Cannot delete the last remaining workspace (HTTP 409).

| Parameter | Required | Notes |
|-----------|----------|-------|
| `client_id` | Yes | |
| `workspace_id` | Yes | |

Returns `{"ok":true,"replacement_workspace":"...","active_workspace":"..."}`.

### `POST /api/workspaces/scene_draft`

Store an unsaved scene source in the workspace. The draft is used automatically when rendering the same scene name.

| Parameter | Required | Notes |
|-----------|----------|-------|
| `client_id` | Yes | |
| `scene` | Yes | Scene filename (must end in `.scn`) |
| `source` | Yes | Full NCF scene source text |

### `POST /api/workspaces/settings`

Persist quality settings for a workspace.

| Parameter | Notes |
|-----------|-------|
| `workspace_id` | Target workspace |
| `samples` | uint |
| `aa` | uint |
| `rdepth` | uint |
| `sample_distribution` | `grid` or `random` |

---

## Scenes

### `GET /api/scenes`

List all `.scn` files in the scene directory. Returns `{"scenes":["a.scn","b.scn",...]}`.

### `GET /api/scenes/<scene>/source`

Return raw NCF source text for a named scene. Pass `variant=<name>` to fetch the variant-merged source.

### `GET /api/scenes/<scene>/cameras`

List cameras in a scene. Pass `variant=<name>` to resolve against a variant.

```json
{
  "cameras": ["hero", "wide"],
  "default_camera": "hero",
  "loading": false,
  "load_job_id": 0
}
```

`loading: true` means the scene is still being parsed asynchronously; poll until `loading` is false.

### `GET /api/scenes/<scene>/geometry`

Return raw mesh triangle/normal/UV data and BVH AABB arrays for the visual editor. Expensive — cache client-side.

### `GET /api/scenes/<scene>/runtime_graph`

Return the full parsed scene graph as JSON: cameras, surfaces, materials (with sampler types and scalar values), objects, and media.

### `GET /api/scenes/<scene>/camera_resolve`

Resolve a camera name to its runtime parameters (position, target, up, hfov, basis vectors). Useful for initialising interactive camera controls.

| Parameter | Notes |
|-----------|-------|
| `camera` | Camera name; falls back to `default_camera` then first camera |
| `variant` | Optional variant name |

### `GET /api/scenes/<scene>/runtime_texture`

Return a texture asset as an image. Pass `asset=<relative_path>` (relative to the scene's directory).

### `GET /api/scenes/<scene>/asset`

Return a raw binary asset file from the scene directory. Pass `path=<relative_path>`.

### `GET /api/scenes/template/empty`

Return a minimal empty scene template as plain text.

### `POST /api/scenes/save`

Write a scene file to the scene directory.

| Parameter | Required | Notes |
|-----------|----------|-------|
| `scene` | Yes | Filename (must end in `.scn`, no path separators) |
| `source` | Yes | NCF source text |

### `POST /api/scenes/delete`

Delete a scene file from the scene directory.

| Parameter | Required | Notes |
|-----------|----------|-------|
| `scene` | Yes | Filename |

### `GET /api/scenes/load_jobs/<id>`

Poll the state of an async scene-load job started internally by camera/geometry endpoints while the scene is being parsed.

Returns `{"state":"queued"|"running"|"done"|"error","error":"..."}`.

---

## Render Configuration

### `GET /api/integrators`

List available integrators with their metadata, status, and UI controls.

```json
{
  "integrators": [
    {
      "id": "pathtracer_mis",
      "name": "Pathtracer (MIS)",
      "status": "recommended",
      "controls": [...]
    }
  ]
}
```

Status values: `recommended`, `stable`, `experimental`, `legacy`, `hidden`.

### `GET /api/post_filters`

List available post-processing filters with their parameters.

### `GET /api/resolutions`

List built-in resolution presets. Returns `{"resolutions":[{"id":"hd","w":1920,"h":1080},...]}`

---

## Rendering

### `POST /api/render`

Submit a render job. Returns HTTP 202 on success.

```json
{"job_id": "42", "workspace_id": "ws-001"}
```

| Parameter | Constraints | Notes |
|-----------|-------------|-------|
| `scene` | required, `.scn` filename | Scene to render |
| `integrator` | string | Integrator ID (default: server default). See `docs/INTEGRATORS.md`. |
| `render_mode` | `direct`/`normal`, `progressive`, `incremental`, `interactive` | |
| `iopt.<key>` | string | Integrator-specific option; prefix `iopt.` is stripped |
| `camera` | string | Camera name override |
| `cam_px`, `cam_py`, `cam_pz` | float [-1e9, 1e9] | Camera position override (all nine cam_ params required together) |
| `cam_tx`, `cam_ty`, `cam_tz` | float [-1e9, 1e9] | Camera target override |
| `cam_upx`, `cam_upy`, `cam_upz` | float [-1e6, 1e6] | Camera up vector override |
| `cam_hfov` | float [1, 179] | Horizontal field of view override |
| `width` | int [8, 8192] | Render width in pixels |
| `height` | int [8, 8192] | Render height in pixels |
| `samples` | int [1, 1024] | Samples per pixel |
| `aa` | int [1, 16] | Antialiasing level |
| `sample_distribution` | `grid`/`grid_aligned`, `random`/`jittered` | |
| `rdepth` | int [1, 4096] | Ray recursion depth |
| `tile_size` | int [8, 1024] | Tile side length in pixels |
| `threads` | int [0, 256] | Render threads; `0` = auto; capped to server thread budget |
| `tile_order` | `scanline`, `random`, `radial_in`, `radial_out`, `spiral_in`, `spiral_out` | |
| `variant` | string | Scene variant name |
| `workspace_id` | string | Target workspace; inferred from `client_id` if omitted |
| `client_id` | string | Caller identifier |
| Tonemapping params | — | See [Tonemapping Parameters](#tonemapping-parameters) below |

Returns HTTP 503 `{"error":"render queue is full"}` if the job queue is at capacity.

---

## Jobs

### `GET /api/jobs/active`

List all active (queued, preparing, running, recently completed) jobs.

**Job snapshot fields:**

| Field | Notes |
|-------|-------|
| `id` | Job ID string |
| `workspace_id` | Owning workspace |
| `scene` | Scene filename |
| `integrator` | Integrator ID |
| `render_mode` | Render mode string |
| `state` | `queued`, `preparing`, `running`, `done`, `aborted`, `error` |
| `progress` | Float 0.0–1.0 |
| `tiles_done` / `tiles_total` | Tile completion counts |
| `pass_current` / `pass_total` | Pass counts (progressive modes) |
| `elapsed_ms` | Elapsed render time in milliseconds |
| `threads` | Number of threads allocated |
| `queue_index` | Position in the pending queue; -1 if not queued |

### `GET /api/jobs/<id>`

Get the snapshot for a single job. Returns HTTP 404 if not found.

### `GET /api/jobs/<id>/image`

Return the current render as a PNG image. Serves a partial/progressive result if the render is still running.

| Parameter | Notes |
|-----------|-------|
| `final` | `1` = only return the image if the render is complete |
| Tonemapping params | See below |
| `post_filters_enabled` | `0`/`false` or `1`/`true` |
| `post_filters` | Filter pipeline string |

### `GET /api/jobs/<id>/export`

Download the completed render in the requested format. Returns a file with a content-disposition attachment header.

| Parameter | Notes |
|-----------|-------|
| `format` | `png` (default), `exr`, `hdr`, `jpg`, `bmp`, `tga` |
| `post_filters_enabled` | `0`/`false` or `1`/`true` |
| `post_filters` | Filter pipeline string |

### `PUT /api/jobs/<id>/live-tm`

Update the live tonemapping settings for a running or completed job. Takes effect immediately for the `/ws/jobs/<id>` tile stream. Accepts [tonemapping parameters](#tonemapping-parameters).

### `GET /api/jobs/<id>/photons`

Return photon map points for `photon_mapping` renders.

| Parameter | Notes |
|-----------|-------|
| `limit` | int [1, 500000]; default `100000` — cap on points per set |

Returns `{"diffuse":[[x,y,z],...], "caustic":[[x,y,z],...]}`.

### `POST /api/jobs/abort/<id>`

Abort a running or queued job. Requires `client_id` and `workspace_id` matching the job's owner.

### `POST /api/jobs/queue/up/<id>`

Move a queued job one position earlier in the queue.

### `POST /api/jobs/queue/down/<id>`

Move a queued job one position later in the queue.

---

## Gallery

Completed renders are stored in the gallery directory. Each entry may have multiple passes (e.g. from progressive rendering).

### `GET /api/gallery`

List all gallery entries.

### `GET /api/gallery/<id>/image`

Return the gallery entry's final image as PNG. Supports tonemapping parameters and `post_filters_enabled`/`post_filters`.

### `GET /api/gallery/<id>/pass/<n>/image`

Return a specific pass image (0-indexed) for a multi-pass gallery entry.

### `GET /api/gallery/<id>/export`

Download the gallery entry in the requested format (`format=png|exr|hdr|jpg|bmp|tga`). Returns an attachment.

### `GET /api/gallery/<id>/pass/<n>/export`

Download a specific pass in the requested format.

### `DELETE /api/gallery/<id>`

Delete a gallery entry.

---

## Logs

### `GET /api/logs`

Return recent backend log entries as JSON.

```json
{
  "entries": [
    {"id": 1, "ts": "2026-04-17T12:00:00Z", "level": "info", "message": "..."}
  ]
}
```

Log levels: `debug`, `info`, `warn`, `error`.

---

## WebSocket Endpoints

### `WS /ws/jobs`

Job list event stream. No authentication required.

**On connect:** immediately receives the current active job list.

**Messages received (text, JSON):**

```json
{"type": "jobs_changed", "jobs": [...]}
```

Each entry in `jobs` contains the same fields as the `GET /api/jobs/active` snapshot, except `active_tiles`. Sent on connect, on any job state change, and every second as a heartbeat.

### `WS /ws/jobs/<id>`

Per-job tile stream and status for a specific job.

**On connect:**
- Receives a full-frame catchup as a binary XTDR packet (if tiles have been rendered).
- Receives the current job snapshot as a text JSON message.

**Binary messages (XTDR format):**

Each binary message is an XTDR packet containing one or more rendered tiles.

```
Offset  Size  Field
0       4     Magic: 'X' 'T' 'D' 'R'
4       4     Canvas width (u32 LE)
8       4     Canvas height (u32 LE)
12      4     tiles_done (u32 LE)
16      4     tiles_total (u32 LE)
20      4     job_state (u32 LE)
24      4     tile_count (u32 LE)
28      4     elapsed_ms (u32 LE)

Per tile (repeated tile_count times):
  0     4     x0 (u32 LE)
  4     4     y0 (u32 LE)
  8     4     x1 (u32 LE)
  12    4     y1 (u32 LE)
  16    4     done_index (u32 LE)
  20    4     data_size (u32 LE)
  24    N     raw RGBA u8 data (width*height*4 bytes, sRGB, alpha=255)

After all tiles:
  0     4     active_tile_count (u32 LE)
  (active tiles section is present but currently always 0)
```

Tile pixel data is raw RGBA (not PNG-encoded) for direct use with `putImageData`. Tonemapping is applied server-side with the job's current live-TM settings.

**Text messages (JSON):** job state snapshots, sent on terminal state transitions (`done`, `aborted`, `error`).

**Sending to the server (text):** send any JSON containing `"abort"` to request job cancellation.

### `WS /ws/logs`

Backend log stream.

**Query parameter:** `since=<id>` — receive only log entries with id > `since`. Entries are replayed on connect, then pushed live.

**Messages (text, JSON):**

```json
{"entries": [{"id": 5, "ts": "...", "level": "info", "message": "..."}]}
```

---

## Tonemapping Parameters

These parameters are accepted by `/api/render`, `PUT /api/jobs/<id>/live-tm`, `/api/jobs/<id>/image`, and gallery image endpoints.

| Parameter | Type | Notes |
|-----------|------|-------|
| `tm` | string | Operator. One of: `aces`, `reinhard`, `reinhard_luma`, `mantiuk_2006`, `hable`, `exponential`, `lottes`, `cineon`, `uchimura`, `agx`, `khronos_pbr`, `none` |
| `tm_exposure` | float > 0 | Exposure multiplier |
| `tm_white_point` | float > 0 | White point |
| `tm_mantiuk_contrast` | float [0, 1] | Mantiuk 2006 contrast (default `0.1`) |
| `tm_mantiuk_saturation` | float [0, 2] | Mantiuk 2006 saturation (default `0.8`) |
| `tm_mantiuk_detail` | float [1, 99] | Mantiuk 2006 detail (default `1.0`) |
