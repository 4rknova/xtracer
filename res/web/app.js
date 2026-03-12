const $ = (id) => document.getElementById(id);

const el = {
  tabRender: $("tabRender"),
  tabVisual: $("tabVisual"),
  tabEditor: $("tabEditor"),
  tabSettings: $("tabSettings"),
  tabLogs: $("tabLogs"),
  tabAbout: $("tabAbout"),
  paneRender: $("paneRender"),
  paneVisual: $("paneVisual"),
  paneEditor: $("paneEditor"),
  paneSettings: $("paneSettings"),
  paneLogs: $("paneLogs"),
  paneAbout: $("paneAbout"),
  theme: $("theme"),
  pollInterval: $("pollInterval"),
  autoLoadEditor: $("autoLoadEditor"),
  autoScrollLogs: $("autoScrollLogs"),
  fontSizeDown: $("fontSizeDown"),
  fontSizeUp: $("fontSizeUp"),
  fontSizeValue: $("fontSizeValue"),
  clearLogsBtn: $("clearLogsBtn"),
  logOutput: $("logOutput"),
  aboutVersionRow: $("aboutVersionRow"),
  aboutVersion: $("aboutVersion"),
  aboutHomepage: $("aboutHomepage"),
  aboutWebsite: $("aboutWebsite"),
  aboutCopyright: $("aboutCopyright"),
  aboutLicense: $("aboutLicense"),
  aboutBuildPill: $("aboutBuildPill"),
  aboutBackend: $("aboutBackend"),
  aboutDefaultUrl: $("aboutDefaultUrl"),
  aboutSceneDir: $("aboutSceneDir"),
  aboutStaticAssets: $("aboutStaticAssets"),
  sceneDependencyPill: $("sceneDependencyPill"),
  scene: $("scene"),
  camera: $("camera"),
  integrator: $("integrator"),
  integratorControlsSection: $("integratorControlsSection"),
  integratorControls: $("integratorControls"),
  resolutionPreset: $("resolutionPreset"),
  width: $("width"),
  height: $("height"),
  samples: $("samples"),
  aa: $("aa"),
  rdepth: $("rdepth"),
  tileSize: $("tile_size"),
  tileOrder: $("tile_order"),
  threads: $("threads"),
  toneMapping: $("toneMapping"),
  toneMappingParamsRow: $("toneMappingParamsRow"),
  toneMappingExposureControl: $("toneMappingExposureControl"),
  toneMappingExposure: $("toneMappingExposure"),
  toneMappingWhitePointControl: $("toneMappingWhitePointControl"),
  toneMappingWhitePoint: $("toneMappingWhitePoint"),
  toneMappingMantiukContrastControl: $("toneMappingMantiukContrastControl"),
  toneMappingMantiukContrast: $("toneMappingMantiukContrast"),
  toneMappingMantiukSaturationControl: $("toneMappingMantiukSaturationControl"),
  toneMappingMantiukSaturation: $("toneMappingMantiukSaturation"),
  toneMappingMantiukDetailControl: $("toneMappingMantiukDetailControl"),
  toneMappingMantiukDetail: $("toneMappingMantiukDetail"),
  clearPreviewOnRender: $("clearPreviewOnRender"),
  renderBtn: $("renderBtn"),
  status: $("status"),
  renderTimer: $("renderTimer"),
  progressBar: $("progressBar"),
  progress: $("progress"),
  previewFrame: $("previewFrame"),
  previewEmpty: $("previewEmpty"),
  previewCanvas: $("previewCanvas"),
  preview: $("preview"),
  resetViewBtn: $("resetViewBtn"),
  previewSampling: $("previewSampling"),
  exportFormat: $("exportFormat"),
  download: $("download"),
  sceneName: $("sceneName"),
  lineNumbers: $("lineNumbers"),
  lineCount: $("lineCount"),
  charCount: $("charCount"),
  sceneSource: $("sceneSource"),
  loadSceneBtn: $("loadSceneBtn"),
  newSceneBtn: $("newSceneBtn"),
  saveSceneBtn: $("saveSceneBtn"),
  visualLoadBtn: $("visualLoadBtn"),
  visualCamera: $("visualCamera"),
  visualStatus: $("visualStatus"),
  visualShowGrid: $("visualShowGrid"),
  visualViewport: $("visualViewport"),
};

const uiOptions = {
  pollMs: 300,
  autoLoadEditor: true,
  autoScrollLogs: true,
  clearPreviewOnRender: false,
  previewSampling: "smooth",
  fontScale: 1.0,
};
let resolutionPresets = [];
let sceneDependencyByFile = new Map();
let lastBackendLogId = 0;
let previewObjectUrl = "";
let previewPendingRevokeUrl = "";
let previewPinnedBaseUrl = "";
let previewPinnedBaseBitmapPromise = null;
let preservePreviewUnderlay = false;
let pendingLogScroll = false;
let renderActive = false;
let renderStartMs = 0;
let renderTimerInterval = null;
let activeJobId = "";
let lastCompletedJobId = "";
let lastCompletedJobScene = "";
let lastCompletedJobIntegrator = "";
let integratorCatalog = [];
let integratorById = new Map();
const integratorControlState = new Map();
const BACKEND_MODE_KEY = "xtracer-backend-mode";
let api = null;
let backendMode = "server";
let visualEditor = null;
let visualLoadedSceneName = "";
const previewView = {
  scale: 1,
  tx: 0,
  ty: 0,
  minScale: 1,
  maxScale: 12,
  panning: false,
  pointerId: null,
  lastX: 0,
  lastY: 0,
};

function nowStamp() {
  const d = new Date();
  return d.toLocaleTimeString();
}

function scrollLogToBottom(force) {
  if (!el.logOutput) return;
  if (!uiOptions.autoScrollLogs && !force) return;

  const logsVisible = el.paneLogs && el.paneLogs.classList.contains("active");
  if (!logsVisible && !force) {
    pendingLogScroll = true;
    return;
  }

  pendingLogScroll = false;
  requestAnimationFrame(() => {
    el.logOutput.scrollTop = el.logOutput.scrollHeight;
  });
}

function appendLog(message) {
  if (!message) return;
  el.logOutput.textContent += `[UI ${nowStamp()}] ${message}\n`;
  scrollLogToBottom(false);
}

function appendBackendLog(entry) {
  const id = entry.id || 0;
  const ts = entry.ts || "";
  const level = (entry.level || "info").toUpperCase();
  const msg = entry.message || "";
  el.logOutput.textContent += `#${id} ${ts} ${level} ${msg}\n`;
  scrollLogToBottom(false);
}

function setStatus(text) {
  const statusText = renderActive ? text : "Idle";
  const lower = String(statusText || "").toLowerCase();
  let state = "idle";
  if (renderActive) {
    if (lower.indexOf("error") >= 0) state = "error";
    else state = "running";
  }
  el.status.textContent = statusText;
  el.status.classList.remove("is-idle", "is-running", "is-error");
  el.status.classList.add(`is-${state}`);
}

function formatElapsed(ms) {
  const totalSeconds = Math.max(0, Math.floor((ms || 0) / 1000));
  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;
  if (hours > 0) {
    return `${String(hours).padStart(2, "0")}:${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
  }
  return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
}

function updateRenderTimer() {
  if (!el.renderTimer) return;
  if (!renderActive || renderStartMs <= 0) {
    el.renderTimer.hidden = true;
    el.renderTimer.textContent = "00:00";
    return;
  }
  el.renderTimer.hidden = false;
  el.renderTimer.textContent = formatElapsed(Date.now() - renderStartMs);
}

function setRenderActive(active) {
  renderActive = !!active;
  if (renderActive) {
    renderStartMs = Date.now();
    if (renderTimerInterval) clearInterval(renderTimerInterval);
    renderTimerInterval = setInterval(updateRenderTimer, 250);
  } else {
    renderStartMs = 0;
    if (renderTimerInterval) {
      clearInterval(renderTimerInterval);
      renderTimerInterval = null;
    }
  }
  updateRenderTimer();
  if (el.progressBar) el.progressBar.hidden = !renderActive;
  if (!renderActive) {
    el.progress.style.width = "0%";
    setStatus("Idle");
  }
}

async function getJSON(url) {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}

function createHttpError(status, message) {
  const err = new Error(message || `HTTP ${status}`);
  err.status = status;
  return err;
}

function blobUrlForJobImage(jobId, opts) {
  const parts = [];
  if (opts && opts.partial) parts.push("partial=1");
  if (opts && opts.final) parts.push("final=1");
  const tm = String((opts && opts.toneMapping) || (el.toneMapping && el.toneMapping.value) || "aces").toLowerCase();
  if (tm === "aces" || tm === "reinhard" || tm === "reinhard_luma" || tm === "mantiuk_2006" || tm === "none") {
    parts.push(`tm=${encodeURIComponent(tm)}`);
  } else {
    parts.push("tm=aces");
  }
  const spec = toneMappingControlSpec(tm);
  const tmExposureRaw = String((opts && opts.toneMappingExposure)
    || (el.toneMappingExposure && el.toneMappingExposure.value)
    || "1.0");
  const tmWhitePointRaw = String((opts && opts.toneMappingWhitePoint)
    || (el.toneMappingWhitePoint && el.toneMappingWhitePoint.value)
    || "1.0");
  const tmMantiukContrastRaw = String((opts && opts.toneMappingMantiukContrast)
    || (el.toneMappingMantiukContrast && el.toneMappingMantiukContrast.value)
    || "0.1");
  const tmMantiukSaturationRaw = String((opts && opts.toneMappingMantiukSaturation)
    || (el.toneMappingMantiukSaturation && el.toneMappingMantiukSaturation.value)
    || "0.8");
  const tmMantiukDetailRaw = String((opts && opts.toneMappingMantiukDetail)
    || (el.toneMappingMantiukDetail && el.toneMappingMantiukDetail.value)
    || "1.0");
  const tmExposure = Number(tmExposureRaw);
  const tmWhitePoint = Number(tmWhitePointRaw);
  const tmMantiukContrast = Number(tmMantiukContrastRaw);
  const tmMantiukSaturation = Number(tmMantiukSaturationRaw);
  const tmMantiukDetail = Number(tmMantiukDetailRaw);
  const effectiveExposure = spec.usesExposure
    ? (Number.isFinite(tmExposure) && tmExposure > 0 ? tmExposure : 1.0)
    : 1.0;
  parts.push(`tm_exposure=${encodeURIComponent(effectiveExposure)}`);
  const effectiveWhitePoint = spec.usesWhitePoint
    ? (Number.isFinite(tmWhitePoint) && tmWhitePoint > 0 ? tmWhitePoint : 1.0)
    : 1.0;
  parts.push(`tm_white_point=${encodeURIComponent(effectiveWhitePoint)}`);
  const effectiveMantiukContrast = spec.usesMantiuk
    ? (Number.isFinite(tmMantiukContrast) ? clamp(tmMantiukContrast, 0.0, 1.0) : 0.1)
    : 0.1;
  const effectiveMantiukSaturation = spec.usesMantiuk
    ? (Number.isFinite(tmMantiukSaturation) ? clamp(tmMantiukSaturation, 0.0, 2.0) : 0.8)
    : 0.8;
  const effectiveMantiukDetail = spec.usesMantiuk
    ? (Number.isFinite(tmMantiukDetail) ? clamp(tmMantiukDetail, 1.0, 99.0) : 1.0)
    : 1.0;
  parts.push(`tm_mantiuk_contrast=${encodeURIComponent(effectiveMantiukContrast)}`);
  parts.push(`tm_mantiuk_saturation=${encodeURIComponent(effectiveMantiukSaturation)}`);
  parts.push(`tm_mantiuk_detail=${encodeURIComponent(effectiveMantiukDetail)}`);
  if (opts && opts.cacheBust) parts.push(`t=${Date.now()}`);
  const qs = parts.length ? `?${parts.join("&")}` : "";
  return `/api/jobs/${jobId}/image${qs}`;
}

function createServerApi() {
  return {
    mode: "server",
    async getLogsSince(sinceId) {
      const data = await getJSON(`/api/logs?since=${sinceId}`);
      return data.entries || [];
    },
    async getScenes() {
      const data = await getJSON("/api/scenes");
      return data.scenes || [];
    },
    async getCameras(scene) {
      if (!scene) return [];
      const data = await getJSON(`/api/scenes/${encodeURIComponent(scene)}/cameras`);
      return data.cameras || [];
    },
    async getIntegrators() {
      const data = await getJSON("/api/integrators");
      return data.integrators || [];
    },
    async getResolutionPresets() {
      const data = await getJSON("/api/resolutions");
      return data.presets || [];
    },
    async getSceneSource(scene) {
      if (!scene) return { scene: "", source: "" };
      const data = await getJSON(`/api/scenes/${encodeURIComponent(scene)}/source`);
      return { scene: data.scene || scene, source: data.source || "" };
    },
    async getSceneGeometry(scene) {
      if (!scene) return { meshes: {} };
      const data = await getJSON(`/api/scenes/${encodeURIComponent(scene)}/geometry`);
      return { meshes: (data && data.meshes) || {} };
    },
    async getSceneAssetText(scene, relpath) {
      if (!scene || !relpath) return "";
      const url = `/api/scenes/${encodeURIComponent(scene)}/asset?path=${encodeURIComponent(relpath)}`;
      const res = await fetch(url);
      if (!res.ok) throw createHttpError(res.status, `asset fetch failed: ${relpath}`);
      return res.text();
    },
    async getEmptySceneTemplate() {
      const data = await getJSON("/api/scenes/template/empty");
      return data.source || "";
    },
    async getAbout() {
      return getJSON("/api/about");
    },
    async startRender(params) {
      const body = new URLSearchParams();
      Object.keys(params).forEach((k) => {
        const v = params[k];
        if (v !== undefined && v !== null && v !== "") body.set(k, String(v));
      });

      const res = await fetch("/api/render", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });

      const data = await res.json();
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return data.job_id;
    },
    async getJob(jobId) {
      return getJSON(`/api/jobs/${jobId}`);
    },
    async getJobPhotons(jobId, limit) {
      const lim = Number.isFinite(limit) && limit > 0 ? Math.floor(limit) : 100000;
      return getJSON(`/api/jobs/${encodeURIComponent(jobId)}/photons?limit=${encodeURIComponent(lim)}`);
    },
    async getJobImage(jobId, opts) {
      const res = await fetch(blobUrlForJobImage(jobId, opts));
      if (!res.ok) return null;
      return res.blob();
    },
    async getJobExport(jobId, format) {
      const fmt = encodeURIComponent(String(format || "png").toLowerCase());
      const res = await fetch(`/api/jobs/${encodeURIComponent(jobId)}/export?format=${fmt}`);
      if (!res.ok) {
        let message = `HTTP ${res.status}`;
        try {
          const data = await res.json();
          if (data && data.error) message = data.error;
        } catch (_) {
          // keep default message
        }
        throw createHttpError(res.status, message);
      }
      return res.blob();
    },
    async saveScene(name, source, overwrite) {
      const body = new URLSearchParams();
      body.set("name", name);
      body.set("source", source);
      if (overwrite) body.set("overwrite", "1");

      const res = await fetch("/api/scenes/save", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });

      const data = await res.json();
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return data.scene;
    },
  };
}

function hasBackendMethod(candidate, name) {
  return candidate && typeof candidate[name] === "function";
}

function isValidBackendApi(candidate) {
  return hasBackendMethod(candidate, "getLogsSince")
    && hasBackendMethod(candidate, "getScenes")
    && hasBackendMethod(candidate, "getCameras")
    && hasBackendMethod(candidate, "getIntegrators")
    && hasBackendMethod(candidate, "getResolutionPresets")
    && hasBackendMethod(candidate, "getSceneSource")
    && hasBackendMethod(candidate, "getAbout")
    && hasBackendMethod(candidate, "startRender")
    && hasBackendMethod(candidate, "getJob")
    && hasBackendMethod(candidate, "getJobImage")
    && hasBackendMethod(candidate, "saveScene");
}

function createWasmApiOrFallback(serverApi) {
  const factory = window.XTracerWasmAdapter;
  if (typeof factory !== "function") {
    appendLog("wasm backend requested but adapter is not loaded; using server backend");
    return serverApi;
  }

  let wasmApi = null;
  try {
    wasmApi = factory({ serverApi, appendLog });
  } catch (err) {
    appendLog(`wasm backend failed to initialize: ${err.message}`);
    return serverApi;
  }

  if (!isValidBackendApi(wasmApi)) {
    appendLog("wasm backend adapter is invalid; using server backend");
    return serverApi;
  }

  return wasmApi;
}

function detectRequestedBackendMode() {
  const queryMode = new URLSearchParams(window.location.search).get("backend");
  if (queryMode === "server" || queryMode === "wasm") {
    localStorage.setItem(BACKEND_MODE_KEY, queryMode);
    return queryMode;
  }
  // Prefer server by default; only use wasm when explicitly requested.
  return "server";
}

function initializeBackendApi() {
  const serverApi = createServerApi();
  const requested = detectRequestedBackendMode();
  if (requested === "wasm") {
    const resolved = createWasmApiOrFallback(serverApi);
    if (resolved !== serverApi) {
      backendMode = "wasm";
      return resolved;
    }
  }

  backendMode = "server";
  localStorage.setItem(BACKEND_MODE_KEY, "server");
  return serverApi;
}

async function pollBackendLogs() {
  if (!api) return;
  try {
    const entries = await api.getLogsSince(lastBackendLogId);
    for (let i = 0; i < entries.length; i += 1) {
      appendBackendLog(entries[i]);
      if ((entries[i].id || 0) > lastBackendLogId) lastBackendLogId = entries[i].id;
    }
  } catch (err) {
    appendLog(`backend logs unavailable: ${err.message}`);
  } finally {
    setTimeout(pollBackendLogs, Math.max(500, uiOptions.pollMs));
  }
}

function setProgress(value) {
  if (!renderActive) {
    el.progress.style.width = "0%";
    return;
  }
  const p = Math.max(0, Math.min(1, value || 0));
  el.progress.style.width = `${(p * 100).toFixed(1)}%`;
}

function currentRenderSize() {
  const w = parseInt(el.width.value || "0", 10);
  const h = parseInt(el.height.value || "0", 10);
  const width = Number.isFinite(w) ? Math.max(32, Math.min(8192, w)) : 500;
  const height = Number.isFinite(h) ? Math.max(32, Math.min(8192, h)) : 500;
  return { width, height };
}

function updatePreviewSizing() {
  applyPreviewTransform();
}

function clamp(value, lo, hi) {
  return Math.min(hi, Math.max(lo, value));
}

function isNearestPreviewSampling() {
  return String(uiOptions.previewSampling || "").toLowerCase() === "nearest";
}

function hasPreviewImage() {
  return !el.previewFrame.classList.contains("is-empty")
    && !!el.preview.getAttribute("src")
    && !!el.preview.naturalWidth
    && !!el.preview.naturalHeight;
}

function getPreviewFittedSize() {
  const frameW = el.previewFrame.clientWidth;
  const frameH = el.previewFrame.clientHeight;
  const imgW = el.preview.naturalWidth;
  const imgH = el.preview.naturalHeight;
  if (!frameW || !frameH || !imgW || !imgH) return null;
  const fit = Math.min(frameW / imgW, frameH / imgH);
  return {
    frameW,
    frameH,
    width: imgW * fit,
    height: imgH * fit,
  };
}

function clearPreviewCanvas() {
  if (!el.previewCanvas) return;
  const ctx = el.previewCanvas.getContext("2d");
  if (!ctx) return;
  ctx.setTransform(1, 0, 0, 1, 0, 0);
  ctx.clearRect(0, 0, el.previewCanvas.width, el.previewCanvas.height);
}

function ensurePreviewCanvasSize() {
  if (!el.previewCanvas || !el.previewFrame) return null;
  const cssW = Math.max(1, Math.floor(el.previewFrame.clientWidth));
  const cssH = Math.max(1, Math.floor(el.previewFrame.clientHeight));
  const dpr = window.devicePixelRatio || 1;
  const pxW = Math.max(1, Math.floor(cssW * dpr));
  const pxH = Math.max(1, Math.floor(cssH * dpr));
  if (el.previewCanvas.width !== pxW || el.previewCanvas.height !== pxH) {
    el.previewCanvas.width = pxW;
    el.previewCanvas.height = pxH;
  }
  return { cssW, cssH, dpr };
}

function drawPreviewCanvas() {
  if (!el.previewCanvas) return;
  const dims = ensurePreviewCanvasSize();
  const ctx = el.previewCanvas.getContext("2d");
  if (!dims || !ctx) return;

  ctx.setTransform(1, 0, 0, 1, 0, 0);
  ctx.clearRect(0, 0, el.previewCanvas.width, el.previewCanvas.height);
  if (!hasPreviewImage()) return;

  const fitted = getPreviewFittedSize();
  if (!fitted) return;

  const nearest = isNearestPreviewSampling();
  const tx = nearest ? Math.round(previewView.tx) : previewView.tx;
  const ty = nearest ? Math.round(previewView.ty) : previewView.ty;
  const scale = nearest ? Math.max(1, Math.round(previewView.scale)) : previewView.scale;
  const drawW = fitted.width * scale;
  const drawH = fitted.height * scale;
  const x = (fitted.frameW - drawW) * 0.5 + tx;
  const y = (fitted.frameH - drawH) * 0.5 + ty;

  ctx.setTransform(dims.dpr, 0, 0, dims.dpr, 0, 0);
  ctx.imageSmoothingEnabled = !nearest;
  ctx.imageSmoothingQuality = "high";
  ctx.drawImage(el.preview, x, y, drawW, drawH);
}

function clampPreviewPan() {
  const fitted = getPreviewFittedSize();
  if (!fitted) {
    previewView.tx = 0;
    previewView.ty = 0;
    return;
  }
  const scaledW = fitted.width * previewView.scale;
  const scaledH = fitted.height * previewView.scale;
  const maxX = Math.max(0, (scaledW - fitted.frameW) * 0.5);
  const maxY = Math.max(0, (scaledH - fitted.frameH) * 0.5);
  previewView.tx = clamp(previewView.tx, -maxX, maxX);
  previewView.ty = clamp(previewView.ty, -maxY, maxY);
}

function applyPreviewTransform() {
  if (!el.preview || !el.previewCanvas) return;
  if (!hasPreviewImage()) {
    if (el.preview.getAttribute("src")) {
      // Keep the last drawn frame visible while the next blob is decoding.
      updateResetViewUi(false);
      return;
    }
    el.previewFrame.classList.remove("is-zoomed");
    el.previewFrame.classList.remove("is-panning");
    clearPreviewCanvas();
    updateResetViewUi(false);
    return;
  }
  clampPreviewPan();
  const isZoomed = previewView.scale > 1.001 || Math.abs(previewView.tx) > 0.5 || Math.abs(previewView.ty) > 0.5;
  el.previewFrame.classList.toggle("is-zoomed", isZoomed);
  el.previewFrame.classList.toggle("is-panning", !!previewView.panning);
  drawPreviewCanvas();
  updateResetViewUi(isZoomed);
}

function resetPreviewView() {
  previewView.scale = 1;
  previewView.tx = 0;
  previewView.ty = 0;
  previewView.panning = false;
  previewView.pointerId = null;
  applyPreviewTransform();
}

function updateResetViewUi(enabled) {
  if (!el.resetViewBtn) return;
  const active = !!enabled;
  el.resetViewBtn.classList.toggle("is-disabled", !active);
  el.resetViewBtn.disabled = !active;
  el.resetViewBtn.setAttribute("aria-disabled", active ? "false" : "true");
}

function zoomPreviewAt(clientX, clientY, wheelDeltaY) {
  if (!hasPreviewImage()) return;
  const rect = el.previewFrame.getBoundingClientRect();
  const cx = clientX - rect.left - rect.width * 0.5;
  const cy = clientY - rect.top - rect.height * 0.5;
  let nextScale = previewView.scale;
  if (isNearestPreviewSampling()) {
    const dir = wheelDeltaY < 0 ? 1 : -1;
    nextScale = clamp(previewView.scale + dir, previewView.minScale, previewView.maxScale);
  } else {
    const zoomFactor = Math.exp((-wheelDeltaY) * 0.0015);
    nextScale = clamp(previewView.scale * zoomFactor, previewView.minScale, previewView.maxScale);
  }
  if (!Number.isFinite(nextScale) || Math.abs(nextScale - previewView.scale) < 1e-6) return;
  const k = nextScale / previewView.scale;
  previewView.tx = cx - (cx - previewView.tx) * k;
  previewView.ty = cy - (cy - previewView.ty) * k;
  previewView.scale = nextScale;
  applyPreviewTransform();
}

function bindPreviewInteraction() {
  if (!el.previewFrame || !el.preview) return;
  el.preview.draggable = false;

  el.preview.addEventListener("load", () => {
    if (previewPendingRevokeUrl && previewPendingRevokeUrl !== previewPinnedBaseUrl) {
      URL.revokeObjectURL(previewPendingRevokeUrl);
    }
    previewPendingRevokeUrl = "";
    applyPreviewTransform();
  });

  el.previewFrame.addEventListener("wheel", (evt) => {
    if (!hasPreviewImage()) return;
    evt.preventDefault();
    zoomPreviewAt(evt.clientX, evt.clientY, evt.deltaY);
  }, { passive: false });

  el.previewFrame.addEventListener("dblclick", (evt) => {
    if (!hasPreviewImage()) return;
    evt.preventDefault();
    resetPreviewView();
  });

  el.previewFrame.addEventListener("pointerdown", (evt) => {
    if (!hasPreviewImage()) return;
    if (evt.button !== 0 && evt.button !== 1) return;
    evt.preventDefault();
    previewView.panning = true;
    previewView.pointerId = evt.pointerId;
    previewView.lastX = evt.clientX;
    previewView.lastY = evt.clientY;
    el.previewFrame.setPointerCapture(evt.pointerId);
    applyPreviewTransform();
  });

  el.previewFrame.addEventListener("pointermove", (evt) => {
    if (!previewView.panning || previewView.pointerId !== evt.pointerId) return;
    const dx = evt.clientX - previewView.lastX;
    const dy = evt.clientY - previewView.lastY;
    previewView.lastX = evt.clientX;
    previewView.lastY = evt.clientY;
    previewView.tx += dx;
    previewView.ty += dy;
    applyPreviewTransform();
  });

  const endPan = (evt) => {
    if (!previewView.panning || previewView.pointerId !== evt.pointerId) return;
    previewView.panning = false;
    previewView.pointerId = null;
    try {
      el.previewFrame.releasePointerCapture(evt.pointerId);
    } catch (_) {
      // Ignore release errors from non-captured pointers.
    }
    applyPreviewTransform();
  };

  el.previewFrame.addEventListener("pointerup", endPan);
  el.previewFrame.addEventListener("pointercancel", endPan);
  el.previewFrame.addEventListener("pointerleave", (evt) => {
    if (!previewView.panning || previewView.pointerId !== evt.pointerId) return;
    endPan(evt);
  });
}

function updateToneMappingControlState() {
  if (!el.toneMapping || !el.toneMappingExposure || !el.toneMappingWhitePoint) return;
  const op = String(el.toneMapping.value || "aces").toLowerCase();
  const spec = toneMappingControlSpec(op);
  if (el.toneMappingParamsRow) el.toneMappingParamsRow.hidden = !spec.usesExposure && !spec.usesWhitePoint && !spec.usesMantiuk;
  if (el.toneMappingExposureControl) el.toneMappingExposureControl.hidden = !spec.usesExposure;
  if (el.toneMappingWhitePointControl) el.toneMappingWhitePointControl.hidden = !spec.usesWhitePoint;
  if (el.toneMappingMantiukContrastControl) el.toneMappingMantiukContrastControl.hidden = !spec.usesMantiuk;
  if (el.toneMappingMantiukSaturationControl) el.toneMappingMantiukSaturationControl.hidden = !spec.usesMantiuk;
  if (el.toneMappingMantiukDetailControl) el.toneMappingMantiukDetailControl.hidden = !spec.usesMantiuk;
  el.toneMappingExposure.disabled = !spec.usesExposure;
  el.toneMappingWhitePoint.disabled = !spec.usesWhitePoint;
  if (el.toneMappingMantiukContrast) el.toneMappingMantiukContrast.disabled = !spec.usesMantiuk;
  if (el.toneMappingMantiukSaturation) el.toneMappingMantiukSaturation.disabled = !spec.usesMantiuk;
  if (el.toneMappingMantiukDetail) el.toneMappingMantiukDetail.disabled = !spec.usesMantiuk;
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

function editorLineCount(text) {
  if (!text) return 1;
  return text.split("\n").length;
}

function updateEditorMetrics() {
  const text = el.sceneSource.value || "";
  const lines = editorLineCount(text);
  const chars = text.length;

  const gutter = Array.from({ length: lines }, (_, i) => String(i + 1)).join("\n");
  el.lineNumbers.textContent = gutter;
  el.lineCount.textContent = `${lines} ${lines === 1 ? "line" : "lines"}`;
  el.charCount.textContent = `${chars} ${chars === 1 ? "char" : "chars"}`;
}

function syncEditorScroll() {
  el.lineNumbers.scrollTop = el.sceneSource.scrollTop;
}

function handleEditorTabKey(event) {
  if (event && (event.ctrlKey || event.metaKey) && String(event.key || "").toLowerCase() === "s") {
    event.preventDefault();
    triggerSceneSave();
    return;
  }

  if (!event || event.key !== "Tab" || !el.sceneSource) return;
  event.preventDefault();

  const ta = el.sceneSource;
  const value = ta.value || "";
  const start = ta.selectionStart || 0;
  const end = ta.selectionEnd || 0;

  if (start === end && !event.shiftKey) {
    ta.value = `${value.slice(0, start)}\t${value.slice(end)}`;
    ta.selectionStart = start + 1;
    ta.selectionEnd = start + 1;
    updateEditorMetrics();
    syncEditorScroll();
    return;
  }

  const lineStart = value.lastIndexOf("\n", Math.max(0, start - 1)) + 1;
  let lineEnd = value.indexOf("\n", end);
  if (lineEnd < 0) lineEnd = value.length;

  const lines = value.slice(lineStart, lineEnd).split("\n");
  if (event.shiftKey) {
    let removedTotal = 0;
    let removedOnFirst = 0;
    const outdented = lines.map((line, idx) => {
      if (line.startsWith("\t")) {
        if (idx === 0) removedOnFirst = 1;
        removedTotal += 1;
        return line.slice(1);
      }
      return line;
    });
    ta.value = `${value.slice(0, lineStart)}${outdented.join("\n")}${value.slice(lineEnd)}`;
    ta.selectionStart = Math.max(lineStart, start - removedOnFirst);
    ta.selectionEnd = Math.max(ta.selectionStart, end - removedTotal);
  } else {
    const indented = lines.map((line) => `\t${line}`);
    ta.value = `${value.slice(0, lineStart)}${indented.join("\n")}${value.slice(lineEnd)}`;
    ta.selectionStart = start + 1;
    ta.selectionEnd = end + lines.length;
  }

  updateEditorMetrics();
  syncEditorScroll();
}

function selectedExportFormat() {
  const raw = String(el.exportFormat && el.exportFormat.value ? el.exportFormat.value : "png").toLowerCase();
  if (raw === "exr" || raw === "hdr") return raw;
  return "png";
}

function updateDownloadUi() {
  const hasExportApi = hasBackendMethod(api, "getJobExport");
  const enabled = hasExportApi && !!lastCompletedJobId;
  const fmt = selectedExportFormat().toUpperCase();
  if (enabled) {
    el.download.setAttribute("aria-disabled", "false");
    el.download.classList.remove("is-disabled");
    el.download.setAttribute("title", `Export ${fmt}`);
    el.download.setAttribute("aria-label", `Export ${fmt}`);
  } else {
    el.download.setAttribute("aria-disabled", "true");
    el.download.classList.add("is-disabled");
    if (!hasExportApi) {
      el.download.setAttribute("title", "Export unavailable on this backend");
      el.download.setAttribute("aria-label", "Export unavailable on this backend");
    } else {
      el.download.setAttribute("title", `Export ${fmt}`);
      el.download.setAttribute("aria-label", `Export ${fmt}`);
    }
  }
}

function setPreviewEmptyState(isEmpty) {
  el.previewFrame.classList.toggle("is-empty", isEmpty);
  if (isEmpty) {
    if (previewPinnedBaseUrl && previewPinnedBaseUrl.startsWith("blob:") && previewPinnedBaseUrl !== previewObjectUrl) {
      URL.revokeObjectURL(previewPinnedBaseUrl);
    }
    if (previewPendingRevokeUrl && previewPendingRevokeUrl !== previewPinnedBaseUrl) {
      URL.revokeObjectURL(previewPendingRevokeUrl);
    }
    previewPendingRevokeUrl = "";
    previewPinnedBaseUrl = "";
    previewPinnedBaseBitmapPromise = null;
    preservePreviewUnderlay = false;
    if (previewObjectUrl) {
      URL.revokeObjectURL(previewObjectUrl);
      previewObjectUrl = "";
    }
    el.preview.removeAttribute("src");
    clearPreviewCanvas();
    lastCompletedJobId = "";
    resetPreviewView();
    updateDownloadUi();
    return;
  }
  updateDownloadUi();
}

function setPreviewFromBlob(blob) {
  const url = URL.createObjectURL(blob);
  if (previewObjectUrl && previewObjectUrl !== url) {
    previewPendingRevokeUrl = previewObjectUrl;
  }
  previewObjectUrl = url;
  el.preview.src = previewObjectUrl;
  const nearest = isNearestPreviewSampling();
  el.previewFrame.classList.toggle("sampling-nearest", nearest);
  setPreviewEmptyState(false);
}

function applyPreviewSampling() {
  if (!el.preview || !el.previewCanvas) return;
  const nearest = isNearestPreviewSampling();
  el.previewFrame.classList.toggle("sampling-nearest", nearest);
  applyPreviewTransform();
}

async function getPinnedBaseBitmap() {
  if (!preservePreviewUnderlay || !previewPinnedBaseUrl) return null;
  if (!previewPinnedBaseBitmapPromise) {
    previewPinnedBaseBitmapPromise = (async () => {
      const res = await fetch(previewPinnedBaseUrl);
      if (!res.ok) throw new Error("failed to load base preview");
      const blob = await res.blob();
      return createImageBitmap(blob);
    })();
  }
  return previewPinnedBaseBitmapPromise;
}

async function composeWithPinnedPreview(overlayBlob) {
  const baseBitmap = await getPinnedBaseBitmap();
  if (!baseBitmap) return overlayBlob;

  const overlayBitmap = await createImageBitmap(overlayBlob);
  const width = overlayBitmap.width || baseBitmap.width;
  const height = overlayBitmap.height || baseBitmap.height;
  const canvas = document.createElement("canvas");
  canvas.width = width;
  canvas.height = height;
  const ctx = canvas.getContext("2d");
  if (!ctx) return overlayBlob;

  ctx.clearRect(0, 0, width, height);
  ctx.drawImage(baseBitmap, 0, 0, width, height);
  ctx.drawImage(overlayBitmap, 0, 0, width, height);

  const composedBlob = await new Promise((resolve) => {
    canvas.toBlob((b) => resolve(b || overlayBlob), "image/png");
  });
  return composedBlob || overlayBlob;
}

async function refreshProgressivePreview(jobId) {
  const blob = await api.getJobImage(jobId, {
    partial: true,
    cacheBust: true,
    toneMapping: el.toneMapping ? el.toneMapping.value : "aces",
    toneMappingExposure: el.toneMappingExposure ? el.toneMappingExposure.value : "1.0",
    toneMappingWhitePoint: el.toneMappingWhitePoint ? el.toneMappingWhitePoint.value : "1.0",
    toneMappingMantiukContrast: el.toneMappingMantiukContrast ? el.toneMappingMantiukContrast.value : "0.1",
    toneMappingMantiukSaturation: el.toneMappingMantiukSaturation ? el.toneMappingMantiukSaturation.value : "0.8",
    toneMappingMantiukDetail: el.toneMappingMantiukDetail ? el.toneMappingMantiukDetail.value : "1.0",
  });
  if (!blob || blob.size === 0) return false;

  const imageBlob = preservePreviewUnderlay
    ? await composeWithPinnedPreview(blob)
    : blob;

  setPreviewFromBlob(imageBlob);
  return true;
}

async function refreshPreviewForToneMapping() {
  try {
    if (renderActive && activeJobId) {
      await refreshProgressivePreview(activeJobId);
      return;
    }
    if (lastCompletedJobId) {
      const finalBlob = await api.getJobImage(lastCompletedJobId, {
        final: true,
        cacheBust: true,
        toneMapping: el.toneMapping ? el.toneMapping.value : "aces",
        toneMappingExposure: el.toneMappingExposure ? el.toneMappingExposure.value : "1.0",
        toneMappingWhitePoint: el.toneMappingWhitePoint ? el.toneMappingWhitePoint.value : "1.0",
        toneMappingMantiukContrast: el.toneMappingMantiukContrast ? el.toneMappingMantiukContrast.value : "0.1",
        toneMappingMantiukSaturation: el.toneMappingMantiukSaturation ? el.toneMappingMantiukSaturation.value : "0.8",
        toneMappingMantiukDetail: el.toneMappingMantiukDetail ? el.toneMappingMantiukDetail.value : "1.0",
      });
      if (finalBlob && finalBlob.size > 0) setPreviewFromBlob(finalBlob);
    }
  } catch (err) {
    appendLog(`tone mapping preview refresh failed: ${err.message}`);
  }
}

function addOption(select, value, label) {
  const opt = document.createElement("option");
  opt.value = value;
  opt.textContent = label || value;
  select.appendChild(opt);
}

function parseIntegratorNumber(raw, fallback) {
  const n = Number(raw);
  return Number.isFinite(n) ? n : fallback;
}

function getIntegratorControlValue(state, ctrl) {
  const id = ctrl && ctrl.id ? ctrl.id : "";
  if (!id) return "";
  if (Object.prototype.hasOwnProperty.call(state, id)) return String(state[id] ?? "");
  if (ctrl.default !== undefined && ctrl.default !== null) return String(ctrl.default);
  return "";
}

function isIntegratorControlVisible(ctrl, state) {
  const vw = ctrl && ctrl.visible_when ? ctrl.visible_when : null;
  if (!vw || !vw.id) return true;
  return getIntegratorControlValue(state, { id: vw.id, default: "" }) === String(vw.value ?? "");
}

function renderIntegratorControls() {
  const selected = el.integrator.value || "";
  const info = integratorById.get(selected) || null;
  const controls = info && Array.isArray(info.controls) ? info.controls : [];

  el.integratorControls.innerHTML = "";
  if (!controls.length) {
    el.integratorControlsSection.hidden = true;
    return;
  }

  el.integratorControlsSection.hidden = false;
  const saved = { ...(integratorControlState.get(selected) || {}) };

  controls.forEach((ctrl) => {
    const id = ctrl.id || "";
    if (!id) return;
    if (!Object.prototype.hasOwnProperty.call(saved, id) && ctrl.default !== undefined) {
      saved[id] = String(ctrl.default);
    }
  });
  integratorControlState.set(selected, saved);

  controls.forEach((ctrl) => {
    const id = ctrl.id || "";
    if (!id) return;
    if (!isIntegratorControlVisible(ctrl, saved)) return;

    const label = document.createElement("label");
    label.className = "integrator-control";
    label.textContent = ctrl.label || id;

    let input = null;
    if (ctrl.type === "enum") {
      input = document.createElement("select");
      const options = Array.isArray(ctrl.options) ? ctrl.options : [];
      options.forEach((opt) => {
        const optEl = document.createElement("option");
        optEl.value = String(opt.value ?? "");
        optEl.textContent = opt.label || opt.value || "";
        input.appendChild(optEl);
      });
    } else if (ctrl.type === "bool") {
      input = document.createElement("select");
      addOption(input, "false", "False");
      addOption(input, "true", "True");
    } else {
      input = document.createElement("input");
      input.type = "number";
      if (ctrl.type === "int") input.step = ctrl.step || "1";
      else input.step = ctrl.step || "0.01";
      if (ctrl.min !== undefined && ctrl.min !== null && ctrl.min !== "") input.min = String(ctrl.min);
      if (ctrl.max !== undefined && ctrl.max !== null && ctrl.max !== "") input.max = String(ctrl.max);
    }

    input.dataset.ioptId = id;
    input.dataset.ioptType = ctrl.type || "string";
    const value = getIntegratorControlValue(saved, ctrl);
    input.value = String(value);

    input.addEventListener("change", () => {
      const curr = integratorControlState.get(selected) || {};
      curr[id] = input.value;
      integratorControlState.set(selected, curr);
      renderIntegratorControls();
    });

    label.appendChild(input);
    if (ctrl.description) {
      const hint = document.createElement("small");
      hint.className = "control-hint";
      hint.textContent = ctrl.description;
      label.appendChild(hint);
    }
    el.integratorControls.appendChild(label);
  });
}

function gatherIntegratorOptionParams() {
  const out = {};
  const selected = el.integrator.value || "";
  const nodes = el.integratorControls.querySelectorAll("[data-iopt-id]");
  const save = integratorControlState.get(selected) || {};

  nodes.forEach((node) => {
    const id = node.dataset.ioptId || "";
    if (!id) return;
    const type = node.dataset.ioptType || "string";
    const raw = String(node.value ?? "").trim();
    if (!raw) return;

    let normalized = raw;
    if (type === "int") normalized = String(Math.round(parseIntegratorNumber(raw, 0)));
    else if (type === "float") normalized = String(parseIntegratorNumber(raw, 0));
    else if (type === "bool") normalized = (raw === "1" || raw === "true") ? "true" : "false";

    save[id] = normalized;
    out[`iopt.${id}`] = normalized;
  });

  integratorControlState.set(selected, save);
  return out;
}

function extractSceneTitle(source) {
  const m = /^\s*title\s*=\s*(.+)$/im.exec(source || "");
  if (!m) return "";
  let title = (m[1] || "").trim();
  if (!title) return "";
  if ((title[0] === "\"" && title[title.length - 1] === "\"")
    || (title[0] === "'" && title[title.length - 1] === "'")) {
    title = title.slice(1, -1).trim();
  }
  return title;
}

function sceneDependsOnExternalFiles(source) {
  const text = source || "";
  if (!text) return false;

  const lines = text.split("\n");
  for (let i = 0; i < lines.length; i += 1) {
    const raw = lines[i] || "";
    const line = raw.replace(/#.*/, "").trim();
    if (!line) continue;

    // Explicit scene path aliases, e.g. path_data = data or path_mesh = <data>/mesh
    const pathMatch = /^path_[A-Za-z0-9_]*\s*=\s*(.+)$/i.exec(line);
    if (pathMatch) {
      const v = (pathMatch[1] || "").trim();
      if (/[\\/]/.test(v) || /<[^>]+>/.test(v)) return true;
      continue;
    }

    // Any non-procedural source assignment is a filesystem dependency.
    const srcMatch = /^source\s*=\s*(.+)$/i.exec(line);
    if (srcMatch) {
      const v = (srcMatch[1] || "").trim();
      if (!/^gen\s*\(/i.test(v) && v.length > 0) return true;
    }
  }

  return false;
}

async function buildSceneLabels(sceneFiles) {
  const items = await Promise.all((sceneFiles || []).map(async (sceneFile) => {
    try {
      const data = await api.getSceneSource(sceneFile);
      const source = (data && data.source) || "";
      const title = extractSceneTitle(source);
      const dependsExternal = sceneDependsOnExternalFiles(source);
      return { sceneFile, label: title || sceneFile, dependsExternal };
    } catch (_) {
      return { sceneFile, label: sceneFile, dependsExternal: false };
    }
  }));

  return items;
}

function updateSceneDependencyPill(sceneFile) {
  const pill = el.sceneDependencyPill;
  if (!pill) return;
  const dependsExternal = !!sceneDependencyByFile.get(sceneFile || "");
  pill.hidden = !dependsExternal;
  pill.textContent = "EXT";
  pill.classList.toggle("scene-kind-ext", dependsExternal);
  pill.classList.toggle("scene-kind-self", !dependsExternal);
}

function presetId(index) {
  return String(index).padStart(2, "0");
}

function syncResolutionPresetFromInputs() {
  const { width, height } = currentRenderSize();
  const index = resolutionPresets.findIndex((p) => p.width === width && p.height === height);
  el.resolutionPreset.value = index >= 0 ? String(index) : "custom";
}

function syncVisualFrameAspect() {
  if (!visualEditor || !visualEditor.setFrameAspect) return;
  const { width, height } = currentRenderSize();
  visualEditor.setFrameAspect(width, height);
}

async function loadResolutionPresets() {
  const presets = await api.getResolutionPresets();
  resolutionPresets = (presets || []).map((p) => ({
    width: parseInt(p.width, 10),
    height: parseInt(p.height, 10),
    description: p.description || "",
  })).filter((p) => Number.isFinite(p.width) && Number.isFinite(p.height) && p.width > 0 && p.height > 0);

  el.resolutionPreset.innerHTML = "";
  addOption(el.resolutionPreset, "custom", "Custom");
  resolutionPresets.forEach((preset, index) => {
    addOption(
      el.resolutionPreset,
      String(index),
      `${presetId(index)} ${preset.description} ${preset.width}x${preset.height}`,
    );
  });
  syncResolutionPresetFromInputs();
}

function setActiveTab(mode) {
  const isRender = mode === "render";
  const isVisual = mode === "visual";
  const isEditor = mode === "editor";
  const isSettings = mode === "settings";
  const isLogs = mode === "logs";
  const isAbout = mode === "about";
  el.tabRender.classList.toggle("active", isRender);
  el.tabVisual.classList.toggle("active", isVisual);
  el.tabEditor.classList.toggle("active", isEditor);
  el.tabSettings.classList.toggle("active", isSettings);
  el.tabLogs.classList.toggle("active", isLogs);
  el.tabAbout.classList.toggle("active", isAbout);
  el.paneRender.classList.toggle("active", isRender);
  el.paneVisual.classList.toggle("active", isVisual);
  el.paneEditor.classList.toggle("active", isEditor);
  el.paneSettings.classList.toggle("active", isSettings);
  el.paneLogs.classList.toggle("active", isLogs);
  el.paneAbout.classList.toggle("active", isAbout);
  document.body.classList.toggle("visual-tab-active", isVisual);
  document.documentElement.classList.toggle("visual-tab-active", isVisual);
  if (isVisual && visualEditor) visualEditor.onShow();
  if (isLogs && (uiOptions.autoScrollLogs || pendingLogScroll)) {
    scrollLogToBottom(true);
  }
}

async function loadVisualSceneFromSelected() {
  if (!visualEditor) return;
  const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!sceneName) {
    visualEditor.setStatus("No scene selected.");
    return;
  }

  visualEditor.setStatus("Loading " + sceneName + " ...");
  const pair = await Promise.all([
    api.getSceneSource(sceneName),
    api.getSceneGeometry(sceneName),
  ]);
  const data = pair[0] || {};
  const geometryData = pair[1] || { meshes: {} };
  await visualEditor.buildScene(sceneName, data.source || "", geometryData);
  visualLoadedSceneName = sceneName;
  refreshVisualCameraOptions();
  syncVisualCameraFromRenderSelection();
  refreshVisualPhotonOverlay().catch(() => {});
  appendLog("visual loaded: " + sceneName);
}

function refreshVisualCameraOptions() {
  if (!visualEditor || !el.visualCamera) return;
  var names = visualEditor.getCameraNames ? visualEditor.getCameraNames() : [];
  var active = visualEditor.getActiveCamera ? visualEditor.getActiveCamera() : "";
  el.visualCamera.innerHTML = "";
  addOption(el.visualCamera, "", "Free (orbit)");
  (names || []).forEach(function (name) {
    addOption(el.visualCamera, name, name);
  });
  if (active && names.indexOf(active) >= 0) el.visualCamera.value = active;
  else el.visualCamera.value = "";
}

function syncVisualCameraFromRenderSelection() {
  if (!visualEditor || !el.visualCamera || !el.camera) return;
  const selected = String(el.camera.value || "").trim();
  if (!selected) return;
  const values = Array.from(el.visualCamera.options || []).map((o) => o.value);
  if (values.indexOf(selected) < 0) return;
  el.visualCamera.value = selected;
  if (visualEditor.setActiveCamera) visualEditor.setActiveCamera(selected);
}

async function refreshVisualPhotonOverlay() {
  if (!visualEditor || !visualEditor.setPhotonPoints || !visualEditor.clearPhotonPoints) return;
  if (!hasBackendMethod(api, "getJobPhotons")) {
    visualEditor.clearPhotonPoints();
    return;
  }

  const selectedScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!lastCompletedJobId
    || lastCompletedJobIntegrator !== "photon_mapping"
    || !lastCompletedJobScene
    || (selectedScene && selectedScene !== lastCompletedJobScene)) {
    visualEditor.clearPhotonPoints();
    return;
  }

  try {
    const data = await api.getJobPhotons(lastCompletedJobId, 100000);
    const diffuse = (data && Array.isArray(data.diffuse)) ? data.diffuse : [];
    const caustic = (data && Array.isArray(data.caustic)) ? data.caustic : [];
    visualEditor.setPhotonPoints(diffuse, caustic);
    appendLog(`visual photons diffuse=${diffuse.length} caustic=${caustic.length}`);
  } catch (err) {
    appendLog(`visual photons unavailable: ${err.message || "request failed"}`);
    visualEditor.clearPhotonPoints();
  }
}

function applyTheme(mode) {
  const root = document.documentElement;
  if (mode === "light" || mode === "dark") root.setAttribute("data-theme", mode);
  else root.setAttribute("data-theme", "system");
  localStorage.setItem("xtracer-theme", mode);
}

function clampFontScale(v) {
  return Math.max(0.8, Math.min(1.4, Number(v) || 1.0));
}

function updateFontScaleUI() {
  if (!el.fontSizeValue) return;
  el.fontSizeValue.textContent = `${Math.round(uiOptions.fontScale * 100)}%`;
}

function applyFontScale(scale) {
  uiOptions.fontScale = clampFontScale(scale);
  document.documentElement.style.fontSize = `${(uiOptions.fontScale * 100).toFixed(1)}%`;
  updateFontScaleUI();
}

function loadUIOptions() {
  const poll = parseInt(localStorage.getItem("xtracer-poll-ms") || "300", 10);
  uiOptions.pollMs = Number.isFinite(poll) ? Math.max(100, Math.min(10000, poll)) : 300;
  uiOptions.autoLoadEditor = localStorage.getItem("xtracer-auto-load-editor") !== "0";
  uiOptions.autoScrollLogs = localStorage.getItem("xtracer-auto-scroll-logs") !== "0";
  uiOptions.clearPreviewOnRender = localStorage.getItem("xtracer-clear-preview-on-render") === "1";
  const fontScaleRaw = parseFloat(localStorage.getItem("xtracer-ui-font-scale") || "1");
  uiOptions.fontScale = Number.isFinite(fontScaleRaw) ? clampFontScale(fontScaleRaw) : 1.0;
  const previewSamplingRaw = String(localStorage.getItem("xtracer-preview-sampling") || "smooth").toLowerCase();
  const previewSampling = (previewSamplingRaw === "linear" || previewSamplingRaw === "bilinear")
    ? "smooth"
    : previewSamplingRaw;
  uiOptions.previewSampling = (previewSampling === "nearest" || previewSampling === "smooth")
    ? previewSampling
    : "smooth";
  el.pollInterval.value = String(uiOptions.pollMs);
  el.autoLoadEditor.checked = uiOptions.autoLoadEditor;
  el.autoScrollLogs.checked = uiOptions.autoScrollLogs;
  el.clearPreviewOnRender.checked = uiOptions.clearPreviewOnRender;
  if (el.previewSampling) el.previewSampling.value = uiOptions.previewSampling;
  applyFontScale(uiOptions.fontScale);
}

function persistUIOptions() {
  localStorage.setItem("xtracer-poll-ms", String(uiOptions.pollMs));
  localStorage.setItem("xtracer-auto-load-editor", uiOptions.autoLoadEditor ? "1" : "0");
  localStorage.setItem("xtracer-auto-scroll-logs", uiOptions.autoScrollLogs ? "1" : "0");
  localStorage.setItem("xtracer-clear-preview-on-render", uiOptions.clearPreviewOnRender ? "1" : "0");
  localStorage.setItem("xtracer-preview-sampling", uiOptions.previewSampling);
  localStorage.setItem("xtracer-ui-font-scale", String(uiOptions.fontScale));
}

async function loadScenes() {
  const scenes = await api.getScenes();
  const sceneItems = await buildSceneLabels(scenes);
  const prev = el.scene.value;
  el.scene.innerHTML = "";
  sceneDependencyByFile = new Map(sceneItems.map((item) => [item.sceneFile, !!item.dependsExternal]));
  sceneItems.forEach((item) => addOption(el.scene, item.sceneFile, item.label));
  if (prev) el.scene.value = prev;
  if (!el.scene.value && el.scene.options.length > 0) el.scene.selectedIndex = 0;
  updateSceneDependencyPill(el.scene.value);
}

async function loadCameras(scene) {
  el.camera.innerHTML = "";
  addOption(el.camera, "", "Auto (first camera)");
  const cameras = await api.getCameras(scene);
  (cameras || []).forEach((name) => addOption(el.camera, name, name));
}

async function loadIntegrators() {
  const integrators = await api.getIntegrators();
  integratorCatalog = Array.isArray(integrators) ? integrators : [];
  integratorById = new Map(integratorCatalog.map((it) => [it.id, it]));
  const prev = el.integrator.value;
  el.integrator.innerHTML = "";
  integratorCatalog.forEach((it) => addOption(el.integrator, it.id, it.label));
  if (prev && integratorCatalog.some((it) => it.id === prev)) {
    el.integrator.value = prev;
  } else if (integratorCatalog.some((it) => it.id === "pathtracer_mis")) {
    el.integrator.value = "pathtracer_mis";
  } else if (integratorCatalog.some((it) => it.id === "pathtracer_is")) {
    el.integrator.value = "pathtracer_is";
  } else if (!el.integrator.value && el.integrator.options.length > 0) {
    el.integrator.selectedIndex = 0;
  }
  renderIntegratorControls();
}

async function loadSceneSource(scene) {
  if (!scene) {
    el.sceneSource.value = "";
    updateEditorMetrics();
    return;
  }
  const data = await api.getSceneSource(scene);
  el.sceneName.value = data.scene || scene;
  el.sceneSource.value = data.source || "";
  updateEditorMetrics();
  syncEditorScroll();
}

async function loadAbout() {
  const data = await api.getAbout();
  const rawVersion = (data.version || "").trim();
  const hideStandaloneVersion = !rawVersion || rawVersion.toLowerCase() === "standalone";
  const version = hideStandaloneVersion ? "" : rawVersion;
  el.aboutVersion.textContent = version;
  el.aboutVersionRow.hidden = hideStandaloneVersion;
  const homepage = data.homepage || "https://www.4rknova.com";
  el.aboutHomepage.href = homepage;
  el.aboutHomepage.textContent = homepage;
  const repository = data.website || "https://github.com/4rknova/xtracer";
  el.aboutWebsite.href = repository;
  el.aboutWebsite.textContent = repository;
  el.aboutCopyright.textContent = data.copyright || "unknown";
  el.aboutLicense.textContent = data.license || "Unavailable";
  const backendLabel = data.backend || (backendMode === "wasm" ? "xtracer_wasm_adapter" : "xtracer_web");
  const isWasmBackend = String(backendLabel).toLowerCase().indexOf("wasm") >= 0;
  el.aboutBuildPill.hidden = !isWasmBackend;
  el.aboutBackend.textContent = backendLabel;
  el.aboutDefaultUrl.textContent = data.default_url || window.location.origin;
  el.aboutSceneDir.textContent = data.scene_dir || (backendMode === "wasm" ? "scenes/" : "scene/");
  el.aboutStaticAssets.textContent = data.static_assets || "/";
}

async function loadEmptySceneTemplate() {
  if (hasBackendMethod(api, "getEmptySceneTemplate")) {
    return api.getEmptySceneTemplate();
  }
  const data = await getJSON("/api/scenes/template/empty");
  return data.source || "";
}

async function startRender() {
  return api.startRender({
    scene: el.scene.value,
    integrator: el.integrator.value,
    camera: el.camera.value || "",
    width: el.width.value,
    height: el.height.value,
    samples: el.samples.value,
    aa: el.aa.value,
    rdepth: el.rdepth.value,
    tile_size: el.tileSize.value,
    tile_order: el.tileOrder.value,
    threads: el.threads.value,
    ...gatherIntegratorOptionParams(),
  });
}

async function saveScene() {
  const name = (el.sceneName.value || "").trim();
  const source = el.sceneSource.value || "";
  if (!name) throw new Error("scene name is required");

  try {
    return await api.saveScene(name, source, false);
  } catch (err) {
    if (err.status !== 409) throw err;
    if (!window.confirm("Scene exists. Overwrite it?")) return null;
    return api.saveScene(name, source, true);
  }
}

function triggerSceneSave() {
  saveScene()
    .then((scene) => {
      if (!scene) return;
      loadScenes()
        .then(() => {
          el.scene.value = scene;
          const tasks = [loadCameras(scene)];
          if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(scene));
          return Promise.all(tasks);
        })
        .then(() => {
          setStatus(`saved ${scene}`);
          appendLog(`saved scene: ${scene}`);
        })
        .catch((err) => {
          setStatus(`error: ${err.message}`);
          appendLog(`post-save error: ${err.message}`);
        });
    })
    .catch((err) => {
      setStatus(`error: ${err.message}`);
      appendLog(`save error: ${err.message}`);
    });
}

async function pollJob(jobId) {
  let lastState = "";
  while (true) {
    const data = await api.getJob(jobId);
    const state = data.state || "unknown";
    const progress = data.progress || 0;
    setProgress(progress);
    const stateLabel = state === "running" ? "rendering" : state;
    setStatus(`${stateLabel} ${(100 * progress).toFixed(1)}%`);

    if (state !== lastState) {
      appendLog(`job ${jobId} -> ${state}`);
      lastState = state;
    }

    await refreshProgressivePreview(jobId);

    if (state === "done") {
      const finalBlob = await api.getJobImage(jobId, {
        final: true,
        cacheBust: true,
        toneMapping: el.toneMapping ? el.toneMapping.value : "aces",
        toneMappingExposure: el.toneMappingExposure ? el.toneMappingExposure.value : "1.0",
        toneMappingWhitePoint: el.toneMappingWhitePoint ? el.toneMappingWhitePoint.value : "1.0",
        toneMappingMantiukContrast: el.toneMappingMantiukContrast ? el.toneMappingMantiukContrast.value : "0.1",
        toneMappingMantiukSaturation: el.toneMappingMantiukSaturation ? el.toneMappingMantiukSaturation.value : "0.8",
        toneMappingMantiukDetail: el.toneMappingMantiukDetail ? el.toneMappingMantiukDetail.value : "1.0",
      });
      if (finalBlob && finalBlob.size > 0) {
        setPreviewFromBlob(finalBlob);
      }
      lastCompletedJobId = jobId;
      lastCompletedJobScene = String(data.scene || el.scene.value || "");
      lastCompletedJobIntegrator = String(data.integrator || el.integrator.value || "");
      updateDownloadUi();
      refreshVisualPhotonOverlay().catch(() => {});
      setStatus(`done in ${Math.round(data.elapsed_ms || 0)} ms`);
      appendLog(`job ${jobId} finished in ${Math.round(data.elapsed_ms || 0)} ms`);
      return;
    }

    if (state === "error") {
      throw new Error(data.error || "render failed");
    }

    await new Promise((r) => setTimeout(r, uiOptions.pollMs));
  }
}

async function handleExportClick(event) {
  if (event) event.preventDefault();
  if (el.download.classList.contains("is-disabled")) return;
  if (!lastCompletedJobId) return;
  if (!hasBackendMethod(api, "getJobExport")) return;

  const format = selectedExportFormat();
  const filename = `xtracer_${lastCompletedJobId}.${format}`;
  try {
    const blob = await api.getJobExport(lastCompletedJobId, format);
    if (!blob || blob.size <= 0) {
      throw new Error("empty export payload");
    }

    const url = URL.createObjectURL(blob);
    const tmp = document.createElement("a");
    tmp.href = url;
    tmp.download = filename;
    document.body.appendChild(tmp);
    tmp.click();
    tmp.remove();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
    appendLog(`exported ${filename}`);
  } catch (err) {
    appendLog(`export failed: ${err.message}`);
    setStatus(`error: ${err.message}`);
  }
}

async function handleRender() {
  el.renderBtn.disabled = true;
  lastCompletedJobId = "";
  lastCompletedJobScene = "";
  lastCompletedJobIntegrator = "";
  updateDownloadUi();
  if (previewPinnedBaseUrl && previewPinnedBaseUrl.startsWith("blob:") && previewPinnedBaseUrl !== previewObjectUrl) {
    URL.revokeObjectURL(previewPinnedBaseUrl);
  }
  previewPinnedBaseUrl = "";
  preservePreviewUnderlay = false;
  previewPinnedBaseBitmapPromise = null;
  if (uiOptions.clearPreviewOnRender) {
    setPreviewEmptyState(true);
  } else if (!el.previewFrame.classList.contains("is-empty") && (previewObjectUrl || el.preview.getAttribute("src"))) {
    preservePreviewUnderlay = true;
    previewPinnedBaseUrl = previewObjectUrl || el.preview.getAttribute("src") || "";
  }
  setRenderActive(true);
  setProgress(0);
  setStatus("submitting job...");
  appendLog(`submit render scene=${el.scene.value} integrator=${el.integrator.value} tile_order=${el.tileOrder.value}`);
  try {
    const jobId = await startRender();
    activeJobId = jobId;
    appendLog(`job accepted: ${jobId}`);
    await pollJob(jobId);
  } catch (err) {
    setStatus(`error: ${err.message}`);
    appendLog(`render error: ${err.message}`);
  } finally {
    if (previewPinnedBaseUrl && previewPinnedBaseUrl.startsWith("blob:") && previewPinnedBaseUrl !== previewObjectUrl) {
      URL.revokeObjectURL(previewPinnedBaseUrl);
    }
    previewPinnedBaseUrl = "";
    previewPinnedBaseBitmapPromise = null;
    preservePreviewUnderlay = false;
    activeJobId = "";
    setRenderActive(false);
    el.renderBtn.disabled = false;
  }
}

async function boot() {
  api = initializeBackendApi();
  const savedTheme = localStorage.getItem("xtracer-theme") || "system";
  el.theme.value = savedTheme;
  applyTheme(savedTheme);
  loadUIOptions();
  pollBackendLogs();

  setStatus("loading...");
  appendLog(`boot (backend=${backendMode})`);
  await Promise.all([loadScenes(), loadIntegrators(), loadResolutionPresets()]);
  await loadCameras(el.scene.value);
  await loadSceneSource(el.scene.value);
  await loadAbout();
  updatePreviewSizing();
  bindPreviewInteraction();
  applyPreviewSampling();
  setPreviewEmptyState(true);
  setRenderActive(false);
  if (!el.scene.value) setStatus("no scenes found in scene/ directory");
  else setStatus("idle");

  setActiveTab("render");
  if (el.visualViewport && window.SceneVisualEditor) {
    visualEditor = new window.SceneVisualEditor(
      el.visualViewport,
      el.visualStatus || null,
      async (sceneName) => {
        if (!hasBackendMethod(api, "getSceneGeometry")) throw new Error("geometry endpoint unavailable");
        return api.getSceneGeometry(sceneName);
      }
    );
    if (visualEditor.init()) {
      syncVisualFrameAspect();
      try {
        await loadVisualSceneFromSelected();
      } catch (err) {
        appendLog("visual load error: " + err.message);
      }
    } else {
      visualEditor = null;
    }
  }
  el.renderBtn.addEventListener("click", handleRender);
  if (el.visualLoadBtn) {
    el.visualLoadBtn.addEventListener("click", () => {
      loadVisualSceneFromSelected().catch((err) => {
        if (visualEditor) visualEditor.setStatus("Visual load failed: " + err.message);
        appendLog("visual load error: " + err.message);
      });
    });
  }
  if (el.visualCamera) {
    el.visualCamera.addEventListener("change", () => {
      if (!visualEditor || !visualEditor.setActiveCamera) return;
      visualEditor.setActiveCamera(el.visualCamera.value || "");
      appendLog(`visual camera=${el.visualCamera.value || "free"}`);
    });
  }
  if (el.visualShowGrid) {
    el.visualShowGrid.addEventListener("change", () => {
      if (visualEditor && visualEditor.setGridVisible) visualEditor.setGridVisible(!!el.visualShowGrid.checked);
      appendLog(`visual grid=${el.visualShowGrid.checked ? "on" : "off"}`);
    });
    if (visualEditor && visualEditor.setGridVisible) visualEditor.setGridVisible(!!el.visualShowGrid.checked);
  }
  if (el.resetViewBtn) {
    el.resetViewBtn.addEventListener("click", () => {
      resetPreviewView();
      appendLog("preview view reset");
    });
  }
  el.download.addEventListener("click", handleExportClick);
  if (el.exportFormat) {
    el.exportFormat.addEventListener("change", () => {
      updateDownloadUi();
      appendLog(`export format=${selectedExportFormat()}`);
    });
  }
  if (el.previewSampling) {
    el.previewSampling.addEventListener("change", () => {
      const mode = String(el.previewSampling.value || "smooth").toLowerCase();
      uiOptions.previewSampling = (mode === "nearest" || mode === "smooth")
        ? mode
        : "smooth";
      persistUIOptions();
      applyPreviewSampling();
      appendLog(`preview sampling=${uiOptions.previewSampling}`);
    });
  }
  if (!hasBackendMethod(api, "getJobExport")) {
    appendLog("backend export endpoint unavailable; export disabled");
  }

  el.scene.addEventListener("change", () => {
    updateSceneDependencyPill(el.scene.value);
    const tasks = [loadCameras(el.scene.value)];
    if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(el.scene.value));
    Promise.all(tasks)
      .then(() => {
        if (visualEditor) {
          return loadVisualSceneFromSelected();
        }
        return null;
      })
      .then(() => appendLog(`scene changed: ${el.scene.value}`))
      .catch((err) => {
        setStatus(`error: ${err.message}`);
        appendLog(`scene change error: ${err.message}`);
      });
  });

  el.camera.addEventListener("change", () => {
    syncVisualCameraFromRenderSelection();
  });

  el.theme.addEventListener("change", () => {
    applyTheme(el.theme.value);
    appendLog(`theme=${el.theme.value}`);
  });

  el.integrator.addEventListener("change", () => {
    renderIntegratorControls();
    appendLog(`integrator=${el.integrator.value}`);
    refreshVisualPhotonOverlay().catch(() => {});
  });

  el.tileOrder.addEventListener("change", () => {
    appendLog(`tile_order=${el.tileOrder.value}`);
  });

  if (el.toneMapping) {
    el.toneMapping.addEventListener("change", () => {
      updateToneMappingControlState();
      appendLog(`tone_mapping=${el.toneMapping.value}`);
      refreshPreviewForToneMapping();
    });
  }
  if (el.toneMappingExposure) {
    el.toneMappingExposure.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingExposure.addEventListener("change", () => {
      appendLog(`tone_mapping_exposure=${el.toneMappingExposure.value}`);
      refreshPreviewForToneMapping();
    });
  }
  if (el.toneMappingWhitePoint) {
    el.toneMappingWhitePoint.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingWhitePoint.addEventListener("change", () => {
      appendLog(`tone_mapping_white_point=${el.toneMappingWhitePoint.value}`);
      refreshPreviewForToneMapping();
    });
  }
  if (el.toneMappingMantiukContrast) {
    el.toneMappingMantiukContrast.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingMantiukContrast.addEventListener("change", () => {
      appendLog(`tone_mapping_mantiuk_contrast=${el.toneMappingMantiukContrast.value}`);
      refreshPreviewForToneMapping();
    });
  }
  if (el.toneMappingMantiukSaturation) {
    el.toneMappingMantiukSaturation.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingMantiukSaturation.addEventListener("change", () => {
      appendLog(`tone_mapping_mantiuk_saturation=${el.toneMappingMantiukSaturation.value}`);
      refreshPreviewForToneMapping();
    });
  }
  if (el.toneMappingMantiukDetail) {
    el.toneMappingMantiukDetail.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingMantiukDetail.addEventListener("change", () => {
      appendLog(`tone_mapping_mantiuk_detail=${el.toneMappingMantiukDetail.value}`);
      refreshPreviewForToneMapping();
    });
  }
  updateToneMappingControlState();

  const onSizeChanged = () => {
    syncResolutionPresetFromInputs();
    updatePreviewSizing();
    syncVisualFrameAspect();
  };
  el.resolutionPreset.addEventListener("change", () => {
    if (el.resolutionPreset.value === "custom") return;
    const index = parseInt(el.resolutionPreset.value, 10);
    if (!Number.isFinite(index) || index < 0 || index >= resolutionPresets.length) return;
    const preset = resolutionPresets[index];
    el.width.value = String(preset.width);
    el.height.value = String(preset.height);
    updatePreviewSizing();
    syncVisualFrameAspect();
  });
  el.width.addEventListener("input", onSizeChanged);
  el.width.addEventListener("change", onSizeChanged);
  el.height.addEventListener("input", onSizeChanged);
  el.height.addEventListener("change", onSizeChanged);
  window.addEventListener("resize", () => {
    applyPreviewTransform();
    if (visualEditor) visualEditor.resize();
  });

  el.pollInterval.addEventListener("change", () => {
    const v = parseInt(el.pollInterval.value || "300", 10);
    uiOptions.pollMs = Number.isFinite(v) ? Math.max(100, Math.min(10000, v)) : 300;
    el.pollInterval.value = String(uiOptions.pollMs);
    persistUIOptions();
    appendLog(`poll interval=${uiOptions.pollMs}ms`);
  });

  el.autoLoadEditor.addEventListener("change", () => {
    uiOptions.autoLoadEditor = !!el.autoLoadEditor.checked;
    persistUIOptions();
    appendLog(`auto-load editor=${uiOptions.autoLoadEditor ? "on" : "off"}`);
  });

  el.autoScrollLogs.addEventListener("change", () => {
    uiOptions.autoScrollLogs = !!el.autoScrollLogs.checked;
    persistUIOptions();
    if (uiOptions.autoScrollLogs) scrollLogToBottom(true);
    appendLog(`auto-scroll logs=${uiOptions.autoScrollLogs ? "on" : "off"}`);
  });

  el.clearPreviewOnRender.addEventListener("change", () => {
    uiOptions.clearPreviewOnRender = !!el.clearPreviewOnRender.checked;
    persistUIOptions();
    appendLog(`clear preview before render=${uiOptions.clearPreviewOnRender ? "on" : "off"}`);
  });

  if (el.fontSizeDown) {
    el.fontSizeDown.addEventListener("click", () => {
      applyFontScale(uiOptions.fontScale - 0.05);
      persistUIOptions();
      appendLog(`ui font size=${Math.round(uiOptions.fontScale * 100)}%`);
    });
  }

  if (el.fontSizeUp) {
    el.fontSizeUp.addEventListener("click", () => {
      applyFontScale(uiOptions.fontScale + 0.05);
      persistUIOptions();
      appendLog(`ui font size=${Math.round(uiOptions.fontScale * 100)}%`);
    });
  }

  el.tabRender.addEventListener("click", () => setActiveTab("render"));
  el.tabVisual.addEventListener("click", () => setActiveTab("visual"));
  el.tabEditor.addEventListener("click", () => setActiveTab("editor"));
  el.tabSettings.addEventListener("click", () => setActiveTab("settings"));
  el.tabLogs.addEventListener("click", () => setActiveTab("logs"));
  el.tabAbout.addEventListener("click", () => setActiveTab("about"));

  el.loadSceneBtn.addEventListener("click", () => {
    loadSceneSource(el.scene.value)
      .then(() => {
        setStatus(`loaded ${el.scene.value}`);
        appendLog(`loaded source: ${el.scene.value}`);
      })
      .catch((err) => {
        setStatus(`error: ${err.message}`);
        appendLog(`load source error: ${err.message}`);
      });
  });

  el.newSceneBtn.addEventListener("click", () => {
    loadEmptySceneTemplate()
      .then((source) => {
        el.sceneName.value = "new_scene.scn";
        el.sceneSource.value = source || "";
        updateEditorMetrics();
        syncEditorScroll();
        setStatus("new scene initialized");
        appendLog("new scene template");
      })
      .catch((err) => {
        setStatus(`error: ${err.message}`);
        appendLog(`new scene template error: ${err.message}`);
      });
  });

  el.saveSceneBtn.addEventListener("click", triggerSceneSave);

  el.clearLogsBtn.addEventListener("click", () => {
    el.logOutput.textContent = "";
  });

  el.sceneSource.addEventListener("input", updateEditorMetrics);
  el.sceneSource.addEventListener("keydown", handleEditorTabKey);
  el.sceneSource.addEventListener("scroll", syncEditorScroll);
  el.sceneSource.addEventListener("keyup", syncEditorScroll);
  el.sceneSource.addEventListener("click", syncEditorScroll);
  updateEditorMetrics();
  syncEditorScroll();
}

boot().catch((err) => {
  setStatus(`error: ${err.message}`);
  appendLog(`boot error: ${err.message}`);
});
