const DARK_PALETTE_OPTIONS = [
  { value: "slate", label: "Slate" },
  { value: "crimson", label: "Crimson" },
  { value: "graphite", label: "Graphite" },
  { value: "emerald", label: "Emerald" },
  { value: "ember", label: "Ember" },
  { value: "arctic", label: "Arctic" },
];
const LIGHT_PALETTE_OPTIONS = [
  { value: "coastal", label: "Coastal" },
  { value: "amber", label: "Amber" },
  { value: "sage", label: "Sage" },
  { value: "rose", label: "Rose" },
  { value: "sky", label: "Sky" },
  { value: "olive", label: "Olive" },
];
const DARK_PALETTES = new Set(DARK_PALETTE_OPTIONS.map((p) => p.value));
const LIGHT_PALETTES = new Set(LIGHT_PALETTE_OPTIONS.map((p) => p.value));
const FONT_SIZE_PRESET_DEFAULT = "default";
const FONT_SIZE_PRESET_LARGE = "large";
let POST_FILTER_CATALOG = [
  { id: "desaturate", label: "Desaturate" },
  { id: "chromatic_aberration", label: "Chromatic Aberration" },
  { id: "vignette", label: "Vignette" },
  { id: "film_grain", label: "Film Grain" },
  { id: "denoise", label: "Bilateral Denoise" },
  { id: "fxaa", label: "FXAA" },
  { id: "sharpen", label: "Sharpen" },
  { id: "brightness", label: "Brightness" },
  { id: "contrast", label: "Contrast" },
  { id: "raindrops_lens", label: "Raindrops on Lens" },
];

function normalizeFontSizePreset(value) {
  const preset = String(value || "").toLowerCase();
  if (preset === FONT_SIZE_PRESET_LARGE) return FONT_SIZE_PRESET_LARGE;
  return FONT_SIZE_PRESET_DEFAULT;
}

function scaleForFontSizePreset(preset) {
  return normalizeFontSizePreset(preset) === FONT_SIZE_PRESET_LARGE ? 1.1 : 1.0;
}

function fontSizePresetFromScale(scale) {
  return Number(scale) > 1.02 ? FONT_SIZE_PRESET_LARGE : FONT_SIZE_PRESET_DEFAULT;
}

function nowIsoStamp() {
  return new Date().toISOString().replace(/\.\d{3}Z$/, "Z");
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

function normalizeLogLevel(levelRaw) {
  const level = String(levelRaw || "").toLowerCase();
  if (level === "debug") return "debug";
  if (level === "warn" || level === "warning") return "warning";
  if (level === "err" || level === "error") return "error";
  return "message";
}

function isLogLevelEnabled(level) {
  if (level === "debug") return !!logFilters.debug;
  if (level === "warning") return !!logFilters.warning;
  if (level === "error") return !!logFilters.error;
  return !!logFilters.message;
}

function isUiLogEntry(entry) {
  if (!entry) return false;
  if (entry.source === "ui") return true;
  const line = String(entry.line || "");
  return /^#UI\s+\S+\s+UI(\s|$)/.test(line) || /^\[UI [^\]]+\]/.test(line);
}

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

function renderLogLineHtml(entry) {
  const line = String((entry && entry.line) || "");
  const level = normalizeLogLevel(entry && entry.level);

  // Standard format: #<id-or-type> <iso-ts> <LEVEL> <message...>
  const backendMatch = line.match(/^#([A-Za-z0-9_-]+)\s+(\S+)\s+([A-Z_]+)\s*(.*)$/);
  if (backendMatch) {
    const id = backendMatch[1];
    const tsRaw = backendMatch[2];
    const ts = tsRaw.replace("T", " ").replace(/Z$/, "");
    const lvl = backendMatch[3];
    const msg = backendMatch[4] || "";
    const idClass = id.toUpperCase() === "UI" ? "log-token-ui" : "log-token-id";
    return `<span class="log-line log-line-backend log-level-${level}">`
      + `<span class="${idClass}">#${escapeHtml(id)}</span>`
      + `<span class="log-token-ts">${escapeHtml(ts)}</span>`
      + `<span class="log-token-level">${escapeHtml(lvl)}</span>`
      + `<span class="log-token-msg">${escapeHtml(msg)}</span>`
      + `</span>`;
  }

  return `<span class="log-line log-level-${level}">${escapeHtml(line)}</span>`;
}

function renderLogOutput() {
  if (!el.logOutput) return;
  const html = logEntries
    .filter((entry) => {
      if (isUiLogEntry(entry)) return !!logFilters.ui;
      return isLogLevelEnabled(entry.level);
    })
    .map((entry) => renderLogLineHtml(entry))
    .join("");
  el.logOutput.innerHTML = html;
  scrollLogToBottom(false);
}

function appendLogEntry(level, line, source) {
  if (!line) return;
  logEntries.push({
    level: normalizeLogLevel(level),
    line: String(line),
    source: String(source || ""),
  });
  if (logEntries.length > LOG_HISTORY_LIMIT) {
    logEntries.splice(0, logEntries.length - LOG_HISTORY_LIMIT);
  }
  renderLogOutput();
}

function appendLog(message) {
  if (!message) return;
  appendLogEntry("message", `#UI ${nowIsoStamp()} UI ${message}`, "ui");
}

function appendBackendLog(entry) {
  const id = entry.id || 0;
  const ts = entry.ts || "";
  const level = normalizeLogLevel(entry.level || "info");
  const levelLabel = String(entry.level || "info").toUpperCase();
  const msg = entry.message || "";
  appendLogEntry(level, `#${id} ${ts} ${levelLabel} ${msg}`, "backend");
}

function setStatus(text) {
  const controls = (typeof el !== "undefined" && el) ? el : null;
  if (!controls || !controls.status) return;
  const isRenderActive = (typeof renderActive !== "undefined") && !!renderActive;
  const statusText = isRenderActive ? text : "Idle";
  const lower = String(statusText || "").toLowerCase();
  let state = "idle";
  if (isRenderActive) {
    if (lower.indexOf("error") >= 0) state = "error";
    else state = "running";
  }
  const match = String(statusText || "").match(/^(.+?)\s+(\d+(?:\.\d+)?)%(?:\s*\(([^)]+)\))?$/);
  const statusLabel = match ? String(match[1] || "").trim() : String(statusText || "").trim();
  const statusPercent = match ? `${match[2]}%` : "";
  const statusTimer = match ? String(match[3] || "").trim() : "";

  controls.status.textContent = statusLabel || "Idle";
  controls.status.classList.remove("is-idle", "is-running", "is-error");
  controls.status.classList.add(`is-${state}`);
  if (controls.statusPercent) {
    controls.statusPercent.hidden = !statusPercent;
    controls.statusPercent.textContent = statusPercent || "0.0%";
  }
  if (controls.renderTimer) {
    controls.renderTimer.hidden = !statusTimer;
    controls.renderTimer.textContent = statusTimer || "00:00";
  }
}

function setStatusThreads(threads) {
  if (!el.statusThreads) return;
  const n = Number(threads);
  const valid = Number.isFinite(n) && n > 0;
  el.statusThreads.hidden = !valid;
  if (valid) {
    el.statusThreads.textContent = `threads ${Math.max(1, Math.floor(n))}`;
  } else {
    el.statusThreads.textContent = "threads 0";
  }
}

function setStatusPass(currentPass, totalPasses, mode) {
  if (!el.statusPass) return;
  const normalizedMode = String(mode || "").toLowerCase();
  const isProgressive = normalizedMode === RENDER_MODE_PROGRESSIVE || normalizedMode === RENDER_MODE_INCREMENTAL;
  const total = Math.max(0, Number(totalPasses) || 0);
  let current = Math.max(0, Number(currentPass) || 0);
  if (!isProgressive || total <= 0 || !renderActive) {
    el.statusPass.hidden = true;
    el.statusPass.textContent = "pass 1/1";
    return;
  }
  if (current <= 0) current = 1;
  if (current > total) current = total;
  el.statusPass.hidden = false;
  el.statusPass.textContent = `pass ${Math.floor(current)}/${Math.floor(total)}`;
}

function formatElapsedShort(ms) {
  const totalSeconds = Math.max(0, Math.floor((ms || 0) / 1000));
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
}

function renderSceneLoadStatus() {
  if (el.sceneLoadState) {
    const rawState = String(sceneLoadStatusState.state || "idle").toLowerCase();
    let stateClass = "is-idle";
    if (rawState === "loading" || rawState === "queued" || rawState === "running") stateClass = "is-loading";
    else if (rawState === "done" || rawState === "ready" || rawState === "success") stateClass = "is-done";
    else if (rawState === "error" || rawState === "failed") stateClass = "is-error";
    el.sceneLoadState.classList.remove("is-idle", "is-loading", "is-done", "is-error");
    el.sceneLoadState.classList.add(stateClass);
    el.sceneLoadState.textContent = rawState;
  }
  if (el.sceneLoadMessage) el.sceneLoadMessage.textContent = String(sceneLoadStatusState.message || "");
  if (el.sceneLoadJobId) {
    const jobLabel = sceneLoadStatusState.jobId ? `#${sceneLoadStatusState.jobId}` : "-";
    el.sceneLoadJobId.textContent = jobLabel;
  }
  if (el.sceneLoadElapsed) {
    const elapsed = sceneLoadStatusState.startedAtMs > 0
      ? (Date.now() - sceneLoadStatusState.startedAtMs)
      : 0;
    el.sceneLoadElapsed.textContent = formatElapsedShort(elapsed);
  }
}

function setSceneLoadStatus(state, message, jobId) {
  const nextState = String(state || "idle").toLowerCase();
  const nextMsg = String(message || "");
  const nextJob = String(jobId || "").trim();
  const timingState = (nextState === "queued" || nextState === "running" || nextState === "loading");
  if (!timingState) {
    sceneLoadStatusState.startedAtMs = 0;
  } else if (sceneLoadStatusState.startedAtMs <= 0) {
    sceneLoadStatusState.startedAtMs = Date.now();
  }
  sceneLoadStatusState.state = nextState;
  sceneLoadStatusState.message = nextMsg;
  sceneLoadStatusState.jobId = nextJob;
  renderSceneLoadStatus();
}

function setEditorOpStatus(kind, text) {
  if (!el.editorOpStatus) return;
  const msg = String(text || "").trim();
  el.editorOpStatus.hidden = !msg;
  el.editorOpStatus.textContent = msg;
  el.editorOpStatus.classList.remove("is-success", "is-error", "is-info");
  if (!msg) return;
  if (kind === "success") el.editorOpStatus.classList.add("is-success");
  else if (kind === "error") el.editorOpStatus.classList.add("is-error");
  else el.editorOpStatus.classList.add("is-info");
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

function formatBytesShort(bytes) {
  const n = Number(bytes) || 0;
  if (n < 1024) return `${n} B`;
  if (n < (1024 * 1024)) return `${(n / 1024).toFixed(1)} KB`;
  return `${(n / (1024 * 1024)).toFixed(2)} MB`;
}

function renderPreviewTransferStats() {
  if (!el.previewTransferStats) return;
  const renderStat = (node, label, value) => {
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
  };

  if (el.statsFrameRender) {
    const frameMs = Math.max(0, Number(previewTransferStatsState.fullFrameRenderMs) || 0);
    const frameText = frameMs > 0
      ? `${formatElapsed(frameMs)} (${Math.round(frameMs)} ms)`
      : "-";
    renderStat(el.statsFrameRender, "Full Frame Render", frameText);
  }

  const hasData = (previewTransferStatsState.deltaReqs + previewTransferStatsState.fullReqs) > 0;
  if (!hasData) {
    renderStat(el.statsDeltaBytes, "Delta Bytes", "-");
    renderStat(el.statsDeltaReqs, "Delta Requests", "-");
    renderStat(el.statsFullBytes, "Full Bytes", "-");
    renderStat(el.statsFullReqs, "Full Requests", "-");
    return;
  }
  renderStat(el.statsDeltaBytes, "Delta Bytes", formatBytesShort(previewTransferStatsState.deltaBytes));
  renderStat(el.statsDeltaReqs, "Delta Requests", previewTransferStatsState.deltaReqs);
  renderStat(el.statsFullBytes, "Full Bytes", formatBytesShort(previewTransferStatsState.fullBytes));
  renderStat(el.statsFullReqs, "Full Requests", previewTransferStatsState.fullReqs);
}

function resetPreviewTransferStats() {
  previewTransferStatsState.fullFrameRenderMs = 0;
  previewTransferStatsState.deltaReqs = 0;
  previewTransferStatsState.deltaBytes = 0;
  previewTransferStatsState.fullReqs = 0;
  previewTransferStatsState.fullBytes = 0;
  renderPreviewTransferStats();
}

function recordFullFrameRenderTime(ms) {
  const value = Math.max(0, Number(ms) || 0);
  previewTransferStatsState.fullFrameRenderMs = value;
  renderPreviewTransferStats();
}

function recordPreviewTransfer(kind, bytes) {
  const size = Math.max(0, Number(bytes) || 0);
  if (size <= 0) return;
  if (kind === "delta") {
    previewTransferStatsState.deltaReqs += 1;
    previewTransferStatsState.deltaBytes += size;
  } else {
    previewTransferStatsState.fullReqs += 1;
    previewTransferStatsState.fullBytes += size;
  }
  renderPreviewTransferStats();
}

function updateRenderTimer() {
  // Timer text is set from server-provided elapsed_ms in setStatus().
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
  if (el.previewHeadline) {
    el.previewHeadline.classList.toggle("is-running", renderActive);
    el.previewHeadline.classList.toggle("is-idle", !renderActive);
  }
  if (el.progressBar) {
    el.progressBar.hidden = false;
    el.progressBar.classList.toggle("is-visible", renderActive);
  }
  if (!renderActive) {
    el.progress.style.width = "0%";
    setStatusThreads(0);
    setStatusPass(0, 0, "");
    setStatus("Idle");
  }
  if (typeof updateRenderActionButton === "function") {
    updateRenderActionButton();
  }
}

async function getJSON(url) {
  const res = await fetch(url, { cache: "no-store" });
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}

function sleep(ms) {
  const waitMs = Number.isFinite(ms) ? Math.max(0, Math.floor(ms)) : 0;
  return new Promise((resolve) => setTimeout(resolve, waitMs));
}

function createHttpError(status, message) {
  const err = new Error(message || `HTTP ${status}`);
  err.status = status;
  return err;
}

async function readJsonMaybe(res) {
  try {
    return await res.json();
  } catch (_) {
    return {};
  }
}

async function waitForSceneLoadJob(jobId, timeoutMs) {
  const id = encodeURIComponent(String(jobId || ""));
  if (!id) throw createHttpError(400, "invalid scene load job id");
  const timeout = Number.isFinite(timeoutMs) ? Math.max(1000, timeoutMs) : 120000;
  const t0 = Date.now();
  setSceneLoadStatus("queued", "Scene load job queued.", id);

  while ((Date.now() - t0) < timeout) {
    const res = await fetch(`/api/scenes/load_jobs/${id}`, { cache: "no-store" });
    let data = {};
    try {
      data = await res.json();
    } catch (_) {
      data = {};
    }

    if (res.status === 404) {
      setSceneLoadStatus("done", "Scene load completed.", id);
      return;
    }
    if (!res.ok) {
      setSceneLoadStatus("error", (data && data.error) || `HTTP ${res.status}`, "");
      throw createHttpError(res.status, (data && data.error) || `HTTP ${res.status}`);
    }

    const state = String(data && data.state ? data.state : "").toLowerCase();
    if (state === "done") {
      setSceneLoadStatus("done", "Scene load completed.", id);
      return;
    }
    if (state === "error") {
      setSceneLoadStatus("error", (data && data.error) || "Scene load failed.", id);
      throw createHttpError(500, (data && data.error) || "scene load failed");
    }
    setSceneLoadStatus(state || "running", `Scene load ${state || "running"}...`, id);

    await sleep(120);
  }

  setSceneLoadStatus("error", "Scene load timeout.", id);
  throw createHttpError(504, "scene load timeout");
}

async function getSceneJSONWithAsyncLoad(url, timeoutMs) {
  const timeout = Number.isFinite(timeoutMs) ? Math.max(1000, timeoutMs) : 120000;
  const t0 = Date.now();
  setSceneLoadStatus("loading", "Loading scene data from backend...", "");

  while ((Date.now() - t0) < timeout) {
    const res = await fetch(url, { cache: "no-store" });
    let data = {};
    try {
      data = await res.json();
    } catch (_) {
      data = {};
    }

    if (res.status === 202) {
      const jobId = Number(data && data.job_id);
      if (!Number.isFinite(jobId) || jobId <= 0) {
        setSceneLoadStatus("loading", "Scene is loading...", "");
        throw createHttpError(202, "scene loading");
      }
      const remaining = timeout - (Date.now() - t0);
      await waitForSceneLoadJob(jobId, remaining);
      continue;
    }

    if (!res.ok) throw createHttpError(res.status, (data && data.error) || `HTTP ${res.status}`);
    setSceneLoadStatus("done", "Scene data ready.", "");
    return data;
  }

  setSceneLoadStatus("error", "Scene load timeout.", "");
  throw createHttpError(504, "scene load timeout");
}

function generateClientId() {
  if (window.crypto && typeof window.crypto.randomUUID === "function") {
    return `client_${window.crypto.randomUUID()}`;
  }
  const ts = Date.now().toString(36);
  const rnd = Math.random().toString(36).slice(2, 14);
  return `client_${ts}_${rnd}`;
}

function ensureClientId() {
  const existing = String(localStorage.getItem(CLIENT_ID_KEY) || "").trim();
  if (existing && /^[A-Za-z0-9_.-]{1,96}$/.test(existing)) {
    clientId = existing;
    return clientId;
  }
  clientId = generateClientId();
  localStorage.setItem(CLIENT_ID_KEY, clientId);
  return clientId;
}

function workspaceRuntimeState(workspaceId) {
  const id = String(workspaceId || "").trim();
  if (!id) return null;
  let state = workspaceRuntimeById.get(id);
  if (!state) {
    state = {
      activeJobId: "",
      lastCompletedJobId: "",
      lastCompletedJobScene: "",
      lastCompletedJobIntegrator: "",
    };
    workspaceRuntimeById.set(id, state);
  }
  return state;
}

function syncWorkspaceRuntimeToGlobals() {
  const state = workspaceRuntimeState(activeWorkspaceId);
  if (!state) return;
  activeJobId = state.activeJobId || "";
  lastCompletedJobId = state.lastCompletedJobId || "";
  lastCompletedJobScene = state.lastCompletedJobScene || "";
  lastCompletedJobIntegrator = state.lastCompletedJobIntegrator || "";
}

function syncGlobalsToWorkspaceRuntime() {
  const state = workspaceRuntimeState(activeWorkspaceId);
  if (!state) return;
  state.activeJobId = activeJobId || "";
  state.lastCompletedJobId = lastCompletedJobId || "";
  state.lastCompletedJobScene = lastCompletedJobScene || "";
  state.lastCompletedJobIntegrator = lastCompletedJobIntegrator || "";
}

function beginPollSession() {
  activePollToken += 1;
  resetProgressiveDeltaState("");
  resetPreviewTransferStats();
  if (typeof resetTileHeatmapState === "function") resetTileHeatmapState("");
  return activePollToken;
}

function dismissStartupScreen(immediate) {
  if (startupDismissed) return;
  if (!el.startupScreen) {
    startupDismissed = true;
    return;
  }
  startupDismissed = true;
  if (immediate) {
    el.startupScreen.hidden = true;
    return;
  }
  el.startupScreen.classList.add("is-leaving");
  setTimeout(() => {
    if (el.startupScreen) el.startupScreen.hidden = true;
  }, 460);
}

let startupProgressPhases = [];
let startupLabelMotionTimer = null;

function startupPhaseText(done, total) {
  if (!Array.isArray(startupProgressPhases) || !startupProgressPhases.length) {
    return done >= total ? "Ready" : "Preparing runtime";
  }
  if (done >= total) return "Ready";
  return startupProgressPhases[Math.max(0, Math.min(done, startupProgressPhases.length - 1))] || "Preparing runtime";
}

function setStartupLabel(text) {
  if (!el.startupLabel) return;
  const nextText = String(text || "");
  if (el.startupLabel.textContent === nextText) return;
  el.startupLabel.textContent = nextText;
  el.startupLabel.classList.remove("is-entering");
  void el.startupLabel.offsetWidth;
  el.startupLabel.classList.add("is-entering");
  if (startupLabelMotionTimer) clearTimeout(startupLabelMotionTimer);
  startupLabelMotionTimer = setTimeout(() => {
    if (el.startupLabel) el.startupLabel.classList.remove("is-entering");
    startupLabelMotionTimer = null;
  }, 320);
}

function renderStartupProgress() {
  const total = Math.max(1, Number(startupProgressTotal) || 1);
  const done = Math.max(0, Math.min(total, Number(startupProgressDone) || 0));
  const ratio = done / total;
  const pct = Math.round(ratio * 100);
  const stageText = startupPhaseText(done, total);
  if (el.startupProgressFill) {
    if (pct >= 100) {
      el.startupProgressFill.style.transition = "none";
    } else {
      el.startupProgressFill.style.transition = "";
    }
    el.startupProgressFill.style.width = `${pct}%`;
    if (pct >= 100) {
      void el.startupProgressFill.offsetWidth;
      el.startupProgressFill.style.transition = "";
    }
  }
  if (el.startupPercent) {
    el.startupPercent.textContent = `${pct}%`;
  }
  setStartupLabel(stageText);
}

function resetStartupProgress(total, phases) {
  startupProgressTotal = Math.max(1, Number(total) || 1);
  startupProgressDone = 0;
  startupProgressPhases = Array.isArray(phases) ? phases.slice() : [];
  renderStartupProgress();
}

function advanceStartupProgress(step) {
  startupProgressDone += Math.max(1, Number(step) || 1);
  renderStartupProgress();
}

function cancelActivePollingUi() {
  beginPollSession();
  clearActivePreviewTiles();
  setRenderActive(false);
  if (typeof updateRenderActionButton === "function") {
    updateRenderActionButton();
  } else if (el.renderBtn) {
    el.renderBtn.disabled = false;
    el.renderBtn.textContent = "Render";
  }
}

function resetProgressiveDeltaState(jobId) {
  progressiveDeltaJobId = String(jobId || "").trim();
  progressiveDeltaSinceDone = 0;
  progressiveDeltaTmKey = "";
}

function appendToneMappingQuery(parts, opts) {
  const tm = String((opts && opts.toneMapping) || (el.toneMapping && el.toneMapping.value) || "aces").toLowerCase();
  const knownTmOps = ["aces", "reinhard", "reinhard_luma", "mantiuk_2006", "hable", "exponential", "lottes", "cineon", "uchimura", "agx", "khronos_pbr", "none"];
  if (knownTmOps.indexOf(tm) !== -1) {
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
}

function appendPostFiltersQuery(parts, opts) {
  const enabled = !!(opts && opts.postFiltersEnabled);
  if (!enabled) return;
  const raw = String((opts && opts.postFilters) || "").trim();
  if (!raw) return;
  parts.push("post_filters_enabled=1");
  parts.push(`post_filters=${encodeURIComponent(raw)}`);
}

function blobUrlForJobImage(jobId, opts) {
  const parts = [];
  if (opts && opts.partial) parts.push("partial=1");
  if (opts && opts.final) parts.push("final=1");
  appendToneMappingQuery(parts, opts);
  appendPostFiltersQuery(parts, opts);
  if (opts && opts.cacheBust) parts.push(`t=${Date.now()}`);
  const qs = parts.length ? `?${parts.join("&")}` : "";
  return `/api/jobs/${jobId}/image${qs}`;
}

function urlForJobImageDelta(jobId, opts) {
  const parts = [];
  const since = Number(opts && opts.since);
  const limit = Number(opts && opts.limit);
  parts.push(`since=${encodeURIComponent(Number.isFinite(since) && since >= 0 ? Math.floor(since) : 0)}`);
  parts.push(`limit=${encodeURIComponent(Number.isFinite(limit) && limit > 0 ? Math.floor(limit) : 16)}`);
  appendToneMappingQuery(parts, opts);
  appendPostFiltersQuery(parts, opts);
  if (opts && opts.cacheBust) parts.push(`t=${Date.now()}`);
  const qs = parts.length ? `?${parts.join("&")}` : "";
  return `/api/jobs/${encodeURIComponent(jobId)}/image_delta${qs}`;
}

function normalizeVariantName(variant) {
  return String(variant || "").trim();
}

function selectedVariantForApi(variant) {
  if (variant !== undefined && variant !== null) return normalizeVariantName(variant);
  if (typeof el === "undefined" || !el || !el.variant) return "";
  return normalizeVariantName(el.variant.value);
}

function withVariantQuery(url, variant) {
  const name = selectedVariantForApi(variant);
  if (!name) return url;
  return `${url}${url.indexOf("?") >= 0 ? "&" : "?"}variant=${encodeURIComponent(name)}`;
}

function createServerApi() {
  return {
    mode: "server",
    async getLogsSince(sinceId) {
      const data = await getJSON(`/api/logs?since=${sinceId}`);
      return data.entries || [];
    },
    async waitForLogsSince(sinceId, timeoutMs, opts) {
      const waitMs = Number.isFinite(timeoutMs) ? Math.max(1000, Math.min(60000, Math.floor(timeoutMs))) : 15000;
      const res = await fetch(`/api/logs/wait?since=${sinceId}&timeout_ms=${waitMs}`, {
        cache: "no-store",
        signal: opts && opts.signal ? opts.signal : undefined,
      });
      if (!res.ok) throw createHttpError(res.status, `HTTP ${res.status}`);
      const data = await readJsonMaybe(res);
      return data.entries || [];
    },
    async getScenes() {
      const data = await getJSON("/api/scenes");
      return data.scenes || [];
    },
    async getCameras(scene, variant) {
      if (!scene) return { cameras: [], cameraEntries: [], defaultCamera: "" };
      const data = await getSceneJSONWithAsyncLoad(withVariantQuery(`/api/scenes/${encodeURIComponent(scene)}/cameras`, variant));
      return {
        cameras: data.cameras || [],
        cameraEntries: Array.isArray(data.camera_entries) ? data.camera_entries : [],
        defaultCamera: data.default_camera || "",
      };
    },
    async getIntegrators() {
      const data = await getJSON("/api/integrators");
      return data.integrators || [];
    },
    async getPostFilters() {
      const data = await getJSON("/api/post_filters");
      return data.post_filters || [];
    },
    async getResolutionPresets() {
      const data = await getJSON("/api/resolutions");
      return data.presets || [];
    },
    async getSceneSource(scene) {
      if (!scene) return { scene: "", source: "" };
      const cid = encodeURIComponent(clientId || ensureClientId());
      const data = await getJSON(`/api/scenes/${encodeURIComponent(scene)}/source?client_id=${cid}`);
      return { scene: data.scene || scene, source: data.source || "" };
    },
    async getSceneGeometry(scene, variant) {
      if (!scene) return { meshes: {} };
      const data = await getSceneJSONWithAsyncLoad(withVariantQuery(`/api/scenes/${encodeURIComponent(scene)}/geometry`, variant));
      return {
        meshes: (data && data.meshes) || {},
        debug: (data && data.debug) || null,
      };
    },
    async getSceneRuntimeGraph(scene, variant) {
      if (!scene) return { cameras: [], objects: [], surfaces: [], materials: [], media: [] };
      const data = await getSceneJSONWithAsyncLoad(withVariantQuery(`/api/scenes/${encodeURIComponent(scene)}/runtime_graph`, variant));
      return {
        cameras: Array.isArray(data && data.cameras) ? data.cameras : [],
        objects: Array.isArray(data && data.objects) ? data.objects : [],
        surfaces: Array.isArray(data && data.surfaces) ? data.surfaces : [],
        materials: Array.isArray(data && data.materials) ? data.materials : [],
        media: Array.isArray(data && data.media) ? data.media : [],
      };
    },
    async getSceneResolvedCamera(scene, variant, camera) {
      if (!scene) return null;
      const cam = String(camera || "").trim();
      const base = `/api/scenes/${encodeURIComponent(scene)}/camera_resolve`
        + (cam ? `?camera=${encodeURIComponent(cam)}` : "");
      const data = await getSceneJSONWithAsyncLoad(withVariantQuery(base, variant));
      return data && typeof data === "object" ? data : null;
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
      body.set("client_id", clientId || ensureClientId());
      if (activeWorkspaceId) body.set("workspace_id", activeWorkspaceId);
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
    async getActiveJob() {
      const list = await this.getActiveJobs();
      return list.length > 0 ? list[0] : null;
    },
    async getActiveJobs() {
      const res = await fetch("/api/jobs/active", { cache: "no-store" });
      const data = await readJsonMaybe(res);
      if (!res.ok) throw createHttpError(res.status, (data && data.error) || `HTTP ${res.status}`);
      return Array.isArray(data && data.jobs) ? data.jobs : [];
    },
    async abortJob(jobId) {
      const encoded = encodeURIComponent(jobId);
      const body = new URLSearchParams();
      body.set("client_id", clientId || ensureClientId());
      if (activeWorkspaceId) body.set("workspace_id", activeWorkspaceId);
      const res = await fetch(`/api/jobs/abort/${encoded}`, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      const data = await readJsonMaybe(res);
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return {
        ok: !!data.ok,
        state: String((data && data.state) || ""),
      };
    },
    async moveJobQueueUp(jobId) {
      const encoded = encodeURIComponent(String(jobId || ""));
      if (!encoded) throw createHttpError(400, "invalid job id");
      const body = new URLSearchParams();
      body.set("client_id", clientId || ensureClientId());
      const res = await fetch(`/api/jobs/queue/up/${encoded}`, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      const data = await readJsonMaybe(res);
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return !!data.ok;
    },
    async moveJobQueueDown(jobId) {
      const encoded = encodeURIComponent(String(jobId || ""));
      if (!encoded) throw createHttpError(400, "invalid job id");
      const body = new URLSearchParams();
      body.set("client_id", clientId || ensureClientId());
      const res = await fetch(`/api/jobs/queue/down/${encoded}`, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      const data = await readJsonMaybe(res);
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return !!data.ok;
    },
    async getJobPhotons(jobId, limit) {
      const lim = Number.isFinite(limit) && limit > 0 ? Math.floor(limit) : 100000;
      return getJSON(`/api/jobs/${encodeURIComponent(jobId)}/photons?limit=${encodeURIComponent(lim)}`);
    },
    async getJobImage(jobId, opts) {
      const res = await fetch(blobUrlForJobImage(jobId, opts), { cache: "no-store" });
      if (!res.ok) return null;
      return res.blob();
    },
    async getJobImageDelta(jobId, opts) {
      const res = await fetch(urlForJobImageDelta(jobId, opts), { cache: "no-store" });
      if (!res.ok) return null;
      return res.arrayBuffer();
    },
    async getJobExport(jobId, format, opts) {
      const fmt = encodeURIComponent(String(format || "png").toLowerCase());
      const parts = [`format=${fmt}`];
      appendPostFiltersQuery(parts, opts || {});
      const res = await fetch(`/api/jobs/${encodeURIComponent(jobId)}/export?${parts.join("&")}`);
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
      body.set("client_id", clientId || ensureClientId());
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
    async deleteScene(name) {
      const body = new URLSearchParams();
      body.set("name", String(name || ""));
      body.set("client_id", clientId || ensureClientId());

      const res = await fetch("/api/scenes/delete", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });

      const data = await res.json();
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return data.scene || "";
    },
    async getWorkspaces() {
      const cid = encodeURIComponent(clientId || ensureClientId());
      return getJSON(`/api/workspaces?client_id=${cid}`);
    },
    async createWorkspace(name) {
      const body = new URLSearchParams();
      body.set("client_id", clientId || ensureClientId());
      if (name) body.set("name", String(name));
      const res = await fetch("/api/workspaces", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      const data = await res.json();
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return data;
    },
    async setActiveWorkspace(workspaceId) {
      const body = new URLSearchParams();
      body.set("client_id", clientId || ensureClientId());
      body.set("workspace_id", String(workspaceId || ""));
      const res = await fetch("/api/workspaces/active", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      const data = await res.json();
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return !!data.ok;
    },
    async deleteWorkspace(workspaceId) {
      const body = new URLSearchParams();
      body.set("client_id", clientId || ensureClientId());
      body.set("workspace_id", String(workspaceId || ""));
      const res = await fetch("/api/workspaces/delete", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      const data = await res.json();
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return data || {};
    },
    async saveWorkspaceSceneDraft(scene, source) {
      const body = new URLSearchParams();
      body.set("client_id", clientId || ensureClientId());
      body.set("scene", String(scene || ""));
      body.set("source", String(source || ""));
      const res = await fetch("/api/workspaces/scene_draft", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      const data = await res.json();
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return !!data.ok;
    },
    async saveWorkspaceSettings(settingsJson) {
      const body = new URLSearchParams();
      body.set("client_id", clientId || ensureClientId());
      body.set("settings_json", String(settingsJson || "{}"));
      const res = await fetch("/api/workspaces/settings", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      const data = await res.json();
      if (!res.ok) throw createHttpError(res.status, data.error || `HTTP ${res.status}`);
      return !!data.ok;
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
  const supportsLogWait = hasBackendMethod(api, "waitForLogsSince");
  const onLogsTab = String(activeTabMode || "") === "logs";
  const waitMs = onLogsTab
    ? Math.max(1000, Math.min(60000, Number(uiOptions.logPollActiveMs) || 3000))
    : Math.max(1000, Math.min(120000, Number(uiOptions.logPollBackgroundMs) || 20000));
  try {
    if (supportsLogWait && typeof AbortController === "function") {
      backendLogWaitAbortController = new AbortController();
    } else {
      backendLogWaitAbortController = null;
    }
    const entries = supportsLogWait
      ? await api.waitForLogsSince(lastBackendLogId, waitMs, {
        signal: backendLogWaitAbortController ? backendLogWaitAbortController.signal : undefined,
      })
      : await api.getLogsSince(lastBackendLogId);
    for (let i = 0; i < entries.length; i += 1) {
      appendBackendLog(entries[i]);
      if ((entries[i].id || 0) > lastBackendLogId) lastBackendLogId = entries[i].id;
    }
  } catch (err) {
    if (err && (err.name === "AbortError" || String(err.message || "").toLowerCase().indexOf("aborted") >= 0)) {
      // expected when prioritizing render/abort requests
    } else {
      appendLog(`backend logs unavailable: ${err.message}`);
    }
    await new Promise((r) => setTimeout(r, 1000));
  } finally {
    backendLogWaitAbortController = null;
    // Long-poll already waits on the server; schedule the next cycle immediately.
    const delay = supportsLogWait ? 0 : Math.max(500, uiOptions.pollMs);
    setTimeout(pollBackendLogs, delay);
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
