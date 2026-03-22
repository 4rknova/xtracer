"use strict";

const jobs = new Map();
let nextJobId = 1;
let backendEngine = "proxy-http";
let engineInitDone = false;
let wasmFns = null;
let wasmInitPromise = null;

function postLog(level, message) {
  self.postMessage({
    type: "log",
    level: level || "info",
    message: message || "",
  });
}

function responseOk(requestId, result, transfer) {
  const msg = {
    type: "response",
    request_id: requestId,
    ok: true,
    result: result || {},
  };
  if (transfer && transfer.length) self.postMessage(msg, transfer);
  else self.postMessage(msg);
}

function responseErr(requestId, error) {
  self.postMessage({
    type: "response",
    request_id: requestId,
    ok: false,
    error: error || "request failed",
  });
}

function createJob(params) {
  const id = `wasm_${String(nextJobId).padStart(6, "0")}`;
  nextJobId += 1;
  const now = Date.now();
  const job = {
    id,
    scene: params.scene || "",
    integrator: params.integrator || "",
    state: "queued",
    progress: 0,
    elapsed_ms: 0,
    error: "",
    started_at_ms: now,
    has_image: false,
    image_bytes: null,
    export_cache: {},
    remote_job_id: "",
  };
  jobs.set(id, job);
  return job;
}

function toPublicJob(job) {
  return {
    id: job.id,
    scene: job.scene,
    integrator: job.integrator,
    state: job.state,
    progress: job.progress,
    elapsed_ms: job.elapsed_ms,
    has_image: !!job.has_image,
    error: job.error || "",
  };
}

function clampProgress(v) {
  if (!Number.isFinite(v)) return 0;
  if (v < 0) return 0;
  if (v > 1) return 1;
  return v;
}

function toneMappingControlSpec(opRaw) {
  const op = String(opRaw || "").toLowerCase();
  if (op === "none") return { usesExposure: false, usesWhitePoint: false, usesMantiuk: false };
  if (op === "aces") return { usesExposure: true, usesWhitePoint: false, usesMantiuk: false };
  if (op === "mantiuk_2006") return { usesExposure: false, usesWhitePoint: false, usesMantiuk: true };
  if (op === "reinhard" || op === "reinhard_luma") {
    return { usesExposure: true, usesWhitePoint: true, usesMantiuk: false };
  }
  return { usesExposure: true, usesWhitePoint: false, usesMantiuk: false };
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function fetchJson(url, options) {
  const res = await fetch(url, options);
  const data = await res.json();
  if (!res.ok) {
    throw new Error((data && data.error) || `HTTP ${res.status}`);
  }
  return data;
}

function loadWasmModule() {
  if (wasmInitPromise) return wasmInitPromise;

  wasmInitPromise = new Promise((resolve, reject) => {
    const timeoutId = setTimeout(() => {
      reject(new Error("wasm runtime init timeout"));
    }, 10000);

    const moduleRef = self.Module || {};
    moduleRef.locateFile = (path) => `/${path}`;
    const prevInit = moduleRef.onRuntimeInitialized;
    moduleRef.onRuntimeInitialized = () => {
      clearTimeout(timeoutId);
      if (typeof prevInit === "function") prevInit();
      resolve(moduleRef);
    };
    self.Module = moduleRef;

    try {
      self.importScripts("/xtracer_wasm.js");
      if (moduleRef.calledRun || moduleRef.runtimeInitialized) {
        clearTimeout(timeoutId);
        resolve(moduleRef);
      }
    } catch (err) {
      clearTimeout(timeoutId);
      reject(err);
    }
  });

  return wasmInitPromise;
}

async function initWasmFunctions() {
  if (wasmFns) return wasmFns;

  const mod = await loadWasmModule();
  if (typeof mod.cwrap !== "function") throw new Error("cwrap is unavailable");

  wasmFns = {
    module: mod,
    renderPng: mod.cwrap("xtracer_wasm_render_png", "number", [
      "string",
      "string",
      "string",
      "string",
      "number",
      "number",
      "number",
      "number",
      "number",
      "number",
      "number",
      "number",
    ]),
    renderBegin: mod.cwrap("xtracer_wasm_render_begin", "number", [
      "string",
      "string",
      "string",
      "string",
      "number",
      "number",
      "number",
      "number",
      "number",
      "number",
      "number",
    ]),
    renderStep: mod.cwrap("xtracer_wasm_render_step", "number", ["number"]),
    renderTilesDone: mod.cwrap("xtracer_wasm_render_tiles_done", "number", []),
    renderTilesTotal: mod.cwrap("xtracer_wasm_render_tiles_total", "number", []),
    renderIsDone: mod.cwrap("xtracer_wasm_render_is_done", "number", []),
    renderSnapshotPng: mod.cwrap("xtracer_wasm_render_snapshot_png", "number", ["number", "number"]),
    renderSnapshotExr: mod.cwrap("xtracer_wasm_render_snapshot_exr", "number", ["number", "number"]),
    renderSnapshotHdr: mod.cwrap("xtracer_wasm_render_snapshot_hdr", "number", ["number", "number"]),
    freeBuffer: mod.cwrap("xtracer_wasm_free", null, ["number"]),
    getLastError: mod.cwrap("xtracer_wasm_get_last_error", "string", []),
  };

  return wasmFns;
}

async function maybeInitEngine() {
  if (engineInitDone) return;
  engineInitDone = true;

  try {
    await initWasmFunctions();
    backendEngine = "wasm";
    postLog("info", "native wasm runtime initialized");
  } catch (err) {
    backendEngine = "proxy-http";
    postLog("warn", `native wasm unavailable (${err.message}); using proxy-http`);
  }
}

async function ensureSceneInWasmFs(mod, sceneName, sceneSource) {
  if (!sceneName) throw new Error("scene is required");

  const fs = mod.FS;
  const outPath = `/scenes/${sceneName}`;

  try {
    fs.mkdir("/scenes");
  } catch (_) {
    // directory already exists
  }

  if (sceneSource && String(sceneSource).length > 0) {
    fs.writeFile(outPath, String(sceneSource), { encoding: "utf8" });
    return outPath;
  }

  try {
    const source = await fetch(`/scenes/${encodeURIComponent(sceneName)}`, { cache: "no-store" }).then((r) => {
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      return r.text();
    });
    fs.writeFile(outPath, source, { encoding: "utf8" });
    return outPath;
  } catch (_) {
    // fallback below
  }

  try {
    const data = await fetchJson(`/api/scenes/${encodeURIComponent(sceneName)}/source`);
    const source = (data && data.source) || "";
    fs.writeFile(outPath, source, { encoding: "utf8" });
    return outPath;
  } catch (_) {
    throw new Error(`scene source not found: ${sceneName}`);
  }
}

async function runProxyRender(job, params) {
  const body = new URLSearchParams();
  Object.keys(params || {}).forEach((k) => {
    const v = params[k];
    if (v !== undefined && v !== null && v !== "") body.set(k, String(v));
  });

  const accepted = await fetchJson("/api/render", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: body.toString(),
  });
  job.remote_job_id = accepted.job_id || "";
  if (!job.remote_job_id) throw new Error("remote job id missing");
  postLog("info", `proxy render accepted remote job ${job.remote_job_id}`);

  while (true) {
    const snap = await fetchJson(`/api/jobs/${job.remote_job_id}`);
    job.state = snap.state || "running";
    job.progress = clampProgress(Number(snap.progress));
    job.elapsed_ms = Math.max(0, Number(snap.elapsed_ms) || 0);
    job.error = snap.error || "";

    try {
      const partial = await fetch(`/api/jobs/${job.remote_job_id}/image?partial=1&t=${Date.now()}`);
      if (partial.ok) {
        const buf = await partial.arrayBuffer();
        if (buf.byteLength > 0) {
          job.image_bytes = buf;
          job.has_image = true;
        }
      }
    } catch (_) {
      // progressive image is best effort
    }

    if (job.state === "done") {
      const finalRes = await fetch(`/api/jobs/${job.remote_job_id}/image?final=1&t=${Date.now()}`);
      if (!finalRes.ok) throw new Error(`failed to fetch final image: HTTP ${finalRes.status}`);
      const finalBuf = await finalRes.arrayBuffer();
      if (finalBuf.byteLength > 0) {
        job.image_bytes = finalBuf;
        job.export_cache.png = finalBuf;
        job.has_image = true;
      }
      job.progress = 1;
      return;
    }

    if (job.state === "error") {
      throw new Error(job.error || "render failed");
    }

    await sleep(150);
  }
}

async function runWasmRender(job, params) {
  const fns = await initWasmFunctions();
  const mod = fns.module;

  function snapshotPng(finalOnly) {
    const outSizePtr = mod._malloc(4);
    let pngPtr = 0;
    try {
      pngPtr = fns.renderSnapshotPng(finalOnly ? 1 : 0, outSizePtr);
      const size = mod.HEAP32[outSizePtr >> 2] | 0;
      if (!pngPtr || size <= 0) return null;
      const bytes = new Uint8Array(size);
      bytes.set(mod.HEAPU8.subarray(pngPtr, pngPtr + size));
      return bytes.buffer;
    } finally {
      if (pngPtr) fns.freeBuffer(pngPtr);
      mod._free(outSizePtr);
    }
  }

  const scenePath = await ensureSceneInWasmFs(mod, params.scene || "", params.scene_source || "");
  const ok = fns.renderBegin(
    scenePath,
    params.integrator || "pathtracer",
    params.camera || "",
    params.variant || "",
    Number(params.width) || 500,
    Number(params.height) || 500,
    Number(params.samples) || 1,
    Number(params.aa) || 1,
    Number(params.tile_size) || 32,
    Number(params.threads) || 0,
    Number(params.rdepth) || 10,
  );
  if (!ok) {
    throw new Error(fns.getLastError() || "wasm render init failed");
  }

  let totalTiles = Number(fns.renderTilesTotal()) || 0;
  if (totalTiles <= 0) totalTiles = 1;
  let lastPreviewAt = 0;

  while (!fns.renderIsDone()) {
    const stepped = fns.renderStep(1);
    if (stepped < 0) {
      throw new Error(fns.getLastError() || "wasm render step failed");
    }

    const doneTiles = Math.max(0, Number(fns.renderTilesDone()) || 0);
    totalTiles = Math.max(totalTiles, Number(fns.renderTilesTotal()) || 0, 1);
    job.progress = clampProgress(doneTiles / totalTiles);
    job.elapsed_ms = Date.now() - job.started_at_ms;

    const now = Date.now();
    if (now - lastPreviewAt >= 160 || job.progress >= 1) {
      const partialPng = snapshotPng(false);
      if (partialPng && partialPng.byteLength > 0) {
        job.image_bytes = partialPng;
        job.has_image = true;
      }
      lastPreviewAt = now;
    }

    await sleep(0);
  }

  const finalPng = snapshotPng(true);
  if (finalPng && finalPng.byteLength > 0) {
    job.image_bytes = finalPng;
    job.export_cache.png = finalPng;
    job.has_image = true;
  }
  job.progress = 1;
  job.elapsed_ms = Date.now() - job.started_at_ms;
}

async function runJob(job, params) {
  job.state = "running";
  job.progress = 0;
  job.error = "";
  job.started_at_ms = Date.now();

  try {
    await maybeInitEngine();
    if (backendEngine === "wasm") await runWasmRender(job, params);
    else await runProxyRender(job, params);

    if (job.state !== "error") {
      job.state = "done";
      job.progress = 1;
      job.elapsed_ms = Math.max(job.elapsed_ms, Date.now() - job.started_at_ms);
      postLog("info", `job ${job.id} completed`);
    }
  } catch (err) {
    job.state = "error";
    job.error = err && err.message ? err.message : "render failed";
    job.elapsed_ms = Math.max(job.elapsed_ms, Date.now() - job.started_at_ms);
    postLog("error", `job ${job.id} failed: ${job.error}`);
    if (err && err.stack) postLog("error", err.stack);
  }
}

async function handleStartRender(payload) {
  const params = payload && payload.params ? payload.params : {};
  const job = createJob(params);
  runJob(job, params);
  return { job_id: job.id };
}

function handleGetJob(payload) {
  const id = payload && payload.job_id ? String(payload.job_id) : "";
  const job = jobs.get(id);
  if (!job) throw new Error("job not found");
  return toPublicJob(job);
}

async function handleGetJobImage(payload) {
  const id = payload && payload.job_id ? String(payload.job_id) : "";
  const job = jobs.get(id);
  if (!job) {
    return { bytes: new ArrayBuffer(0), mime: "image/png" };
  }

  const opts = (payload && payload.opts) ? payload.opts : {};
  if (backendEngine === "proxy-http" && job.remote_job_id) {
    const parts = [];
    if (opts && opts.partial) parts.push("partial=1");
    if (opts && opts.final) parts.push("final=1");
    const tm = String((opts && opts.toneMapping) || "aces").toLowerCase();
    if (tm === "aces" || tm === "reinhard" || tm === "reinhard_luma" || tm === "mantiuk_2006" || tm === "none") {
      parts.push(`tm=${encodeURIComponent(tm)}`);
    } else {
      parts.push("tm=aces");
    }
    const spec = toneMappingControlSpec(tm);
    const exposure = Number((opts && opts.toneMappingExposure) || 1.0);
    const whitePoint = Number((opts && opts.toneMappingWhitePoint) || 1.0);
    const mantiukContrast = Number((opts && opts.toneMappingMantiukContrast) || 0.1);
    const mantiukSaturation = Number((opts && opts.toneMappingMantiukSaturation) || 0.8);
    const mantiukDetail = Number((opts && opts.toneMappingMantiukDetail) || 1.0);
    const effectiveExposure = spec.usesExposure
      ? (Number.isFinite(exposure) && exposure > 0 ? exposure : 1.0)
      : 1.0;
    const effectiveWhitePoint = spec.usesWhitePoint
      ? (Number.isFinite(whitePoint) && whitePoint > 0 ? whitePoint : 1.0)
      : 1.0;
    const effectiveMantiukContrast = spec.usesMantiuk
      ? (Number.isFinite(mantiukContrast) ? Math.min(1.0, Math.max(0.0, mantiukContrast)) : 0.1)
      : 0.1;
    const effectiveMantiukSaturation = spec.usesMantiuk
      ? (Number.isFinite(mantiukSaturation) ? Math.min(2.0, Math.max(0.0, mantiukSaturation)) : 0.8)
      : 0.8;
    const effectiveMantiukDetail = spec.usesMantiuk
      ? (Number.isFinite(mantiukDetail) ? Math.min(99.0, Math.max(1.0, mantiukDetail)) : 1.0)
      : 1.0;
    parts.push(`tm_exposure=${encodeURIComponent(effectiveExposure)}`);
    parts.push(`tm_white_point=${encodeURIComponent(effectiveWhitePoint)}`);
    parts.push(`tm_mantiuk_contrast=${encodeURIComponent(effectiveMantiukContrast)}`);
    parts.push(`tm_mantiuk_saturation=${encodeURIComponent(effectiveMantiukSaturation)}`);
    parts.push(`tm_mantiuk_detail=${encodeURIComponent(effectiveMantiukDetail)}`);
    parts.push(`t=${Date.now()}`);
    const qs = parts.length ? `?${parts.join("&")}` : "";
    const res = await fetch(`/api/jobs/${encodeURIComponent(job.remote_job_id)}/image${qs}`);
    if (!res.ok) return { bytes: new ArrayBuffer(0), mime: "image/png" };
    const buf = await res.arrayBuffer();
    if (buf && buf.byteLength > 0) job.image_bytes = buf;
  }

  if (!job.image_bytes || !job.image_bytes.byteLength) {
    return { bytes: new ArrayBuffer(0), mime: "image/png" };
  }
  const copy = job.image_bytes.slice(0);
  return {
    bytes: copy,
    mime: "image/png",
  };
}

function exportMimeForFormat(format) {
  if (format === "exr") return "image/x-exr";
  if (format === "hdr") return "image/vnd.radiance";
  return "image/png";
}

async function handleGetJobExport(payload) {
  const id = payload && payload.job_id ? String(payload.job_id) : "";
  const rawFormat = payload && payload.format ? String(payload.format) : "png";
  const format = rawFormat.toLowerCase();
  if (format !== "png" && format !== "exr" && format !== "hdr") {
    throw new Error("unsupported format");
  }

  const job = jobs.get(id);
  if (!job) throw new Error("job not found");
  if (job.state !== "done") throw new Error("export not available");

  const cached = job.export_cache && job.export_cache[format];
  if (cached && cached.byteLength > 0) {
    return { bytes: cached.slice(0), mime: exportMimeForFormat(format) };
  }

  if (backendEngine === "proxy-http") {
    if (!job.remote_job_id) throw new Error("remote job id missing");
    const res = await fetch(`/api/jobs/${encodeURIComponent(job.remote_job_id)}/export?format=${encodeURIComponent(format)}&t=${Date.now()}`);
    if (!res.ok) throw new Error(`export failed: HTTP ${res.status}`);
    const bytes = await res.arrayBuffer();
    if (!bytes || bytes.byteLength <= 0) throw new Error("empty export payload");
    job.export_cache[format] = bytes;
    return { bytes: bytes.slice(0), mime: exportMimeForFormat(format) };
  }

  const fns = await initWasmFunctions();
  const mod = fns.module;
  const outSizePtr = mod._malloc(4);
  let imgPtr = 0;
  try {
    if (format === "png") imgPtr = fns.renderSnapshotPng(1, outSizePtr);
    else if (format === "exr") imgPtr = fns.renderSnapshotExr(1, outSizePtr);
    else imgPtr = fns.renderSnapshotHdr(1, outSizePtr);

    const size = mod.HEAP32[outSizePtr >> 2] | 0;
    if (!imgPtr || size <= 0) {
      throw new Error(fns.getLastError() || "export failed");
    }

    const copy = new Uint8Array(size);
    copy.set(mod.HEAPU8.subarray(imgPtr, imgPtr + size));
    const bytes = copy.buffer;
    job.export_cache[format] = bytes;
    return { bytes: bytes.slice(0), mime: exportMimeForFormat(format) };
  } finally {
    if (imgPtr) fns.freeBuffer(imgPtr);
    mod._free(outSizePtr);
  }
}

self.onmessage = async (ev) => {
  const msg = ev.data || {};
  if (msg.type !== "request") return;

  const requestId = msg.request_id;
  const op = msg.op || "";
  const payload = msg.payload || {};

  try {
    if (op === "startRender") {
      const result = await handleStartRender(payload);
      responseOk(requestId, result);
      return;
    }
    if (op === "getJob") {
      responseOk(requestId, handleGetJob(payload));
      return;
    }
    if (op === "getJobImage") {
      const result = await handleGetJobImage(payload);
      responseOk(requestId, result, [result.bytes]);
      return;
    }
    if (op === "getJobExport") {
      const result = await handleGetJobExport(payload);
      responseOk(requestId, result, [result.bytes]);
      return;
    }
    throw new Error(`unsupported op: ${op}`);
  } catch (err) {
    responseErr(requestId, err && err.message ? err.message : "request failed");
  }
};

postLog("info", "wasm worker online");
