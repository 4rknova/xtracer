const $ = (id) => document.getElementById(id);

const el = {
  startupScreen: $("startupScreen"),
  startupLabel: $("startupLabel"),
  startupProgressFill: $("startupProgressFill"),
  tabScene: $("tabScene"),
  tabRender: $("tabRender"),
  tabVisual: $("tabVisual"),
  tabWorkspaces: $("tabWorkspaces"),
  tabSettings: $("tabSettings"),
  tabLogs: $("tabLogs"),
  paneScene: $("paneScene"),
  paneRender: $("paneRender"),
  paneVisual: $("paneVisual"),
  paneWorkspaces: $("paneWorkspaces"),
  paneSettings: $("paneSettings"),
  paneLogs: $("paneLogs"),
  qualityControlsCard: $("qualityControlsCard"),
  exportControlsCard: $("exportControlsCard"),
  theme: $("theme"),
  darkPalette: $("darkPalette"),
  pollInterval: $("pollInterval"),
  textHistorySize: $("textHistorySize"),
  visualHistorySize: $("visualHistorySize"),
  autoLoadEditor: $("autoLoadEditor"),
  autoScrollLogs: $("autoScrollLogs"),
  fontSizePreset: $("fontSizePreset"),
  clearLogsBtn: $("clearLogsBtn"),
  logFilterDebug: $("logFilterDebug"),
  logFilterMessage: $("logFilterMessage"),
  logFilterWarning: $("logFilterWarning"),
  logFilterError: $("logFilterError"),
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
  aboutThirdPartyList: $("aboutThirdPartyList"),
  workspaceCreateName: $("workspaceCreateName"),
  workspaceCreateBtn: $("workspaceCreateBtn"),
  workspaceRefreshBtn: $("workspaceRefreshBtn"),
  workspaceViewMode: $("workspaceViewMode"),
  workspaceViewCardsBtn: $("workspaceViewCardsBtn"),
  workspaceViewListBtn: $("workspaceViewListBtn"),
  workspaceActiveHint: $("workspaceActiveHint"),
  workspaceCountHint: $("workspaceCountHint"),
  workspaceMaxConcurrentHint: $("workspaceMaxConcurrentHint"),
  workspaceThreadsHint: $("workspaceThreadsHint"),
  workspaceOpenmpHint: $("workspaceOpenmpHint"),
  workspaceRenderReserveHint: $("workspaceRenderReserveHint"),
  workspaceRenderAutoHint: $("workspaceRenderAutoHint"),
  workspaceList: $("workspaceList"),
  sceneRefreshBtn: $("sceneRefreshBtn"),
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
  samplesPills: Array.from(document.querySelectorAll(".samples-pill")),
  sampleDistribution: $("sample_distribution"),
  aaPills: Array.from(document.querySelectorAll(".aa-pill[data-aa]")),
  rdepth: $("rdepth"),
  tileSize: $("tile_size"),
  tileOrder: $("tile_order"),
  threads: $("threads"),
  threadsPolicyHint: $("threadsPolicyHint"),
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
  postFilterType: $("postFilterType"),
  postFilterAddBtn: $("postFilterAddBtn"),
  postFiltersChain: $("postFiltersChain"),
  clearPreviewOnRender: $("clearPreviewOnRender"),
  renderBtn: $("renderBtn"),
  status: $("status"),
  sceneLoadState: $("sceneLoadState"),
  sceneLoadMessage: $("sceneLoadMessage"),
  sceneLoadJobId: $("sceneLoadJobId"),
  sceneLoadElapsed: $("sceneLoadElapsed"),
  previewTransferStats: $("previewTransferStats"),
  statsDeltaBytes: $("statsDeltaBytes"),
  statsDeltaReqs: $("statsDeltaReqs"),
  statsFullBytes: $("statsFullBytes"),
  statsFullReqs: $("statsFullReqs"),
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
  editorOpStatus: $("editorOpStatus"),
  editObjectSelect: $("editObjectSelect"),
  editGeometryType: $("editGeometryType"),
  editTranslateX: $("editTranslateX"),
  editTranslateY: $("editTranslateY"),
  editTranslateZ: $("editTranslateZ"),
  editRotateX: $("editRotateX"),
  editRotateY: $("editRotateY"),
  editRotateZ: $("editRotateZ"),
  editScaleX: $("editScaleX"),
  editScaleY: $("editScaleY"),
  editScaleZ: $("editScaleZ"),
  editApplyTransformBtn: $("editApplyTransformBtn"),
  editSyncFromVisualBtn: $("editSyncFromVisualBtn"),
  createGeometryType: $("createGeometryType"),
  createMaterialSelect: $("createMaterialSelect"),
  createGeometryId: $("createGeometryId"),
  createObjectId: $("createObjectId"),
  createGeometryBtn: $("createGeometryBtn"),
  lineNumbers: $("lineNumbers"),
  lineCount: $("lineCount"),
  charCount: $("charCount"),
  sceneSource: $("sceneSource"),
  loadSceneBtn: $("loadSceneBtn"),
  newSceneBtn: $("newSceneBtn"),
  saveSceneBtn: $("saveSceneBtn"),
  visualLoadBtn: $("visualLoadBtn"),
  visualCamera: $("visualCamera"),
  visualProjection: $("visualProjection"),
  visualSelectionTag: $("visualSelectionTag"),
  visualShowGrid: $("visualShowGrid"),
  visualViewport: $("visualViewport"),
  visualPanel: $("visualPanel"),
  graphPanel: $("graphPanel"),
  graphCanvas: $("graphCanvas"),
  graphLegend: $("graphLegend"),
  graphResetLayoutBtn: $("graphResetLayoutBtn"),
  textEditorPanel: $("textEditorPanel"),
  editorView3dBtn: $("editorView3dBtn"),
  editorViewGraphBtn: $("editorViewGraphBtn"),
  editorViewTextBtn: $("editorViewTextBtn"),
};

const DEFAULT_THIRD_PARTY_LICENSES = [
  { name: "TinyObjLoader", license: "MIT", url: "https://github.com/syoyo/tinyobjloader" },
  { name: "STB", license: "Public Domain / MIT", url: "https://github.com/nothings/stb" },
  { name: "TinyEXR", license: "BSD-3-Clause", url: "https://github.com/syoyo/tinyexr" },
  { name: "strpool", license: "Public Domain", url: "https://github.com/mattiasgustavsson/libs" },
  { name: "cpp-httplib", license: "MIT", url: "https://github.com/yhirose/cpp-httplib" },
  { name: "RtMidi", license: "MIT-style", url: "https://github.com/thestk/rtmidi" },
  { name: "Three.js", license: "MIT", url: "https://github.com/mrdoob/three.js" },
];

const uiOptions = {
  pollMs: 300,
  textHistoryLimit: 200,
  visualHistoryLimit: 200,
  autoLoadEditor: true,
  autoScrollLogs: true,
  clearPreviewOnRender: false,
  previewSampling: "smooth",
  fontSizePreset: "default",
  fontScale: 1.0,
  darkPalette: "slate",
  lightPalette: "coastal",
};
let resolutionPresets = [];
let sceneDependencyByFile = new Map();
let lastBackendLogId = 0;
let previewObjectUrl = "";
let previewPendingRevokeUrl = "";
let previewPinnedBaseUrl = "";
let previewPinnedBaseBitmapPromise = null;
let preservePreviewUnderlay = false;
let previewSwapToken = 0;
let activePreviewTiles = [];
let activePreviewTileWidth = 0;
let activePreviewTileHeight = 0;
let progressiveDeltaJobId = "";
let progressiveDeltaSinceDone = 0;
let progressiveDeltaEnabled = true;
let progressiveDeltaTmKey = "";
let pendingLogScroll = false;
const LOG_HISTORY_LIMIT = 10000;
const logEntries = [];
const logFilters = {
  debug: false,
  message: true,
  warning: true,
  error: true,
};
let renderActive = false;
let renderStartMs = 0;
let renderTimerInterval = null;
let activeJobId = "";
let lastCompletedJobId = "";
let lastCompletedJobScene = "";
let lastCompletedJobIntegrator = "";
let activePollToken = 0;
const previewTransferStatsState = {
  deltaReqs: 0,
  deltaBytes: 0,
  fullReqs: 0,
  fullBytes: 0,
};
const sceneLoadStatusState = {
  state: "idle",
  message: "No active scene load.",
  jobId: "",
  startedAtMs: 0,
};
let startupDismissed = false;
let startupProgressDone = 0;
let startupProgressTotal = 1;
const historyStores = {
  text: { states: [], index: -1 },
  visual: { states: [], index: -1 },
};
let textHistoryCommitTimer = null;
let suppressHistoryTracking = false;
let postFilterChain = [];
let integratorCatalog = [];
let integratorById = new Map();
const integratorControlState = new Map();
const BACKEND_MODE_KEY = "xtracer-backend-mode";
const ACTIVE_TAB_KEY = "xtracer-active-tab";
const EDITOR_VIEW_MODE_KEY = "xtracer-editor-view-mode";
const WORKSPACE_VIEW_MODE_KEY = "xtracer-workspace-view-mode";
const LAST_SCENE_KEY = "xtracer-last-scene";
const LOG_FILTERS_KEY = "xtracer-log-filters";
const CLIENT_ID_KEY = "xtracer-client-id";
const SIDEBAR_VISIBILITY_CONFIG_URL = "/sidebar_cards.json";
const TAB_MODES = ["scene", "render", "visual", "workspaces", "logs", "settings"];
let sidebarCardVisibility = null;
let sidebarCardVisibilityRaw = "";
let api = null;
let backendMode = "server";
let clientId = "";
let activeWorkspaceId = "";
let workspaceViewMode = "cards";
const workspaceSnapshotById = new Map();
const workspaceRuntimeById = new Map();
let workspaceSpatialIndexStats = null;
let workspaceDraftSaveTimer = null;
let workspaceSettingsSaveTimer = null;
let suppressWorkspaceSettingsSave = false;
let visualEditor = null;
let visualLoadedSceneName = "";
let editorViewMode = "visual";
let runtimeGraphByScene = new Map();
let activeTabMode = "scene";
let graphRenderRafPrimary = 0;
let graphRenderRafSecondary = 0;
const graphView = {
  scale: 1,
  tx: 0,
  ty: 0,
  minScale: 0.35,
  maxScale: 3.0,
  panning: false,
  pointerId: null,
  lastX: 0,
  lastY: 0,
  worldW: 0,
  worldH: 0,
  userAdjusted: false,
  bound: false,
  hoverKey: "",
  data: null,
  expandedNodeKeys: new Set(),
  manualNodePos: new Map(),
  pointerDown: false,
  pointerDownNodeKey: "",
  dragStartX: 0,
  dragStartY: 0,
  dragNodeKey: "",
  dragNodeOffsetX: 0,
  dragNodeOffsetY: 0,
  movedSincePointerDown: false,
};
const graphTexturePreviewCache = new Map();
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

const DARK_PALETTE_OPTIONS = [
  { value: "slate", label: "Slate" },
  { value: "crimson", label: "Crimson" },
  { value: "graphite", label: "Graphite" },
];
const LIGHT_PALETTE_OPTIONS = [
  { value: "coastal", label: "Coastal" },
  { value: "amber", label: "Amber" },
  { value: "sage", label: "Sage" },
];
const DARK_PALETTES = new Set(DARK_PALETTE_OPTIONS.map((p) => p.value));
const LIGHT_PALETTES = new Set(LIGHT_PALETTE_OPTIONS.map((p) => p.value));
const FONT_SIZE_PRESET_DEFAULT = "default";
const FONT_SIZE_PRESET_LARGE = "large";
const POST_FILTER_CATALOG = [
  { id: "desaturate", label: "Desaturate" },
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

function escapeHtml(value) {
  return String(value)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

function renderLogLineHtml(entry) {
  const line = String((entry && entry.line) || "");
  const level = normalizeLogLevel(entry && entry.level);

  // Backend format: #<id> <iso-ts> <LEVEL> <message...>
  const backendMatch = line.match(/^#(\d+)\s+(\S+)\s+([A-Z_]+)\s*(.*)$/);
  if (backendMatch) {
    const id = backendMatch[1];
    const tsRaw = backendMatch[2];
    const ts = tsRaw.replace("T", " ").replace(/Z$/, "");
    const lvl = backendMatch[3];
    const msg = backendMatch[4] || "";
    return `<span class="log-line log-line-backend log-level-${level}">`
      + `<span class="log-token-id">#${escapeHtml(id)}</span>`
      + `<span class="log-token-ts">${escapeHtml(ts)}</span>`
      + `<span class="log-token-level">${escapeHtml(lvl)}</span>`
      + `<span class="log-token-msg">${escapeHtml(msg)}</span>`
      + `</span>`;
  }

  // UI format: [UI hh:mm:ss] <message...>
  const uiMatch = line.match(/^(\[UI [^\]]+\])\s*(.*)$/);
  if (uiMatch) {
    const tag = uiMatch[1];
    const msg = uiMatch[2] || "";
    return `<span class="log-line log-line-ui log-level-${level}">`
      + `<span class="log-token-ui">${escapeHtml(tag)}</span>`
      + `<span class="log-token-msg">${escapeHtml(msg)}</span>`
      + `</span>`;
  }

  return `<span class="log-line log-level-${level}">${escapeHtml(line)}</span>`;
}

function renderLogOutput() {
  if (!el.logOutput) return;
  const html = logEntries
    .filter((entry) => isLogLevelEnabled(entry.level))
    .map((entry) => renderLogLineHtml(entry))
    .join("");
  el.logOutput.innerHTML = html;
  scrollLogToBottom(false);
}

function appendLogEntry(level, line) {
  if (!line) return;
  logEntries.push({
    level: normalizeLogLevel(level),
    line: String(line),
  });
  if (logEntries.length > LOG_HISTORY_LIMIT) {
    logEntries.splice(0, logEntries.length - LOG_HISTORY_LIMIT);
  }
  renderLogOutput();
}

function appendLog(message) {
  if (!message) return;
  appendLogEntry("message", `[UI ${nowStamp()}] ${message}`);
}

function appendBackendLog(entry) {
  const id = entry.id || 0;
  const ts = entry.ts || "";
  const level = normalizeLogLevel(entry.level || "info");
  const levelLabel = String(entry.level || "info").toUpperCase();
  const msg = entry.message || "";
  appendLogEntry(level, `#${id} ${ts} ${levelLabel} ${msg}`);
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
  const hasData = (previewTransferStatsState.deltaReqs + previewTransferStatsState.fullReqs) > 0;
  if (!hasData) {
    if (el.statsDeltaBytes) {
      el.statsDeltaBytes.innerHTML = `<span class="workspace-active-label">Delta Bytes</span><code class="workspace-active-value">-</code>`;
    }
    if (el.statsDeltaReqs) {
      el.statsDeltaReqs.innerHTML = `<span class="workspace-active-label">Delta Requests</span><code class="workspace-active-value">-</code>`;
    }
    if (el.statsFullBytes) {
      el.statsFullBytes.innerHTML = `<span class="workspace-active-label">Full Bytes</span><code class="workspace-active-value">-</code>`;
    }
    if (el.statsFullReqs) {
      el.statsFullReqs.innerHTML = `<span class="workspace-active-label">Full Requests</span><code class="workspace-active-value">-</code>`;
    }
    return;
  }
  if (el.statsDeltaBytes) {
    el.statsDeltaBytes.innerHTML = `<span class="workspace-active-label">Delta Bytes</span><code class="workspace-active-value">${formatBytesShort(previewTransferStatsState.deltaBytes)}</code>`;
  }
  if (el.statsDeltaReqs) {
    el.statsDeltaReqs.innerHTML = `<span class="workspace-active-label">Delta Requests</span><code class="workspace-active-value">${previewTransferStatsState.deltaReqs}</code>`;
  }
  if (el.statsFullBytes) {
    el.statsFullBytes.innerHTML = `<span class="workspace-active-label">Full Bytes</span><code class="workspace-active-value">${formatBytesShort(previewTransferStatsState.fullBytes)}</code>`;
  }
  if (el.statsFullReqs) {
    el.statsFullReqs.innerHTML = `<span class="workspace-active-label">Full Requests</span><code class="workspace-active-value">${previewTransferStatsState.fullReqs}</code>`;
  }
}

function resetPreviewTransferStats() {
  previewTransferStatsState.deltaReqs = 0;
  previewTransferStatsState.deltaBytes = 0;
  previewTransferStatsState.fullReqs = 0;
  previewTransferStatsState.fullBytes = 0;
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

function sleep(ms) {
  const waitMs = Number.isFinite(ms) ? Math.max(0, Math.floor(ms)) : 0;
  return new Promise((resolve) => setTimeout(resolve, waitMs));
}

function createHttpError(status, message) {
  const err = new Error(message || `HTTP ${status}`);
  err.status = status;
  return err;
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
  const ts = Date.now().toString(36);
  const rnd = Math.random().toString(36).slice(2, 10);
  return `client_${ts}_${rnd}`;
}

function ensureClientId() {
  const existing = String(localStorage.getItem(CLIENT_ID_KEY) || "").trim();
  if (existing) {
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

function renderStartupProgress() {
  const total = Math.max(1, Number(startupProgressTotal) || 1);
  const done = Math.max(0, Math.min(total, Number(startupProgressDone) || 0));
  const ratio = done / total;
  const pct = Math.round(ratio * 100);
  if (el.startupProgressFill) {
    el.startupProgressFill.style.width = `${pct}%`;
  }
  if (el.startupLabel) {
    el.startupLabel.textContent = `Loading app... ${pct}%`;
  }
}

function resetStartupProgress(total) {
  startupProgressTotal = Math.max(1, Number(total) || 1);
  startupProgressDone = 0;
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
  if (el.renderBtn) el.renderBtn.disabled = false;
}

function resetProgressiveDeltaState(jobId) {
  progressiveDeltaJobId = String(jobId || "").trim();
  progressiveDeltaSinceDone = 0;
  progressiveDeltaTmKey = "";
}

function appendToneMappingQuery(parts, opts) {
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
}

function blobUrlForJobImage(jobId, opts) {
  const parts = [];
  if (opts && opts.partial) parts.push("partial=1");
  if (opts && opts.final) parts.push("final=1");
  appendToneMappingQuery(parts, opts);
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
  if (opts && opts.cacheBust) parts.push(`t=${Date.now()}`);
  const qs = parts.length ? `?${parts.join("&")}` : "";
  return `/api/jobs/${encodeURIComponent(jobId)}/image_delta${qs}`;
}

function createServerApi() {
  return {
    mode: "server",
    async getLogsSince(sinceId) {
      const data = await getJSON(`/api/logs?since=${sinceId}`);
      return data.entries || [];
    },
    async waitForLogsSince(sinceId, timeoutMs) {
      const waitMs = Number.isFinite(timeoutMs) ? Math.max(1000, Math.min(60000, Math.floor(timeoutMs))) : 15000;
      const data = await getJSON(`/api/logs/wait?since=${sinceId}&timeout_ms=${waitMs}`);
      return data.entries || [];
    },
    async getScenes() {
      const data = await getJSON("/api/scenes");
      return data.scenes || [];
    },
    async getCameras(scene) {
      if (!scene) return { cameras: [], defaultCamera: "" };
      const data = await getSceneJSONWithAsyncLoad(`/api/scenes/${encodeURIComponent(scene)}/cameras`);
      return {
        cameras: data.cameras || [],
        defaultCamera: data.default_camera || "",
      };
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
      const cid = encodeURIComponent(clientId || ensureClientId());
      const data = await getJSON(`/api/scenes/${encodeURIComponent(scene)}/source?client_id=${cid}`);
      return { scene: data.scene || scene, source: data.source || "" };
    },
    async getSceneGeometry(scene) {
      if (!scene) return { meshes: {} };
      const data = await getSceneJSONWithAsyncLoad(`/api/scenes/${encodeURIComponent(scene)}/geometry`);
      return { meshes: (data && data.meshes) || {} };
    },
    async getSceneRuntimeGraph(scene) {
      if (!scene) return { cameras: [], objects: [], surfaces: [], materials: [] };
      const data = await getSceneJSONWithAsyncLoad(`/api/scenes/${encodeURIComponent(scene)}/runtime_graph`);
      return {
        cameras: Array.isArray(data && data.cameras) ? data.cameras : [],
        objects: Array.isArray(data && data.objects) ? data.objects : [],
        surfaces: Array.isArray(data && data.surfaces) ? data.surfaces : [],
        materials: Array.isArray(data && data.materials) ? data.materials : [],
      };
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
  try {
    const entries = supportsLogWait
      ? await api.waitForLogsSince(lastBackendLogId, Math.max(1000, Math.min(60000, uiOptions.pollMs * 10)))
      : await api.getLogsSince(lastBackendLogId);
    for (let i = 0; i < entries.length; i += 1) {
      appendBackendLog(entries[i]);
      if ((entries[i].id || 0) > lastBackendLogId) lastBackendLogId = entries[i].id;
    }
  } catch (err) {
    appendLog(`backend logs unavailable: ${err.message}`);
    await new Promise((r) => setTimeout(r, 1000));
  } finally {
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
  drawActivePreviewTileOverlay(ctx, x, y, drawW, drawH);
}

function drawActivePreviewTileOverlay(ctx, imageX, imageY, imageW, imageH) {
  if (!ctx || !activePreviewTiles.length) return;
  const srcW = Number(activePreviewTileWidth) || el.preview.naturalWidth || 0;
  const srcH = Number(activePreviewTileHeight) || el.preview.naturalHeight || 0;
  if (srcW <= 0 || srcH <= 0 || imageW <= 0 || imageH <= 0) return;

  const sx = imageW / srcW;
  const sy = imageH / srcH;
  ctx.save();
  ctx.fillStyle = "rgba(255, 48, 48, 0.22)";
  ctx.strokeStyle = "rgba(255, 90, 90, 0.95)";
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
    ctx.fillRect(ox, oy, ow, oh);
    ctx.strokeRect(ox + 0.5, oy + 0.5, Math.max(0, ow - 1), Math.max(0, oh - 1));
  }
  ctx.restore();
}

function updateActivePreviewTilesFromJob(data) {
  const nextTiles = Array.isArray(data && data.active_tiles) ? data.active_tiles : [];
  activePreviewTiles = nextTiles;
  activePreviewTileWidth = Number(data && data.width) || 0;
  activePreviewTileHeight = Number(data && data.height) || 0;
}

function clearActivePreviewTiles() {
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

function postFilterCatalogById(id) {
  const key = String(id || "").trim().toLowerCase();
  return POST_FILTER_CATALOG.find((it) => it.id === key) || null;
}

function normalizePostFilterStage(stage) {
  return String(stage || "").toLowerCase() === "before" ? "before" : "after";
}

function normalizePostFilterId(id) {
  const item = postFilterCatalogById(id);
  return item ? item.id : "";
}

function renderPostFilterChain() {
  if (!el.postFiltersChain) return;
  el.postFiltersChain.innerHTML = "";
  if (!Array.isArray(postFilterChain) || postFilterChain.length === 0) {
    const empty = document.createElement("p");
    empty.className = "post-filters-empty";
    empty.textContent = "No filters in chain.";
    el.postFiltersChain.appendChild(empty);
    return;
  }

  postFilterChain.forEach((entry, index) => {
    const filterId = normalizePostFilterId(entry && entry.filter);
    const filterInfo = postFilterCatalogById(filterId);
    if (!filterInfo) return;

    const row = document.createElement("div");
    row.className = "post-filter-entry";
    row.dataset.index = String(index);

    const name = document.createElement("span");
    name.className = "post-filter-name";
    name.textContent = filterInfo.label;

    const stage = document.createElement("select");
    stage.className = "post-filter-stage";
    stage.setAttribute("aria-label", `Filter stage for ${filterInfo.label}`);
    const beforeOpt = document.createElement("option");
    beforeOpt.value = "before";
    beforeOpt.textContent = "Before TM";
    const afterOpt = document.createElement("option");
    afterOpt.value = "after";
    afterOpt.textContent = "After TM";
    stage.appendChild(beforeOpt);
    stage.appendChild(afterOpt);
    stage.value = normalizePostFilterStage(entry && entry.stage);
    stage.addEventListener("change", () => {
      const idx = Number(row.dataset.index);
      if (!Number.isFinite(idx) || idx < 0 || idx >= postFilterChain.length) return;
      postFilterChain[idx].stage = normalizePostFilterStage(stage.value);
      appendLog(`post_filter stage idx=${idx} stage=${postFilterChain[idx].stage}`);
      queueWorkspaceSettingsSave();
    });

    const removeBtn = document.createElement("button");
    removeBtn.type = "button";
    removeBtn.className = "post-filter-remove";
    removeBtn.textContent = "Remove";
    removeBtn.addEventListener("click", () => {
      const idx = Number(row.dataset.index);
      if (!Number.isFinite(idx) || idx < 0 || idx >= postFilterChain.length) return;
      postFilterChain.splice(idx, 1);
      renderPostFilterChain();
      appendLog(`post_filter removed idx=${idx}`);
      queueWorkspaceSettingsSave();
    });

    row.appendChild(name);
    row.appendChild(stage);
    row.appendChild(removeBtn);
    el.postFiltersChain.appendChild(row);
  });
}

function addPostFilterToChain() {
  const filterId = normalizePostFilterId(el.postFilterType && el.postFilterType.value);
  if (!filterId) return;
  postFilterChain.push({ filter: filterId, stage: "after" });
  renderPostFilterChain();
  appendLog(`post_filter add filter=${filterId} stage=after`);
  queueWorkspaceSettingsSave();
}

function populatePostFilterTypeOptions() {
  if (!el.postFilterType) return;
  const prev = String(el.postFilterType.value || "").trim();
  el.postFilterType.innerHTML = "";
  POST_FILTER_CATALOG.forEach((item) => {
    addOption(el.postFilterType, item.id, item.label);
  });
  if (prev && normalizePostFilterId(prev)) {
    el.postFilterType.value = normalizePostFilterId(prev);
  } else if (POST_FILTER_CATALOG.length > 0) {
    el.postFilterType.value = POST_FILTER_CATALOG[0].id;
  }
}

function gatherPostFilterParams() {
  const parts = [];
  (postFilterChain || []).forEach((entry) => {
    const filterId = normalizePostFilterId(entry && entry.filter);
    if (!filterId) return;
    const stage = normalizePostFilterStage(entry && entry.stage);
    parts.push(`${stage}:${filterId}`);
  });
  return parts.join(",");
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

function formatSceneNumber(v, fallback) {
  const n = Number(v);
  const x = Number.isFinite(n) ? n : (fallback || 0);
  const s = x.toFixed(6);
  return s.replace(/\.?0+$/, "") || "0";
}

function readSceneNumber(v, fallback) {
  const n = Number(v);
  return Number.isFinite(n) ? n : fallback;
}

function findSceneGroupRange(source, groupName) {
  const re = new RegExp(`\\b${groupName}\\s*=\\s*\\{`, "m");
  const m = re.exec(source || "");
  if (!m) return null;
  const open = source.indexOf("{", m.index);
  if (open < 0) return null;
  let depth = 0;
  for (let i = open; i < source.length; i += 1) {
    const ch = source[i];
    if (ch === "{") depth += 1;
    else if (ch === "}") {
      depth -= 1;
      if (depth === 0) {
        return {
          groupStart: m.index,
          groupEnd: i + 1,
          bodyStart: open + 1,
          bodyEnd: i,
        };
      }
    }
  }
  return null;
}

function splitTopLevelSceneEntries(source, range) {
  const out = [];
  if (!range) return out;
  const body = source.slice(range.bodyStart, range.bodyEnd);
  let i = 0;
  while (i < body.length) {
    while (i < body.length && /\s/.test(body[i])) i += 1;
    const m = /^([A-Za-z0-9_\-]+)\s*=\s*\{/.exec(body.slice(i));
    if (!m) {
      i += 1;
      continue;
    }
    const name = m[1];
    const localStart = i;
    const openLocal = i + m[0].lastIndexOf("{");
    let depth = 0;
    let endLocal = -1;
    for (let j = openLocal; j < body.length; j += 1) {
      const ch = body[j];
      if (ch === "{") depth += 1;
      else if (ch === "}") {
        depth -= 1;
        if (depth === 0) {
          endLocal = j + 1;
          break;
        }
      }
    }
    if (endLocal < 0) break;
    const entryStart = range.bodyStart + localStart;
    const entryEnd = range.bodyStart + endLocal;
    const bodyStart = range.bodyStart + openLocal + 1;
    const bodyEnd = entryEnd - 1;
    out.push({
      id: name,
      entryStart,
      entryEnd,
      bodyStart,
      bodyEnd,
      body: source.slice(bodyStart, bodyEnd),
    });
    i = endLocal;
  }
  return out;
}

function readSceneRefProp(block, key) {
  const m = new RegExp(`\\b${key}\\s*=\\s*([A-Za-z0-9_.\\-]+)`, "i").exec(block || "");
  return m ? String(m[1] || "").trim() : "";
}

function readSceneStringProp(block, key) {
  const m = new RegExp(`\\b${key}\\s*=\\s*([^\\n\\r]+)`, "i").exec(block || "");
  return m ? String(m[1] || "").trim() : "";
}

function findNamedBlockRange(block, key) {
  const m = new RegExp(`\\b${key}\\s*=\\s*\\{`, "i").exec(block || "");
  if (!m) return null;
  const open = block.indexOf("{", m.index);
  if (open < 0) return null;
  let depth = 0;
  for (let i = open; i < block.length; i += 1) {
    const ch = block[i];
    if (ch === "{") depth += 1;
    else if (ch === "}") {
      depth -= 1;
      if (depth === 0) {
        return {
          start: m.index,
          end: i + 1,
          bodyStart: open + 1,
          bodyEnd: i,
          body: block.slice(open + 1, i),
        };
      }
    }
  }
  return null;
}

function readSceneVec3Prop(block, key, fallback) {
  const fb = fallback || [0, 0, 0];
  const inline = new RegExp(`\\b${key}\\s*=\\s*vec3\\(([^\\)]*)\\)`, "i").exec(block || "");
  if (inline) {
    const p = String(inline[1] || "").split(",").map((x) => Number(x.trim()));
    if (p.length >= 3 && Number.isFinite(p[0]) && Number.isFinite(p[1]) && Number.isFinite(p[2])) {
      return [p[0], p[1], p[2]];
    }
  }
  const group = findNamedBlockRange(block || "", key);
  if (group) {
    const x = readSceneNumber(readSceneStringProp(group.body, "x"), fb[0]);
    const y = readSceneNumber(readSceneStringProp(group.body, "y"), fb[1]);
    const z = readSceneNumber(readSceneStringProp(group.body, "z"), fb[2]);
    return [x, y, z];
  }
  return fb.slice();
}

function addVec3(a, b) {
  return [
    (Number(a && a[0]) || 0) + (Number(b && b[0]) || 0),
    (Number(a && a[1]) || 0) + (Number(b && b[1]) || 0),
    (Number(a && a[2]) || 0) + (Number(b && b[2]) || 0),
  ];
}

function subVec3(a, b) {
  return [
    (Number(a && a[0]) || 0) - (Number(b && b[0]) || 0),
    (Number(a && a[1]) || 0) - (Number(b && b[1]) || 0),
    (Number(a && a[2]) || 0) - (Number(b && b[2]) || 0),
  ];
}

function scaleVec3(a, s) {
  const k = Number(s) || 0;
  return [
    (Number(a && a[0]) || 0) * k,
    (Number(a && a[1]) || 0) * k,
    (Number(a && a[2]) || 0) * k,
  ];
}

function vec3Literal(v, fallback) {
  const fb = fallback || [0, 0, 0];
  return `vec3(${formatSceneNumber(v && v[0], fb[0])}, ${formatSceneNumber(v && v[1], fb[1])}, ${formatSceneNumber(v && v[2], fb[2])})`;
}

function upsertSceneVec3Prop(block, key, vec, indent) {
  const baseIndent = String(indent || "");
  const lit = vec3Literal(vec, [0, 0, 0]);
  const inlineRe = new RegExp(`(\\b${key}\\s*=\\s*)vec3\\([^\\)]*\\)`, "i");
  if (inlineRe.test(block || "")) return String(block || "").replace(inlineRe, `$1${lit}`);
  const nested = findNamedBlockRange(block || "", key);
  if (nested) {
    return `${block.slice(0, nested.start)}${key} = ${lit}${block.slice(nested.end)}`;
  }
  const trimmed = String(block || "").replace(/\s*$/, "");
  const suffix = String(block || "").slice(trimmed.length);
  const join = trimmed.length > 0 ? (trimmed.endsWith("\n") ? "" : "\n") : "";
  return `${trimmed}${join}${baseIndent}${key} = ${lit}${suffix}`;
}

function removeNamedSceneBlock(block, key) {
  const found = findNamedBlockRange(block || "", key);
  if (!found) return String(block || "");
  let out = `${block.slice(0, found.start)}${block.slice(found.end)}`;
  out = out.replace(/\n{3,}/g, "\n\n");
  return out;
}

function parseSceneEditModel(source) {
  const text = String(source || "");
  const cameraGroup = findSceneGroupRange(text, "camera");
  const geometryGroup = findSceneGroupRange(text, "geometry");
  const objectGroup = findSceneGroupRange(text, "object");
  const materialGroup = findSceneGroupRange(text, "material");
  const cameras = [];
  const geometries = new Map();
  const objects = new Map();
  const materials = new Map();
  splitTopLevelSceneEntries(text, cameraGroup).forEach((entry) => {
    cameras.push({
      ...entry,
      type: readSceneStringProp(entry.body, "type").replace(/["']/g, "").toLowerCase(),
    });
  });
  splitTopLevelSceneEntries(text, geometryGroup).forEach((entry) => {
    geometries.set(entry.id, {
      ...entry,
      type: readSceneStringProp(entry.body, "type").replace(/["']/g, "").toLowerCase(),
    });
  });
  splitTopLevelSceneEntries(text, objectGroup).forEach((entry) => {
    const geometry = readSceneRefProp(entry.body, "geometry");
    if (!geometry) return;
    objects.set(entry.id, {
      ...entry,
      geometry,
      material: readSceneRefProp(entry.body, "material"),
    });
  });
  splitTopLevelSceneEntries(text, materialGroup).forEach((entry) => {
    materials.set(entry.id, {
      ...entry,
      type: readSceneStringProp(entry.body, "type").replace(/["']/g, "").toLowerCase(),
    });
  });
  return {
    source: text,
    cameraGroup,
    geometryGroup,
    objectGroup,
    materialGroup,
    cameras,
    geometries,
    objects,
    materials,
  };
}

function ensureGraphCanvasSize() {
  if (!el.graphCanvas) return null;
  const cssW = Math.max(1, Math.floor(el.graphCanvas.clientWidth));
  const cssH = Math.max(1, Math.floor(el.graphCanvas.clientHeight));
  const dpr = window.devicePixelRatio || 1;
  const pxW = Math.max(1, Math.floor(cssW * dpr));
  const pxH = Math.max(1, Math.floor(cssH * dpr));
  if (el.graphCanvas.width !== pxW || el.graphCanvas.height !== pxH) {
    el.graphCanvas.width = pxW;
    el.graphCanvas.height = pxH;
  }
  return { cssW, cssH, dpr };
}

function getGraphTexturePreview(url) {
  const key = String(url || "").trim();
  if (!key) return null;
  let entry = graphTexturePreviewCache.get(key);
  if (entry) return entry;
  const img = new Image();
  entry = { img, loaded: false, failed: false };
  graphTexturePreviewCache.set(key, entry);
  img.onload = () => {
    entry.loaded = true;
    drawGraphCanvas();
  };
  img.onerror = () => {
    entry.failed = true;
    drawGraphCanvas();
  };
  img.src = key;
  return entry;
}

function formatGraphNumeric(v) {
  const n = Number(v);
  if (!Number.isFinite(n)) return String(v ?? "-");
  return Number.isInteger(n) ? String(n) : n.toFixed(3).replace(/\.?0+$/, "");
}

function graphColorLabel(rgb) {
  if (!Array.isArray(rgb) || rgb.length < 3) return "-";
  const fmt = (v) => {
    const n = Number(v);
    return Number.isFinite(n) ? n.toFixed(3) : "0.000";
  };
  return `${fmt(rgb[0])}, ${fmt(rgb[1])}, ${fmt(rgb[2])}`;
}

function graphVec3Label(v) {
  if (!Array.isArray(v) || v.length < 3) return "-";
  return `${formatGraphNumeric(v[0])}, ${formatGraphNumeric(v[1])}, ${formatGraphNumeric(v[2])}`;
}

function drawGraphFittedText(ctx, text, x, y, maxWidth) {
  const raw = String(text ?? "");
  const limit = Number(maxWidth) || 0;
  if (!ctx || limit <= 0) return;
  if (ctx.measureText(raw).width <= limit) {
    ctx.fillText(raw, x, y);
    return;
  }
  const ell = "...";
  let lo = 0;
  let hi = raw.length;
  while (lo < hi) {
    const mid = Math.ceil((lo + hi) / 2);
    const probe = `${raw.slice(0, mid)}${ell}`;
    if (ctx.measureText(probe).width <= limit) lo = mid;
    else hi = mid - 1;
  }
  const out = `${raw.slice(0, Math.max(0, lo))}${ell}`;
  ctx.fillText(out, x, y);
}

function drawGraphCanvas() {
  if (!el.graphCanvas) return;
  const dims = ensureGraphCanvasSize();
  const ctx = el.graphCanvas.getContext("2d");
  if (!dims || !ctx) return;

  ctx.setTransform(1, 0, 0, 1, 0, 0);
  ctx.clearRect(0, 0, el.graphCanvas.width, el.graphCanvas.height);

  const data = graphView.data;
  if (!data) return;

  const hoveredKey = graphView.hoverKey || "";
  const linkedKeys = new Set();
  if (hoveredKey) {
    linkedKeys.add(hoveredKey);
    (data.links || []).forEach((ln) => {
      if (ln.from.key === hoveredKey) linkedKeys.add(ln.to.key);
      if (ln.to.key === hoveredKey) linkedKeys.add(ln.from.key);
    });
  }

  const { cssW, cssH, dpr } = dims;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.fillStyle = "rgba(9,15,21,0.22)";
  ctx.fillRect(0, 0, cssW, cssH);

  // Miro-like infinite grid in screen space derived from world transform.
  const baseStep = 40;
  const step = baseStep * graphView.scale;
  if (step >= 12) {
    const ox = ((graphView.tx % step) + step) % step;
    const oy = ((graphView.ty % step) + step) % step;
    ctx.beginPath();
    ctx.strokeStyle = "rgba(120,145,170,0.14)";
    ctx.lineWidth = 1;
    for (let x = ox; x <= cssW; x += step) {
      ctx.moveTo(x, 0);
      ctx.lineTo(x, cssH);
    }
    for (let y = oy; y <= cssH; y += step) {
      ctx.moveTo(0, y);
      ctx.lineTo(cssW, y);
    }
    ctx.stroke();
  }

  ctx.save();
  ctx.translate(graphView.tx, graphView.ty);
  ctx.scale(graphView.scale, graphView.scale);

  ctx.fillStyle = "rgba(9,15,21,0.22)";
  ctx.fillRect(0, 0, data.viewW, data.viewH);

  ctx.font = "700 14px IBM Plex Sans, sans-serif";
  data.columns.forEach((c) => {
    ctx.fillStyle = c.color;
    ctx.fillText(String(c.title || "").toUpperCase(), c.x, 36);
  });

  data.links.forEach((ln) => {
    const x1 = ln.from.x + ln.from.w;
    const y1 = ln.from.y + ln.from.h / 2;
    const x2 = ln.to.x;
    const y2 = ln.to.y + ln.to.h / 2;
    const c1 = x1 + 60;
    const c2 = x2 - 60;
    const active = !!hoveredKey && (ln.from.key === hoveredKey || ln.to.key === hoveredKey);
    ctx.strokeStyle = active ? "rgba(238,246,255,0.92)" : "rgba(179,195,214,0.55)";
    ctx.lineWidth = active ? 2.8 : 2;
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.bezierCurveTo(c1, y1, c2, y2, x2, y2);
    ctx.stroke();
  });

  data.nodes.forEach((n) => {
    const isHovered = hoveredKey && n.key === hoveredKey;
    const isLinked = hoveredKey && linkedKeys.has(n.key);
    const dimmed = hoveredKey && !isLinked;
    const r = 9;
    ctx.beginPath();
    ctx.moveTo(n.x + r, n.y);
    ctx.lineTo(n.x + n.w - r, n.y);
    ctx.quadraticCurveTo(n.x + n.w, n.y, n.x + n.w, n.y + r);
    ctx.lineTo(n.x + n.w, n.y + n.h - r);
    ctx.quadraticCurveTo(n.x + n.w, n.y + n.h, n.x + n.w - r, n.y + n.h);
    ctx.lineTo(n.x + r, n.y + n.h);
    ctx.quadraticCurveTo(n.x, n.y + n.h, n.x, n.y + n.h - r);
    ctx.lineTo(n.x, n.y + r);
    ctx.quadraticCurveTo(n.x, n.y, n.x + r, n.y);
    ctx.closePath();
    ctx.fillStyle = isHovered
      ? "rgba(46,71,99,0.96)"
      : (dimmed ? "rgba(15,22,29,0.55)" : "rgba(17,27,36,0.86)");
    ctx.fill();
    ctx.strokeStyle = isHovered ? "#ecf6ff" : n.color;
    ctx.globalAlpha = isHovered ? 1 : (dimmed ? 0.35 : 0.68);
    ctx.lineWidth = isHovered ? 2.2 : 1.2;
    ctx.stroke();
    ctx.globalAlpha = 1;

    ctx.fillStyle = dimmed ? "rgba(180,194,208,0.45)" : "#e6eff7";
    ctx.font = "600 13px IBM Plex Sans, sans-serif";
    drawGraphFittedText(ctx, n.id, n.x + 10, n.y + 20, n.w - 20);

    ctx.fillStyle = dimmed ? "rgba(145,160,176,0.42)" : "rgba(170,186,202,0.9)";
    ctx.font = "11px IBM Plex Sans, sans-serif";
    drawGraphFittedText(ctx, n.subtitle, n.x + 10, n.y + 37, n.w - 20);

    if (n.expanded) {
      const rows = Array.isArray(n.propertyRows) ? n.propertyRows : [];
      const rowH = 18;
      const tableTop = n.y + 46;
      const tableH = rows.length * rowH;
      const keyColW = 74;

      if (tableH > 0) {
        ctx.fillStyle = "rgba(8,13,19,0.45)";
        ctx.fillRect(n.x + 6, tableTop, n.w - 12, tableH);
        ctx.strokeStyle = "rgba(140,160,182,0.26)";
        ctx.lineWidth = 1;
        ctx.strokeRect(n.x + 6.5, tableTop + 0.5, n.w - 13, tableH - 1);

        rows.forEach((row, idx) => {
          const y0 = tableTop + idx * rowH;
          if (idx > 0) {
            ctx.strokeStyle = "rgba(130,152,176,0.18)";
            ctx.beginPath();
            ctx.moveTo(n.x + 7, y0 + 0.5);
            ctx.lineTo(n.x + n.w - 7, y0 + 0.5);
            ctx.stroke();
          }
          ctx.strokeStyle = "rgba(130,152,176,0.14)";
          ctx.beginPath();
          ctx.moveTo(n.x + 6 + keyColW, y0 + 1);
          ctx.lineTo(n.x + 6 + keyColW, y0 + rowH - 1);
          ctx.stroke();

          ctx.fillStyle = dimmed ? "rgba(150,166,180,0.42)" : "rgba(162,182,202,0.92)";
          ctx.font = "600 9px IBM Plex Sans, sans-serif";
          drawGraphFittedText(ctx, String(row && row.key ? row.key : "-").toUpperCase(), n.x + 11, y0 + 12, keyColW - 10);

          const valueX = n.x + 12 + keyColW;
          const valueMaxW = Math.max(8, n.w - (valueX - n.x) - 10);
          if (row && Array.isArray(row.color) && row.color.length >= 3) {
            const r = clamp(Number(row.color[0]) || 0, 0, 1);
            const g = clamp(Number(row.color[1]) || 0, 0, 1);
            const b = clamp(Number(row.color[2]) || 0, 0, 1);
            const sw = 12;
            const sh = 12;
            const sy = y0 + 3;
            ctx.fillStyle = `rgb(${Math.round(r * 255)}, ${Math.round(g * 255)}, ${Math.round(b * 255)})`;
            ctx.fillRect(valueX, sy, sw, sh);
            ctx.strokeStyle = "rgba(214,228,240,0.64)";
            ctx.lineWidth = 1;
            ctx.strokeRect(valueX + 0.5, sy + 0.5, sw - 1, sh - 1);
            ctx.fillStyle = dimmed ? "rgba(172,186,198,0.46)" : "rgba(206,220,234,0.95)";
            ctx.font = "10px IBM Plex Sans, sans-serif";
            drawGraphFittedText(ctx, graphColorLabel(row.color), valueX + sw + 6, y0 + 13, Math.max(8, valueMaxW - sw - 6));
          } else {
            ctx.fillStyle = dimmed ? "rgba(172,186,198,0.46)" : "rgba(206,220,234,0.95)";
            ctx.font = "10px IBM Plex Sans, sans-serif";
            drawGraphFittedText(ctx, String(row && row.value !== undefined ? row.value : "-"), valueX, y0 + 13, valueMaxW);
          }
        });
      }

      const previews = Array.isArray(n.texturePreviews) ? n.texturePreviews.slice(0, 3) : [];
      if (previews.length > 0) {
        const sw = 40;
        const sh = 40;
        const gap = 8;
        const py = tableTop + tableH + 6;
        previews.forEach((p, idx) => {
          const px = n.x + 10 + idx * (sw + gap);
          ctx.fillStyle = "rgba(12,18,24,0.7)";
          ctx.fillRect(px, py, sw, sh);
          ctx.strokeStyle = "rgba(165,182,198,0.65)";
          ctx.lineWidth = 1;
          ctx.strokeRect(px + 0.5, py + 0.5, sw - 1, sh - 1);

          const preview = getGraphTexturePreview(p && p.url ? p.url : "");
          if (preview && preview.loaded && preview.img) {
            ctx.drawImage(preview.img, px, py, sw, sh);
          } else {
            ctx.fillStyle = "rgba(168,184,200,0.72)";
            ctx.font = "9px IBM Plex Sans, sans-serif";
            ctx.fillText("tex", px + 11, py + 22);
          }
        });
      }
    }
  });

  ctx.restore();
  el.graphCanvas.classList.toggle("is-panning", !!graphView.panning);
}

function scheduleGraphRender() {
  if (!el.graphCanvas) return;
  if (graphRenderRafPrimary) {
    cancelAnimationFrame(graphRenderRafPrimary);
    graphRenderRafPrimary = 0;
  }
  if (graphRenderRafSecondary) {
    cancelAnimationFrame(graphRenderRafSecondary);
    graphRenderRafSecondary = 0;
  }
  graphRenderRafPrimary = requestAnimationFrame(() => {
    graphRenderRafPrimary = 0;
    graphRenderRafSecondary = requestAnimationFrame(() => {
      graphRenderRafSecondary = 0;
      if (activeTabMode !== "visual" || editorViewMode !== "graph") return;
      renderSceneGraphView();
    });
  });
}

function applyGraphTransform() {
  drawGraphCanvas();
}

function graphWorldPointFromClient(clientX, clientY) {
  if (!el.graphCanvas) return null;
  const rect = el.graphCanvas.getBoundingClientRect();
  const x = (clientX - rect.left - graphView.tx) / graphView.scale;
  const y = (clientY - rect.top - graphView.ty) / graphView.scale;
  return { x, y };
}

function findGraphNodeAt(clientX, clientY) {
  const data = graphView.data;
  if (!data || !data.nodes || !data.nodes.length) return "";
  const p = graphWorldPointFromClient(clientX, clientY);
  if (!p) return "";
  for (let i = data.nodes.length - 1; i >= 0; i -= 1) {
    const n = data.nodes[i];
    if (p.x >= n.x && p.x <= n.x + n.w && p.y >= n.y && p.y <= n.y + n.h) return n.key;
  }
  return "";
}

function findGraphNodeData(key) {
  const data = graphView.data;
  if (!data || !Array.isArray(data.nodes) || !key) return null;
  for (let i = 0; i < data.nodes.length; i += 1) {
    if (data.nodes[i] && data.nodes[i].key === key) return data.nodes[i];
  }
  return null;
}

function isGraphExpandableNode(key) {
  const raw = String(key || "");
  return raw.indexOf("camera:") === 0
    || raw.indexOf("object:") === 0
    || raw.indexOf("geometry:") === 0
    || raw.indexOf("material:") === 0;
}

function fitGraphToViewport() {
  if (!el.graphCanvas || !graphView.worldW || !graphView.worldH) return;
  const rect = el.graphCanvas.getBoundingClientRect();
  const cw = Math.max(1, rect.width);
  const ch = Math.max(1, rect.height);
  const fit = Math.min(cw / graphView.worldW, ch / graphView.worldH);
  graphView.scale = clamp(fit, graphView.minScale, graphView.maxScale);
  graphView.tx = (cw - graphView.worldW * graphView.scale) * 0.5;
  graphView.ty = (ch - graphView.worldH * graphView.scale) * 0.5;
}

function resetGraphView() {
  graphView.userAdjusted = false;
  fitGraphToViewport();
  applyGraphTransform();
}

function bindGraphInteraction() {
  if (!el.graphCanvas || graphView.bound) return;
  graphView.bound = true;

  el.graphCanvas.addEventListener("wheel", (evt) => {
    if (!graphView.worldW || !graphView.worldH) return;
    evt.preventDefault();
    const rect = el.graphCanvas.getBoundingClientRect();
    const cx = evt.clientX - rect.left;
    const cy = evt.clientY - rect.top;
    const k = Math.exp((-evt.deltaY) * 0.0015);
    const nextScale = clamp(graphView.scale * k, graphView.minScale, graphView.maxScale);
    if (!Number.isFinite(nextScale) || Math.abs(nextScale - graphView.scale) < 1e-6) return;
    const ratio = nextScale / graphView.scale;
    graphView.tx = cx - (cx - graphView.tx) * ratio;
    graphView.ty = cy - (cy - graphView.ty) * ratio;
    graphView.scale = nextScale;
    graphView.userAdjusted = true;
    applyGraphTransform();
  }, { passive: false });

  el.graphCanvas.addEventListener("dblclick", (evt) => {
    evt.preventDefault();
    resetGraphView();
  });

  el.graphCanvas.addEventListener("pointerdown", (evt) => {
    if (evt.button !== 0 && evt.button !== 1) return;
    evt.preventDefault();
    graphView.pointerDown = true;
    graphView.pointerId = evt.pointerId;
    graphView.pointerDownNodeKey = findGraphNodeAt(evt.clientX, evt.clientY);
    graphView.dragStartX = evt.clientX;
    graphView.dragStartY = evt.clientY;
    graphView.lastX = evt.clientX;
    graphView.lastY = evt.clientY;
    graphView.dragNodeKey = "";
    graphView.dragNodeOffsetX = 0;
    graphView.dragNodeOffsetY = 0;
    graphView.movedSincePointerDown = false;
    if (evt.button === 0 && graphView.pointerDownNodeKey) {
      const node = findGraphNodeData(graphView.pointerDownNodeKey);
      const p = graphWorldPointFromClient(evt.clientX, evt.clientY);
      if (node && p) {
        graphView.dragNodeKey = node.key;
        graphView.dragNodeOffsetX = p.x - node.x;
        graphView.dragNodeOffsetY = p.y - node.y;
      }
      graphView.panning = false;
    } else {
      graphView.panning = true;
    }
    el.graphCanvas.setPointerCapture(evt.pointerId);
    applyGraphTransform();
  });

  el.graphCanvas.addEventListener("pointermove", (evt) => {
    if (graphView.pointerDown && graphView.pointerId === evt.pointerId) {
      if (!graphView.panning) {
        const moveX = Math.abs(evt.clientX - graphView.dragStartX);
        const moveY = Math.abs(evt.clientY - graphView.dragStartY);
        if ((moveX + moveY) > 4) graphView.movedSincePointerDown = true;
      }
      if (graphView.dragNodeKey) {
        const node = findGraphNodeData(graphView.dragNodeKey);
        const p = graphWorldPointFromClient(evt.clientX, evt.clientY);
        if (node && p) {
          node.x = p.x - graphView.dragNodeOffsetX;
          node.y = p.y - graphView.dragNodeOffsetY;
          graphView.userAdjusted = true;
          if (node.manualKey) {
            graphView.manualNodePos.set(node.manualKey, { x: node.x, y: node.y });
          }
          applyGraphTransform();
          return;
        }
      }
    }
    if (graphView.panning && graphView.pointerId === evt.pointerId) {
      const dx = evt.clientX - graphView.lastX;
      const dy = evt.clientY - graphView.lastY;
      graphView.lastX = evt.clientX;
      graphView.lastY = evt.clientY;
      graphView.tx += dx;
      graphView.ty += dy;
      graphView.userAdjusted = true;
      applyGraphTransform();
      return;
    }

    const nextHover = findGraphNodeAt(evt.clientX, evt.clientY);
    if (nextHover === graphView.hoverKey) return;
    graphView.hoverKey = nextHover;
    drawGraphCanvas();
  });

  const endPan = (evt) => {
    if (!graphView.pointerDown || graphView.pointerId !== evt.pointerId) return;
    const wasPanning = !!graphView.panning;
    const wasDraggingNode = !!graphView.dragNodeKey;
    const moved = !!graphView.movedSincePointerDown;
    const downNodeKey = String(graphView.pointerDownNodeKey || "");
    graphView.pointerDown = false;
    graphView.pointerDownNodeKey = "";
    graphView.panning = false;
    graphView.pointerId = null;
    graphView.dragNodeKey = "";
    graphView.dragNodeOffsetX = 0;
    graphView.dragNodeOffsetY = 0;
    graphView.movedSincePointerDown = false;
    try {
      el.graphCanvas.releasePointerCapture(evt.pointerId);
    } catch (_) {
      // ignore release errors
    }
    if (!wasPanning && !wasDraggingNode && evt.type === "pointerup" && evt.button === 0) {
      const upNodeKey = findGraphNodeAt(evt.clientX, evt.clientY);
      if (upNodeKey && upNodeKey === downNodeKey && isGraphExpandableNode(upNodeKey)) {
        if (graphView.expandedNodeKeys.has(upNodeKey)) graphView.expandedNodeKeys.delete(upNodeKey);
        else graphView.expandedNodeKeys.add(upNodeKey);
        renderSceneGraphView();
        return;
      }
    } else if (wasDraggingNode && !moved && evt.type === "pointerup" && evt.button === 0) {
      const upNodeKey = findGraphNodeAt(evt.clientX, evt.clientY);
      if (upNodeKey && upNodeKey === downNodeKey && isGraphExpandableNode(upNodeKey)) {
        if (graphView.expandedNodeKeys.has(upNodeKey)) graphView.expandedNodeKeys.delete(upNodeKey);
        else graphView.expandedNodeKeys.add(upNodeKey);
        renderSceneGraphView();
        return;
      }
    }
    applyGraphTransform();
  };

  el.graphCanvas.addEventListener("pointerup", endPan);
  el.graphCanvas.addEventListener("pointercancel", endPan);
  el.graphCanvas.addEventListener("pointerleave", (evt) => {
    endPan(evt);
    if (!graphView.hoverKey) return;
    graphView.hoverKey = "";
    drawGraphCanvas();
  });
}

function renderSceneGraphView() {
  if (!el.graphCanvas) return;

  const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const runtime = sceneName ? runtimeGraphByScene.get(sceneName) : null;
  let cameras = [];
  let objects = [];
  let geometries = [];
  let materials = [];

  if (runtime) {
    cameras = (runtime.cameras || [])
      .map((c) => ({
        id: String((c && c.id) || "").trim(),
        type: String((c && c.type) || "camera"),
        position: Array.isArray(c && c.position) ? c.position.slice(0, 3).map((v) => Number(v) || 0) : null,
        target: Array.isArray(c && c.target) ? c.target.slice(0, 3).map((v) => Number(v) || 0) : null,
        up: Array.isArray(c && c.up) ? c.up.slice(0, 3).map((v) => Number(v) || 0) : null,
        orientation: Array.isArray(c && c.orientation) ? c.orientation.slice(0, 3).map((v) => Number(v) || 0) : null,
        fov: Number.isFinite(Number(c && c.fov)) ? Number(c.fov) : null,
        aperture: Number.isFinite(Number(c && c.aperture)) ? Number(c.aperture) : null,
        flength: Number.isFinite(Number(c && c.flength)) ? Number(c.flength) : null,
        ipd: Number.isFinite(Number(c && c.ipd)) ? Number(c.ipd) : null,
      }))
      .filter((c) => c.id)
      .sort((a, b) => a.id.localeCompare(b.id));
    objects = (runtime.objects || [])
      .map((o) => ({
        id: String((o && o.id) || "").trim(),
        geometry: String((o && o.surface) || "").trim(),
        material: String((o && o.material) || "").trim(),
      }))
      .filter((o) => o.id)
      .sort((a, b) => a.id.localeCompare(b.id));
    geometries = (runtime.surfaces || [])
      .map((g) => ({
        id: String((g && g.id) || "").trim(),
        type: String((g && g.type) || "surface"),
        position: Array.isArray(g && g.position) ? g.position.slice(0, 3).map((v) => Number(v) || 0) : null,
        radius: Number.isFinite(Number(g && g.radius)) ? Number(g.radius) : null,
        normal: Array.isArray(g && g.normal) ? g.normal.slice(0, 3).map((v) => Number(v) || 0) : null,
        distance: Number.isFinite(Number(g && g.distance)) ? Number(g.distance) : null,
        triangles: Number.isFinite(Number(g && g.triangles)) ? Number(g.triangles) : null,
        v0: Array.isArray(g && g.v0) ? g.v0.slice(0, 3).map((v) => Number(v) || 0) : null,
        v1: Array.isArray(g && g.v1) ? g.v1.slice(0, 3).map((v) => Number(v) || 0) : null,
        v2: Array.isArray(g && g.v2) ? g.v2.slice(0, 3).map((v) => Number(v) || 0) : null,
      }))
      .filter((g) => g.id)
      .sort((a, b) => a.id.localeCompare(b.id));
    materials = (runtime.materials || [])
      .map((m) => ({
        id: String((m && m.id) || "").trim(),
        type: String((m && m.type) || "material"),
        scalars: Array.isArray(m && m.scalars) ? m.scalars.map((s) => ({
          name: String((s && s.name) || "").trim(),
          value: Number.isFinite(Number(s && s.value)) ? Number(s.value) : (s && s.value),
        })).filter((s) => s.name) : [],
        samplers: Array.isArray(m && m.samplers) ? m.samplers.map((s) => {
          const asset = String((s && s.asset) || "").trim();
          const previewUrl = (sceneName && asset)
            ? `/api/scenes/${encodeURIComponent(sceneName)}/asset?path=${encodeURIComponent(asset)}`
            : "";
          const color = Array.isArray(s && s.color) ? s.color.slice(0, 3).map((v) => Number(v) || 0) : null;
          return {
            name: String((s && s.name) || "").trim(),
            type: String((s && s.type) || "sampler"),
            asset,
            previewUrl,
            color,
          };
        }).filter((s) => s.name) : [],
      }))
      .filter((m) => m.id)
      .sort((a, b) => a.id.localeCompare(b.id));
  } else {
    const source = el.sceneSource ? String(el.sceneSource.value || "") : "";
    const model = parseSceneEditModel(source);
    cameras = (model.cameras || []).slice().sort((a, b) => a.id.localeCompare(b.id));
    objects = Array.from(model.objects.values()).sort((a, b) => a.id.localeCompare(b.id));
    geometries = Array.from(model.geometries.values()).map((g) => ({ ...g })).sort((a, b) => a.id.localeCompare(b.id));
    materials = Array.from((model.materials || new Map()).values())
      .map((m) => ({ ...m, scalars: [], samplers: [] }))
      .sort((a, b) => a.id.localeCompare(b.id));
  }

  const sortByObjectRefs = (items, idFromItem, refFromObject) => {
    const ranks = new Map();
    objects.forEach((o, idx) => {
      const ref = String(refFromObject(o) || "");
      if (!ref) return;
      if (!ranks.has(ref)) ranks.set(ref, []);
      ranks.get(ref).push(idx);
    });
    return (items || []).slice().sort((a, b) => {
      const aId = String(idFromItem(a) || "");
      const bId = String(idFromItem(b) || "");
      const ar = ranks.get(aId) || [];
      const br = ranks.get(bId) || [];
      const as = ar.length ? (ar.reduce((s, v) => s + v, 0) / ar.length) : 1e9;
      const bs = br.length ? (br.reduce((s, v) => s + v, 0) / br.length) : 1e9;
      if (Math.abs(as - bs) > 1e-6) return as - bs;
      return aId.localeCompare(bId);
    });
  };

  geometries = sortByObjectRefs(geometries, (g) => g.id, (o) => o.geometry);
  materials = sortByObjectRefs(materials, (m) => m.id, (o) => o.material);

  const nodeW = 248;
  const nodeH = 48;
  const topPad = 70;
  const rowGap = 14;
  const bottomPad = 40;
  const laneGap = 44;
  const sectionGap = 84;
  const graphRect = el.graphCanvas.getBoundingClientRect();
  const availableH = Math.max(320, (Number(graphRect && graphRect.height) || 720) - topPad - bottomPad);
  const nominalRows = Math.max(4, Math.floor(availableH / (nodeH + rowGap)));
  const maxRowsPerLane = clamp(nominalRows, 4, 16);

  const nodes = [];
  const pos = new Map();
  const columns = [];
  const kindLayout = new Map();

  const detailsNodeHeight = (expanded, rowCount, previewCount) => {
    if (!expanded) return nodeH;
    const detailsH = Math.max(0, rowCount) * 18;
    const previewH = previewCount > 0 ? 48 : 0;
    return nodeH + 6 + detailsH + previewH;
  };

  const kinds = [
    { key: "camera", title: "Camera", color: "#5a88cf", count: cameras.length },
    { key: "object", title: "Object", color: "#9a6846", count: objects.length },
    { key: "geometry", title: "Surface", color: "#4f9a8f", count: geometries.length },
    { key: "material", title: "Material", color: "#5a9a4f", count: materials.length },
  ];
  let xCursor = 40;
  kinds.forEach((k) => {
    const laneCount = Math.max(1, Math.ceil(Math.max(1, k.count) / maxRowsPerLane));
    const laneXs = [];
    for (let i = 0; i < laneCount; i += 1) laneXs.push(xCursor + i * (nodeW + laneGap));
    const laneNextY = new Array(laneCount).fill(topPad);
    kindLayout.set(k.key, {
      laneCount,
      laneXs,
      laneNextY,
      color: k.color,
      title: k.title,
      x: xCursor,
    });
    columns.push({ key: k.key, title: k.title, x: xCursor, color: k.color });
    xCursor += laneCount * (nodeW + laneGap) - laneGap + sectionGap;
  });
  const viewW = Math.max(980, xCursor - sectionGap + 40);

  const pushNode = (kind, id, subtitle, extras) => {
    const layout = kindLayout.get(kind);
    if (!layout) return;
    const ext = (extras && typeof extras === "object") ? extras : {};
    const h = Number(ext.h) > 0 ? Number(ext.h) : nodeH;
    let lane = 0;
    for (let i = 1; i < layout.laneNextY.length; i += 1) {
      if (layout.laneNextY[i] < layout.laneNextY[lane]) lane = i;
    }
    const y = layout.laneNextY[lane] || topPad;
    const node = {
      key: `${kind}:${id}`,
      id,
      kind,
      subtitle: subtitle || "",
      x: layout.laneXs[lane],
      y,
      w: nodeW,
      h,
      color: layout.color,
      expanded: !!ext.expanded,
      propertyRows: Array.isArray(ext.propertyRows) ? ext.propertyRows : [],
      texturePreviews: Array.isArray(ext.texturePreviews) ? ext.texturePreviews : [],
    };
    node.manualKey = `${sceneName || ""}|${node.key}`;
    const manual = graphView.manualNodePos.get(node.manualKey);
    if (manual && Number.isFinite(manual.x) && Number.isFinite(manual.y)) {
      node.x = manual.x;
      node.y = manual.y;
    }
    nodes.push(node);
    pos.set(node.key, node);
    layout.laneNextY[lane] = y + node.h + rowGap;
  };

  cameras.forEach((c) => {
    const nodeKey = `camera:${c.id}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows = [{ key: "type", value: String(c.type || "camera") }];
    if (Array.isArray(c.position)) propertyRows.push({ key: "position", value: graphVec3Label(c.position) });
    if (Array.isArray(c.target)) propertyRows.push({ key: "target", value: graphVec3Label(c.target) });
    if (Array.isArray(c.up)) propertyRows.push({ key: "up", value: graphVec3Label(c.up) });
    if (Array.isArray(c.orientation)) propertyRows.push({ key: "orientation", value: graphVec3Label(c.orientation) });
    if (Number.isFinite(c.fov)) propertyRows.push({ key: "fov", value: formatGraphNumeric(c.fov) });
    if (Number.isFinite(c.aperture)) propertyRows.push({ key: "aperture", value: formatGraphNumeric(c.aperture) });
    if (Number.isFinite(c.flength)) propertyRows.push({ key: "flength", value: formatGraphNumeric(c.flength) });
    if (Number.isFinite(c.ipd)) propertyRows.push({ key: "ipd", value: formatGraphNumeric(c.ipd) });
    pushNode("camera", c.id, c.type || "camera", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded,
      propertyRows,
      texturePreviews: [],
    });
  });
  objects.forEach((o) => {
    const nodeKey = `object:${o.id}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows = [
      { key: "surface", value: String(o.geometry || "-") },
      { key: "material", value: String(o.material || "-") },
    ];
    pushNode("object", o.id, runtime ? "runtime object" : "scene object", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded,
      propertyRows,
      texturePreviews: [],
    });
  });
  geometries.forEach((g) => {
    const nodeKey = `geometry:${g.id}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows = [{ key: "type", value: String(g.type || "surface") }];
    if (Array.isArray(g.position)) propertyRows.push({ key: "position", value: graphVec3Label(g.position) });
    if (Number.isFinite(g.radius)) propertyRows.push({ key: "radius", value: formatGraphNumeric(g.radius) });
    if (Array.isArray(g.normal)) propertyRows.push({ key: "normal", value: graphVec3Label(g.normal) });
    if (Number.isFinite(g.distance)) propertyRows.push({ key: "distance", value: formatGraphNumeric(g.distance) });
    if (Number.isFinite(g.triangles)) propertyRows.push({ key: "triangles", value: formatGraphNumeric(g.triangles) });
    if (Array.isArray(g.v0)) propertyRows.push({ key: "v0", value: graphVec3Label(g.v0) });
    if (Array.isArray(g.v1)) propertyRows.push({ key: "v1", value: graphVec3Label(g.v1) });
    if (Array.isArray(g.v2)) propertyRows.push({ key: "v2", value: graphVec3Label(g.v2) });
    pushNode("geometry", g.id, g.type || "surface", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded,
      propertyRows,
      texturePreviews: [],
    });
  });
  materials.forEach((m) => {
    const nodeKey = `material:${String(m.id || "")}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const scalars = Array.isArray(m && m.scalars) ? m.scalars : [];
    const samplers = Array.isArray(m && m.samplers) ? m.samplers : [];
    const propertyRows = [];
    propertyRows.push({ key: "type", value: String(m && m.type ? m.type : "material") });
    scalars.forEach((s) => {
      const name = String(s && s.name ? s.name : "scalar");
      const value = (s && s.value !== undefined) ? formatGraphNumeric(s.value) : "-";
      propertyRows.push({ key: name, value });
    });
    samplers.forEach((s) => {
      const name = String(s && s.name ? s.name : "sampler");
      const type = String(s && s.type ? s.type : "sampler");
      const row = { key: name, value: type };
      if (Array.isArray(s && s.color) && s.color.length >= 3) row.color = s.color.slice(0, 3);
      propertyRows.push(row);
    });
    const texturePreviews = samplers
      .filter((s) => !!(s && s.previewUrl))
      .map((s) => ({ name: String(s.name || "texture"), url: String(s.previewUrl) }));
    pushNode("material", m.id, m.type || "material", {
      h: detailsNodeHeight(expanded, propertyRows.length, texturePreviews.length),
      expanded,
      propertyRows,
      texturePreviews,
    });
  });

  const links = [];
  objects.forEach((o) => {
    const src = pos.get(`object:${o.id}`);
    const geo = pos.get(`geometry:${o.geometry || ""}`);
    const mat = pos.get(`material:${o.material || ""}`);
    if (src && geo) links.push({ from: src, to: geo });
    if (src && mat) links.push({ from: src, to: mat });
  });

  const viewH = Math.max(
    topPad + rowGap + bottomPad,
    ...Array.from(kindLayout.values()).map((layout) => {
      const laneMax = Math.max(topPad, ...layout.laneNextY);
      return laneMax + bottomPad;
    }),
    ...nodes.map((n) => n.y + n.h + bottomPad)
  );

  graphView.data = {
    columns,
    nodes,
    links,
    viewW,
    viewH,
  };
  graphView.worldW = viewW;
  graphView.worldH = viewH;
  if (!graphView.userAdjusted) {
    fitGraphToViewport();
  }
  applyGraphTransform();
  if (el.graphLegend) {
    const tbody = el.graphLegend.querySelector("tbody");
    if (tbody) {
      tbody.innerHTML = "";
      const rows = [
        ["Camera", cameras.length],
        ["Object", objects.length],
        ["Surface", geometries.length],
        ["Material", materials.length],
      ];
      rows.forEach(([label, count]) => {
        const tr = document.createElement("tr");
        const tdLabel = document.createElement("td");
        const tdCount = document.createElement("td");
        tdLabel.textContent = String(label);
        tdCount.textContent = String(count);
        tr.appendChild(tdLabel);
        tr.appendChild(tdCount);
        tbody.appendChild(tr);
      });
    }
  }
}

function getObjectTransformFromSource(source, objectId) {
  const model = parseSceneEditModel(source);
  const obj = model.objects.get(objectId);
  if (!obj) return null;
  const geo = model.geometries.get(obj.geometry);
  if (!geo) return null;
  const geoType = String(geo.type || "").toLowerCase();
  const modifiers = findNamedBlockRange(geo.body, "modifiers");
  const modsBody = modifiers ? modifiers.body : "";
  let translation = readSceneVec3Prop(modsBody, "translation", [0, 0, 0]);
  if (geoType === "sphere" || geoType === "point") {
    translation = readSceneVec3Prop(geo.body, "position", [0, 0, 0]);
  } else if (geoType === "triangle") {
    const vecData = findNamedBlockRange(geo.body, "vecdata");
    const vBody = vecData ? vecData.body : geo.body;
    const v0 = readSceneVec3Prop(vBody, "v0", [0, 0, 0]);
    const v1 = readSceneVec3Prop(vBody, "v1", [0, 0, 0]);
    const v2 = readSceneVec3Prop(vBody, "v2", [0, 0, 0]);
    translation = scaleVec3(addVec3(addVec3(v0, v1), v2), 1 / 3);
  }
  return {
    objectId,
    geometryId: obj.geometry,
    materialId: obj.material || "",
    geometryType: geoType,
    translation,
    rotation: readSceneVec3Prop(modsBody, "rotation", [0, 0, 0]),
    scale: readSceneVec3Prop(modsBody, "scale", [1, 1, 1]),
  };
}

function geometryEntryIndent(source, entryStart) {
  const lineStart = source.lastIndexOf("\n", Math.max(0, entryStart - 1)) + 1;
  const linePrefix = source.slice(lineStart, entryStart);
  const m = /^(\s*)/.exec(linePrefix);
  return m ? m[1] : "";
}

function replaceGeometryBody(source, geo, newGeoBody) {
  return `${source.slice(0, geo.bodyStart)}${newGeoBody}${source.slice(geo.bodyEnd)}`;
}

function buildModifiersBlock(indent, transform) {
  const inner = `${indent}\t`;
  const t = transform.translation || [0, 0, 0];
  const r = transform.rotation || [0, 0, 0];
  const s = transform.scale || [1, 1, 1];
  return `${indent}modifiers = {\n`
    + `${inner}rotation = vec3(${formatSceneNumber(r[0], 0)}, ${formatSceneNumber(r[1], 0)}, ${formatSceneNumber(r[2], 0)})\n`
    + `${inner}scale = vec3(${formatSceneNumber(s[0], 1)}, ${formatSceneNumber(s[1], 1)}, ${formatSceneNumber(s[2], 1)})\n`
    + `${inner}translation = vec3(${formatSceneNumber(t[0], 0)}, ${formatSceneNumber(t[1], 0)}, ${formatSceneNumber(t[2], 0)})\n`
    + `${indent}}`;
}

function updateObjectTransformInSource(source, objectId, transform, options) {
  const model = parseSceneEditModel(source);
  const obj = model.objects.get(String(objectId || ""));
  if (!obj) throw new Error("object not found");
  const geo = model.geometries.get(obj.geometry);
  if (!geo) throw new Error("geometry not found");
  const geoType = String(geo.type || "").toLowerCase();
  const deltaTranslation = Array.isArray(options && options.deltaTranslation)
    ? options.deltaTranslation.slice(0, 3).map((v) => Number(v) || 0)
    : null;
  const useDelta = !!(options && options.useDelta && deltaTranslation);

  const geoBody = source.slice(geo.bodyStart, geo.bodyEnd);
  const entryIndent = `${geometryEntryIndent(source, geo.entryStart)}\t`;

  if (geoType === "mesh") {
    const existingModifiers = findNamedBlockRange(geoBody, "modifiers");
    const block = buildModifiersBlock(entryIndent, transform);
    let newGeoBody = geoBody;
    if (existingModifiers) {
      newGeoBody = `${geoBody.slice(0, existingModifiers.start)}${block}${geoBody.slice(existingModifiers.end)}`;
    } else {
      const trimmed = geoBody.replace(/\s*$/, "");
      const suffix = geoBody.slice(trimmed.length);
      const leadNewline = trimmed.length > 0 && !trimmed.endsWith("\n") ? "\n" : "";
      newGeoBody = `${trimmed}${leadNewline}${block}\n${suffix.replace(/^\s*/, "")}`;
    }
    return replaceGeometryBody(source, geo, newGeoBody);
  }

  if (geoType === "sphere" || geoType === "point") {
    const currPos = readSceneVec3Prop(geoBody, "position", [0, 0, 0]);
    const nextPos = useDelta ? addVec3(currPos, deltaTranslation) : (transform.translation || currPos);
    let newGeoBody = upsertSceneVec3Prop(geoBody, "position", nextPos, entryIndent);
    newGeoBody = removeNamedSceneBlock(newGeoBody, "modifiers");
    return replaceGeometryBody(source, geo, newGeoBody);
  }

  if (geoType === "triangle") {
    const vecData = findNamedBlockRange(geoBody, "vecdata");
    if (!vecData) throw new Error("triangle vecdata not found");
    const vBody = vecData.body;
    const v0 = readSceneVec3Prop(vBody, "v0", [0, 0, 0]);
    const v1 = readSceneVec3Prop(vBody, "v1", [0, 0, 0]);
    const v2 = readSceneVec3Prop(vBody, "v2", [0, 0, 0]);
    const centroid = scaleVec3(addVec3(addVec3(v0, v1), v2), 1 / 3);
    const delta = useDelta
      ? deltaTranslation
      : subVec3(transform.translation || centroid, centroid);
    let nextVBody = vBody;
    nextVBody = upsertSceneVec3Prop(nextVBody, "v0", addVec3(v0, delta), `${entryIndent}\t`);
    nextVBody = upsertSceneVec3Prop(nextVBody, "v1", addVec3(v1, delta), `${entryIndent}\t`);
    nextVBody = upsertSceneVec3Prop(nextVBody, "v2", addVec3(v2, delta), `${entryIndent}\t`);
    let newGeoBody = `${geoBody.slice(0, vecData.bodyStart)}${nextVBody}${geoBody.slice(vecData.bodyEnd)}`;
    newGeoBody = removeNamedSceneBlock(newGeoBody, "modifiers");
    return replaceGeometryBody(source, geo, newGeoBody);
  }

  throw new Error(`move/edit not supported for geometry type: ${geoType}`);
}

function ensureSceneGroup(source, groupName) {
  const existing = findSceneGroupRange(source, groupName);
  if (existing) return source;
  const suffix = source.endsWith("\n") ? "" : "\n";
  return `${source}${suffix}\n${groupName} = {\n}\n`;
}

function appendEntryToSceneGroup(source, groupName, entryText) {
  const text = ensureSceneGroup(source, groupName);
  const range = findSceneGroupRange(text, groupName);
  if (!range) return text;
  const body = text.slice(range.bodyStart, range.bodyEnd);
  const pre = body.replace(/\s*$/, "");
  const post = body.slice(pre.length);
  const join = pre.length > 0 ? (pre.endsWith("\n") ? "" : "\n") : "";
  const nextBody = `${pre}${join}${entryText}\n${post.replace(/^\s*/, "")}`;
  return `${text.slice(0, range.bodyStart)}${nextBody}${text.slice(range.bodyEnd)}`;
}

function sanitizeSceneId(raw, fallback) {
  const cleaned = String(raw || "").trim().replace(/[^A-Za-z0-9_\-]/g, "_");
  if (cleaned) return cleaned;
  return fallback;
}

function uniqueSceneId(existing, base) {
  let id = base;
  let i = 1;
  while (existing.has(id)) {
    id = `${base}_${i}`;
    i += 1;
  }
  return id;
}

function addMeshObjectToSceneSource(source, options) {
  const model = parseSceneEditModel(source);
  const geometryIds = new Set(Array.from(model.geometries.keys()));
  const objectIds = new Set(Array.from(model.objects.keys()));
  const geometryBase = sanitizeSceneId(options.geometryId, "geo_new");
  const objectBase = sanitizeSceneId(options.objectId, "obj_new");
  const geometryId = uniqueSceneId(geometryIds, geometryBase);
  const objectId = uniqueSceneId(objectIds, objectBase);
  const materialIds = Array.from((model.materials || new Map()).keys());
  const material = String(options.material || "").trim() || (materialIds[0] || "");
  if (!material) throw new Error("no material available; create a material first");

  const transform = {
    translation: options.translation || [0, 0, 0],
    rotation: options.rotation || [0, 0, 0],
    scale: options.scale || [1, 1, 1],
  };
  const modifiers = buildModifiersBlock("\t\t", transform);
  const geometryEntry = `\t${geometryId} = {\n`
    + `\t\ttype = mesh\n`
    + `\t\tsource = gen(${options.generator || "cube"})\n`
    + `\t\tresolution = 24\n`
    + `${modifiers}\n`
    + `\t}`;
  const objectEntry = `\t${objectId} = {\n`
    + `\t\tgeometry = ${geometryId}\n`
    + `\t\tmaterial = ${material}\n`
    + `\t}`;

  let next = appendEntryToSceneGroup(source, "geometry", geometryEntry);
  next = appendEntryToSceneGroup(next, "object", objectEntry);
  return { source: next, objectId, geometryId };
}

function historyLimitFor(type) {
  return type === "text"
    ? Math.max(10, Number(uiOptions.textHistoryLimit) || 200)
    : Math.max(10, Number(uiOptions.visualHistoryLimit) || 200);
}

function trimHistoryStore(type) {
  const store = historyStores[type];
  if (!store) return;
  const limit = historyLimitFor(type);
  if (store.states.length <= limit) return;
  const drop = store.states.length - limit;
  store.states.splice(0, drop);
  store.index = Math.max(0, store.index - drop);
}

function applyHistoryLimits() {
  trimHistoryStore("text");
  trimHistoryStore("visual");
}

function commitHistoryState(type, source, selStart, selEnd) {
  const store = historyStores[type];
  if (!store) return;
  const snapshot = {
    source: String(source || ""),
    selStart: Number.isFinite(selStart) ? selStart : 0,
    selEnd: Number.isFinite(selEnd) ? selEnd : 0,
  };
  const current = (store.index >= 0 && store.index < store.states.length)
    ? store.states[store.index]
    : null;
  if (current && current.source === snapshot.source) {
    current.selStart = snapshot.selStart;
    current.selEnd = snapshot.selEnd;
    return;
  }
  if (store.index < store.states.length - 1) {
    store.states.splice(store.index + 1);
  }
  store.states.push(snapshot);
  store.index = store.states.length - 1;
  trimHistoryStore(type);
}

function resetSceneHistoriesFromCurrentSource() {
  if (textHistoryCommitTimer) {
    clearTimeout(textHistoryCommitTimer);
    textHistoryCommitTimer = null;
  }
  const source = String((el.sceneSource && el.sceneSource.value) || "");
  const selStart = el.sceneSource ? (el.sceneSource.selectionStart || 0) : 0;
  const selEnd = el.sceneSource ? (el.sceneSource.selectionEnd || selStart) : selStart;
  historyStores.text.states = [];
  historyStores.text.index = -1;
  historyStores.visual.states = [];
  historyStores.visual.index = -1;
  commitHistoryState("text", source, selStart, selEnd);
  commitHistoryState("visual", source, selStart, selEnd);
}

function scheduleTextHistoryCommit() {
  if (suppressHistoryTracking) return;
  if (textHistoryCommitTimer) clearTimeout(textHistoryCommitTimer);
  textHistoryCommitTimer = setTimeout(() => {
    textHistoryCommitTimer = null;
    if (!el.sceneSource) return;
    commitHistoryState("text", el.sceneSource.value || "", el.sceneSource.selectionStart, el.sceneSource.selectionEnd);
  }, 180);
}

function flushTextHistoryCommit() {
  if (!textHistoryCommitTimer) return;
  clearTimeout(textHistoryCommitTimer);
  textHistoryCommitTimer = null;
  if (!el.sceneSource) return;
  commitHistoryState("text", el.sceneSource.value || "", el.sceneSource.selectionStart, el.sceneSource.selectionEnd);
}

function applyHistoryStep(type, direction) {
  const store = historyStores[type];
  if (!store || store.states.length === 0) return false;
  if (type === "text") flushTextHistoryCommit();

  const nextIndex = store.index + direction;
  if (nextIndex < 0 || nextIndex >= store.states.length) return false;
  store.index = nextIndex;
  const snap = store.states[store.index];

  suppressHistoryTracking = true;
  try {
    el.sceneSource.value = String(snap.source || "");
    updateEditorMetrics();
    refreshSceneEditControls();
    syncEditorScroll();
    renderSceneGraphView();
    if (type === "text" && el.sceneSource && editorViewMode === "text") {
      const max = el.sceneSource.value.length;
      const a = Math.max(0, Math.min(max, Number(snap.selStart) || 0));
      const b = Math.max(0, Math.min(max, Number(snap.selEnd) || a));
      el.sceneSource.focus();
      el.sceneSource.setSelectionRange(a, b);
    }
  } finally {
    suppressHistoryTracking = false;
  }

  queueWorkspaceDraftSave();
  if (type === "visual" && visualEditor) {
    rebuildVisualFromEditorSource()
      .then(() => {
        const selected = String(el.editObjectSelect && el.editObjectSelect.value ? el.editObjectSelect.value : "").trim();
        if (selected && visualEditor.selectObjectById) visualEditor.selectObjectById(selected, false);
      })
      .catch((err) => appendLog(`visual refresh error: ${err.message}`));
  }
  return true;
}

function handleUndoRedoShortcut(event) {
  const key = String(event.key || "").toLowerCase();
  if (key !== "z") return;
  if (!(event.ctrlKey || event.metaKey) || event.altKey) return;
  if (activeTabMode !== "visual") return;

  const active = document.activeElement;
  if (active && active !== el.sceneSource) {
    const tag = String(active.tagName || "").toLowerCase();
    if (tag === "input" || tag === "textarea" || tag === "select" || active.isContentEditable) return;
  }

  const type = (active === el.sceneSource || editorViewMode === "text") ? "text" : "visual";
  const ok = event.shiftKey ? applyHistoryStep(type, +1) : applyHistoryStep(type, -1);
  if (ok) event.preventDefault();
}

function updateSceneSourceText(nextSource, options) {
  const opts = options && typeof options === "object" ? options : {};
  const historyType = opts.history || "visual";
  const prev = String(el.sceneSource.value || "");
  const next = String(nextSource || "");
  if (prev === next) return;

  el.sceneSource.value = next;
  updateEditorMetrics();
  syncEditorScroll();
  refreshSceneEditControls();
  renderSceneGraphView();
  if (!suppressHistoryTracking) {
    if (historyType === "visual") {
      commitHistoryState("visual", next, el.sceneSource.selectionStart, el.sceneSource.selectionEnd);
    } else if (historyType === "text") {
      commitHistoryState("text", next, el.sceneSource.selectionStart, el.sceneSource.selectionEnd);
    }
  }
  queueWorkspaceDraftSave();
}

function refreshSceneEditControls() {
  if (!el.editObjectSelect || !el.createMaterialSelect) return;
  const source = el.sceneSource ? (el.sceneSource.value || "") : "";
  const model = parseSceneEditModel(source);
  const prevObject = String(el.editObjectSelect.value || "");
  const prevMaterial = String(el.createMaterialSelect.value || "");

  el.editObjectSelect.innerHTML = "";
  addOption(el.editObjectSelect, "", "Select object...");
  Array.from(model.objects.values())
    .sort((a, b) => a.id.localeCompare(b.id))
    .forEach((obj) => {
      const g = model.geometries.get(obj.geometry);
      const type = g ? (g.type || "?") : "?";
      addOption(el.editObjectSelect, obj.id, `${obj.id} (${type})`);
    });
  if (prevObject && model.objects.has(prevObject)) el.editObjectSelect.value = prevObject;
  else el.editObjectSelect.value = "";

  el.createMaterialSelect.innerHTML = "";
  const materialIds = Array.from((model.materials || new Map()).keys());
  if (!materialIds.length) {
    addOption(el.createMaterialSelect, "", "No material");
  } else {
    materialIds.forEach((id) => addOption(el.createMaterialSelect, id, id));
    if (prevMaterial && materialIds.includes(prevMaterial)) el.createMaterialSelect.value = prevMaterial;
    else el.createMaterialSelect.value = materialIds[0];
  }
  syncTransformInputsFromObject(el.editObjectSelect.value || "");
}

function setTransformInputs(values) {
  const t = values && values.translation ? values.translation : [0, 0, 0];
  const r = values && values.rotation ? values.rotation : [0, 0, 0];
  const s = values && values.scale ? values.scale : [1, 1, 1];
  if (el.editTranslateX) el.editTranslateX.value = formatSceneNumber(t[0], 0);
  if (el.editTranslateY) el.editTranslateY.value = formatSceneNumber(t[1], 0);
  if (el.editTranslateZ) el.editTranslateZ.value = formatSceneNumber(t[2], 0);
  if (el.editRotateX) el.editRotateX.value = formatSceneNumber(r[0], 0);
  if (el.editRotateY) el.editRotateY.value = formatSceneNumber(r[1], 0);
  if (el.editRotateZ) el.editRotateZ.value = formatSceneNumber(r[2], 0);
  if (el.editScaleX) el.editScaleX.value = formatSceneNumber(s[0], 1);
  if (el.editScaleY) el.editScaleY.value = formatSceneNumber(s[1], 1);
  if (el.editScaleZ) el.editScaleZ.value = formatSceneNumber(s[2], 1);
}

function currentTransformInputs() {
  return {
    translation: [
      readSceneNumber(el.editTranslateX ? el.editTranslateX.value : 0, 0),
      readSceneNumber(el.editTranslateY ? el.editTranslateY.value : 0, 0),
      readSceneNumber(el.editTranslateZ ? el.editTranslateZ.value : 0, 0),
    ],
    rotation: [
      readSceneNumber(el.editRotateX ? el.editRotateX.value : 0, 0),
      readSceneNumber(el.editRotateY ? el.editRotateY.value : 0, 0),
      readSceneNumber(el.editRotateZ ? el.editRotateZ.value : 0, 0),
    ],
    scale: [
      Math.max(0.0001, readSceneNumber(el.editScaleX ? el.editScaleX.value : 1, 1)),
      Math.max(0.0001, readSceneNumber(el.editScaleY ? el.editScaleY.value : 1, 1)),
      Math.max(0.0001, readSceneNumber(el.editScaleZ ? el.editScaleZ.value : 1, 1)),
    ],
  };
}

function syncTransformInputsFromObject(objectId) {
  const info = objectId ? getObjectTransformFromSource(el.sceneSource.value || "", objectId) : null;
  if (el.editGeometryType) el.editGeometryType.value = info ? (info.geometryType || "") : "";
  setTransformInputs(info || null);
}

async function rebuildVisualFromEditorSource() {
  if (!visualEditor) return;
  const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  await visualEditor.buildScene(sceneName, el.sceneSource.value || "", { meshes: {} });
  refreshVisualCameraOptions();
  syncVisualCameraFromRenderSelection();
}

function selectedExportFormat() {
  const raw = String(el.exportFormat && el.exportFormat.value ? el.exportFormat.value : "png").toLowerCase();
  if (raw === "png" || raw === "jpg" || raw === "bmp" || raw === "tga" || raw === "exr" || raw === "hdr" || raw === "ply") return raw;
  return "png";
}

function updateDownloadUi() {
  const hasExportApi = hasBackendMethod(api, "getJobExport");
  const enabled = hasExportApi && !!lastCompletedJobId;
  const fmt = selectedExportFormat().toUpperCase();
  if (enabled) {
    el.download.disabled = false;
    el.download.setAttribute("aria-disabled", "false");
    el.download.classList.remove("is-disabled");
    el.download.setAttribute("title", `Export ${fmt}`);
    el.download.setAttribute("aria-label", `Export ${fmt}`);
  } else {
    el.download.disabled = true;
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
  if (!hasBackendMethod(api, "getJobImageDelta")) return false;
  if (!progressiveDeltaEnabled) return false;

  const id = String(jobId || "").trim();
  if (!id) return false;
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
  if (!packet) return false;
  if (renderActive && activeJobId && id === String(activeJobId)) {
    recordPreviewTransfer("delta", packet.byteLength || 0);
  }

  const delta = parseImageDeltaPacket(packet);
  if (!delta) return false;
  if (!Array.isArray(delta.tiles) || delta.tiles.length === 0) return true;
  return drawDeltaTilesToPreviewCanvas(delta);
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
  if (activeJobId) {
    try {
      const data = await api.getJob(activeJobId);
      const state = String((data && data.state) || "").toLowerCase();
      if (state === "queued" || state === "running") {
        const progress = Number((data && data.progress) || 0);
        updateActivePreviewTilesFromJob(data);
        setProgress(progress);
        setStatus(`${state} ${(100 * progress).toFixed(1)}%`);
        await refreshProgressivePreview(activeJobId);
        return;
      }
      if (state === "done") {
        lastCompletedJobId = activeJobId;
        lastCompletedJobScene = String((data && data.scene) || el.scene.value || "");
        lastCompletedJobIntegrator = String((data && data.integrator) || el.integrator.value || "");
        activeJobId = "";
        syncGlobalsToWorkspaceRuntime();
      } else if (state === "error") {
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
  if (renderActive && activeJobId === id) return;

  activeJobId = id;
  syncGlobalsToWorkspaceRuntime();
  const token = beginPollSession();
  setRenderActive(true);
  if (el.renderBtn) el.renderBtn.disabled = true;
  pollJob(id, token)
    .catch((err) => {
      setStatus(`error: ${err.message}`);
      appendLog(`render error: ${err.message}`);
    })
    .finally(() => {
      if (activeJobId === id) {
        activeJobId = "";
        syncGlobalsToWorkspaceRuntime();
      }
      if (!activeJobId) {
        setRenderActive(false);
        if (el.renderBtn) el.renderBtn.disabled = false;
      }
    });
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
      queueWorkspaceSettingsSave();
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

function formatResolutionPresetLabel(index, preset, descWidth) {
  const baseDesc = String((preset && preset.description) || "").trim() || `Preset ${presetId(index)}`;
  const paddedDesc = baseDesc.padEnd(Math.max(1, descWidth), " ");
  const sizeLabel = `${preset.width}x${preset.height}`;
  return `${paddedDesc}   ${sizeLabel}`;
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
  const descWidth = resolutionPresets.reduce((max, preset, index) => {
    const desc = String((preset && preset.description) || "").trim() || `Preset ${presetId(index)}`;
    return Math.max(max, desc.length);
  }, 0);
  resolutionPresets.forEach((preset, index) => {
    addOption(
      el.resolutionPreset,
      String(index),
      formatResolutionPresetLabel(index, preset, descWidth),
    );
  });
  syncResolutionPresetFromInputs();
}

function normalizeTabMode(mode) {
  const raw = String(mode || "").toLowerCase();
  if (raw === "editor") return "visual";
  if (raw === "workspace") return "workspaces";
  if (raw === "scene_setup" || raw === "scenesetup") return "scene";
  // Backward compatibility for previously stored "about" tab.
  if (raw === "about") return "settings";
  if (raw === "scene" || raw === "render" || raw === "visual" || raw === "workspaces" || raw === "settings" || raw === "logs") {
    return raw;
  }
  return "scene";
}

function normalizeSidebarCardVisibilityConfig(rawConfig) {
  const normalizeIds = (source) => (source || [])
    .filter((id) => typeof id === "string" && id.trim())
    .map((id) => id.trim())
    .filter((id, idx, arr) => arr.indexOf(id) === idx);
  const normalized = {
    visual_by_editor: {},
  };

  const visualModeRaw = rawConfig
    && ((rawConfig.visual && typeof rawConfig.visual === "object" && !Array.isArray(rawConfig.visual))
      ? rawConfig.visual
      : (rawConfig.visual_by_editor && typeof rawConfig.visual_by_editor === "object" ? rawConfig.visual_by_editor : null));
  if (visualModeRaw) {
    const visual3d = Array.isArray(visualModeRaw.visual) ? visualModeRaw.visual : (Array.isArray(visualModeRaw["3d"]) ? visualModeRaw["3d"] : []);
    const graph = Array.isArray(visualModeRaw.graph) ? visualModeRaw.graph : [];
    const text = Array.isArray(visualModeRaw.text) ? visualModeRaw.text : [];
    if (visual3d.length > 0) normalized.visual_by_editor.visual = normalizeIds(visual3d);
    if (graph.length > 0) normalized.visual_by_editor.graph = normalizeIds(graph);
    if (text.length > 0) normalized.visual_by_editor.text = normalizeIds(text);
  }

  TAB_MODES.forEach((mode) => {
    let source = [];
    if (rawConfig && mode === "visual" && rawConfig.visual && typeof rawConfig.visual === "object" && !Array.isArray(rawConfig.visual)) {
      source = Array.isArray(rawConfig.visual.default) ? rawConfig.visual.default : [];
    } else if (rawConfig && Array.isArray(rawConfig[mode])) {
      source = rawConfig[mode];
    } else if (rawConfig && mode === "visual" && Array.isArray(rawConfig.editor)) {
      source = rawConfig.editor;
    } else if (rawConfig && mode === "settings" && Array.isArray(rawConfig.about)) {
      source = rawConfig.about;
    }
    normalized[mode] = normalizeIds(source);
  });

  if (!normalized.visual_by_editor.visual && Array.isArray(normalized.visual) && normalized.visual.length > 0) {
    normalized.visual_by_editor.visual = [...normalized.visual];
  }

  return normalized;
}

async function fetchSidebarCardConfigJson() {
  const sep = SIDEBAR_VISIBILITY_CONFIG_URL.includes("?") ? "&" : "?";
  const url = `${SIDEBAR_VISIBILITY_CONFIG_URL}${sep}t=${Date.now()}`;
  const response = await fetch(url, { cache: "no-store" });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  return response.json();
}

async function loadSidebarCardVisibilityConfig() {
  const data = await fetchSidebarCardConfigJson();
  if (!data || typeof data !== "object" || Array.isArray(data)) {
    throw new Error("invalid sidebar card config payload");
  }
  sidebarCardVisibilityRaw = JSON.stringify(data);
  sidebarCardVisibility = normalizeSidebarCardVisibilityConfig(data);
  appendLog(`loaded sidebar config from ${SIDEBAR_VISIBILITY_CONFIG_URL}`);
}

async function refreshSidebarCardVisibilityConfig(activeMode) {
  try {
    const data = await fetchSidebarCardConfigJson();
    if (!data || typeof data !== "object" || Array.isArray(data)) {
      throw new Error("invalid sidebar card config payload");
    }
    const raw = JSON.stringify(data);
    if (raw === sidebarCardVisibilityRaw) return;
    sidebarCardVisibilityRaw = raw;
    sidebarCardVisibility = normalizeSidebarCardVisibilityConfig(data);
    applySidebarCardLayout(activeMode || activeTabMode);
    appendLog(`reloaded sidebar config from ${SIDEBAR_VISIBILITY_CONFIG_URL}`);
  } catch (err) {
    appendLog(`sidebar config reload error: ${err.message}`);
  }
}

function normalizeEditorViewMode(mode) {
  const raw = String(mode || "").toLowerCase();
  if (raw === "3d") return "visual";
  if (raw === "visual" || raw === "graph" || raw === "text") return raw;
  return "visual";
}

function syncAaPresetUi() {
  if (!el.aaPills || el.aaPills.length === 0) return;
  const current = String(el.aa && el.aa.value ? el.aa.value : "");
  el.aaPills.forEach((btn) => {
    if (btn.classList.contains("samples-pill")) return;
    const value = String(btn.getAttribute("data-aa") || "");
    btn.classList.toggle("active", value === current);
  });
}

function syncSamplesPresetUi() {
  if (!el.samplesPills || el.samplesPills.length === 0) return;
  const current = String(el.samples && el.samples.value ? el.samples.value : "");
  el.samplesPills.forEach((btn) => {
    const value = String(btn.getAttribute("data-samples") || "");
    btn.classList.toggle("active", value === current);
  });
}

function setEditorViewMode(mode, persist) {
  const nextMode = normalizeEditorViewMode(mode);
  const isVisual = nextMode === "visual";
  const isGraph = nextMode === "graph";
  editorViewMode = nextMode;

  if (el.visualPanel) el.visualPanel.hidden = !isVisual;
  if (el.graphPanel) el.graphPanel.hidden = !isGraph;
  if (el.textEditorPanel) el.textEditorPanel.hidden = (isVisual || isGraph);
  if (el.graphResetLayoutBtn) el.graphResetLayoutBtn.hidden = !isGraph;

  const setActive = (node, state) => {
    if (!node) return;
    node.classList.toggle("active", state);
    node.setAttribute("aria-selected", state ? "true" : "false");
    node.setAttribute("aria-pressed", state ? "true" : "false");
  };
  setActive(el.editorView3dBtn, isVisual);
  setActive(el.editorViewGraphBtn, isGraph);
  setActive(el.editorViewTextBtn, !isVisual && !isGraph);

  if (persist !== false) localStorage.setItem(EDITOR_VIEW_MODE_KEY, nextMode);
  if (activeTabMode === "visual") {
    applySidebarCardLayout("visual");
  }
  if (isVisual && visualEditor) {
    if (visualEditor.onShow) visualEditor.onShow();
    if (visualEditor.resize) visualEditor.resize();
  }
  if (isGraph) scheduleGraphRender();
}

function setSidebarCardVisibility(card, visible) {
  if (!card) return;
  const reduceMotion = window.matchMedia
    && window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  if (card._visibilityTimer) {
    clearTimeout(card._visibilityTimer);
    card._visibilityTimer = null;
  }
  if (reduceMotion) {
    card.hidden = !visible;
    card.classList.remove("is-visibility-animated", "is-visibility-hidden");
    return;
  }
  if (visible && !card.hidden && !card.classList.contains("is-visibility-hidden")) return;
  card.classList.add("is-visibility-animated");
  if (visible) {
    card.hidden = false;
    card.classList.add("is-visibility-hidden");
    requestAnimationFrame(() => {
      card.classList.remove("is-visibility-hidden");
    });
    return;
  }
  if (card.hidden) return;
  card.classList.add("is-visibility-hidden");
  card._visibilityTimer = setTimeout(() => {
    card.hidden = true;
  }, 180);
}

function applySidebarCardLayout(mode) {
  if (!sidebarCardVisibility || typeof sidebarCardVisibility !== "object") return;
  const container = document.querySelector(".panel-controls");
  if (!container) return;

  let visibleIds = sidebarCardVisibility[mode] || [];
  if (mode === "visual" && sidebarCardVisibility.visual_by_editor) {
    const byEditor = sidebarCardVisibility.visual_by_editor;
    visibleIds = byEditor[editorViewMode] || byEditor.visual || visibleIds;
  }
  const visibleSet = new Set(visibleIds);
  const cards = Array.from(container.querySelectorAll("details.control-section"));
  const cardById = new Map(cards.map((card) => [card.id, card]));

  // Order is config-driven: listed cards first (in listed order), then remaining cards.
  const orderedCards = [];
  visibleIds.forEach((id) => {
    const card = cardById.get(id);
    if (card) orderedCards.push(card);
  });
  cards.forEach((card) => {
    if (!visibleSet.has(card.id)) orderedCards.push(card);
  });

  orderedCards.forEach((card) => {
    container.appendChild(card);
    setSidebarCardVisibility(card, visibleSet.has(card.id));
  });

  // On tab switch, keep at least one visible card expanded.
  const visibleCards = visibleIds
    .map((id) => cardById.get(id))
    .filter((card) => !!card);
  if (visibleCards.length > 0 && !visibleCards.some((card) => card.open)) {
    visibleCards[0].open = true;
  }
}

function setActiveTab(mode) {
  const nextMode = normalizeTabMode(mode);
  activeTabMode = nextMode;
  const isScene = nextMode === "scene";
  const isRender = nextMode === "render";
  const isVisual = nextMode === "visual";
  const isWorkspaces = nextMode === "workspaces";
  const isSettings = nextMode === "settings";
  const isLogs = nextMode === "logs";
  const setActive = (node, state) => { if (node) node.classList.toggle("active", state); };
  setActive(el.tabScene, isScene);
  setActive(el.tabRender, isRender);
  setActive(el.tabVisual, isVisual);
  setActive(el.tabWorkspaces, isWorkspaces);
  setActive(el.tabSettings, isSettings);
  setActive(el.tabLogs, isLogs);
  setActive(el.paneScene, isScene);
  setActive(el.paneRender, isRender);
  setActive(el.paneVisual, isVisual);
  setActive(el.paneWorkspaces, isWorkspaces);
  setActive(el.paneSettings, isSettings);
  setActive(el.paneLogs, isLogs);
  applySidebarCardLayout(nextMode);
  void refreshSidebarCardVisibilityConfig(nextMode);
  localStorage.setItem(ACTIVE_TAB_KEY, nextMode);
  if (isVisual && editorViewMode === "visual" && visualEditor) visualEditor.onShow();
  if (isVisual && editorViewMode === "graph") scheduleGraphRender();
  if (isLogs && (uiOptions.autoScrollLogs || pendingLogScroll)) {
    scrollLogToBottom(true);
  }
  if (isWorkspaces && hasBackendMethod(api, "getWorkspaces")) {
    refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
  }
  if (isRender) {
    requestAnimationFrame(() => {
      updatePreviewSizing();
    });
    restorePreviewForActiveWorkspace().catch((err) => {
      appendLog(`preview restore error: ${err.message}`);
    });
  }
}

function initSidebarAccordion() {
  const container = document.querySelector(".panel-controls");
  if (!container) return;

  const cards = Array.from(container.querySelectorAll("details.control-section"));
  if (cards.length < 2) return;
  const reduceMotion = window.matchMedia
    && window.matchMedia("(prefers-reduced-motion: reduce)").matches;

  const clearBodyAnimStyles = (body) => {
    if (!body) return;
    if (body._accordionTimer) {
      clearTimeout(body._accordionTimer);
      body._accordionTimer = null;
    }
    body.style.transition = "";
    body.style.overflow = "";
    body.style.maxHeight = "";
    body.style.opacity = "";
  };

  const animateOpen = (card) => {
    if (!card || card.open) return;
    const body = card.querySelector(".control-section-body");
    card.open = true;
    if (!body || reduceMotion) return;

    clearBodyAnimStyles(body);
    const target = body.scrollHeight;
    body.style.overflow = "hidden";
    body.style.maxHeight = "0px";
    body.style.opacity = "0";
    void body.offsetHeight;
    body.style.transition = "max-height 190ms ease, opacity 160ms ease";
    body.style.maxHeight = `${target}px`;
    body.style.opacity = "1";
    body._accordionTimer = setTimeout(() => {
      clearBodyAnimStyles(body);
    }, 220);
  };

  const animateClose = (card) => {
    if (!card || !card.open) return;
    const body = card.querySelector(".control-section-body");
    if (!body || reduceMotion) {
      card.open = false;
      return;
    }

    clearBodyAnimStyles(body);
    const start = body.scrollHeight;
    body.style.overflow = "hidden";
    body.style.maxHeight = `${start}px`;
    body.style.opacity = "1";
    void body.offsetHeight;
    body.style.transition = "max-height 190ms ease, opacity 150ms ease";
    body.style.maxHeight = "0px";
    body.style.opacity = "0";
    body._accordionTimer = setTimeout(() => {
      card.open = false;
      clearBodyAnimStyles(body);
    }, 220);
  };

  // Enforce one-open initial state.
  const firstOpen = cards.find((card) => card.open) || cards[0];
  cards.forEach((card) => {
    card.open = card === firstOpen;
  });

  cards.forEach((card) => {
    const summary = card.querySelector("summary");
    if (!summary) return;
    summary.addEventListener("click", (ev) => {
      ev.preventDefault();
      if (card.open) {
        animateClose(card);
        return;
      }
      cards.forEach((other) => {
        if (other === card) return;
        animateClose(other);
      });
      animateOpen(card);
    });
  });
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
  if (el.editObjectSelect && el.editObjectSelect.value && visualEditor.selectObjectById) {
    visualEditor.selectObjectById(el.editObjectSelect.value, false);
  }
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

function effectiveThemeMode(mode) {
  const value = String(mode || "").toLowerCase();
  if (value === "dark" || value === "light") return value;
  if (window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches) return "dark";
  return "light";
}

function normalizeDarkPalette(value) {
  const palette = String(value || "").toLowerCase();
  return DARK_PALETTES.has(palette) ? palette : "slate";
}

function normalizeLightPalette(value) {
  const palette = String(value || "").toLowerCase();
  return LIGHT_PALETTES.has(palette) ? palette : "coastal";
}

function applyDarkPalette(palette) {
  const normalized = normalizeDarkPalette(palette);
  uiOptions.darkPalette = normalized;
  document.documentElement.setAttribute("data-dark-palette", normalized);
}

function applyLightPalette(palette) {
  const normalized = normalizeLightPalette(palette);
  uiOptions.lightPalette = normalized;
  document.documentElement.setAttribute("data-light-palette", normalized);
}

function refreshPaletteOptions() {
  if (!el.darkPalette) return;
  const mode = effectiveThemeMode(el.theme ? el.theme.value : "system");
  const options = mode === "dark" ? DARK_PALETTE_OPTIONS : LIGHT_PALETTE_OPTIONS;
  const selected = mode === "dark" ? uiOptions.darkPalette : uiOptions.lightPalette;
  el.darkPalette.innerHTML = "";
  options.forEach((opt) => addOption(el.darkPalette, opt.value, opt.label));
  el.darkPalette.value = selected;
}

function clampFontScale(v) {
  return Math.max(0.8, Math.min(1.4, Number(v) || 1.0));
}

function clampHistoryLimit(v) {
  const n = Number(v);
  if (!Number.isFinite(n)) return 200;
  return Math.max(10, Math.min(2000, Math.floor(n)));
}

function updateFontScaleUI() {
  if (!el.fontSizePreset) return;
  el.fontSizePreset.value = normalizeFontSizePreset(uiOptions.fontSizePreset);
}

function applyFontScale(scale) {
  uiOptions.fontScale = clampFontScale(scale);
  document.documentElement.style.fontSize = `${(uiOptions.fontScale * 100).toFixed(1)}%`;
  updateFontScaleUI();
}

function loadUIOptions() {
  const poll = parseInt(localStorage.getItem("xtracer-poll-ms") || "300", 10);
  uiOptions.pollMs = Number.isFinite(poll) ? Math.max(100, Math.min(10000, poll)) : 300;
  uiOptions.textHistoryLimit = clampHistoryLimit(localStorage.getItem("xtracer-text-history-limit") || "200");
  uiOptions.visualHistoryLimit = clampHistoryLimit(localStorage.getItem("xtracer-visual-history-limit") || "200");
  uiOptions.autoLoadEditor = localStorage.getItem("xtracer-auto-load-editor") !== "0";
  uiOptions.autoScrollLogs = localStorage.getItem("xtracer-auto-scroll-logs") !== "0";
  uiOptions.clearPreviewOnRender = localStorage.getItem("xtracer-clear-preview-on-render") === "1";
  const presetRaw = localStorage.getItem("xtracer-ui-font-size-preset");
  if (presetRaw) {
    uiOptions.fontSizePreset = normalizeFontSizePreset(presetRaw);
  } else {
    const fontScaleRaw = parseFloat(localStorage.getItem("xtracer-ui-font-scale") || "1");
    const legacyScale = Number.isFinite(fontScaleRaw) ? clampFontScale(fontScaleRaw) : 1.0;
    uiOptions.fontSizePreset = fontSizePresetFromScale(legacyScale);
  }
  uiOptions.fontScale = scaleForFontSizePreset(uiOptions.fontSizePreset);
  const previewSamplingRaw = String(localStorage.getItem("xtracer-preview-sampling") || "smooth").toLowerCase();
  const previewSampling = (previewSamplingRaw === "linear" || previewSamplingRaw === "bilinear")
    ? "smooth"
    : previewSamplingRaw;
  uiOptions.previewSampling = (previewSampling === "nearest" || previewSampling === "smooth")
    ? previewSampling
    : "smooth";
  uiOptions.darkPalette = normalizeDarkPalette(localStorage.getItem("xtracer-dark-palette") || "slate");
  uiOptions.lightPalette = normalizeLightPalette(localStorage.getItem("xtracer-light-palette") || "coastal");
  el.pollInterval.value = String(uiOptions.pollMs);
  if (el.textHistorySize) el.textHistorySize.value = String(uiOptions.textHistoryLimit);
  if (el.visualHistorySize) el.visualHistorySize.value = String(uiOptions.visualHistoryLimit);
  el.autoLoadEditor.checked = uiOptions.autoLoadEditor;
  el.autoScrollLogs.checked = uiOptions.autoScrollLogs;
  el.clearPreviewOnRender.checked = uiOptions.clearPreviewOnRender;
  if (el.previewSampling) el.previewSampling.value = uiOptions.previewSampling;
  applyDarkPalette(uiOptions.darkPalette);
  applyLightPalette(uiOptions.lightPalette);
  refreshPaletteOptions();
  applyFontScale(uiOptions.fontScale);

  try {
    const raw = localStorage.getItem(LOG_FILTERS_KEY);
    if (raw) {
      const parsed = JSON.parse(raw);
      if (parsed && typeof parsed === "object") {
        if (typeof parsed.debug === "boolean") logFilters.debug = parsed.debug;
        if (typeof parsed.message === "boolean") logFilters.message = parsed.message;
        if (typeof parsed.warning === "boolean") logFilters.warning = parsed.warning;
        if (typeof parsed.error === "boolean") logFilters.error = parsed.error;
      }
    }
  } catch (_) {
    // keep defaults if local storage has invalid JSON
  }
}

function persistUIOptions() {
  localStorage.setItem("xtracer-poll-ms", String(uiOptions.pollMs));
  localStorage.setItem("xtracer-text-history-limit", String(uiOptions.textHistoryLimit));
  localStorage.setItem("xtracer-visual-history-limit", String(uiOptions.visualHistoryLimit));
  localStorage.setItem("xtracer-auto-load-editor", uiOptions.autoLoadEditor ? "1" : "0");
  localStorage.setItem("xtracer-auto-scroll-logs", uiOptions.autoScrollLogs ? "1" : "0");
  localStorage.setItem("xtracer-clear-preview-on-render", uiOptions.clearPreviewOnRender ? "1" : "0");
  localStorage.setItem("xtracer-preview-sampling", uiOptions.previewSampling);
  localStorage.setItem("xtracer-ui-font-size-preset", uiOptions.fontSizePreset);
  localStorage.setItem("xtracer-ui-font-scale", String(uiOptions.fontScale));
  localStorage.setItem("xtracer-dark-palette", uiOptions.darkPalette);
  localStorage.setItem("xtracer-light-palette", uiOptions.lightPalette);
  localStorage.setItem(LOG_FILTERS_KEY, JSON.stringify(logFilters));
}

async function loadScenes() {
  const scenes = await api.getScenes();
  const sceneItems = await buildSceneLabels(scenes);
  const prev = el.scene.value;
  const saved = String(localStorage.getItem(LAST_SCENE_KEY) || "").trim();
  el.scene.innerHTML = "";
  sceneDependencyByFile = new Map(sceneItems.map((item) => [item.sceneFile, !!item.dependsExternal]));
  sceneItems.forEach((item) => addOption(el.scene, item.sceneFile, item.label));
  const preferred = prev || saved;
  if (preferred) el.scene.value = preferred;
  if (!el.scene.value && el.scene.options.length > 0) el.scene.selectedIndex = 0;
  updateSceneDependencyPill(el.scene.value);
  if (el.scene.value) localStorage.setItem(LAST_SCENE_KEY, el.scene.value);
  else localStorage.removeItem(LAST_SCENE_KEY);
}

function updateWorkspaceActiveHint() {
  if (!el.workspaceActiveHint) return;
  const id = String(activeWorkspaceId || "").trim();
  const label = "Active workspace";
  const value = id || "-";
  el.workspaceActiveHint.innerHTML = `<span class="workspace-active-label">${label}</span><code class="workspace-active-value">${value}</code>`;
}

function updateWorkspaceCountHint(count) {
  if (!el.workspaceCountHint) return;
  const n = Math.max(0, Number(count) || 0);
  el.workspaceCountHint.innerHTML = `<span class="workspace-active-label">Workspaces</span><code class="workspace-active-value">${n}</code>`;
}

function updateWorkspaceServerStatHint(node, label, value) {
  if (!node) return;
  const rendered = (value === null || value === undefined || value === "") ? "-" : String(value);
  node.innerHTML = `<span class="workspace-active-label">${label}</span><code class="workspace-active-value">${rendered}</code>`;
}

function updateWorkspaceServerStatsHints(data) {
  const maxConcurrent = Number(data && data.max_concurrent_renders);
  const logicalCores = Number(data && data.logical_cores);
  const openmpThreads = Number(data && data.openmp_max_threads);
  const renderReserveThreads = Number(data && data.render_reserve_threads);
  const renderAutoThreads = Number(data && data.render_auto_threads);
  updateWorkspaceServerStatHint(
    el.workspaceMaxConcurrentHint,
    "Max Concurrent Renders",
    Number.isFinite(maxConcurrent) && maxConcurrent > 0 ? Math.floor(maxConcurrent) : "-"
  );
  updateWorkspaceServerStatHint(
    el.workspaceThreadsHint,
    "Hardware Threads",
    Number.isFinite(logicalCores) && logicalCores > 0 ? Math.floor(logicalCores) : "-"
  );
  updateWorkspaceServerStatHint(
    el.workspaceOpenmpHint,
    "OpenMP Max Threads",
    Number.isFinite(openmpThreads) && openmpThreads > 0 ? Math.floor(openmpThreads) : "-"
  );
  updateWorkspaceServerStatHint(
    el.workspaceRenderReserveHint,
    "Render Reserve Threads",
    Number.isFinite(renderReserveThreads) && renderReserveThreads >= 0 ? Math.floor(renderReserveThreads) : "-"
  );
  updateWorkspaceServerStatHint(
    el.workspaceRenderAutoHint,
    "Render Auto Threads",
    Number.isFinite(renderAutoThreads) && renderAutoThreads > 0 ? Math.floor(renderAutoThreads) : "-"
  );
  if (el.threadsPolicyHint) {
    const reserveText = Number.isFinite(renderReserveThreads) && renderReserveThreads >= 0
      ? String(Math.floor(renderReserveThreads))
      : "?";
    const autoText = Number.isFinite(renderAutoThreads) && renderAutoThreads > 0
      ? String(Math.floor(renderAutoThreads))
      : "?";
    el.threadsPolicyHint.textContent = `Threads 0 uses auto mode (${autoText}), reserving ${reserveText} for server responsiveness.`;
  }
}

function normalizeWorkspaceViewMode(value) {
  return String(value || "").toLowerCase() === "list" ? "list" : "cards";
}

function setWorkspaceViewMode(mode, persist) {
  workspaceViewMode = normalizeWorkspaceViewMode(mode);
  if (el.workspaceViewMode) el.workspaceViewMode.value = workspaceViewMode;
  if (el.workspaceViewCardsBtn) {
    const active = workspaceViewMode === "cards";
    el.workspaceViewCardsBtn.classList.toggle("active", active);
    el.workspaceViewCardsBtn.setAttribute("aria-pressed", active ? "true" : "false");
  }
  if (el.workspaceViewListBtn) {
    const active = workspaceViewMode === "list";
    el.workspaceViewListBtn.classList.toggle("active", active);
    el.workspaceViewListBtn.setAttribute("aria-pressed", active ? "true" : "false");
  }
  if (el.workspaceList) {
    el.workspaceList.classList.toggle("workspace-list-cards", workspaceViewMode === "cards");
    el.workspaceList.classList.toggle("workspace-list-list", workspaceViewMode === "list");
  }
  if (persist !== false) {
    localStorage.setItem(WORKSPACE_VIEW_MODE_KEY, workspaceViewMode);
  }
}

function formatWorkspaceUpdated(updatedMs) {
  const ms = Number(updatedMs || 0);
  if (!Number.isFinite(ms) || ms <= 0) return "-";
  const d = new Date(ms);
  if (Number.isNaN(d.getTime())) return "-";
  return d.toLocaleString();
}

function buildWorkspacePreviewUrl(lastJobId) {
  const jobId = String(lastJobId || "").trim();
  if (!jobId) return "";
  return `/api/jobs/${encodeURIComponent(jobId)}/image?final=1&tm=aces`;
}

function workspaceStateLabel(workspace) {
  const id = String((workspace && workspace.id) || "");
  const activeJob = String((workspace && workspace.active_job_id) || "");
  if (activeJob) return "Rendering";
  if (id && id === activeWorkspaceId) return "Active";
  return "Idle";
}

function serializeIntegratorControlState() {
  const out = {};
  integratorControlState.forEach((value, key) => {
    if (!key || !value || typeof value !== "object") return;
    out[key] = { ...value };
  });
  return out;
}

function workspaceSettingsPayload() {
  return {
    quality: {
      samples: String(el.samples && el.samples.value ? el.samples.value : "1"),
      aa: String(el.aa && el.aa.value ? el.aa.value : "1"),
      sample_distribution: String(el.sampleDistribution && el.sampleDistribution.value ? el.sampleDistribution.value : "grid"),
      rdepth: String(el.rdepth && el.rdepth.value ? el.rdepth.value : "3"),
    },
    frame: {
      width: String(el.width && el.width.value ? el.width.value : "500"),
      height: String(el.height && el.height.value ? el.height.value : "500"),
    },
    integrator: {
      id: String(el.integrator && el.integrator.value ? el.integrator.value : ""),
      tile_size: String(el.tileSize && el.tileSize.value ? el.tileSize.value : "32"),
      tile_order: String(el.tileOrder && el.tileOrder.value ? el.tileOrder.value : "random"),
      threads: String(el.threads && el.threads.value ? el.threads.value : "0"),
      controls_by_integrator: serializeIntegratorControlState(),
    },
    tone_mapping: {
      operator: String(el.toneMapping && el.toneMapping.value ? el.toneMapping.value : "aces"),
      exposure: String(el.toneMappingExposure && el.toneMappingExposure.value ? el.toneMappingExposure.value : "1.0"),
      white_point: String(el.toneMappingWhitePoint && el.toneMappingWhitePoint.value ? el.toneMappingWhitePoint.value : "1.0"),
      mantiuk_contrast: String(el.toneMappingMantiukContrast && el.toneMappingMantiukContrast.value ? el.toneMappingMantiukContrast.value : "0.1"),
      mantiuk_saturation: String(el.toneMappingMantiukSaturation && el.toneMappingMantiukSaturation.value ? el.toneMappingMantiukSaturation.value : "0.8"),
      mantiuk_detail: String(el.toneMappingMantiukDetail && el.toneMappingMantiukDetail.value ? el.toneMappingMantiukDetail.value : "1.0"),
    },
    post_filters: Array.isArray(postFilterChain)
      ? postFilterChain.map((entry) => ({
        filter: normalizePostFilterId(entry && entry.filter),
        stage: normalizePostFilterStage(entry && entry.stage),
      })).filter((entry) => !!entry.filter)
      : [],
  };
}

function queueWorkspaceSettingsSave() {
  if (suppressWorkspaceSettingsSave) return;
  if (!activeWorkspaceId) return;
  if (!hasBackendMethod(api, "saveWorkspaceSettings")) return;
  if (workspaceSettingsSaveTimer) {
    clearTimeout(workspaceSettingsSaveTimer);
    workspaceSettingsSaveTimer = null;
  }
  workspaceSettingsSaveTimer = setTimeout(() => {
    workspaceSettingsSaveTimer = null;
    let json = "{}";
    try {
      json = JSON.stringify(workspaceSettingsPayload());
    } catch (_) {
      return;
    }
    api.saveWorkspaceSettings(json)
      .catch((err) => appendLog(`workspace settings save failed: ${err.message}`));
  }, 250);
}

function applyWorkspaceSettings(settings) {
  const cfg = settings && typeof settings === "object" ? settings : null;
  if (!cfg) return;

  suppressWorkspaceSettingsSave = true;
  try {
    const quality = cfg.quality && typeof cfg.quality === "object" ? cfg.quality : null;
    if (quality) {
      if (el.samples && quality.samples !== undefined) el.samples.value = String(quality.samples);
      if (el.aa && quality.aa !== undefined) el.aa.value = String(quality.aa);
      if (el.sampleDistribution && quality.sample_distribution !== undefined) el.sampleDistribution.value = String(quality.sample_distribution);
      if (el.rdepth && quality.rdepth !== undefined) el.rdepth.value = String(quality.rdepth);
      syncSamplesPresetUi();
      syncAaPresetUi();
    }

    const frame = cfg.frame && typeof cfg.frame === "object" ? cfg.frame : null;
    if (frame) {
      if (el.width && frame.width !== undefined) el.width.value = String(frame.width);
      if (el.height && frame.height !== undefined) el.height.value = String(frame.height);
      syncResolutionPresetFromInputs();
      updatePreviewSizing();
      syncVisualFrameAspect();
    }

    const integrator = cfg.integrator && typeof cfg.integrator === "object" ? cfg.integrator : null;
    if (integrator) {
      integratorControlState.clear();
      const byInt = integrator.controls_by_integrator && typeof integrator.controls_by_integrator === "object"
        ? integrator.controls_by_integrator
        : {};
      Object.keys(byInt).forEach((key) => {
        const value = byInt[key];
        if (!key || !value || typeof value !== "object") return;
        integratorControlState.set(key, { ...value });
      });

      if (el.integrator && integrator.id && integratorById.has(String(integrator.id))) {
        el.integrator.value = String(integrator.id);
      }
      if (el.tileSize && integrator.tile_size !== undefined) el.tileSize.value = String(integrator.tile_size);
      if (el.tileOrder && integrator.tile_order !== undefined) el.tileOrder.value = String(integrator.tile_order);
      if (el.threads && integrator.threads !== undefined) el.threads.value = String(integrator.threads);
      renderIntegratorControls();
    }

    const tm = cfg.tone_mapping && typeof cfg.tone_mapping === "object" ? cfg.tone_mapping : null;
    if (tm) {
      if (el.toneMapping && tm.operator !== undefined) el.toneMapping.value = String(tm.operator);
      if (el.toneMappingExposure && tm.exposure !== undefined) el.toneMappingExposure.value = String(tm.exposure);
      if (el.toneMappingWhitePoint && tm.white_point !== undefined) el.toneMappingWhitePoint.value = String(tm.white_point);
      if (el.toneMappingMantiukContrast && tm.mantiuk_contrast !== undefined) el.toneMappingMantiukContrast.value = String(tm.mantiuk_contrast);
      if (el.toneMappingMantiukSaturation && tm.mantiuk_saturation !== undefined) el.toneMappingMantiukSaturation.value = String(tm.mantiuk_saturation);
      if (el.toneMappingMantiukDetail && tm.mantiuk_detail !== undefined) el.toneMappingMantiukDetail.value = String(tm.mantiuk_detail);
      updateToneMappingControlState();
    }

    if (Array.isArray(cfg.post_filters)) {
      postFilterChain = cfg.post_filters.map((entry) => ({
        filter: normalizePostFilterId(entry && entry.filter),
        stage: normalizePostFilterStage(entry && entry.stage),
      })).filter((entry) => !!entry.filter);
      renderPostFilterChain();
    }
  } finally {
    suppressWorkspaceSettingsSave = false;
  }
}

function parseWorkspaceSettingsFromSnapshot(workspace) {
  const raw = String((workspace && workspace.settings_json) || "").trim();
  if (!raw) {
    if (!workspace) return null;
    const hasLegacyQuality = workspace.quality_samples !== undefined
      || workspace.quality_aa !== undefined
      || workspace.quality_sample_distribution !== undefined
      || workspace.quality_rdepth !== undefined;
    if (!hasLegacyQuality) return null;
    return {
      quality: {
        samples: workspace.quality_samples !== undefined ? String(workspace.quality_samples) : undefined,
        aa: workspace.quality_aa !== undefined ? String(workspace.quality_aa) : undefined,
        sample_distribution: workspace.quality_sample_distribution !== undefined
          ? String(workspace.quality_sample_distribution)
          : undefined,
        rdepth: workspace.quality_rdepth !== undefined ? String(workspace.quality_rdepth) : undefined,
      },
    };
  }
  try {
    const parsed = JSON.parse(raw);
    return parsed && typeof parsed === "object" ? parsed : null;
  } catch (_) {
    return null;
  }
}

function hasSceneOption(scene) {
  const name = String(scene || "").trim();
  if (!name || !el.scene) return false;
  for (let i = 0; i < el.scene.options.length; ++i) {
    if (String(el.scene.options[i].value || "") === name) return true;
  }
  return false;
}

function cacheWorkspaceSnapshots(items) {
  workspaceSnapshotById.clear();
  const list = Array.isArray(items) ? items : [];
  list.forEach((ws) => {
    const id = String((ws && ws.id) || "").trim();
    if (!id) return;
    workspaceSnapshotById.set(id, ws);

    const runtime = workspaceRuntimeState(id);
    if (!runtime) return;
    runtime.activeJobId = String((ws && ws.active_job_id) || "").trim();
    runtime.lastCompletedJobId = String((ws && ws.last_job_id) || "").trim();
    const scene = String((ws && ws.active_scene) || "").trim();
    if (scene) runtime.lastCompletedJobScene = scene;
  });
}

async function applyActiveWorkspaceState(snapshot) {
  cancelActivePollingUi();

  const ws = snapshot || workspaceSnapshotById.get(activeWorkspaceId) || null;
  if (!ws) {
    await restorePreviewForActiveWorkspace();
    return;
  }

  const settings = parseWorkspaceSettingsFromSnapshot(ws);
  if (settings) applyWorkspaceSettings(settings);

  const wsScene = String((ws && ws.active_scene) || "").trim();
  if (wsScene && hasSceneOption(wsScene)) {
    const currentScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
    const sceneChanged = currentScene !== wsScene;
    if (sceneChanged) {
      el.scene.value = wsScene;
      localStorage.setItem(LAST_SCENE_KEY, wsScene);
      updateSceneDependencyPill(wsScene);
      await loadCameras(wsScene);
    }
    await loadSceneSource(wsScene);
    if (visualEditor && editorViewMode === "visual" && sceneChanged) {
      try {
        await loadVisualSceneFromSelected();
      } catch (err) {
        appendLog(`visual load error: ${err.message}`);
      }
    }
    if (editorViewMode === "graph") renderSceneGraphView();
  }

  await restorePreviewForActiveWorkspace();
  const wsActiveJobId = String((ws && ws.active_job_id) || "").trim();
  if (wsActiveJobId) {
    resumeWorkspaceJobPolling(wsActiveJobId);
  }
}

function renderWorkspaceList(items) {
  if (!el.workspaceList) return;
  el.workspaceList.innerHTML = "";
  const list = Array.isArray(items) ? items : [];
  const canDeleteAny = list.length > 1;
  if (list.length === 0) {
    const empty = document.createElement("p");
    empty.className = "workspace-empty";
    empty.textContent = "No workspaces available.";
    el.workspaceList.appendChild(empty);
    return;
  }

  list.forEach((ws) => {
    const id = String((ws && ws.id) || "");
    const scene = String((ws && ws.active_scene) || "").trim();
    const activeJob = String((ws && ws.active_job_id) || "").trim();
    const lastJob = String((ws && ws.last_job_id) || "").trim();
    const clients = Number((ws && ws.client_count) || 0);
    const drafts = Number((ws && ws.draft_count) || 0);
    const spatial = workspaceSpatialIndexStats && typeof workspaceSpatialIndexStats === "object"
      ? workspaceSpatialIndexStats
      : null;
    const spatialNodes = spatial && Number.isFinite(Number(spatial.tlas_nodes))
      ? String(Number(spatial.tlas_nodes))
      : "-";
    const spatialFinite = spatial && Number.isFinite(Number(spatial.finite_objects))
      ? String(Number(spatial.finite_objects))
      : "-";
    const spatialInfinite = spatial && Number.isFinite(Number(spatial.infinite_objects))
      ? String(Number(spatial.infinite_objects))
      : "-";
    const spatialBuildMs = spatial && Number.isFinite(Number(spatial.build_ms))
      ? `${Math.max(0, Number(spatial.build_ms)).toFixed(0)} ms`
      : "-";

    const card = document.createElement("article");
    card.className = "workspace-item";
    if (id && id === activeWorkspaceId) card.classList.add("is-active");
    const isRendering = !!activeJob;
    if (isRendering) card.classList.add("is-rendering");

    const head = document.createElement("header");
    head.className = "workspace-item-head";
    const title = document.createElement("h3");
    title.className = "workspace-item-title";
    title.textContent = String((ws && ws.name) || id || "Workspace");
    const state = document.createElement("span");
    state.className = "workspace-item-state";
    state.textContent = workspaceStateLabel(ws);
    head.appendChild(title);
    head.appendChild(state);

    const actions = document.createElement("div");
    actions.className = "workspace-item-actions";
    const useBtn = document.createElement("button");
    useBtn.type = "button";
    useBtn.textContent = id === activeWorkspaceId ? "Active" : "Use";
    useBtn.disabled = !id || id === activeWorkspaceId;
    useBtn.setAttribute("aria-disabled", useBtn.disabled ? "true" : "false");
    useBtn.addEventListener("click", () => {
      switchActiveWorkspace(id).catch((err) => {
        appendLog(`workspace switch error: ${err.message}`);
      });
    });
    actions.appendChild(useBtn);

    const deleteBtn = document.createElement("button");
    deleteBtn.type = "button";
    deleteBtn.textContent = "Delete";
    deleteBtn.disabled = !id || !canDeleteAny;
    deleteBtn.setAttribute("aria-disabled", deleteBtn.disabled ? "true" : "false");
    deleteBtn.addEventListener("click", () => {
      if (!id || !hasBackendMethod(api, "deleteWorkspace")) return;
      const ok = window.confirm(`Delete workspace ${id}?`);
      if (!ok) return;
      api.deleteWorkspace(id)
        .then(() => refreshWorkspaces())
        .then(() => appendLog(`deleted workspace: ${id}`))
        .catch((err) => appendLog(`workspace delete error: ${err.message}`));
    });
    actions.appendChild(deleteBtn);

    const previewWrap = document.createElement("div");
    previewWrap.className = "workspace-item-preview";
    // Avoid requesting the active job's final image while rendering; it often 404s
    // until completion and can show broken-image placeholders in the card.
    const previewJob = lastJob;
    const previewUrl = buildWorkspacePreviewUrl(previewJob);
    if (previewUrl) {
      const img = document.createElement("img");
      img.loading = "lazy";
      img.decoding = "async";
      img.alt = `${title.textContent} render preview`;
      img.onerror = () => {
        img.remove();
        if (!isRendering && !previewWrap.querySelector(".workspace-item-preview-empty")) {
          const emptyPreview = document.createElement("div");
          emptyPreview.className = "workspace-item-preview-empty";
          emptyPreview.textContent = "No render yet";
          previewWrap.appendChild(emptyPreview);
        }
      };
      img.src = previewUrl;
      previewWrap.appendChild(img);
    } else if (!isRendering) {
      const emptyPreview = document.createElement("div");
      emptyPreview.className = "workspace-item-preview-empty";
      emptyPreview.textContent = "No render yet";
      previewWrap.appendChild(emptyPreview);
    }
    if (isRendering) {
      const loading = document.createElement("div");
      loading.className = "workspace-item-preview-loading";
      loading.setAttribute("aria-label", "Rendering");
      loading.innerHTML = '<span class="workspace-item-preview-spinner" aria-hidden="true"></span>';
      previewWrap.appendChild(loading);
    }

    const meta = document.createElement("dl");
    meta.className = "workspace-item-meta";
    const addMeta = (label, value) => {
      const row = document.createElement("div");
      row.className = "workspace-item-meta-row";
      row.setAttribute("data-meta-key", String(label || "").toLowerCase());
      const dt = document.createElement("dt");
      dt.textContent = label;
      const dd = document.createElement("dd");
      dd.textContent = value;
      row.appendChild(dt);
      row.appendChild(dd);
      meta.appendChild(row);
    };
    addMeta("ID", id || "-");
    addMeta("Scene", scene || "-");
    addMeta("Clients", String(clients));
    addMeta("Drafts", String(drafts));
    addMeta("Job", activeJob || lastJob || "-");
    addMeta("TLAS Nodes", spatialNodes);
    addMeta("Finite/Infinite", `${spatialFinite}/${spatialInfinite}`);
    addMeta("TLAS Build", spatialBuildMs);
    addMeta("Updated", formatWorkspaceUpdated(ws && ws.updated_ms));

    if (workspaceViewMode === "list") {
      card.classList.add("workspace-item-list-compact");

      const previewCol = document.createElement("div");
      previewCol.className = "workspace-item-preview-col";
      const listState = document.createElement("span");
      listState.className = "workspace-item-list-state";
      listState.textContent = workspaceStateLabel(ws);
      previewCol.appendChild(previewWrap);
      previewCol.appendChild(listState);

      const main = document.createElement("div");
      main.className = "workspace-item-list-main";
      const sceneLine = document.createElement("p");
      sceneLine.className = "workspace-item-list-scene";
      sceneLine.textContent = scene || "-";

      const metaStrip = document.createElement("div");
      metaStrip.className = "workspace-item-meta-strip";
      const addChip = (label, value) => {
        const chip = document.createElement("span");
        chip.className = "workspace-item-meta-chip";
        chip.setAttribute("data-meta-key", String(label || "").toLowerCase());
        const key = document.createElement("span");
        key.className = "workspace-item-meta-chip-key";
        key.textContent = label;
        const val = document.createElement("span");
        val.className = "workspace-item-meta-chip-value";
        val.textContent = value;
        chip.appendChild(key);
        chip.appendChild(val);
        metaStrip.appendChild(chip);
      };
      addChip("ID", id || "-");
      addChip("Users", String(clients));
      addChip("Drafts", String(drafts));
      addChip("Job", activeJob || lastJob || "-");
      addChip("TLAS", spatialNodes);
      addChip("Obj", `${spatialFinite}/${spatialInfinite}`);
      addChip("Build", spatialBuildMs);
      addChip("Updated", formatWorkspaceUpdated(ws && ws.updated_ms));

      main.appendChild(head);
      main.appendChild(sceneLine);
      main.appendChild(metaStrip);

      card.appendChild(previewCol);
      card.appendChild(main);
      card.appendChild(actions);
      el.workspaceList.appendChild(card);
      return;
    }

    card.appendChild(head);
    card.appendChild(previewWrap);
    card.appendChild(meta);
    card.appendChild(actions);
    el.workspaceList.appendChild(card);
  });
}

async function refreshWorkspaces() {
  if (!hasBackendMethod(api, "getWorkspaces")) return;
  const payload = await api.getWorkspaces();
  workspaceSpatialIndexStats = payload && payload.spatial_index && typeof payload.spatial_index === "object"
    ? payload.spatial_index
    : null;
  const workspaceItems = (payload && payload.workspaces) || [];
  cacheWorkspaceSnapshots(workspaceItems);
  const nextActive = String((payload && payload.active_workspace) || "").trim();
  let activeChanged = false;
  if (nextActive && nextActive !== activeWorkspaceId) {
    syncGlobalsToWorkspaceRuntime();
    activeWorkspaceId = nextActive;
    syncWorkspaceRuntimeToGlobals();
    activeChanged = true;
  }
  updateWorkspaceActiveHint();
  updateWorkspaceCountHint(Array.isArray(workspaceItems) ? workspaceItems.length : 0);
  renderWorkspaceList(workspaceItems);
  if (activeChanged) {
    await applyActiveWorkspaceState(workspaceSnapshotById.get(activeWorkspaceId) || null);
  }
}

function queueWorkspaceDraftSave() {
  if (!hasBackendMethod(api, "saveWorkspaceSceneDraft")) return;
  if (workspaceDraftSaveTimer) {
    clearTimeout(workspaceDraftSaveTimer);
    workspaceDraftSaveTimer = null;
  }
  const scene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!scene || !is_scene_name_safe_runtime(scene)) return;
  workspaceDraftSaveTimer = setTimeout(() => {
    workspaceDraftSaveTimer = null;
    api.saveWorkspaceSceneDraft(scene, el.sceneSource ? (el.sceneSource.value || "") : "")
      .catch((err) => appendLog(`workspace draft save failed: ${err.message}`));
  }, 450);
}

function is_scene_name_safe_runtime(scene) {
  return /^[A-Za-z0-9_.-]+\.scn$/.test(String(scene || ""));
}

async function switchActiveWorkspace(workspaceId) {
  const nextId = String(workspaceId || "").trim();
  if (!nextId || nextId === activeWorkspaceId) return;
  if (!hasBackendMethod(api, "setActiveWorkspace")) return;
  await api.setActiveWorkspace(nextId);
  await refreshWorkspaces();
  appendLog(`workspace active=${nextId}`);
}

async function loadCameras(scene) {
  el.camera.innerHTML = "";
  addOption(el.camera, "", "Auto (first camera)");
  const info = await api.getCameras(scene);
  const cameras = (info && info.cameras) || [];
  const defaultCamera = (info && info.defaultCamera) || "";
  cameras.forEach((name) => addOption(el.camera, name, name));

  if (defaultCamera && cameras.includes(defaultCamera)) {
    el.camera.value = defaultCamera;
  } else {
    el.camera.value = "";
  }

  syncVisualCameraFromRenderSelection();
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
    refreshSceneEditControls();
    renderSceneGraphView();
    resetSceneHistoriesFromCurrentSource();
    return;
  }
  const data = await api.getSceneSource(scene);
  el.sceneName.value = data.scene || scene;
  el.sceneSource.value = data.source || "";
  updateEditorMetrics();
  refreshSceneEditControls();
  syncEditorScroll();
  renderSceneGraphView();
  resetSceneHistoriesFromCurrentSource();
}

async function loadSceneRuntimeGraph(scene) {
  const sceneName = String(scene || "").trim();
  if (!sceneName || !hasBackendMethod(api, "getSceneRuntimeGraph")) return null;
  const data = await api.getSceneRuntimeGraph(sceneName);
  runtimeGraphByScene.set(sceneName, data || { cameras: [], objects: [], surfaces: [], materials: [] });
  if (String(el.scene && el.scene.value ? el.scene.value : "").trim() === sceneName) {
    renderSceneGraphView();
  }
  return runtimeGraphByScene.get(sceneName) || null;
}

function renderThirdPartyLicenses(rawItems) {
  const items = Array.isArray(rawItems) && rawItems.length
    ? rawItems
    : DEFAULT_THIRD_PARTY_LICENSES;
  if (!el.aboutThirdPartyList) return;
  el.aboutThirdPartyList.replaceChildren();

  items.forEach((item) => {
    const name = item && item.name ? String(item.name) : "Unknown";
    const license = item && item.license ? String(item.license) : "Unknown";
    const url = item && item.url ? String(item.url) : "";

    const row = document.createElement("div");
    row.className = "about-third-party-row";

    const nameNode = document.createElement("span");
    nameNode.className = "about-third-party-name";
    nameNode.textContent = name;
    row.appendChild(nameNode);

    const licenseNode = document.createElement("code");
    licenseNode.className = "about-third-party-license";
    licenseNode.textContent = license;
    row.appendChild(licenseNode);

    if (url) {
      const link = document.createElement("a");
      link.className = "about-third-party-link";
      link.href = url;
      link.target = "_blank";
      link.rel = "noopener noreferrer";
      link.textContent = url;
      row.appendChild(link);
    }

    el.aboutThirdPartyList.appendChild(row);
  });
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
  updateWorkspaceServerStatsHints(data);
  renderThirdPartyLicenses(data.third_party_licenses);
}

async function loadEmptySceneTemplate() {
  if (hasBackendMethod(api, "getEmptySceneTemplate")) {
    return api.getEmptySceneTemplate();
  }
  const data = await getJSON("/api/scenes/template/empty");
  return data.source || "";
}

function sanitizeIntField(input, fallback, min, max) {
  const raw = String(input && input.value !== undefined ? input.value : "").trim();
  let value = Number.parseInt(raw, 10);
  if (!Number.isFinite(value)) value = fallback;
  value = Math.max(min, Math.min(max, value));
  if (input && String(input.value) !== String(value)) {
    input.value = String(value);
  }
  return String(value);
}

async function startRender() {
  const width = sanitizeIntField(el.width, 500, 32, 8192);
  const height = sanitizeIntField(el.height, 500, 32, 8192);
  const samples = sanitizeIntField(el.samples, 1, 1, 1024);
  const aa = sanitizeIntField(el.aa, 1, 1, 16);
  const rdepth = sanitizeIntField(el.rdepth, 3, 1, 4096);
  const tileSize = sanitizeIntField(el.tileSize, 32, 8, 1024);
  const threads = sanitizeIntField(el.threads, 0, 0, 256);

  return api.startRender({
    scene: el.scene.value,
    integrator: el.integrator.value,
    camera: el.camera.value || "",
    width,
    height,
    samples,
    aa,
    sample_distribution: el.sampleDistribution.value,
    rdepth,
    tile_size: tileSize,
    tile_order: el.tileOrder.value,
    threads,
    post_filters: gatherPostFilterParams(),
    ...gatherIntegratorOptionParams(),
  });
}

async function saveScene() {
  const name = (el.sceneName.value || "").trim();
  const source = el.sceneSource.value || "";
  if (!name) throw new Error("scene name is required");
  return api.saveScene(name, source, true);
}

function triggerSceneSave() {
  saveScene()
    .then((scene) => {
      if (!scene) return;
      loadScenes()
        .then(() => {
          el.scene.value = scene;
          localStorage.setItem(LAST_SCENE_KEY, scene);
          const tasks = [loadCameras(scene)];
          if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(scene));
          if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(scene));
          if (visualEditor) tasks.push(loadVisualSceneFromSelected());
          return Promise.all(tasks);
        })
        .then(() => {
          setStatus(`saved ${scene}`);
          setEditorOpStatus("success", `Saved: ${scene}`);
          appendLog(`saved scene: ${scene}`);
        })
        .catch((err) => {
          setStatus(`error: ${err.message}`);
          setEditorOpStatus("error", `Save failed: ${err.message}`);
          appendLog(`post-save error: ${err.message}`);
        });
    })
    .catch((err) => {
      setStatus(`error: ${err.message}`);
      setEditorOpStatus("error", `Save failed: ${err.message}`);
      appendLog(`save error: ${err.message}`);
    });
}

async function pollJob(jobId, token) {
  let lastState = "";
  resetProgressiveDeltaState(jobId);
  progressiveDeltaEnabled = true;
  while (true) {
    if (token !== undefined && token !== activePollToken) return;
    const data = await api.getJob(jobId);
    if (token !== undefined && token !== activePollToken) return;
    const state = data.state || "unknown";
    const progress = data.progress || 0;
    updateActivePreviewTilesFromJob(data);
    setProgress(progress);
    const stateLabel = state === "running" ? "rendering" : state;
    setStatus(`${stateLabel} ${(100 * progress).toFixed(1)}%`);
    applyPreviewTransform();

    if (state !== lastState) {
      appendLog(`job ${jobId} -> ${state}`);
      lastState = state;
    }

    let updatedByDelta = false;
    if (state === "queued" || state === "running") {
      try {
        updatedByDelta = await refreshProgressivePreviewDelta(jobId);
      } catch (_) {
        updatedByDelta = false;
      }
    }
    if (!updatedByDelta) {
      await refreshProgressivePreview(jobId);
      if (state === "queued" || state === "running") progressiveDeltaEnabled = false;
    }
    if (token !== undefined && token !== activePollToken) return;

    if (state === "done") {
      clearActivePreviewTiles();
      resetProgressiveDeltaState("");
      progressiveDeltaEnabled = true;
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
        recordPreviewTransfer("full", finalBlob.size || 0);
        await setPreviewFromBlob(finalBlob);
      }
      lastCompletedJobId = jobId;
      lastCompletedJobScene = String(data.scene || el.scene.value || "");
      lastCompletedJobIntegrator = String(data.integrator || el.integrator.value || "");
      syncGlobalsToWorkspaceRuntime();
      updateDownloadUi();
      refreshVisualPhotonOverlay().catch(() => {});
      setStatus(`done in ${Math.round(data.elapsed_ms || 0)} ms`);
      appendLog(`job ${jobId} finished in ${Math.round(data.elapsed_ms || 0)} ms`);
      return;
    }

    if (state === "error") {
      clearActivePreviewTiles();
      resetProgressiveDeltaState("");
      progressiveDeltaEnabled = true;
      applyPreviewTransform();
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
  setActiveTab("render");
  const pollToken = beginPollSession();
  el.renderBtn.disabled = true;
  lastCompletedJobId = "";
  lastCompletedJobScene = "";
  lastCompletedJobIntegrator = "";
  syncGlobalsToWorkspaceRuntime();
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
  clearActivePreviewTiles();
  applyPreviewTransform();
  setProgress(0);
  setStatus("submitting job...");
  appendLog(`submit render scene=${el.scene.value} integrator=${el.integrator.value} tile_order=${el.tileOrder.value}`);
  try {
    const jobId = await startRender();
    if (pollToken !== activePollToken) return;
    activeJobId = jobId;
    syncGlobalsToWorkspaceRuntime();
    appendLog(`job accepted: ${jobId}`);
    await pollJob(jobId, pollToken);
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
    if (!activeJobId || pollToken === activePollToken) {
      activeJobId = "";
      syncGlobalsToWorkspaceRuntime();
    }
    if (!activeJobId) {
      setRenderActive(false);
      el.renderBtn.disabled = false;
    } else {
      el.renderBtn.disabled = true;
    }
  }
}

async function boot() {
  ensureClientId();
  api = initializeBackendApi();
  renderSceneLoadStatus();
  const hasWorkspaceApi = hasBackendMethod(api, "getWorkspaces");
  const startupRequestTotal = 7 + (hasWorkspaceApi ? 1 : 0);
  resetStartupProgress(startupRequestTotal);
  const trackStartupRequest = (promise) => Promise.resolve(promise).finally(() => advanceStartupProgress(1));

  if (!hasBackendMethod(api, "getWorkspaces")) {
    if (el.tabWorkspaces) el.tabWorkspaces.hidden = true;
    if (el.paneWorkspaces) el.paneWorkspaces.hidden = true;
  }
  initSidebarAccordion();
  await trackStartupRequest(
    loadSidebarCardVisibilityConfig().catch((err) => {
      appendLog(`sidebar config error: ${err.message}`);
      // Keep booting: sidebar config is required but should not brick the UI.
    })
  );
  const savedTheme = localStorage.getItem("xtracer-theme") || "system";
  el.theme.value = savedTheme;
  applyTheme(savedTheme);
  loadUIOptions();
  applyHistoryLimits();
  setWorkspaceViewMode(localStorage.getItem(WORKSPACE_VIEW_MODE_KEY) || "cards", false);
  pollBackendLogs();

  setStatus("loading...");
  setSceneLoadStatus("loading", "Bootstrapping scene metadata...", "");
  appendLog(`boot (backend=${backendMode})`);
  setEditorViewMode(localStorage.getItem(EDITOR_VIEW_MODE_KEY) || "visual", false);
  if (hasWorkspaceApi) {
    await trackStartupRequest(refreshWorkspaces());
  }
  await Promise.all([
    trackStartupRequest(loadScenes()),
    trackStartupRequest(loadIntegrators()),
    trackStartupRequest(loadResolutionPresets()),
  ]);
  await trackStartupRequest(loadCameras(el.scene.value));
  await trackStartupRequest(loadSceneSource(el.scene.value));
  await trackStartupRequest(loadSceneRuntimeGraph(el.scene.value).catch(() => null));
  if (hasWorkspaceApi && activeWorkspaceId) {
    await applyActiveWorkspaceState(workspaceSnapshotById.get(activeWorkspaceId) || null);
  }
  await trackStartupRequest(loadAbout());
  updatePreviewSizing();
  bindPreviewInteraction();
  bindGraphInteraction();
  applyPreviewSampling();
  setPreviewEmptyState(true);
  setRenderActive(false);
  if (!el.scene.value) {
    setStatus("no scenes found in scene/ directory");
    setSceneLoadStatus("idle", "No scenes found in scene/ directory.", "");
  } else {
    setStatus("idle");
    setSceneLoadStatus("idle", "Ready.", "");
  }

  setActiveTab(localStorage.getItem(ACTIVE_TAB_KEY) || "scene");
  if (el.visualViewport && window.SceneVisualEditor) {
    visualEditor = new window.SceneVisualEditor(
      el.visualViewport,
      el.visualSelectionTag || null,
      async (sceneName) => {
        if (!hasBackendMethod(api, "getSceneGeometry")) throw new Error("geometry endpoint unavailable");
        return api.getSceneGeometry(sceneName);
      },
      async (sceneName, relpath) => {
        if (!hasBackendMethod(api, "getSceneAssetText")) throw new Error("asset endpoint unavailable");
        return api.getSceneAssetText(sceneName, relpath);
      }
    );
    if (visualEditor.init()) {
      if (visualEditor.setSelectionChangeHandler) {
        visualEditor.setSelectionChangeHandler((meta) => {
          const id = meta && meta.objectId ? String(meta.objectId) : "";
          if (el.editObjectSelect) {
            el.editObjectSelect.value = id;
            syncTransformInputsFromObject(id);
          }
        });
      }
      if (visualEditor.setObjectTransformChangeHandler) {
        visualEditor.setObjectTransformChangeHandler((evt) => {
          const objectId = evt && evt.objectId ? String(evt.objectId) : "";
          const transform = evt && evt.transform ? evt.transform : null;
          if (!objectId || !transform) return;
          try {
            const deltaTranslation = Array.isArray(evt && evt.deltaTranslation)
              ? evt.deltaTranslation
              : [0, 0, 0];
            const nextSource = updateObjectTransformInSource(
              el.sceneSource.value || "",
              objectId,
              transform,
              { useDelta: true, deltaTranslation }
            );
            updateSceneSourceText(nextSource);
            if (el.editObjectSelect) el.editObjectSelect.value = objectId;
            syncTransformInputsFromObject(objectId);
            appendLog(`scene edit moved: ${objectId}`);
          } catch (err) {
            appendLog(`scene edit move error: ${err.message}`);
          }
        });
      }
      syncVisualFrameAspect();
      if (el.visualProjection && visualEditor.setProjectionMode) {
        visualEditor.setProjectionMode(el.visualProjection.value || "perspective");
      }
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
  if (el.visualProjection) {
    el.visualProjection.addEventListener("change", () => {
      if (!visualEditor || !visualEditor.setProjectionMode) return;
      const mode = String(el.visualProjection.value || "perspective").toLowerCase();
      visualEditor.setProjectionMode(mode);
      appendLog(`visual projection=${mode}`);
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
    localStorage.setItem(LAST_SCENE_KEY, el.scene.value || "");
    updateSceneDependencyPill(el.scene.value);
    setSceneLoadStatus("loading", `Loading ${el.scene.value || "scene"}...`, "");
    const tasks = [loadCameras(el.scene.value)];
    if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(el.scene.value));
    if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(el.scene.value));
    Promise.all(tasks)
      .then(() => {
        if (visualEditor) {
          return loadVisualSceneFromSelected();
        }
        return null;
      })
      .then(() => appendLog(`scene changed: ${el.scene.value}`))
      .then(() => setSceneLoadStatus("idle", `Loaded ${el.scene.value || "scene"}.`, ""))
      .catch((err) => {
        setSceneLoadStatus("error", err.message || "Scene change failed.", "");
        setStatus(`error: ${err.message}`);
        appendLog(`scene change error: ${err.message}`);
      });
  });

  el.camera.addEventListener("change", () => {
    syncVisualCameraFromRenderSelection();
  });

  el.theme.addEventListener("change", () => {
    applyTheme(el.theme.value);
    refreshPaletteOptions();
    persistUIOptions();
    appendLog(`theme=${el.theme.value}`);
  });

  if (el.darkPalette) {
    el.darkPalette.addEventListener("change", () => {
      const mode = effectiveThemeMode(el.theme ? el.theme.value : "system");
      if (mode === "dark") applyDarkPalette(el.darkPalette.value);
      else applyLightPalette(el.darkPalette.value);
      refreshPaletteOptions();
      persistUIOptions();
      appendLog(`${mode} palette=${el.darkPalette.value}`);
    });
  }

  if (window.matchMedia) {
    const mq = window.matchMedia("(prefers-color-scheme: dark)");
    const onSchemeChange = () => {
      if (el.theme && el.theme.value === "system") {
        refreshPaletteOptions();
      }
    };
    if (typeof mq.addEventListener === "function") mq.addEventListener("change", onSchemeChange);
    else if (typeof mq.addListener === "function") mq.addListener(onSchemeChange);
  }

  el.integrator.addEventListener("change", () => {
    renderIntegratorControls();
    appendLog(`integrator=${el.integrator.value}`);
    refreshVisualPhotonOverlay().catch(() => {});
    queueWorkspaceSettingsSave();
  });

  el.tileOrder.addEventListener("change", () => {
    appendLog(`tile_order=${el.tileOrder.value}`);
    queueWorkspaceSettingsSave();
  });
  if (el.tileSize) {
    el.tileSize.addEventListener("change", () => {
      appendLog(`tile_size=${el.tileSize.value}`);
      queueWorkspaceSettingsSave();
    });
  }
  if (el.threads) {
    el.threads.addEventListener("change", () => {
      appendLog(`threads=${el.threads.value}`);
      queueWorkspaceSettingsSave();
    });
  }
  if (el.samples) {
    el.samples.addEventListener("input", () => {
      syncSamplesPresetUi();
    });
    el.samples.addEventListener("change", () => {
      syncSamplesPresetUi();
      appendLog(`samples=${el.samples.value}`);
      queueWorkspaceSettingsSave();
    });
  }
  if (el.samplesPills && el.samplesPills.length > 0) {
    el.samplesPills.forEach((btn) => {
      btn.addEventListener("click", () => {
        const value = String(btn.getAttribute("data-samples") || "");
        if (!el.samples || !value) return;
        el.samples.value = value;
        syncSamplesPresetUi();
        appendLog(`samples=${el.samples.value}`);
        queueWorkspaceSettingsSave();
      });
    });
  }
  if (el.rdepth) {
    el.rdepth.addEventListener("change", () => {
      appendLog(`rdepth=${el.rdepth.value}`);
      queueWorkspaceSettingsSave();
    });
  }
  if (el.sampleDistribution) {
    el.sampleDistribution.addEventListener("change", () => {
      appendLog(`sample_distribution=${el.sampleDistribution.value}`);
      queueWorkspaceSettingsSave();
    });
  }
  if (el.aaPills && el.aaPills.length > 0) {
    el.aaPills.forEach((btn) => {
      btn.addEventListener("click", () => {
        const value = String(btn.getAttribute("data-aa") || "");
        if (!el.aa || !value) return;
        el.aa.value = value;
        syncAaPresetUi();
        appendLog(`aa=${el.aa.value}`);
        queueWorkspaceSettingsSave();
      });
    });
  }
  if (el.aa) {
    el.aa.addEventListener("input", () => {
      syncAaPresetUi();
    });
    el.aa.addEventListener("change", () => {
      syncAaPresetUi();
      appendLog(`aa=${el.aa.value}`);
      queueWorkspaceSettingsSave();
    });
  }
  syncSamplesPresetUi();
  syncAaPresetUi();

  if (el.toneMapping) {
    el.toneMapping.addEventListener("change", () => {
      updateToneMappingControlState();
      appendLog(`tone_mapping=${el.toneMapping.value}`);
      refreshPreviewForToneMapping();
      queueWorkspaceSettingsSave();
    });
  }
  if (el.toneMappingExposure) {
    el.toneMappingExposure.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingExposure.addEventListener("change", () => {
      appendLog(`tone_mapping_exposure=${el.toneMappingExposure.value}`);
      refreshPreviewForToneMapping();
      queueWorkspaceSettingsSave();
    });
  }
  if (el.toneMappingWhitePoint) {
    el.toneMappingWhitePoint.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingWhitePoint.addEventListener("change", () => {
      appendLog(`tone_mapping_white_point=${el.toneMappingWhitePoint.value}`);
      refreshPreviewForToneMapping();
      queueWorkspaceSettingsSave();
    });
  }
  if (el.toneMappingMantiukContrast) {
    el.toneMappingMantiukContrast.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingMantiukContrast.addEventListener("change", () => {
      appendLog(`tone_mapping_mantiuk_contrast=${el.toneMappingMantiukContrast.value}`);
      refreshPreviewForToneMapping();
      queueWorkspaceSettingsSave();
    });
  }
  if (el.toneMappingMantiukSaturation) {
    el.toneMappingMantiukSaturation.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingMantiukSaturation.addEventListener("change", () => {
      appendLog(`tone_mapping_mantiuk_saturation=${el.toneMappingMantiukSaturation.value}`);
      refreshPreviewForToneMapping();
      queueWorkspaceSettingsSave();
    });
  }
  if (el.toneMappingMantiukDetail) {
    el.toneMappingMantiukDetail.addEventListener("input", () => {
      refreshPreviewForToneMapping();
    });
    el.toneMappingMantiukDetail.addEventListener("change", () => {
      appendLog(`tone_mapping_mantiuk_detail=${el.toneMappingMantiukDetail.value}`);
      refreshPreviewForToneMapping();
      queueWorkspaceSettingsSave();
    });
  }
  updateToneMappingControlState();
  populatePostFilterTypeOptions();
  renderPostFilterChain();
  if (el.postFilterAddBtn) {
    el.postFilterAddBtn.addEventListener("click", () => {
      addPostFilterToChain();
    });
  }

  const onSizeChanged = () => {
    syncResolutionPresetFromInputs();
    updatePreviewSizing();
    syncVisualFrameAspect();
    queueWorkspaceSettingsSave();
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
    queueWorkspaceSettingsSave();
  });
  el.width.addEventListener("input", onSizeChanged);
  el.width.addEventListener("change", onSizeChanged);
  el.height.addEventListener("input", onSizeChanged);
  el.height.addEventListener("change", onSizeChanged);
  window.addEventListener("resize", () => {
    applyPreviewTransform();
    if (visualEditor) visualEditor.resize();
    if (editorViewMode === "graph") {
      if (!graphView.userAdjusted) fitGraphToViewport();
      applyGraphTransform();
    }
  });

  el.pollInterval.addEventListener("change", () => {
    const v = parseInt(el.pollInterval.value || "300", 10);
    uiOptions.pollMs = Number.isFinite(v) ? Math.max(100, Math.min(10000, v)) : 300;
    el.pollInterval.value = String(uiOptions.pollMs);
    persistUIOptions();
    appendLog(`render poll interval=${uiOptions.pollMs}ms`);
  });

  if (el.textHistorySize) {
    el.textHistorySize.addEventListener("change", () => {
      uiOptions.textHistoryLimit = clampHistoryLimit(el.textHistorySize.value || "200");
      el.textHistorySize.value = String(uiOptions.textHistoryLimit);
      applyHistoryLimits();
      persistUIOptions();
      appendLog(`text history size=${uiOptions.textHistoryLimit}`);
    });
  }

  if (el.visualHistorySize) {
    el.visualHistorySize.addEventListener("change", () => {
      uiOptions.visualHistoryLimit = clampHistoryLimit(el.visualHistorySize.value || "200");
      el.visualHistorySize.value = String(uiOptions.visualHistoryLimit);
      applyHistoryLimits();
      persistUIOptions();
      appendLog(`3d history size=${uiOptions.visualHistoryLimit}`);
    });
  }

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

  if (el.fontSizePreset) {
    el.fontSizePreset.addEventListener("change", () => {
      uiOptions.fontSizePreset = normalizeFontSizePreset(el.fontSizePreset.value);
      applyFontScale(scaleForFontSizePreset(uiOptions.fontSizePreset));
      persistUIOptions();
      appendLog(`ui font size=${uiOptions.fontSizePreset}`);
    });
  }

  if (el.tabScene) el.tabScene.addEventListener("click", () => setActiveTab("scene"));
  el.tabRender.addEventListener("click", () => setActiveTab("render"));
  el.tabVisual.addEventListener("click", () => setActiveTab("visual"));
  if (el.tabWorkspaces) el.tabWorkspaces.addEventListener("click", () => setActiveTab("workspaces"));
  el.tabSettings.addEventListener("click", () => setActiveTab("settings"));
  el.tabLogs.addEventListener("click", () => setActiveTab("logs"));
  if (el.editorView3dBtn) {
    el.editorView3dBtn.addEventListener("click", () => setEditorViewMode("visual"));
  }
  if (el.editorViewGraphBtn) {
    el.editorViewGraphBtn.addEventListener("click", () => setEditorViewMode("graph"));
  }
  if (el.graphResetLayoutBtn) {
    el.graphResetLayoutBtn.addEventListener("click", () => {
      graphView.manualNodePos.clear();
      graphView.userAdjusted = false;
      renderSceneGraphView();
      appendLog("graph layout reset");
    });
  }
  if (el.editorViewTextBtn) {
    el.editorViewTextBtn.addEventListener("click", () => setEditorViewMode("text"));
  }
  document.addEventListener("keydown", handleUndoRedoShortcut);

  if (el.workspaceRefreshBtn) {
    el.workspaceRefreshBtn.addEventListener("click", () => {
      refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
    });
  }
  if (el.workspaceViewMode) {
    el.workspaceViewMode.addEventListener("change", () => {
      setWorkspaceViewMode(el.workspaceViewMode.value || "cards");
      refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
    });
  }
  if (el.workspaceViewCardsBtn) {
    el.workspaceViewCardsBtn.addEventListener("click", () => {
      setWorkspaceViewMode("cards");
      refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
    });
  }
  if (el.workspaceViewListBtn) {
    el.workspaceViewListBtn.addEventListener("click", () => {
      setWorkspaceViewMode("list");
      refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
    });
  }
  if (el.workspaceCreateBtn) {
    el.workspaceCreateBtn.addEventListener("click", () => {
      if (!hasBackendMethod(api, "createWorkspace")) return;
      const rawName = el.workspaceCreateName ? String(el.workspaceCreateName.value || "").trim() : "";
      api.createWorkspace(rawName)
        .then((data) => {
          const id = String((data && data.id) || "").trim();
          if (el.workspaceCreateName) el.workspaceCreateName.value = "";
          if (!id) return refreshWorkspaces();
          return switchActiveWorkspace(id);
        })
        .catch((err) => appendLog(`workspace create error: ${err.message}`));
    });
  }
  if (el.sceneRefreshBtn) {
    el.sceneRefreshBtn.addEventListener("click", () => {
      setSceneLoadStatus("loading", "Refreshing scene list...", "");
      loadScenes()
        .then(() => {
          const tasks = [loadCameras(el.scene.value)];
          if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(el.scene.value));
          if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(el.scene.value));
          return Promise.all(tasks);
        })
        .then(() => {
          if (visualEditor) {
            return loadVisualSceneFromSelected();
          }
          return null;
        })
        .then(() => {
          setStatus(`scenes refreshed (${el.scene.value || "none"})`);
          setSceneLoadStatus("idle", `Scenes refreshed (${el.scene.value || "none"}).`, "");
          appendLog("scene list refreshed");
        })
        .catch((err) => {
          setSceneLoadStatus("error", err.message || "Scene refresh failed.", "");
          setStatus(`error: ${err.message}`);
          appendLog(`scene refresh error: ${err.message}`);
        });
    });
  }
  el.loadSceneBtn.addEventListener("click", () => {
    setSceneLoadStatus("loading", `Loading source for ${el.scene.value || "scene"}...`, "");
    const tasks = [loadSceneSource(el.scene.value)];
    if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(el.scene.value));
    Promise.all(tasks)
      .then(() => {
        setStatus(`loaded ${el.scene.value}`);
        setSceneLoadStatus("idle", `Loaded ${el.scene.value || "scene"}.`, "");
        setEditorOpStatus("success", `Loaded: ${el.scene.value}`);
        appendLog(`loaded source: ${el.scene.value}`);
      })
      .catch((err) => {
        setSceneLoadStatus("error", err.message || "Load source failed.", "");
        setStatus(`error: ${err.message}`);
        setEditorOpStatus("error", `Load failed: ${err.message}`);
        appendLog(`load source error: ${err.message}`);
      });
  });

  el.newSceneBtn.addEventListener("click", () => {
    loadEmptySceneTemplate()
      .then((source) => {
        el.sceneName.value = "new_scene.scn";
        el.sceneSource.value = source || "";
        updateEditorMetrics();
        refreshSceneEditControls();
        syncEditorScroll();
        renderSceneGraphView();
        resetSceneHistoriesFromCurrentSource();
        setStatus("new scene initialized");
        setEditorOpStatus("info", "New scene template initialized");
        appendLog("new scene template");
      })
      .catch((err) => {
        setStatus(`error: ${err.message}`);
        setEditorOpStatus("error", `New scene failed: ${err.message}`);
        appendLog(`new scene template error: ${err.message}`);
      });
  });

  el.saveSceneBtn.addEventListener("click", triggerSceneSave);

  if (el.editObjectSelect) {
    el.editObjectSelect.addEventListener("change", () => {
      const objectId = String(el.editObjectSelect.value || "").trim();
      syncTransformInputsFromObject(objectId);
      if (visualEditor && visualEditor.selectObjectById) visualEditor.selectObjectById(objectId, false);
    });
  }

  if (el.editSyncFromVisualBtn) {
    el.editSyncFromVisualBtn.addEventListener("click", () => {
      if (!visualEditor || !visualEditor.getSelectedObjectId) return;
      const objectId = String(visualEditor.getSelectedObjectId() || "").trim();
      if (!objectId) {
        setStatus("error: no visual selection");
        appendLog("scene edit: no visual selection");
        return;
      }
      if (el.editObjectSelect) el.editObjectSelect.value = objectId;
      syncTransformInputsFromObject(objectId);
      appendLog(`scene edit selection=${objectId}`);
    });
  }

  if (el.editApplyTransformBtn) {
    el.editApplyTransformBtn.addEventListener("click", () => {
      const objectId = String(el.editObjectSelect && el.editObjectSelect.value ? el.editObjectSelect.value : "").trim();
      if (!objectId) {
        setStatus("error: select an object first");
        appendLog("scene edit: apply transform failed (no object)");
        return;
      }
      try {
        const nextSource = updateObjectTransformInSource(el.sceneSource.value || "", objectId, currentTransformInputs());
        updateSceneSourceText(nextSource);
        rebuildVisualFromEditorSource()
          .then(() => {
            if (visualEditor && visualEditor.selectObjectById) visualEditor.selectObjectById(objectId, false);
          })
          .catch((err) => appendLog(`visual refresh error: ${err.message}`));
        setStatus(`updated ${objectId}`);
        appendLog(`scene edit transform updated: ${objectId}`);
      } catch (err) {
        setStatus(`error: ${err.message}`);
        appendLog(`scene edit transform error: ${err.message}`);
      }
    });
  }

  if (el.createGeometryBtn) {
    el.createGeometryBtn.addEventListener("click", () => {
      try {
        const added = addMeshObjectToSceneSource(el.sceneSource.value || "", {
          generator: String(el.createGeometryType && el.createGeometryType.value ? el.createGeometryType.value : "cube").toLowerCase(),
          material: String(el.createMaterialSelect && el.createMaterialSelect.value ? el.createMaterialSelect.value : "").trim(),
          geometryId: String(el.createGeometryId && el.createGeometryId.value ? el.createGeometryId.value : "").trim(),
          objectId: String(el.createObjectId && el.createObjectId.value ? el.createObjectId.value : "").trim(),
          ...currentTransformInputs(),
        });
        updateSceneSourceText(added.source);
        if (el.editObjectSelect) el.editObjectSelect.value = added.objectId;
        syncTransformInputsFromObject(added.objectId);
        rebuildVisualFromEditorSource()
          .then(() => {
            if (visualEditor && visualEditor.selectObjectById) visualEditor.selectObjectById(added.objectId, true);
          })
          .catch((err) => appendLog(`visual refresh error: ${err.message}`));
        setStatus(`created ${added.objectId}`);
        appendLog(`scene edit created: object=${added.objectId} geometry=${added.geometryId}`);
      } catch (err) {
        setStatus(`error: ${err.message}`);
        appendLog(`scene edit create error: ${err.message}`);
      }
    });
  }

  el.clearLogsBtn.addEventListener("click", () => {
    logEntries.length = 0;
    renderLogOutput();
  });

  const bindLogFilter = (node, key) => {
    if (!node) return;
    node.checked = !!logFilters[key];
    node.addEventListener("change", () => {
      logFilters[key] = !!node.checked;
      persistUIOptions();
      renderLogOutput();
    });
  };
  bindLogFilter(el.logFilterDebug, "debug");
  bindLogFilter(el.logFilterMessage, "message");
  bindLogFilter(el.logFilterWarning, "warning");
  bindLogFilter(el.logFilterError, "error");

  el.sceneSource.addEventListener("input", () => {
    updateEditorMetrics();
    refreshSceneEditControls();
    renderSceneGraphView();
    if (!suppressHistoryTracking) {
      scheduleTextHistoryCommit();
    }
    queueWorkspaceDraftSave();
  });
  el.sceneSource.addEventListener("keydown", handleEditorTabKey);
  el.sceneSource.addEventListener("scroll", syncEditorScroll);
  el.sceneSource.addEventListener("keyup", syncEditorScroll);
  el.sceneSource.addEventListener("click", syncEditorScroll);
  updateEditorMetrics();
  refreshSceneEditControls();
  syncEditorScroll();
}

setTimeout(() => {
  // Safety valve: avoid a permanent loading overlay on unexpected stalls.
  dismissStartupScreen(false);
}, 15000);

boot()
  .then(() => {
    dismissStartupScreen(false);
  })
  .catch((err) => {
    setStatus(`error: ${err.message}`);
    appendLog(`boot error: ${err.message}`);
    dismissStartupScreen(false);
  });
