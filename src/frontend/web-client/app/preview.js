function updatePreviewSizing() {
  syncPreviewFrameSquareSize();
  applyPreviewTransform();
}

const previewState = {
  pollingJobId: "",

  minimapLayout: null,
  layoutObserver: null,

  // Persistent canvas for progressive tile accumulation.
  // Tiles are drawn directly here via putImageData (raw) or drawImage (PNG),
  // eliminating the per-tile toBlob() → fetch → decode → redraw cycle.
  tileAccumCanvas: null,
  tileAccumCtx: null,

  // Coalescing guard for refreshPreviewForToneMapping.
  // Prevents concurrent fetches; ensures the last slider position always lands.
  tmInFlight: false,
  tmPending: false,
};

function resetTileAccumCanvas() {
  previewState.tileAccumCanvas = null;
  previewState.tileAccumCtx = null;
}

function ensureTileAccumCanvas(width, height) {
  if (!previewState.tileAccumCanvas || previewState.tileAccumCanvas.width !== width || previewState.tileAccumCanvas.height !== height) {
    previewState.tileAccumCanvas = document.createElement("canvas");
    previewState.tileAccumCanvas.width = width;
    previewState.tileAccumCanvas.height = height;
    previewState.tileAccumCtx = previewState.tileAccumCanvas.getContext("2d");
  }
  return previewState.tileAccumCtx;
}

function normalizeRenderMode(value) {
  const mode = String(value || "").trim().toLowerCase();
  if (mode === "normal") return RENDER_MODE_DIRECT;
  if (mode === RENDER_MODE_PROGRESSIVE) return RENDER_MODE_PROGRESSIVE;
  if (mode === RENDER_MODE_INCREMENTAL) return RENDER_MODE_INCREMENTAL;
  if (mode === RENDER_MODE_INTERACTIVE) return RENDER_MODE_INTERACTIVE;
  return RENDER_MODE_DIRECT;
}

function isInteractiveRenderMode() {
  return normalizeRenderMode(renderMode) === RENDER_MODE_INTERACTIVE;
}

function isProgressiveRenderMode() {
  const mode = normalizeRenderMode(renderMode);
  return mode === RENDER_MODE_PROGRESSIVE || mode === RENDER_MODE_INCREMENTAL;
}


function isTileHeatmapEnabled() {
  return !!uiOptions.tileHeatmapEnabled;
}

function tileRectKey(x0, y0, x1, y1) {
  return `${x0},${y0},${x1},${y1}`;
}

function resetTileHeatmapState(jobId) {
  tileHeatmapState.jobId = String(jobId || "").trim();
  tileHeatmapState.tiles.clear();
  tileHeatmapState.progressSamples = [];
  tileHeatmapState.throughputTilesPerSec = 0;
  tileHeatmapState.etaMs = 0;
  tileHeatmapState.etaConfidence = "low";
  tileHeatmapState.bottleneckHint = "-";
  tileHeatmapState.totalTilesEstimate = 0;
  tileHeatmapState.buckets = { fast: 0, medium: 0, slow: 0 };
  renderTileHeatmapStats();
}

function estimateTileBottleneck(activeCount, progress) {
  if (progress >= 0.995) return "Finalizing frame";
  if (tileHeatmapState.throughputTilesPerSec <= 0.5 && activeCount > 0) return "Heavy shading per tile";
  const estimate = Math.max(0, Number(tileHeatmapState.totalTilesEstimate) || 0);
  if (activeCount >= Math.max(4, Math.round(estimate * 0.2))) return "Sampling-bound (many active tiles)";
  if (activeCount <= 2 && progress < 0.95) return "Hotspot tiles (caustics/complex geometry)";
  return "Balanced";
}

function recomputeTileBuckets() {
  const durations = [];
  tileHeatmapState.tiles.forEach((entry) => {
    if (!entry || !entry.done || !Number.isFinite(entry.durationMs) || entry.durationMs <= 0) return;
    durations.push(entry.durationMs);
  });
  if (!durations.length) {
    tileHeatmapState.buckets = { fast: 0, medium: 0, slow: 0 };
    return;
  }
  durations.sort((a, b) => a - b);
  const mid = durations[Math.floor(durations.length / 2)];
  const fastCut = Math.max(80, mid * 0.75);
  const slowCut = Math.max(fastCut + 1, mid * 1.5);
  let fast = 0;
  let medium = 0;
  let slow = 0;
  for (let i = 0; i < durations.length; i += 1) {
    const d = durations[i];
    if (d <= fastCut) fast += 1;
    else if (d >= slowCut) slow += 1;
    else medium += 1;
  }
  tileHeatmapState.buckets = { fast, medium, slow };
}

function refreshThroughputEta(progress, activeCount) {
  const now = Date.now();
  const p = clamp(Number(progress) || 0, 0, 1);
  tileHeatmapState.progressSamples.push({ ms: now, p });
  const horizonMs = 30000;
  tileHeatmapState.progressSamples = tileHeatmapState.progressSamples.filter((s) => (now - s.ms) <= horizonMs);
  if (tileHeatmapState.progressSamples.length < 2) {
    tileHeatmapState.throughputTilesPerSec = 0;
    tileHeatmapState.etaMs = 0;
    tileHeatmapState.etaConfidence = "low";
    tileHeatmapState.bottleneckHint = estimateTileBottleneck(activeCount, p);
    return;
  }

  const doneCount = Array.from(tileHeatmapState.tiles.values()).filter((e) => e && e.done).length;
  const observed = doneCount + Math.max(0, Number(activeCount) || 0);
  if (p > 0.001 && observed > 0) {
    const estimatedTotal = Math.round(observed / p);
    if (estimatedTotal > 0) {
      tileHeatmapState.totalTilesEstimate = Math.max(tileHeatmapState.totalTilesEstimate, estimatedTotal);
    }
  }
  const total = Math.max(tileHeatmapState.totalTilesEstimate, observed, 0);

  const first = tileHeatmapState.progressSamples[0];
  const last = tileHeatmapState.progressSamples[tileHeatmapState.progressSamples.length - 1];
  const dp = Math.max(0, last.p - first.p);
  const dtMs = Math.max(1, last.ms - first.ms);
  if (total > 0 && dp > 0) {
    tileHeatmapState.throughputTilesPerSec = (dp * total) / (dtMs / 1000);
  } else {
    tileHeatmapState.throughputTilesPerSec = 0;
  }

  if (tileHeatmapState.throughputTilesPerSec > 0 && total > 0) {
    const remainingTiles = Math.max(0, total * (1 - p));
    tileHeatmapState.etaMs = (remainingTiles / tileHeatmapState.throughputTilesPerSec) * 1000;
  } else {
    tileHeatmapState.etaMs = 0;
  }

  const inst = [];
  for (let i = 1; i < tileHeatmapState.progressSamples.length; i += 1) {
    const a = tileHeatmapState.progressSamples[i - 1];
    const b = tileHeatmapState.progressSamples[i];
    const dpLocal = Math.max(0, b.p - a.p);
    const dtLocal = Math.max(1, b.ms - a.ms);
    if (dpLocal <= 0 || total <= 0) continue;
    inst.push((dpLocal * total) / (dtLocal / 1000));
  }
  if (inst.length < 3) {
    tileHeatmapState.etaConfidence = "low";
  } else {
    const mean = inst.reduce((acc, v) => acc + v, 0) / inst.length;
    const variance = inst.reduce((acc, v) => acc + ((v - mean) * (v - mean)), 0) / inst.length;
    const std = Math.sqrt(Math.max(0, variance));
    const cv = mean > 1e-6 ? (std / mean) : 1;
    if (cv < 0.22) tileHeatmapState.etaConfidence = "high";
    else if (cv < 0.55) tileHeatmapState.etaConfidence = "medium";
    else tileHeatmapState.etaConfidence = "low";
  }

  tileHeatmapState.bottleneckHint = estimateTileBottleneck(activeCount, p);
}

function setTileHeatmapRow(node, label, value) {
  if (!node) return;
  if (window.XTracerWidgets && typeof window.XTracerWidgets.renderStatHint === "function") {
    window.XTracerWidgets.renderStatHint(node, {
      label,
      value,
      className: "workspace-active-hint",
    });
    return;
  }
  node.innerHTML = `<span class="workspace-active-label">${label}</span><code class="workspace-active-value">${value}</code>`;
}

function renderTileHeatmapStats() {
  if (!isTileHeatmapEnabled()) {
    setTileHeatmapRow(el.tileHeatmapBuckets, "Buckets", "(disabled)");
    setTileHeatmapRow(el.tileHeatmapThroughput, "Throughput", "(disabled)");
    setTileHeatmapRow(el.tileHeatmapEta, "ETA", "(disabled)");
    setTileHeatmapRow(el.tileHeatmapConfidence, "ETA Confidence", "(disabled)");
    setTileHeatmapRow(el.tileHeatmapBottleneck, "Bottleneck Hint", "(disabled)");
    return;
  }
  if (el.tileHeatmapBuckets) {
    const b = tileHeatmapState.buckets || { fast: 0, medium: 0, slow: 0 };
    setTileHeatmapRow(el.tileHeatmapBuckets, "Buckets", `fast ${b.fast} | mid ${b.medium} | slow ${b.slow}`);
  }
  if (el.tileHeatmapThroughput) {
    const tps = Number(tileHeatmapState.throughputTilesPerSec) || 0;
    setTileHeatmapRow(el.tileHeatmapThroughput, "Throughput", `${tps > 0 ? tps.toFixed(1) : "-"} tiles/s`);
  }
  if (el.tileHeatmapEta) {
    const etaMs = Number(tileHeatmapState.etaMs) || 0;
    setTileHeatmapRow(el.tileHeatmapEta, "ETA", etaMs > 0 ? formatElapsed(etaMs) : "-");
  }
  if (el.tileHeatmapConfidence) {
    const c = String(tileHeatmapState.etaConfidence || "low").toUpperCase();
    setTileHeatmapRow(el.tileHeatmapConfidence, "ETA Confidence", c);
  }
  if (el.tileHeatmapBottleneck) {
    setTileHeatmapRow(el.tileHeatmapBottleneck, "Bottleneck Hint", tileHeatmapState.bottleneckHint || "-");
  }
}

function isNearestPreviewSampling() {
  return String(uiOptions.previewSampling || "").toLowerCase() === "nearest";
}

function hasPreviewImage() {
  if (previewState.tileAccumCanvas && previewState.tileAccumCanvas.width > 0 && previewState.tileAccumCanvas.height > 0) return true;
  return !el.previewFrame.classList.contains("is-empty")
    && !!el.preview.getAttribute("src")
    && !!el.preview.naturalWidth
    && !!el.preview.naturalHeight;
}

function getPreviewFittedSize() {
  const frameW = el.previewFrame.clientWidth;
  const frameH = el.previewFrame.clientHeight;
  const imgW = previewState.tileAccumCanvas ? previewState.tileAccumCanvas.width : el.preview.naturalWidth;
  const imgH = previewState.tileAccumCanvas ? previewState.tileAccumCanvas.height : el.preview.naturalHeight;
  if (!frameW || !frameH || !imgW || !imgH) return null;
  const fit = Math.min(frameW / imgW, frameH / imgH);
  return {
    frameW,
    frameH,
    width: imgW * fit,
    height: imgH * fit,
  };
}

function syncPreviewFrameSquareSize() {
  if (!el.previewFrame) return;
  const panel = el.previewFrame.closest(".panel-preview");
  if (!panel) return;
  const toolbar = panel.querySelector(".preview-toolbar");
  const aux = document.getElementById("renderPreviewAuxPanel");
  const panelWidth = Math.max(0, Math.floor(panel.clientWidth || 0));
  const panelHeight = Math.max(0, Math.floor(panel.clientHeight || 0));
  if (panelWidth <= 0) {
    el.previewFrame.style.removeProperty("--preview-frame-size");
    return;
  }

  let size = panelWidth;
  if (panelHeight > 0) {
    const styles = window.getComputedStyle(panel);
    const gap = parseFloat(styles.rowGap || styles.gap || "0");
    const toolbarHeight = toolbar ? Math.max(0, Math.ceil(toolbar.getBoundingClientRect().height || 0)) : 0;
    const auxVisible = !!(aux && !aux.hidden);
    const auxHeight = auxVisible ? Math.max(0, Math.ceil(aux.getBoundingClientRect().height || 0)) : 0;
    const gapCount = 1 + (auxVisible ? 1 : 0);
    const availableHeight = panelHeight - toolbarHeight - auxHeight - (Number.isFinite(gap) ? gap * gapCount : 0);
    if (Number.isFinite(availableHeight) && availableHeight > 0) {
      size = Math.min(size, Math.floor(availableHeight));
    }
  }

  if (!Number.isFinite(size) || size <= 0) {
    el.previewFrame.style.removeProperty("--preview-frame-size");
    return;
  }
  el.previewFrame.style.setProperty("--preview-frame-size", `${size}px`);
}

function bindPreviewLayoutObserver() {
  if (!el.previewFrame || previewState.layoutObserver) return;
  const panel = el.previewFrame.closest(".panel-preview");
  const toolbar = panel ? panel.querySelector(".preview-toolbar") : null;
  const aux = document.getElementById("renderPreviewAuxPanel");
  if (typeof ResizeObserver !== "function" || !panel) {
    syncPreviewFrameSquareSize();
    return;
  }
  previewState.layoutObserver = new ResizeObserver(() => {
    syncPreviewFrameSquareSize();
    applyPreviewTransform();
  });
  previewState.layoutObserver.observe(panel);
  if (toolbar) previewState.layoutObserver.observe(toolbar);
  if (aux) previewState.layoutObserver.observe(aux);
  syncPreviewFrameSquareSize();
}

function clearPreviewCanvas() {
  if (!el.previewCanvas) return;
  previewState.minimapLayout = null;
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

function computePreviewMinimapLayout(dims, fitted, imageX, imageY, imageW, imageH) {
  if (!dims || !fitted) return null;
  if (!hasPreviewImage()) return null;
  if (!(previewView.scale > 1.001 || Math.abs(previewView.tx) > 0.5 || Math.abs(previewView.ty) > 0.5)) {
    return null;
  }

  const miniMargin = 14;
  const miniSize = clamp(Math.round(Math.min(dims.cssW, dims.cssH) * 0.22), 96, 180);
  const miniX = dims.cssW - miniSize - miniMargin;
  const miniY = dims.cssH - miniSize - miniMargin;
  const srcW = previewState.tileAccumCanvas ? previewState.tileAccumCanvas.width : el.preview.naturalWidth;
  const srcH = previewState.tileAccumCanvas ? previewState.tileAccumCanvas.height : el.preview.naturalHeight;
  const aspect = Math.max(1e-6, srcW / Math.max(1, srcH));
  let mapW = miniSize;
  let mapH = Math.round(mapW / aspect);
  if (mapH > miniSize) {
    mapH = miniSize;
    mapW = Math.round(mapH * aspect);
  }
  const mapX = miniX + Math.round((miniSize - mapW) * 0.5);
  const mapY = miniY + Math.round((miniSize - mapH) * 0.5);

  return {
    miniX,
    miniY,
    miniSize,
    mapX,
    mapY,
    mapW,
    mapH,
    imageW,
    imageH,
  };
}

function getPreviewMinimapHit(clientX, clientY, clampToBounds) {
  if (!previewState.minimapLayout || !el.previewFrame) return null;
  const rect = el.previewFrame.getBoundingClientRect();
  const localX = clientX - rect.left;
  const localY = clientY - rect.top;
  const layout = previewState.minimapLayout;
  const minX = layout.mapX;
  const maxX = layout.mapX + layout.mapW;
  const minY = layout.mapY;
  const maxY = layout.mapY + layout.mapH;
  if (!clampToBounds) {
    if (localX < minX || localX > maxX) return null;
    if (localY < minY || localY > maxY) return null;
  }
  const hitX = clampToBounds ? clamp(localX, minX, maxX) : localX;
  const hitY = clampToBounds ? clamp(localY, minY, maxY) : localY;
  return {
    u: clamp((hitX - layout.mapX) / Math.max(1e-6, layout.mapW), 0, 1),
    v: clamp((hitY - layout.mapY) / Math.max(1e-6, layout.mapH), 0, 1),
    layout,
  };
}

function recenterPreviewFromMinimap(clientX, clientY, clampToBounds) {
  const hit = getPreviewMinimapHit(clientX, clientY, !!clampToBounds);
  if (!hit) return false;
  previewView.tx = (0.5 - hit.u) * hit.layout.imageW;
  previewView.ty = (0.5 - hit.v) * hit.layout.imageH;
  applyPreviewTransform();
  return true;
}

function drawPreviewMinimap(ctx, dims, fitted, imageX, imageY, imageW, imageH) {
  previewState.minimapLayout = null;
  if (!ctx || !dims || !fitted) return;
  if (!hasPreviewImage()) return;
  if (!(previewView.scale > 1.001 || Math.abs(previewView.tx) > 0.5 || Math.abs(previewView.ty) > 0.5)) return;

  const layout = computePreviewMinimapLayout(dims, fitted, imageX, imageY, imageW, imageH);
  if (!layout) return;
  previewState.minimapLayout = layout;
  const miniX = layout.miniX;
  const miniY = layout.miniY;
  const miniSize = layout.miniSize;
  const mapX = layout.mapX;
  const mapY = layout.mapY;
  const mapW = layout.mapW;
  const mapH = layout.mapH;

  ctx.save();
  ctx.fillStyle = "rgba(9, 16, 24, 0.62)";
  ctx.strokeStyle = "rgba(173, 214, 255, 0.72)";
  ctx.lineWidth = 1.25;
  ctx.beginPath();
  ctx.roundRect(miniX - 6, miniY - 6, miniSize + 12, miniSize + 12, 10);
  ctx.fill();
  ctx.stroke();

  ctx.save();
  ctx.beginPath();
  ctx.rect(mapX, mapY, mapW, mapH);
  ctx.clip();
  ctx.imageSmoothingEnabled = !isNearestPreviewSampling();
  ctx.drawImage(previewState.tileAccumCanvas || el.preview, mapX, mapY, mapW, mapH);
  ctx.restore();

  const imgToMapX = mapW / Math.max(1e-6, imageW);
  const imgToMapY = mapH / Math.max(1e-6, imageH);
  const viewLeft = Math.max(0, -imageX);
  const viewTop = Math.max(0, -imageY);
  const viewRight = Math.min(imageW, fitted.frameW - imageX);
  const viewBottom = Math.min(imageH, fitted.frameH - imageY);

  if (viewRight > viewLeft && viewBottom > viewTop) {
    const rectX = mapX + viewLeft * imgToMapX;
    const rectY = mapY + viewTop * imgToMapY;
    const rectW = Math.max(6, (viewRight - viewLeft) * imgToMapX);
    const rectH = Math.max(6, (viewBottom - viewTop) * imgToMapY);
    ctx.fillStyle = "rgba(255, 214, 92, 0.14)";
    ctx.strokeStyle = "rgba(255, 196, 64, 0.95)";
    ctx.lineWidth = 1.5;
    ctx.fillRect(rectX, rectY, rectW, rectH);
    ctx.strokeRect(rectX, rectY, rectW, rectH);
  }

  ctx.strokeStyle = "rgba(232, 240, 248, 0.78)";
  ctx.lineWidth = 1;
  ctx.strokeRect(mapX, mapY, mapW, mapH);
  ctx.restore();
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
  ctx.drawImage(previewState.tileAccumCanvas || el.preview, x, y, drawW, drawH);
  drawActivePreviewTileOverlay(ctx, x, y, drawW, drawH);
  drawPreviewMinimap(ctx, dims, fitted, x, y, drawW, drawH);
}

function drawActivePreviewTileOverlay(ctx, imageX, imageY, imageW, imageH) {
  if (!ctx || !activePreviewTiles.length) return;
  const srcW = Number(activePreviewTileWidth) || (previewState.tileAccumCanvas ? previewState.tileAccumCanvas.width : el.preview.naturalWidth) || 0;
  const srcH = Number(activePreviewTileHeight) || (previewState.tileAccumCanvas ? previewState.tileAccumCanvas.height : el.preview.naturalHeight) || 0;
  if (srcW <= 0 || srcH <= 0 || imageW <= 0 || imageH <= 0) return;

  const sx = imageW / srcW;
  const sy = imageH / srcH;
  const now = Date.now();
  let maxActiveMs = 1;
  if (isTileHeatmapEnabled()) {
    for (const t of activePreviewTiles) {
      if (!Array.isArray(t) || t.length < 4) continue;
      const key = tileRectKey(Number(t[0]), Number(t[1]), Number(t[2]), Number(t[3]));
      const entry = tileHeatmapState.tiles.get(key);
      if (!entry) continue;
      const activeMs = Math.max(1, Number(entry.activeMs) || 0) + Math.max(0, now - (Number(entry.lastSeenMs) || now));
      if (activeMs > maxActiveMs) maxActiveMs = activeMs;
    }
  }

  ctx.save();
  ctx.lineWidth = 1;
  for (const t of activePreviewTiles) {
    if (!Array.isArray(t) || t.length < 4) continue;
    const x0 = Number(t[0]);
    const y0 = Number(t[1]);
    const x1 = Number(t[2]);
    const y1 = Number(t[3]);
    if (!Number.isFinite(x0) || !Number.isFinite(y0) || !Number.isFinite(x1) || !Number.isFinite(y1)) continue;
    const ox = imageX + x0 * sx;
    const oy = imageY + y0 * sy;
    const ow = Math.max(1, (x1 - x0) * sx);
    const oh = Math.max(1, (y1 - y0) * sy);

    if (isTileHeatmapEnabled()) {
      const key = tileRectKey(x0, y0, x1, y1);
      const entry = tileHeatmapState.tiles.get(key);
      const activeMs = entry
        ? (Math.max(1, Number(entry.activeMs) || 0) + Math.max(0, now - (Number(entry.lastSeenMs) || now)))
        : 1;
      const heat = clamp(activeMs / Math.max(1, maxActiveMs), 0, 1);
      const r = Math.round(90 + 165 * heat);
      const g = Math.round(220 - 150 * heat);
      const b = Math.round(70 - 40 * heat);
      ctx.fillStyle = `rgba(${r}, ${g}, ${b}, ${0.16 + 0.26 * heat})`;
      ctx.strokeStyle = `rgba(${Math.min(255, r + 22)}, ${Math.max(0, g - 14)}, ${Math.max(0, b - 10)}, 0.95)`;
    } else {
      ctx.fillStyle = "rgba(255, 48, 48, 0.22)";
      ctx.strokeStyle = "rgba(255, 90, 90, 0.95)";
    }
    ctx.fillRect(ox, oy, ow, oh);
    ctx.strokeRect(ox + 0.5, oy + 0.5, Math.max(0, ow - 1), Math.max(0, oh - 1));
  }
  ctx.restore();
}

function updateActivePreviewTilesFromJob(data) {
  const now = Date.now();
  const stateJobId = String(activeJobId || progressiveDeltaJobId || "");
  if (stateJobId && tileHeatmapState.jobId !== stateJobId) {
    resetTileHeatmapState(stateJobId);
  }
  const nextTiles = Array.isArray(data && data.active_tiles) ? data.active_tiles : [];
  activePreviewTiles = nextTiles;
  activePreviewTileWidth = Number(data && data.width) || 0;
  activePreviewTileHeight = Number(data && data.height) || 0;

  const activeKeys = new Set();
  for (let i = 0; i < nextTiles.length; i += 1) {
    const t = nextTiles[i];
    if (!Array.isArray(t) || t.length < 4) continue;
    const x0 = Number(t[0]);
    const y0 = Number(t[1]);
    const x1 = Number(t[2]);
    const y1 = Number(t[3]);
    if (!Number.isFinite(x0) || !Number.isFinite(y0) || !Number.isFinite(x1) || !Number.isFinite(y1)) continue;
    const key = tileRectKey(x0, y0, x1, y1);
    activeKeys.add(key);
    let entry = tileHeatmapState.tiles.get(key);
    if (!entry) {
      entry = {
        firstSeenMs: now,
        lastSeenMs: now,
        activeMs: 0,
        seenCount: 0,
        done: false,
        durationMs: 0,
      };
      tileHeatmapState.tiles.set(key, entry);
    } else {
      entry.activeMs += Math.max(0, now - (Number(entry.lastSeenMs) || now));
      entry.lastSeenMs = now;
      entry.done = false;
    }
    entry.seenCount += 1;
  }

  const finalizeMs = Math.max(220, Math.floor((Number(uiOptions.pollMs) || 300) * 1.4));
  tileHeatmapState.tiles.forEach((entry, key) => {
    if (!entry || entry.done) return;
    if (activeKeys.has(key)) return;
    if ((now - (Number(entry.lastSeenMs) || now)) < finalizeMs) return;
    entry.done = true;
    entry.durationMs = Math.max(1, Number(entry.activeMs) || 0);
  });

  recomputeTileBuckets();
  refreshThroughputEta(Number(data && data.progress) || 0, nextTiles.length);
  renderTileHeatmapStats();
}

function clearActivePreviewTiles() {
  const now = Date.now();
  tileHeatmapState.tiles.forEach((entry) => {
    if (!entry || entry.done) return;
    entry.done = true;
    entry.durationMs = Math.max(1, Number(entry.activeMs) || 0) + Math.max(0, now - (Number(entry.lastSeenMs) || now));
  });
  recomputeTileBuckets();
  renderTileHeatmapStats();
  activePreviewTiles = [];
  activePreviewTileWidth = 0;
  activePreviewTileHeight = 0;
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
  const isZoomed = previewView.scale > 1.001 || Math.abs(previewView.tx) > 0.5 || Math.abs(previewView.ty) > 0.5;
  updateResetViewUi(isZoomed);
  if (!hasPreviewImage()) {
    if (el.preview.getAttribute("src")) {
      // Keep the last drawn frame visible while the next blob is decoding.
      return;
    }
    el.previewFrame.classList.remove("is-zoomed");
    el.previewFrame.classList.remove("is-panning");
    clearPreviewCanvas();
    if (!isZoomed) updateResetViewUi(false);
    return;
  }
  clampPreviewPan();
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
  updateResetViewUi(false);
}

function updateResetViewUi(enabled) {
  if (!el.resetViewBtn) return;
  const active = !!enabled;
  el.resetViewBtn.hidden = !active;
  el.resetViewBtn.classList.toggle("is-disabled", !active);
  el.resetViewBtn.disabled = !active;
  el.resetViewBtn.setAttribute("aria-disabled", active ? "false" : "true");
}

async function setInteractivePreviewEnabled(enabled) {
  return setRenderMode(enabled ? RENDER_MODE_INTERACTIVE : RENDER_MODE_DIRECT, { log: true });
}

async function setRenderMode(nextModeRaw, options) {
  const opts = options && typeof options === "object" ? options : {};
  const nextMode = normalizeRenderMode(nextModeRaw);
  const prevInteractive = !!interactivePreviewEnabled;
  const changed = normalizeRenderMode(renderMode) !== nextMode;
  renderMode = nextMode;
  interactivePreviewEnabled = isInteractiveRenderMode();
  if (el.renderMode) {
    el.renderMode.value = renderMode;
  }
  if (interactivePreviewEnabled) {
    ensureInteractiveFlyTicker();
    markInteractiveInputActivity();
    const interactiveTargetWidth = Math.max(32, Number.parseInt(String(el.width && el.width.value ? el.width.value : "500"), 10) || 500);
    interactivePreviewAdaptiveMovingWidth = nearestInteractiveMovingWidth(Math.round(interactiveTargetWidth * 0.1), interactiveTargetWidth);
    interactivePreviewHudMode = "LOOK";
    interactivePreviewHudQuality = "active";
    renderInteractivePreviewHud();
    const ok = await refreshInteractivePreviewCameraFromSelection();
    if (ok) {
      if (opts.log !== false) appendLog("interactive preview on");
      if (typeof requestInteractivePreviewRender === "function") requestInteractivePreviewRender();
    } else {
      if (opts.log !== false) appendLog("interactive preview unavailable for selected camera");
    }
  } else {
    stopInteractiveFlyTicker();
    interactivePreviewHudQuality = "idle";
    interactivePreviewHudMode = "LOOK";
    renderInteractivePreviewHud();
    appendLog("interactive preview off");
    if ((prevInteractive || interactivePreviewLoopActive || interactivePreviewJobId)
      && typeof stopInteractivePreviewLoop === "function") {
      stopInteractivePreviewLoop(true).catch(() => {});
    }
    if (prevInteractive && opts.log !== false) appendLog("interactive preview off");
  }
  if (changed && (opts.log || opts.log === undefined)) {
    appendLog(`render mode=${renderMode}`);
  }
  renderInteractivePreviewHud();
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
  bindPreviewLayoutObserver();
  const touchPoints = {};
  let pinchDistance = 0;
  let pinchCenterX = 0;
  let pinchCenterY = 0;

  if (el.resetViewBtn) {
    const swallowPreviewButtonEvent = (evt) => {
      evt.stopPropagation();
    };
    el.resetViewBtn.addEventListener("pointerdown", swallowPreviewButtonEvent);
    el.resetViewBtn.addEventListener("click", swallowPreviewButtonEvent);
    el.resetViewBtn.addEventListener("dblclick", swallowPreviewButtonEvent);
  }

  el.preview.addEventListener("load", () => {
    if (previewPendingRevokeUrl && previewPendingRevokeUrl !== previewPinnedBaseUrl) {
      URL.revokeObjectURL(previewPendingRevokeUrl);
    }
    previewPendingRevokeUrl = "";
    applyPreviewTransform();
  });

  el.previewFrame.addEventListener("contextmenu", (evt) => {
    if (interactivePreviewAvailable()) evt.preventDefault();
  });

  el.previewFrame.addEventListener("wheel", (evt) => {
    const interactive = interactivePreviewAvailable();
    if (!interactive && !hasPreviewImage()) return;
    evt.preventDefault();
    if (interactive) {
      interactiveZoomCamera(evt.deltaY);
      return;
    }
    zoomPreviewAt(evt.clientX, evt.clientY, evt.deltaY);
  }, { passive: false });

  el.previewFrame.addEventListener("dblclick", (evt) => {
    const interactive = interactivePreviewAvailable();
    if (!interactive && !hasPreviewImage()) return;
    evt.preventDefault();
    if (interactive) {
      refreshInteractivePreviewCameraFromSelection()
        .then((ok) => {
          if (ok && typeof requestInteractivePreviewRender === "function") requestInteractivePreviewRender();
        })
        .catch(() => {});
      return;
    }
    resetPreviewView();
  });

  el.previewFrame.addEventListener("pointerdown", (evt) => {
    if (evt.target instanceof Element && evt.target.closest("#resetViewBtn")) return;
    const interactive = interactivePreviewAvailable();
    if (!interactive && !hasPreviewImage()) return;
    if (evt.button !== 0 && evt.button !== 1 && evt.button !== 2) return;
    evt.preventDefault();
    if (interactive) {
      if (evt.pointerType === "touch") {
        touchPoints[String(evt.pointerId)] = { x: evt.clientX, y: evt.clientY };
        if (Object.keys(touchPoints).length >= 2) {
          const ids = Object.keys(touchPoints).slice(0, 2);
          const a = touchPoints[ids[0]];
          const b = touchPoints[ids[1]];
          pinchCenterX = (a.x + b.x) * 0.5;
          pinchCenterY = (a.y + b.y) * 0.5;
          pinchDistance = Math.hypot(a.x - b.x, a.y - b.y);
        }
      }
      previewView.panning = true;
      previewView.pointerId = evt.pointerId;
      previewView.lastX = evt.clientX;
      previewView.lastY = evt.clientY;
      previewView.panMode = (evt.button === 1 || evt.button === 2 || evt.shiftKey) ? "pan" : "orbit";
      interactivePreviewHudMode = previewView.panMode === "pan" ? "PAN" : "LOOK";
      renderInteractivePreviewHud();
      el.previewFrame.setPointerCapture(evt.pointerId);
      return;
    }
    if (evt.button === 0 && recenterPreviewFromMinimap(evt.clientX, evt.clientY)) {
      previewView.panning = true;
      previewView.panMode = "minimap";
      previewView.pointerId = evt.pointerId;
      previewView.lastX = evt.clientX;
      previewView.lastY = evt.clientY;
      el.previewFrame.setPointerCapture(evt.pointerId);
      return;
    }
    previewView.panning = true;
    previewView.panMode = "image";
    previewView.pointerId = evt.pointerId;
    previewView.lastX = evt.clientX;
    previewView.lastY = evt.clientY;
    el.previewFrame.setPointerCapture(evt.pointerId);
    applyPreviewTransform();
  });

  el.previewFrame.addEventListener("pointermove", (evt) => {
    if (interactivePreviewAvailable()) {
      if (evt.pointerType === "touch" && Object.prototype.hasOwnProperty.call(touchPoints, String(evt.pointerId))) {
        touchPoints[String(evt.pointerId)] = { x: evt.clientX, y: evt.clientY };
        const ids = Object.keys(touchPoints);
        if (ids.length >= 2) {
          const a = touchPoints[ids[0]];
          const b = touchPoints[ids[1]];
          const centerX = (a.x + b.x) * 0.5;
          const centerY = (a.y + b.y) * 0.5;
          const dist = Math.max(1e-6, Math.hypot(a.x - b.x, a.y - b.y));
          if (pinchDistance > 1e-6) {
            const zoomDelta = Math.log(dist / pinchDistance) * 1000;
            interactiveZoomCamera(zoomDelta);
          }
          if (Number.isFinite(pinchCenterX) && Number.isFinite(pinchCenterY)) {
            interactivePanCamera(centerX - pinchCenterX, centerY - pinchCenterY);
          }
          pinchDistance = dist;
          pinchCenterX = centerX;
          pinchCenterY = centerY;
          return;
        }
      }
      if (!(previewView.panning && previewView.pointerId === evt.pointerId)) return;
      const dx = evt.clientX - previewView.lastX;
      const dy = evt.clientY - previewView.lastY;
      previewView.lastX = evt.clientX;
      previewView.lastY = evt.clientY;
      if (previewView.panMode === "pan") interactivePanCamera(-dx, -dy);
      else interactiveLookCamera(dx, dy);
      return;
    }
    if (!previewView.panning || previewView.pointerId !== evt.pointerId) return;
    if (previewView.panMode === "minimap") {
      previewView.lastX = evt.clientX;
      previewView.lastY = evt.clientY;
      recenterPreviewFromMinimap(evt.clientX, evt.clientY, true);
      return;
    }
    const dx = evt.clientX - previewView.lastX;
    const dy = evt.clientY - previewView.lastY;
    previewView.lastX = evt.clientX;
    previewView.lastY = evt.clientY;
    previewView.tx += dx;
    previewView.ty += dy;
    applyPreviewTransform();
  });

  const endPan = (evt) => {
    if (evt.pointerType === "touch") {
      delete touchPoints[String(evt.pointerId)];
      const ids = Object.keys(touchPoints);
      if (ids.length < 2) {
        pinchDistance = 0;
        pinchCenterX = 0;
        pinchCenterY = 0;
      }
    }
    if (!previewView.panning || previewView.pointerId !== evt.pointerId) return;
    previewView.panning = false;
    previewView.pointerId = null;
    previewView.panMode = "";
    interactivePreviewHudMode = "LOOK";
    renderInteractivePreviewHud();
    try {
      el.previewFrame.releasePointerCapture(evt.pointerId);
    } catch (_) {
      // Ignore release errors from non-captured pointers.
    }
    if (interactivePreviewAvailable()) return;
    applyPreviewTransform();
  };

  el.previewFrame.addEventListener("pointerup", endPan);
  el.previewFrame.addEventListener("pointercancel", endPan);
  el.previewFrame.addEventListener("pointerleave", (evt) => {
    if (!previewView.panning || previewView.pointerId !== evt.pointerId) return;
    endPan(evt);
  });

  bindInteractivePreviewKeyboard();
  renderInteractivePreviewHud();
}

function setPreviewEmptyState(isEmpty) {
  el.previewFrame.classList.toggle("is-empty", isEmpty);
  if (isEmpty) {
    resetTileAccumCanvas();
    resetTileHeatmapState("");
    previewSwapToken += 1;
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
    syncGlobalsToWorkspaceRuntime();
    resetPreviewView();
    updateDownloadUi();
    return;
  }
  updateDownloadUi();
}

async function setPreviewFromBlob(blob) {
  const url = URL.createObjectURL(blob);
  const token = ++previewSwapToken;
  const probe = new Image();
  const loaded = new Promise((resolve, reject) => {
    probe.onload = () => resolve();
    probe.onerror = () => reject(new Error("preview decode failed"));
  });
  probe.src = url;

  try {
    await loaded;
  } catch (_) {
    URL.revokeObjectURL(url);
    return false;
  }

  if (token !== previewSwapToken) {
    URL.revokeObjectURL(url);
    return false;
  }

  if (previewObjectUrl && previewObjectUrl !== url) {
    previewPendingRevokeUrl = previewObjectUrl;
  }
  previewObjectUrl = url;
  el.preview.src = previewObjectUrl;
  // Clear the tile accumulation canvas so drawPreviewCanvas switches to el.preview.
  resetTileAccumCanvas();
  const nearest = isNearestPreviewSampling();
  el.previewFrame.classList.toggle("sampling-nearest", nearest);
  setPreviewEmptyState(false);
  requestAnimationFrame(() => {
    applyPreviewTransform();
  });
  return true;
}

// Repaints previewState.tileAccumCanvas from a REST-fetched blob (e.g. partial image with
// updated TM/post-FX settings) WITHOUT clearing the canvas or disrupting the
// WS tile accumulation pipeline. Future WS tiles continue to draw on top.
// Safe to call during active rendering; no-op if previewState.tileAccumCanvas is null.
async function repaintTileAccumCanvasFromBlob(blob) {
  if (!previewState.tileAccumCanvas || !previewState.tileAccumCtx) return false;
  try {
    const bmp = await createImageBitmap(blob);
    // Guard against the canvas being cleared while awaiting decode.
    if (!previewState.tileAccumCanvas || !previewState.tileAccumCtx) { bmp.close(); return false; }
    previewState.tileAccumCtx.drawImage(bmp, 0, 0, previewState.tileAccumCanvas.width, previewState.tileAccumCanvas.height);
    bmp.close();
    applyPreviewTransform();
    return true;
  } catch (_) {
    return false;
  }
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

// parseImageDeltaPacket — decode an XTDR or XTD1 binary tile-data packet.
//
// XTDR — xtracer tile-data raw  (WebSocket push, server → client)
// XTD1 — xtracer tile-data v1   (WebSocket catchup on /ws/jobs/<id>, PNG-encoded tiles)
//
// All integers are unsigned 32-bit little-endian.
//
// Header
//   XTDR (32 bytes):
//   [0:4]   magic        "XTDR" (0x58 54 44 52)
//   [4:8]   width        image width in pixels
//   [8:12]  height       image height in pixels
//   [12:16] tiles_done   cumulative finished-tile count
//   [16:20] tiles_total  total tiles for this job
//   [20:24] state        job state integer (see JOB_* constants server-side)
//   [24:28] tile_count   number of tile records that follow
//   [28:32] elapsed_ms   server-authoritative render time in milliseconds (u32)
//
//   XTD1 (28 bytes):
//   [0:4]   magic        "XTD1" (0x58 54 44 31)
//   [4:28]  same as XTDR [4:28] — no elapsed_ms field
//
// Tile record (24 + data_size bytes, repeated tile_count times)
//   x0, y0             top-left corner, 0-based inclusive
//   x1, y1             bottom-right corner, exclusive
//   done_index         tiles_done value when this tile finished
//   data_size          byte count of pixel data
//   data               XTDR: raw RGBA row-major 8bpc (alpha=255 always)
//                      XTD1: PNG-encoded image
//
// Active-tile section — XTDR only, follows tile records (absent in XTD1)
//   active_count       tiles currently in progress
//   per entry: x0 y0 x1 y1 (4 bytes each)
//
// Firing rules (XTDR / WebSocket only)
//   TILE_STARTED:  tile_count=0; active section includes the newly-started tile
//   TILE_FINISHED: tile_count=1; active section already excludes the finished tile
//   Terminal state (done/aborted/error): text JSON only, no binary frame
//
// Server builder: src/frontend/web-server/job_manager.cc (PROGRESS_EVENT_TILE_*)
// Protocol doc:   AGENTS.md § WebSocket Protocol
function parseImageDeltaPacket(buffer) {
  if (!(buffer instanceof ArrayBuffer)) return null;
  if (buffer.byteLength < 28) return null;
  const bytes = new Uint8Array(buffer);
  // Accept "XTD1" (PNG tiles) or "XTDR" (raw RGBA tiles)
  if (bytes[0] !== 0x58 || bytes[1] !== 0x54 || bytes[2] !== 0x44) return null;
  const isRawRgba = bytes[3] === 0x52; // 'R'
  if (bytes[3] !== 0x31 && bytes[3] !== 0x52) return null; // must be '1' or 'R'
  // XTDR header is 32 bytes (includes elapsed_ms); XTD1 header is 28 bytes.
  if (isRawRgba && buffer.byteLength < 32) return null;

  const view = new DataView(buffer);
  let off = 4;
  const readU32 = () => {
    if (off + 4 > buffer.byteLength) return null;
    const v = view.getUint32(off, true);
    off += 4;
    return v;
  };

  const width = readU32();
  const height = readU32();
  const tilesDone = readU32();
  const tilesTotal = readU32();
  const state = readU32();
  const tileCount = readU32();
  if (width === null || height === null || tilesDone === null || tilesTotal === null || state === null || tileCount === null) {
    return null;
  }

  // XTDR carries server-authoritative elapsed_ms at [28:32]; XTD1 does not.
  let elapsedMs = 0;
  if (isRawRgba) {
    const v = readU32();
    if (v === null) return null;
    elapsedMs = v;
  }

  const tiles = [];
  for (let i = 0; i < tileCount; i += 1) {
    const x0 = readU32();
    const y0 = readU32();
    const x1 = readU32();
    const y1 = readU32();
    const doneIndex = readU32();
    const size = readU32();
    if (x0 === null || y0 === null || x1 === null || y1 === null || doneIndex === null || size === null) return null;
    if (off + size > buffer.byteLength) return null;
    const pngBytes = bytes.slice(off, off + size);
    off += size;
    tiles.push({ x0, y0, x1, y1, doneIndex, pngBytes, isRaw: isRawRgba });
  }

  // Active tile section (appended after all finished tiles).
  // Format: active_count(4) | [x0 y0 x1 y1](4 each) * N
  let activeTiles = null;
  if (off + 4 <= buffer.byteLength) {
    const activeCount = view.getUint32(off, true);
    off += 4;
    if (activeCount <= 4096 && off + activeCount * 16 <= buffer.byteLength) {
      activeTiles = [];
      for (let i = 0; i < activeCount; i++) {
        const ax0 = view.getUint32(off, true); off += 4;
        const ay0 = view.getUint32(off, true); off += 4;
        const ax1 = view.getUint32(off, true); off += 4;
        const ay1 = view.getUint32(off, true); off += 4;
        activeTiles.push([ax0, ay0, ax1, ay1]);
      }
    }
  }

  return { width, height, tilesDone, tilesTotal, state, tiles, activeTiles, elapsedMs };
}

async function drawDeltaTilesToPreviewCanvas(delta) {
  if (!delta || !delta.width || !delta.height) return false;

  const ctx = ensureTileAccumCanvas(delta.width, delta.height);
  if (!ctx) return false;

  for (let i = 0; i < delta.tiles.length; i += 1) {
    const t = delta.tiles[i];
    if (!t || !t.pngBytes || !t.pngBytes.length) continue;
    const x0 = Number(t.x0);
    const y0 = Number(t.y0);
    const w = Math.max(1, Number(t.x1) - x0);
    const h = Math.max(1, Number(t.y1) - y0);
    if (t.isRaw) {
      // Raw RGBA — synchronous putImageData, no decode cost
      if (t.pngBytes.length === w * h * 4) {
        const clamped = new Uint8ClampedArray(t.pngBytes.buffer, t.pngBytes.byteOffset, t.pngBytes.length);
        ctx.putImageData(new ImageData(clamped, w, h), x0, y0);
      }
    } else {
      // PNG-encoded tile (from WS catchup)
      const tileBlob = new Blob([t.pngBytes], { type: "image/png" });
      const tileImage = await createImageBitmap(tileBlob);
      ctx.drawImage(tileImage, x0, y0, w, h);
      tileImage.close();
    }
  }

  // Drive display update through the existing transform/redraw path — no toBlob needed.
  setPreviewEmptyState(false);
  applyPreviewTransform();
  return true;
}

function previewToneMappingParamsForJob(jobId) {
  const id = String(jobId || "").trim();
  const isInteractiveMoving = interactivePreviewEnabled
    && !!interactivePreviewActiveMovingJob
    && id
    && id === String(interactivePreviewJobId || "").trim();
  if (isInteractiveMoving) {
    return {
      toneMapping: "none",
      toneMappingExposure: "1.0",
      toneMappingWhitePoint: "1.0",
      toneMappingMantiukContrast: "0.1",
      toneMappingMantiukSaturation: "0.8",
      toneMappingMantiukDetail: "1.0",
      postFiltersEnabled: false,
      postFilters: "",
    };
  }
  return {
    toneMapping: el.toneMapping ? el.toneMapping.value : "aces",
    toneMappingExposure: el.toneMappingExposure ? el.toneMappingExposure.value : "1.0",
    toneMappingWhitePoint: el.toneMappingWhitePoint ? el.toneMappingWhitePoint.value : "1.0",
    toneMappingMantiukContrast: el.toneMappingMantiukContrast ? el.toneMappingMantiukContrast.value : "0.1",
    toneMappingMantiukSaturation: el.toneMappingMantiukSaturation ? el.toneMappingMantiukSaturation.value : "0.8",
    toneMappingMantiukDetail: el.toneMappingMantiukDetail ? el.toneMappingMantiukDetail.value : "1.0",
    postFiltersEnabled: !!postFilterStackEnabled,
    postFilters: gatherPostFilterParams(),
  };
}

async function refreshProgressivePreview(jobId) {
  const id = String(jobId || "").trim();
  const tm = previewToneMappingParamsForJob(id);
  const blob = await api.getJobImage(jobId, {
    partial: true,
    cacheBust: true,
    toneMapping: tm.toneMapping,
    toneMappingExposure: tm.toneMappingExposure,
    toneMappingWhitePoint: tm.toneMappingWhitePoint,
    toneMappingMantiukContrast: tm.toneMappingMantiukContrast,
    toneMappingMantiukSaturation: tm.toneMappingMantiukSaturation,
    toneMappingMantiukDetail: tm.toneMappingMantiukDetail,
    postFiltersEnabled: tm.postFiltersEnabled,
    postFilters: tm.postFilters,
  });
  if (!blob || blob.size === 0) return false;
  if (renderActive && activeJobId && id === String(activeJobId)) {
    recordPreviewTransfer("full", blob.size || 0);
  }

  const imageBlob = preservePreviewUnderlay
    ? await composeWithPinnedPreview(blob)
    : blob;

  return setPreviewFromBlob(imageBlob);
}

async function refreshPreviewForToneMapping() {
  if (typeof renderPostPipelineGraph === "function") renderPostPipelineGraph();
  if (previewState.tmInFlight) {
    previewState.tmPending = true;
    return;
  }
  previewState.tmInFlight = true;
  try {
    if (renderActive && activeJobId) {
      if (previewState.tileAccumCanvas) {
        // WS tile streaming is active. Re-fetch the partial image with the new
        // TM/post-FX settings and repaint the canvas in-place so all completed
        // tiles reflect the new settings. Future WS tiles accumulate on top.
        const id = String(activeJobId || "").trim();
        const tm = previewToneMappingParamsForJob(id);
        const blob = await api.getJobImage(id, {
          partial: true,
          cacheBust: true,
          toneMapping: tm.toneMapping,
          toneMappingExposure: tm.toneMappingExposure,
          toneMappingWhitePoint: tm.toneMappingWhitePoint,
          toneMappingMantiukContrast: tm.toneMappingMantiukContrast,
          toneMappingMantiukSaturation: tm.toneMappingMantiukSaturation,
          toneMappingMantiukDetail: tm.toneMappingMantiukDetail,
          postFiltersEnabled: tm.postFiltersEnabled,
          postFilters: tm.postFilters,
        });
        if (blob && blob.size > 0 && renderActive && activeJobId) {
          await repaintTileAccumCanvasFromBlob(blob);
        }
      } else {
        await refreshProgressivePreview(activeJobId);
      }
    } else if (lastCompletedJobId) {
      const finalBlob = await api.getJobImage(lastCompletedJobId, {
        final: true,
        cacheBust: true,
        toneMapping: el.toneMapping ? el.toneMapping.value : "aces",
        toneMappingExposure: el.toneMappingExposure ? el.toneMappingExposure.value : "1.0",
        toneMappingWhitePoint: el.toneMappingWhitePoint ? el.toneMappingWhitePoint.value : "1.0",
        toneMappingMantiukContrast: el.toneMappingMantiukContrast ? el.toneMappingMantiukContrast.value : "0.1",
        toneMappingMantiukSaturation: el.toneMappingMantiukSaturation ? el.toneMappingMantiukSaturation.value : "0.8",
        toneMappingMantiukDetail: el.toneMappingMantiukDetail ? el.toneMappingMantiukDetail.value : "1.0",
        postFiltersEnabled: !!postFilterStackEnabled,
        postFilters: gatherPostFilterParams(),
      });
      if (finalBlob && finalBlob.size > 0) await setPreviewFromBlob(finalBlob);
    }
  } catch (err) {
    appendLog(`tone mapping preview refresh failed: ${err.message}`);
  } finally {
    previewState.tmInFlight = false;
    if (previewState.tmPending) {
      previewState.tmPending = false;
      refreshPreviewForToneMapping();
    }
  }
}

async function restorePreviewForActiveWorkspace() {
  if (!activeJobId) {
    const jobs = getActiveJobsFromCache();
    const workspaceId = String(activeWorkspaceId || "").trim();
    const candidate = workspaceId
      ? jobs.find((job) => String((job && job.workspace_id) || "").trim() === workspaceId)
      : (jobs.length > 0 ? jobs[0] : null);
    const state = String((candidate && candidate.state) || "").toLowerCase();
    const id = String((candidate && candidate.id) || "").trim();
    if (id && (state === "queued" || state === "running")) {
      activeJobId = id;
      syncGlobalsToWorkspaceRuntime();
      appendLog(`restored active workspace job: ${id}`);
    }
  }

  if (activeJobId) {
    try {
      const data = await api.getJob(activeJobId);
      const state = String((data && data.state) || "").toLowerCase();
      if (state === "queued" || state === "running") {
        const progress = Number((data && data.progress) || 0);
        const elapsedMs = Math.max(0, Number((data && data.elapsed_ms) || 0));
        const threads = Math.max(0, Number((data && data.threads) || 0));
        const stateLabel = state === "running" ? "rendering" : state;
        updateActivePreviewTilesFromJob(data);
        setRenderActive(true);
        if (typeof syncRenderTimerToServer === "function") syncRenderTimerToServer(elapsedMs);
        setProgress(progress);
        setStatusThreads(threads);
        if (elapsedMs > 0) {
          setStatus(`${stateLabel} ${(100 * progress).toFixed(1)}% (${formatElapsed(elapsedMs)})`);
        } else {
          setStatus(`${stateLabel} ${(100 * progress).toFixed(1)}%`);
        }
        if (typeof updateRenderActionButton === "function") updateRenderActionButton();
        // If previewState.tileAccumCanvas is active the WS connection is still streaming tiles —
        // skip refreshProgressivePreview to avoid clearing the accumulated canvas.
        if (!previewState.tileAccumCanvas) {
          await refreshProgressivePreview(activeJobId);
        }
        if (typeof resumeWorkspaceJobPolling === "function") {
          resumeWorkspaceJobPolling(activeJobId);
        }
        return;
      }
      if (state === "done") {
        lastCompletedJobId = activeJobId;
        lastCompletedJobScene = String((data && data.scene) || el.scene.value || "");
        lastCompletedJobIntegrator = String((data && data.integrator) || el.integrator.value || "");
        activeJobId = "";
        syncGlobalsToWorkspaceRuntime();
      } else if (state === "aborted" || state === "error") {
        activeJobId = "";
        syncGlobalsToWorkspaceRuntime();
      }
    } catch (_) {
      // fallthrough to last completed image
    }
  }

  if (!lastCompletedJobId) {
    setPreviewEmptyState(true);
    updateDownloadUi();
    return;
  }
  try {
    const finalBlob = await api.getJobImage(lastCompletedJobId, {
      final: true,
      cacheBust: true,
      toneMapping: el.toneMapping ? el.toneMapping.value : "aces",
      toneMappingExposure: el.toneMappingExposure ? el.toneMappingExposure.value : "1.0",
      toneMappingWhitePoint: el.toneMappingWhitePoint ? el.toneMappingWhitePoint.value : "1.0",
      toneMappingMantiukContrast: el.toneMappingMantiukContrast ? el.toneMappingMantiukContrast.value : "0.1",
      toneMappingMantiukSaturation: el.toneMappingMantiukSaturation ? el.toneMappingMantiukSaturation.value : "0.8",
      toneMappingMantiukDetail: el.toneMappingMantiukDetail ? el.toneMappingMantiukDetail.value : "1.0",
      postFiltersEnabled: !!postFilterStackEnabled,
      postFilters: gatherPostFilterParams(),
    });
    if (finalBlob && finalBlob.size > 0) {
      await setPreviewFromBlob(finalBlob);
      updateDownloadUi();
      return;
    }
  } catch (_) {
    // fallthrough
  }
  setPreviewEmptyState(true);
  updateDownloadUi();
}

function resumeWorkspaceJobPolling(jobId) {
  const id = String(jobId || "").trim();
  if (!id) return;
  if (previewState.pollingJobId === id) return;

  previewState.pollingJobId = id;
  activeJobId = id;
  syncGlobalsToWorkspaceRuntime();
  const token = beginPollSession();
  setRenderActive(true);
  if (typeof updateRenderActionButton === "function") updateRenderActionButton();
  pollJob(id, token)
    .catch((err) => {
      setStatus(`error: ${err.message}`);
      appendLog(`render error: ${err.message}`);
    })
    .finally(() => {
      const superseded = token !== undefined && token !== activePollToken;
      if (previewState.pollingJobId === id) previewState.pollingJobId = "";
      // A newer poll session owns the UI state now. Do not let an older
      // workspace poll tear down the active render badge/button on exit.
      if (superseded) return;
      if (activeJobId === id) {
        activeJobId = "";
        syncGlobalsToWorkspaceRuntime();
      }
      if (!activeJobId) {
        setRenderActive(false);
        if (typeof updateRenderActionButton === "function") updateRenderActionButton();
      }
    });
}
