function updatePreviewSizing() {
  applyPreviewTransform();
}

let workspacePollingJobId = "";

function clamp(value, lo, hi) {
  return Math.min(hi, Math.max(lo, value));
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
  drawActivePreviewTileOverlay(ctx, x, y, drawW, drawH);
}

function drawActivePreviewTileOverlay(ctx, imageX, imageY, imageW, imageH) {
  if (!ctx || !activePreviewTiles.length) return;
  const srcW = Number(activePreviewTileWidth) || el.preview.naturalWidth || 0;
  const srcH = Number(activePreviewTileHeight) || el.preview.naturalHeight || 0;
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

function setPreviewEmptyState(isEmpty) {
  el.previewFrame.classList.toggle("is-empty", isEmpty);
  if (isEmpty) {
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
  const nearest = isNearestPreviewSampling();
  el.previewFrame.classList.toggle("sampling-nearest", nearest);
  setPreviewEmptyState(false);
  requestAnimationFrame(() => {
    applyPreviewTransform();
  });
  return true;
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

function parseImageDeltaPacket(buffer) {
  if (!(buffer instanceof ArrayBuffer)) return null;
  if (buffer.byteLength < 28) return null;
  const bytes = new Uint8Array(buffer);
  if (bytes[0] !== 0x58 || bytes[1] !== 0x54 || bytes[2] !== 0x44 || bytes[3] !== 0x31) return null;

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
    tiles.push({ x0, y0, x1, y1, doneIndex, pngBytes });
  }

  return { width, height, tilesDone, tilesTotal, state, tiles };
}

async function drawDeltaTilesToPreviewCanvas(delta) {
  if (!delta || !delta.width || !delta.height) return false;

  const canvas = document.createElement("canvas");
  canvas.width = delta.width;
  canvas.height = delta.height;
  const ctx = canvas.getContext("2d");
  if (!ctx) return false;

  if (preservePreviewUnderlay && previewPinnedBaseUrl) {
    try {
      const base = await getPinnedBaseBitmap();
      if (base) ctx.drawImage(base, 0, 0, delta.width, delta.height);
    } catch (_) {
      // best-effort underlay only
    }
  }

  if (previewObjectUrl) {
    try {
      const res = await fetch(previewObjectUrl);
      if (res.ok) {
        const blob = await res.blob();
        if (blob && blob.size > 0) {
          const base = await createImageBitmap(blob);
          ctx.drawImage(base, 0, 0, delta.width, delta.height);
        }
      }
    } catch (_) {
      // best-effort base image only
    }
  }

  let maxDone = progressiveDeltaSinceDone;
  for (let i = 0; i < delta.tiles.length; i += 1) {
    const t = delta.tiles[i];
    if (!t || !t.pngBytes || !t.pngBytes.length) continue;
    const tileBlob = new Blob([t.pngBytes], { type: "image/png" });
    const tileImage = await createImageBitmap(tileBlob);
    const w = Math.max(1, Number(t.x1) - Number(t.x0));
    const h = Math.max(1, Number(t.y1) - Number(t.y0));
    ctx.drawImage(tileImage, Number(t.x0), Number(t.y0), w, h);
    if (Number.isFinite(t.doneIndex) && t.doneIndex > maxDone) maxDone = t.doneIndex;
  }

  if (maxDone > progressiveDeltaSinceDone) progressiveDeltaSinceDone = maxDone;
  const composedBlob = await new Promise((resolve) => {
    canvas.toBlob((b) => resolve(b || null), "image/png");
  });
  if (!composedBlob || composedBlob.size === 0) return false;
  return setPreviewFromBlob(composedBlob);
}

async function refreshProgressivePreviewDelta(jobId) {
  if (!hasBackendMethod(api, "getJobImageDelta")) {
    return { updated: false, tilesDone: 0, tilesTotal: 0, state: "" };
  }
  if (!progressiveDeltaEnabled) {
    return { updated: false, tilesDone: 0, tilesTotal: 0, state: "" };
  }

  const id = String(jobId || "").trim();
  if (!id) return { updated: false, tilesDone: 0, tilesTotal: 0, state: "" };
  if (progressiveDeltaJobId !== id) resetProgressiveDeltaState(id);
  const tmKey = [
    el.toneMapping ? el.toneMapping.value : "aces",
    el.toneMappingExposure ? el.toneMappingExposure.value : "1.0",
    el.toneMappingWhitePoint ? el.toneMappingWhitePoint.value : "1.0",
    el.toneMappingMantiukContrast ? el.toneMappingMantiukContrast.value : "0.1",
    el.toneMappingMantiukSaturation ? el.toneMappingMantiukSaturation.value : "0.8",
    el.toneMappingMantiukDetail ? el.toneMappingMantiukDetail.value : "1.0",
  ].join("|");
  if (tmKey !== progressiveDeltaTmKey) {
    progressiveDeltaTmKey = tmKey;
    progressiveDeltaSinceDone = 0;
  }

  const packet = await api.getJobImageDelta(id, {
    since: progressiveDeltaSinceDone,
    limit: 24,
    cacheBust: true,
    toneMapping: el.toneMapping ? el.toneMapping.value : "aces",
    toneMappingExposure: el.toneMappingExposure ? el.toneMappingExposure.value : "1.0",
    toneMappingWhitePoint: el.toneMappingWhitePoint ? el.toneMappingWhitePoint.value : "1.0",
    toneMappingMantiukContrast: el.toneMappingMantiukContrast ? el.toneMappingMantiukContrast.value : "0.1",
    toneMappingMantiukSaturation: el.toneMappingMantiukSaturation ? el.toneMappingMantiukSaturation.value : "0.8",
    toneMappingMantiukDetail: el.toneMappingMantiukDetail ? el.toneMappingMantiukDetail.value : "1.0",
  });
  if (!packet) return { updated: false, tilesDone: 0, tilesTotal: 0, state: "" };
  if (renderActive && activeJobId && id === String(activeJobId)) {
    recordPreviewTransfer("delta", packet.byteLength || 0);
  }

  const delta = parseImageDeltaPacket(packet);
  if (!delta) return { updated: false, tilesDone: 0, tilesTotal: 0, state: "" };
  if (!Array.isArray(delta.tiles) || delta.tiles.length === 0) {
    return {
      updated: true,
      tilesDone: Number(delta.tilesDone) || 0,
      tilesTotal: Number(delta.tilesTotal) || 0,
      state: String(delta.state || ""),
    };
  }
  const updated = await drawDeltaTilesToPreviewCanvas(delta);
  return {
    updated: !!updated,
    tilesDone: Number(delta.tilesDone) || 0,
    tilesTotal: Number(delta.tilesTotal) || 0,
    state: String(delta.state || ""),
  };
}

async function refreshProgressivePreview(jobId) {
  const id = String(jobId || "").trim();
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
  if (renderActive && activeJobId && id === String(activeJobId)) {
    recordPreviewTransfer("full", blob.size || 0);
  }

  const imageBlob = preservePreviewUnderlay
    ? await composeWithPinnedPreview(blob)
    : blob;

  return setPreviewFromBlob(imageBlob);
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
      if (finalBlob && finalBlob.size > 0) await setPreviewFromBlob(finalBlob);
    }
  } catch (err) {
    appendLog(`tone mapping preview refresh failed: ${err.message}`);
  }
}

async function restorePreviewForActiveWorkspace() {
  if (!activeJobId && hasBackendMethod(api, "getActiveJobs")) {
    try {
      const jobs = await api.getActiveJobs();
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
    } catch (_) {
      // No globally active job is fine.
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
        setProgress(progress);
        setStatusThreads(threads);
        if (elapsedMs > 0 && typeof formatElapsed === "function") {
          setStatus(`${stateLabel} ${(100 * progress).toFixed(1)}% (${formatElapsed(elapsedMs)})`);
        } else {
          setStatus(`${stateLabel} ${(100 * progress).toFixed(1)}%`);
        }
        if (typeof updateRenderActionButton === "function") updateRenderActionButton();
        await refreshProgressivePreview(activeJobId);
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
  if (workspacePollingJobId === id) return;

  workspacePollingJobId = id;
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
      if (workspacePollingJobId === id) workspacePollingJobId = "";
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
