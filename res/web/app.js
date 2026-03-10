const $ = (id) => document.getElementById(id);

const el = {
  tabRender: $("tabRender"),
  tabEditor: $("tabEditor"),
  tabSettings: $("tabSettings"),
  tabLogs: $("tabLogs"),
  tabAbout: $("tabAbout"),
  paneRender: $("paneRender"),
  paneEditor: $("paneEditor"),
  paneSettings: $("paneSettings"),
  paneLogs: $("paneLogs"),
  paneAbout: $("paneAbout"),
  theme: $("theme"),
  pollInterval: $("pollInterval"),
  autoLoadEditor: $("autoLoadEditor"),
  autoScrollLogs: $("autoScrollLogs"),
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
  tileSize: $("tile_size"),
  threads: $("threads"),
  renderBtn: $("renderBtn"),
  status: $("status"),
  renderTimer: $("renderTimer"),
  progressBar: $("progressBar"),
  progress: $("progress"),
  previewFrame: $("previewFrame"),
  previewEmpty: $("previewEmpty"),
  preview: $("preview"),
  download: $("download"),
  sceneName: $("sceneName"),
  lineNumbers: $("lineNumbers"),
  lineCount: $("lineCount"),
  charCount: $("charCount"),
  sceneSource: $("sceneSource"),
  loadSceneBtn: $("loadSceneBtn"),
  newSceneBtn: $("newSceneBtn"),
  saveSceneBtn: $("saveSceneBtn"),
};

const uiOptions = {
  pollMs: 300,
  autoLoadEditor: true,
  autoScrollLogs: true,
};
let resolutionPresets = [];
let sceneDependencyByFile = new Map();
let lastBackendLogId = 0;
let previewObjectUrl = "";
let pendingLogScroll = false;
let renderActive = false;
let renderStartMs = 0;
let renderTimerInterval = null;
let integratorCatalog = [];
let integratorById = new Map();
const integratorControlState = new Map();
const BACKEND_MODE_KEY = "xtracer-backend-mode";
let api = null;
let backendMode = "server";

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
    async getJobImage(jobId, opts) {
      const res = await fetch(blobUrlForJobImage(jobId, opts));
      if (!res.ok) return null;
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

  const saved = localStorage.getItem(BACKEND_MODE_KEY);
  if (saved === "server" || saved === "wasm") return saved;
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
  const width = Number.isFinite(w) ? Math.max(32, Math.min(8192, w)) : 640;
  const height = Number.isFinite(h) ? Math.max(32, Math.min(8192, h)) : 480;
  return { width, height };
}

function updatePreviewSizing() {
  const { width, height } = currentRenderSize();
  el.previewFrame.style.aspectRatio = `${width} / ${height}`;
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

function setPreviewEmptyState(isEmpty) {
  el.previewFrame.classList.toggle("is-empty", isEmpty);
  if (isEmpty) {
    if (previewObjectUrl) {
      URL.revokeObjectURL(previewObjectUrl);
      previewObjectUrl = "";
    }
    el.preview.removeAttribute("src");
    el.download.removeAttribute("href");
    el.download.setAttribute("aria-disabled", "true");
    el.download.classList.add("is-disabled");
    return;
  }
  el.download.setAttribute("aria-disabled", "false");
  el.download.classList.remove("is-disabled");
}

async function refreshProgressivePreview(jobId) {
  const blob = await api.getJobImage(jobId, { partial: true, cacheBust: true });
  if (!blob || blob.size === 0) return false;

  const url = URL.createObjectURL(blob);
  if (previewObjectUrl) URL.revokeObjectURL(previewObjectUrl);
  previewObjectUrl = url;
  el.preview.src = previewObjectUrl;
  setPreviewEmptyState(false);
  return true;
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
  const saved = integratorControlState.get(selected) || {};

  controls.forEach((ctrl) => {
    const id = ctrl.id || "";
    if (!id) return;

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
    const value = Object.prototype.hasOwnProperty.call(saved, id)
      ? saved[id]
      : (ctrl.default !== undefined ? ctrl.default : "");
    input.value = String(value);

    input.addEventListener("change", () => {
      const curr = integratorControlState.get(selected) || {};
      curr[id] = input.value;
      integratorControlState.set(selected, curr);
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
  const isEditor = mode === "editor";
  const isSettings = mode === "settings";
  const isLogs = mode === "logs";
  const isAbout = mode === "about";
  el.tabRender.classList.toggle("active", isRender);
  el.tabEditor.classList.toggle("active", isEditor);
  el.tabSettings.classList.toggle("active", isSettings);
  el.tabLogs.classList.toggle("active", isLogs);
  el.tabAbout.classList.toggle("active", isAbout);
  el.paneRender.classList.toggle("active", isRender);
  el.paneEditor.classList.toggle("active", isEditor);
  el.paneSettings.classList.toggle("active", isSettings);
  el.paneLogs.classList.toggle("active", isLogs);
  el.paneAbout.classList.toggle("active", isAbout);
  if (isLogs && (uiOptions.autoScrollLogs || pendingLogScroll)) {
    scrollLogToBottom(true);
  }
}

function applyTheme(mode) {
  const root = document.documentElement;
  if (mode === "light" || mode === "dark") root.setAttribute("data-theme", mode);
  else root.setAttribute("data-theme", "system");
  localStorage.setItem("xtracer-theme", mode);
}

function loadUIOptions() {
  const poll = parseInt(localStorage.getItem("xtracer-poll-ms") || "300", 10);
  uiOptions.pollMs = Number.isFinite(poll) ? Math.max(100, Math.min(10000, poll)) : 300;
  uiOptions.autoLoadEditor = localStorage.getItem("xtracer-auto-load-editor") !== "0";
  uiOptions.autoScrollLogs = localStorage.getItem("xtracer-auto-scroll-logs") !== "0";
  el.pollInterval.value = String(uiOptions.pollMs);
  el.autoLoadEditor.checked = uiOptions.autoLoadEditor;
  el.autoScrollLogs.checked = uiOptions.autoScrollLogs;
}

function persistUIOptions() {
  localStorage.setItem("xtracer-poll-ms", String(uiOptions.pollMs));
  localStorage.setItem("xtracer-auto-load-editor", uiOptions.autoLoadEditor ? "1" : "0");
  localStorage.setItem("xtracer-auto-scroll-logs", uiOptions.autoScrollLogs ? "1" : "0");
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
  el.aboutBuildPill.hidden = backendMode !== "wasm";
  el.aboutBackend.textContent = data.backend || (backendMode === "wasm" ? "xtracer_wasm_adapter" : "xtracer_web");
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
    tile_size: el.tileSize.value,
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
      const finalBlob = await api.getJobImage(jobId, { final: true, cacheBust: true });
      if (finalBlob && finalBlob.size > 0) {
        const url = URL.createObjectURL(finalBlob);
        if (previewObjectUrl) URL.revokeObjectURL(previewObjectUrl);
        previewObjectUrl = url;
        el.preview.src = previewObjectUrl;
        el.download.href = previewObjectUrl;
        setPreviewEmptyState(false);
      }
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

async function handleRender() {
  el.renderBtn.disabled = true;
  setRenderActive(true);
  setProgress(0);
  setStatus("submitting job...");
  appendLog(`submit render scene=${el.scene.value} integrator=${el.integrator.value}`);
  try {
    const jobId = await startRender();
    appendLog(`job accepted: ${jobId}`);
    await pollJob(jobId);
  } catch (err) {
    setStatus(`error: ${err.message}`);
    appendLog(`render error: ${err.message}`);
  } finally {
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
  setPreviewEmptyState(true);
  setRenderActive(false);
  if (!el.scene.value) setStatus("no scenes found in scene/ directory");
  else setStatus("idle");

  setActiveTab("render");
  el.renderBtn.addEventListener("click", handleRender);

  el.scene.addEventListener("change", () => {
    updateSceneDependencyPill(el.scene.value);
    const tasks = [loadCameras(el.scene.value)];
    if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(el.scene.value));
    Promise.all(tasks)
      .then(() => appendLog(`scene changed: ${el.scene.value}`))
      .catch((err) => {
        setStatus(`error: ${err.message}`);
        appendLog(`scene change error: ${err.message}`);
      });
  });

  el.theme.addEventListener("change", () => {
    applyTheme(el.theme.value);
    appendLog(`theme=${el.theme.value}`);
  });

  el.integrator.addEventListener("change", () => {
    renderIntegratorControls();
    appendLog(`integrator=${el.integrator.value}`);
  });

  const onSizeChanged = () => {
    syncResolutionPresetFromInputs();
    updatePreviewSizing();
  };
  el.resolutionPreset.addEventListener("change", () => {
    if (el.resolutionPreset.value === "custom") return;
    const index = parseInt(el.resolutionPreset.value, 10);
    if (!Number.isFinite(index) || index < 0 || index >= resolutionPresets.length) return;
    const preset = resolutionPresets[index];
    el.width.value = String(preset.width);
    el.height.value = String(preset.height);
    updatePreviewSizing();
  });
  el.width.addEventListener("input", onSizeChanged);
  el.width.addEventListener("change", onSizeChanged);
  el.height.addEventListener("input", onSizeChanged);
  el.height.addEventListener("change", onSizeChanged);

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

  el.tabRender.addEventListener("click", () => setActiveTab("render"));
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

  el.saveSceneBtn.addEventListener("click", () => {
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
  });

  el.clearLogsBtn.addEventListener("click", () => {
    el.logOutput.textContent = "";
  });

  el.sceneSource.addEventListener("input", updateEditorMetrics);
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
