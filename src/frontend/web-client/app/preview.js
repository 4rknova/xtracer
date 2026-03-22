function updatePreviewSizing() {
  applyPreviewTransform();
}

let workspacePollingJobId = "";

function normalizeRenderMode(value) {
  const mode = String(value || "").trim().toLowerCase();
  if (mode === RENDER_MODE_PROGRESSIVE) return RENDER_MODE_PROGRESSIVE;
  if (mode === RENDER_MODE_INTERACTIVE) return RENDER_MODE_INTERACTIVE;
  return RENDER_MODE_NORMAL;
}

function isInteractiveRenderMode() {
  return normalizeRenderMode(renderMode) === RENDER_MODE_INTERACTIVE;
}

function isProgressiveRenderMode() {
  return normalizeRenderMode(renderMode) === RENDER_MODE_PROGRESSIVE;
}

function clamp(value, lo, hi) {
  return Math.min(hi, Math.max(lo, value));
}

function v3(x, y, z) {
  return [Number(x) || 0, Number(y) || 0, Number(z) || 0];
}

function v3add(a, b) {
  return [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
}

function v3sub(a, b) {
  return [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
}

function v3scale(a, s) {
  return [a[0] * s, a[1] * s, a[2] * s];
}

function v3dot(a, b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

function v3cross(a, b) {
  return [
    a[1] * b[2] - a[2] * b[1],
    a[2] * b[0] - a[0] * b[2],
    a[0] * b[1] - a[1] * b[0],
  ];
}

function v3len(a) {
  return Math.sqrt(v3dot(a, a));
}

function v3norm(a, fallback) {
  const l = v3len(a);
  if (!Number.isFinite(l) || l < 1e-8) return fallback ? [...fallback] : [0, 0, 1];
  return [a[0] / l, a[1] / l, a[2] / l];
}

function rotateAroundAxis(v, axisUnit, radians) {
  const c = Math.cos(radians);
  const s = Math.sin(radians);
  const term1 = v3scale(v, c);
  const term2 = v3scale(v3cross(axisUnit, v), s);
  const term3 = v3scale(axisUnit, v3dot(axisUnit, v) * (1 - c));
  return v3add(v3add(term1, term2), term3);
}

function interactivePreviewAvailable() {
  return !!interactivePreviewEnabled
    && activeTabMode === "render"
    && interactivePreviewCamera.ready;
}

function markInteractiveInputActivity() {
  interactivePreviewLastInputMs = Date.now();
}

function renderInteractivePreviewHud() {
  if (!el.interactivePreviewHud) return;
  if (el.interactivePreviewControls) {
    el.interactivePreviewControls.hidden = !isInteractiveRenderMode();
  }
  if (el.renderMode) {
    el.renderMode.value = normalizeRenderMode(renderMode);
  }
  const show = !!interactivePreviewEnabled && activeTabMode === "render";
  el.interactivePreviewHud.hidden = !show;
  if (el.interactivePreviewSaveCameraBtn) {
    const canSave = !!interactivePreviewEnabled && !!interactivePreviewCamera.ready;
    el.interactivePreviewSaveCameraBtn.disabled = !canSave;
    el.interactivePreviewSaveCameraBtn.classList.toggle("is-disabled", !canSave);
    el.interactivePreviewSaveCameraBtn.setAttribute("aria-disabled", canSave ? "false" : "true");
  }
  if (!show) return;
  if (el.interactivePreviewHudMode) {
    el.interactivePreviewHudMode.textContent = `Mode: ${String(interactivePreviewHudMode || "LOOK").toUpperCase()}`;
  }
  if (el.interactivePreviewHudSpeed) {
    const sp = Number(interactivePreviewFlySpeedScale) || 1;
    el.interactivePreviewHudSpeed.textContent = `Speed: ${sp.toFixed(1)}x`;
  }
  if (el.interactivePreviewHudQuality) {
    el.interactivePreviewHudQuality.textContent = `Quality: ${interactivePreviewHudQuality || "idle"}`;
  }
}

function interactivePreviewCameraRequestParams() {
  if (!interactivePreviewCamera.ready) return null;
  const p = interactivePreviewCamera.position;
  const t = interactivePreviewCamera.target;
  const u = interactivePreviewCamera.up;
  const nums = [p[0], p[1], p[2], t[0], t[1], t[2], u[0], u[1], u[2], interactivePreviewCamera.hfov];
  for (let i = 0; i < nums.length; i += 1) {
    if (!Number.isFinite(nums[i])) return null;
  }
  return {
    camera: interactivePreviewCamera.sourceCamera || (el.camera ? (el.camera.value || "") : ""),
    cam_px: String(p[0]),
    cam_py: String(p[1]),
    cam_pz: String(p[2]),
    cam_tx: String(t[0]),
    cam_ty: String(t[1]),
    cam_tz: String(t[2]),
    cam_upx: String(u[0]),
    cam_upy: String(u[1]),
    cam_upz: String(u[2]),
    cam_hfov: String(interactivePreviewCamera.hfov || 60),
  };
}

async function refreshInteractivePreviewCameraFromSelection() {
  interactivePreviewCamera.ready = false;
  if (!interactivePreviewEnabled) return false;
  if (!hasBackendMethod(api, "getSceneResolvedCamera")) return false;
  const scene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!scene) return false;
  const variant = selectedSceneVariantValue();
  const cameraName = String(el.camera && el.camera.value ? el.camera.value : "").trim();
  try {
    const resolved = await api.getSceneResolvedCamera(scene, variant, cameraName);
    const pos = Array.isArray(resolved && resolved.position) ? resolved.position : null;
    const target = Array.isArray(resolved && resolved.target) ? resolved.target : null;
    const up = Array.isArray(resolved && resolved.up) ? resolved.up : null;
    const hfov = Number(resolved && resolved.hfov);
    if (!pos || !target || !up) {
      appendLog("interactive preview unavailable for current camera");
      return false;
    }
    const vals = [pos[0], pos[1], pos[2], target[0], target[1], target[2], up[0], up[1], up[2]];
    if (vals.some((v) => !Number.isFinite(Number(v)))) {
      appendLog("interactive preview unavailable: camera metadata is non-finite");
      return false;
    }
    interactivePreviewCamera.ready = true;
    interactivePreviewCamera.type = String((resolved && resolved.type) || "");
    interactivePreviewCamera.sourceScene = scene;
    interactivePreviewCamera.sourceVariant = variant;
    interactivePreviewCamera.sourceCamera = String((resolved && resolved.resolved) || cameraName);
    interactivePreviewCamera.position = v3(pos[0], pos[1], pos[2]);
    interactivePreviewCamera.target = v3(target[0], target[1], target[2]);
    interactivePreviewCamera.up = v3norm(v3(up[0], up[1], up[2]), [0, 1, 0]);
    interactivePreviewCamera.hfov = Number.isFinite(hfov) ? clamp(hfov, 1, 179) : 60;
    interactivePreviewCameraSeq += 1;
    return true;
  } catch (err) {
    appendLog(`interactive camera resolve failed: ${err.message}`);
    return false;
  }
}

function interactiveCameraBasis() {
  const pos = interactivePreviewCamera.position;
  const target = interactivePreviewCamera.target;
  let forward = v3norm(v3sub(target, pos), [0, 0, -1]);
  let right = v3cross(forward, interactivePreviewCamera.up);
  right = v3norm(right, [1, 0, 0]);
  let up = v3cross(right, forward);
  up = v3norm(up, [0, 1, 0]);
  forward = v3norm(forward, [0, 0, -1]);
  return { forward, right, up };
}

function markInteractiveCameraDirty() {
  interactivePreviewCameraSeq += 1;
  interactivePreviewDirty = true;
  markInteractiveInputActivity();
  interactivePreviewHudQuality = "active";
  renderInteractivePreviewHud();
  if (typeof requestInteractivePreviewRender === "function") {
    requestInteractivePreviewRender();
  }
}

function interactiveLookCamera(dx, dy) {
  if (!interactivePreviewCamera.ready) return;
  const basis = interactiveCameraBasis();
  const dist = Math.max(0.1, v3len(v3sub(interactivePreviewCamera.target, interactivePreviewCamera.position)));
  const yaw = -dx * 0.0045;
  const pitch = -dy * 0.0045;
  let forward = rotateAroundAxis(basis.forward, basis.up, yaw);
  let right = v3norm(v3cross(forward, basis.up), basis.right);
  forward = rotateAroundAxis(forward, right, pitch);
  const upDot = clamp(v3dot(forward, basis.up), -0.985, 0.985);
  const horiz = v3sub(forward, v3scale(basis.up, upDot));
  const horizNorm = v3norm(horiz, [0, 0, -1]);
  const corrected = v3norm(v3add(v3scale(horizNorm, Math.sqrt(Math.max(0.0, 1 - upDot * upDot))), v3scale(basis.up, upDot)), basis.forward);
  interactivePreviewCamera.target = v3add(interactivePreviewCamera.position, v3scale(corrected, dist));
  interactivePreviewCamera.up = basis.up;
  markInteractiveCameraDirty();
}

function hasInteractiveFlyInput() {
  return !!(interactivePreviewKeyState.w
    || interactivePreviewKeyState.a
    || interactivePreviewKeyState.s
    || interactivePreviewKeyState.d
    || interactivePreviewKeyState.q
    || interactivePreviewKeyState.e);
}

function tickInteractiveFly() {
  if (!interactivePreviewAvailable()) return;
  const now = Date.now();
  if (!interactivePreviewFlyLastTickMs) interactivePreviewFlyLastTickMs = now;
  const dt = Math.max(0.001, Math.min(0.05, (now - interactivePreviewFlyLastTickMs) / 1000));
  interactivePreviewFlyLastTickMs = now;
  if (!hasInteractiveFlyInput()) return;
  const basis = interactiveCameraBasis();
  const speed = INTERACTIVE_PREVIEW_FLY_SPEED
    * Math.max(0.2, Math.min(5.0, Number(interactivePreviewFlySpeedScale) || 1.0))
    * (interactivePreviewKeyState.shift ? INTERACTIVE_PREVIEW_FLY_SHIFT_MULTIPLIER : 1.0);
  let move = [0, 0, 0];
  if (interactivePreviewKeyState.w) move = v3add(move, basis.forward);
  if (interactivePreviewKeyState.s) move = v3sub(move, basis.forward);
  if (interactivePreviewKeyState.d) move = v3add(move, basis.right);
  if (interactivePreviewKeyState.a) move = v3sub(move, basis.right);
  if (interactivePreviewKeyState.e) move = v3add(move, basis.up);
  if (interactivePreviewKeyState.q) move = v3sub(move, basis.up);
  const moveNorm = v3norm(move, [0, 0, 0]);
  if (v3len(moveNorm) < 1e-6) return;
  const delta = v3scale(moveNorm, speed * dt);
  interactivePreviewCamera.position = v3add(interactivePreviewCamera.position, delta);
  interactivePreviewCamera.target = v3add(interactivePreviewCamera.target, delta);
  markInteractiveCameraDirty();
}

function stopInteractiveFlyTicker() {
  if (interactivePreviewFlyTimer) {
    clearInterval(interactivePreviewFlyTimer);
    interactivePreviewFlyTimer = 0;
  }
  interactivePreviewFlyLastTickMs = 0;
}

function ensureInteractiveFlyTicker() {
  if (interactivePreviewFlyTimer) return;
  interactivePreviewFlyLastTickMs = Date.now();
  interactivePreviewFlyTimer = setInterval(() => {
    tickInteractiveFly();
  }, 16);
}

function bindInteractivePreviewKeyboard() {
  const isTypingTarget = (node) => {
    if (!node || !(node instanceof HTMLElement)) return false;
    const tag = String(node.tagName || "").toLowerCase();
    return tag === "input" || tag === "textarea" || tag === "select" || node.isContentEditable;
  };
  const applyKey = (evt, down) => {
    if (!interactivePreviewEnabled) return;
    if (activeTabMode !== "render") return;
    if (isTypingTarget(evt.target)) return;
    const k = String(evt.key || "").toLowerCase();
    let handled = true;
    if (k === "w") interactivePreviewKeyState.w = down;
    else if (k === "a") interactivePreviewKeyState.a = down;
    else if (k === "s") interactivePreviewKeyState.s = down;
    else if (k === "d") interactivePreviewKeyState.d = down;
    else if (k === "q") interactivePreviewKeyState.q = down;
    else if (k === "e") interactivePreviewKeyState.e = down;
    else if (k === "shift") interactivePreviewKeyState.shift = down;
    else handled = false;
    if (!handled) return;
    evt.preventDefault();
    interactivePreviewHudMode = "FLY";
    renderInteractivePreviewHud();
    markInteractiveInputActivity();
  };
  window.addEventListener("keydown", (evt) => applyKey(evt, true));
  window.addEventListener("keyup", (evt) => applyKey(evt, false));
  window.addEventListener("blur", () => {
    interactivePreviewKeyState.w = false;
    interactivePreviewKeyState.a = false;
    interactivePreviewKeyState.s = false;
    interactivePreviewKeyState.d = false;
    interactivePreviewKeyState.q = false;
    interactivePreviewKeyState.e = false;
    interactivePreviewKeyState.shift = false;
    interactivePreviewHudMode = "LOOK";
    renderInteractivePreviewHud();
  });
}

function interactiveOrbitCamera(dx, dy) {
  if (!interactivePreviewCamera.ready) return;
  const pos = interactivePreviewCamera.position;
  const target = interactivePreviewCamera.target;
  const offset = v3sub(pos, target);
  const radius = Math.max(0.001, v3len(offset));
  const yaw = -dx * 0.005;
  const pitch = -dy * 0.005;
  const basis = interactiveCameraBasis();
  let rotated = rotateAroundAxis(offset, basis.up, yaw);
  const axis = v3norm(v3cross(rotated, basis.up), basis.right);
  rotated = rotateAroundAxis(rotated, axis, pitch);
  const minY = -0.995 * radius;
  const maxY = 0.995 * radius;
  const y = clamp(v3dot(rotated, basis.up), minY, maxY);
  const horiz = v3sub(rotated, v3scale(basis.up, v3dot(rotated, basis.up)));
  const horizLen = Math.max(1e-6, v3len(horiz));
  const targetHorizLen = Math.sqrt(Math.max(0, radius * radius - y * y));
  const fixed = v3add(v3scale(v3scale(horiz, 1 / horizLen), targetHorizLen), v3scale(basis.up, y));
  interactivePreviewCamera.position = v3add(target, fixed);
  interactivePreviewCamera.up = basis.up;
  markInteractiveCameraDirty();
}

function interactivePanCamera(dx, dy) {
  if (!interactivePreviewCamera.ready) return;
  const pos = interactivePreviewCamera.position;
  const target = interactivePreviewCamera.target;
  const basis = interactiveCameraBasis();
  const dist = Math.max(0.001, v3len(v3sub(target, pos)));
  const k = dist * 0.0018;
  const move = v3add(v3scale(basis.right, -dx * k), v3scale(basis.up, dy * k));
  interactivePreviewCamera.position = v3add(pos, move);
  interactivePreviewCamera.target = v3add(target, move);
  interactivePreviewCamera.up = basis.up;
  markInteractiveCameraDirty();
}

function interactiveZoomCamera(deltaY) {
  if (!interactivePreviewCamera.ready) return;
  const pos = interactivePreviewCamera.position;
  const target = interactivePreviewCamera.target;
  const basis = interactiveCameraBasis();
  const dist = Math.max(0.001, v3len(v3sub(target, pos)));
  const amount = clamp(Math.exp(deltaY * 0.0015), 0.8, 1.25);
  const nextDist = clamp(dist * amount, 0.02, 1e6);
  const nextPos = v3sub(target, v3scale(basis.forward, nextDist));
  interactivePreviewCamera.position = nextPos;
  interactivePreviewCamera.up = basis.up;
  markInteractiveCameraDirty();
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

async function setInteractivePreviewEnabled(enabled) {
  return setRenderMode(enabled ? RENDER_MODE_INTERACTIVE : RENDER_MODE_NORMAL, { log: true });
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
    if (typeof stopInteractivePreviewLoop === "function") {
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
  const touchPoints = {};
  let pinchDistance = 0;
  let pinchCenterX = 0;
  let pinchCenterY = 0;

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
    previewView.panning = true;
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
      if (previewView.panMode === "pan") interactivePanCamera(dx, dy);
      else interactiveLookCamera(dx, dy);
      return;
    }
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
    };
  }
  return {
    toneMapping: el.toneMapping ? el.toneMapping.value : "aces",
    toneMappingExposure: el.toneMappingExposure ? el.toneMappingExposure.value : "1.0",
    toneMappingWhitePoint: el.toneMappingWhitePoint ? el.toneMappingWhitePoint.value : "1.0",
    toneMappingMantiukContrast: el.toneMappingMantiukContrast ? el.toneMappingMantiukContrast.value : "0.1",
    toneMappingMantiukSaturation: el.toneMappingMantiukSaturation ? el.toneMappingMantiukSaturation.value : "0.8",
    toneMappingMantiukDetail: el.toneMappingMantiukDetail ? el.toneMappingMantiukDetail.value : "1.0",
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
