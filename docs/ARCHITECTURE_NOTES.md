# Architecture Notes

## High-Level Flow

1. Parse scene (`.scn`) into `xtcore::Scene`.
2. Build render context (`xtcore::render::context_t`) with tiles.
3. Select integrator (`xtcore::render::IIntegrator` implementation).
4. Render tiles (OpenMP parallel path in base integrator flow).
5. Assemble output framebuffer for export/preview.

## Main Components

- Scene parsing/loading:
  - `src/xtcore/parseutil.h`
  - `src/xtcore/parseutil.cc`
- Render context and tile assembly:
  - `src/xtcore/context.h`
  - `src/xtcore/context.cc`
- Integrator interface:
  - `src/xtcore/integrator.h`
  - `src/xtcore/integrator.cc`

## Frontends

- CLI:
  - Entrypoint: `src/frontend/cli/xtracer.cc`
  - Arg parsing: `src/frontend/cli/argparse.cc`
- Web:
  - Entrypoint: `src/frontend/web-server/main.cc`
  - HTTP + WebSocket routing/API: `src/frontend/web-server/routes.cc`
  - Job lifecycle: `src/frontend/web-server/job_manager.cc`
  - WebSocket pub/sub hub: `src/frontend/web-server/ws_hub.h`

## Web Streaming Architecture

The web frontend uses WebSockets for all hot-path server push, replacing the legacy HTTP polling loop.

```
Render thread
  │  PROGRESS_EVENT_TILE_STARTED / _FINISHED
  ▼
job_manager_t::run()   ←── push_callback_ set by setup_routes()
  │  Builds XTDR binary packet
  │  (tile pixel data + active-tile list)
  ▼
job_ws_hub_t::broadcast_binary()   (ws_hub.h)
  │  Mutex-protected fan-out to all connected WS clients
  ▼
/ws/jobs/<id>  ──binary──►  render.js watchJobViaWebSocket()
                                 │  parseImageDeltaPacket()
                                 │  updateActivePreviewTilesFromJob()  ← tile markers
                                 └► drawDeltaTilesToPreviewCanvas()    ← putImageData
                                      │
                                      └► drawPreviewCanvas()           ← visible canvas
```

Terminal state (done/aborted/error) is sent as a text JSON snapshot over the same connection.

Log streaming follows a similar pattern via `log_ws_hub_t` on `/ws/logs`.

**Design invariants to preserve:**
- The push callback fires on both `TILE_STARTED` (tile_count=0) and `TILE_FINISHED` (tile_count=1).
  Changing this breaks active-tile overlay updates.
- `broadcast_text` is intentionally **not** called per tile — it would re-serialize and send a full JSON
  snapshot on every tile, adding latency and redundant DOM updates. Text is sent only for terminal state.
- `tileAccumCanvas` in the browser is the persistent accumulation buffer. Any code path that calls
  `setPreviewFromBlob()` during an active render (e.g., tonemapping refresh) must guard against clearing
  it, or tiles will flash to black. See `refreshPreviewForToneMapping()` in `preview.js`.
- Crow void-return route handlers must call `res.end()` explicitly (return-type handlers do not need it).

**XTDR packet format** is documented in full in `AGENTS.md § WebSocket Protocol` and inline in:
- `src/frontend/web-server/job_manager.cc` (builder)
- `src/frontend/web-client/app/preview.js` (`parseImageDeltaPacket`)
