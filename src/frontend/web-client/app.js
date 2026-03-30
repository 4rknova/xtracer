const $ = (id) => document.getElementById(id);

const el = {
  startupScreen: $("startupScreen"),
  startupLabel: $("startupLabel"),
  startupProgressFill: $("startupProgressFill"),
  mainMenuToggle: $("mainMenuToggle"),
  mainTabs: $("mainTabs"),
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
  logPollActiveInterval: $("logPollActiveInterval"),
  logPollBackgroundInterval: $("logPollBackgroundInterval"),
  textHistorySize: $("textHistorySize"),
  visualHistorySize: $("visualHistorySize"),
  autoLoadEditor: $("autoLoadEditor"),
  ftueShowOnNextLaunch: $("ftueShowOnNextLaunch"),
  ftueStartNowBtn: $("ftueStartNowBtn"),
  autoScrollLogs: $("autoScrollLogs"),
  fontSizePreset: $("fontSizePreset"),
  clearLogsBtn: $("clearLogsBtn"),
  logFilterDebug: $("logFilterDebug"),
  logFilterMessage: $("logFilterMessage"),
  logFilterWarning: $("logFilterWarning"),
  logFilterError: $("logFilterError"),
  logFilterUi: $("logFilterUi"),
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
  workspaceSortNameBtn: $("workspaceSortNameBtn"),
  workspaceSortUpdatedBtn: $("workspaceSortUpdatedBtn"),
  workspaceSortSceneBtn: $("workspaceSortSceneBtn"),
  workspaceActiveHint: $("workspaceActiveHint"),
  workspaceCountHint: $("workspaceCountHint"),
  workspaceMaxConcurrentHint: $("workspaceMaxConcurrentHint"),
  workspaceThreadsHint: $("workspaceThreadsHint"),
  workspaceOpenmpHint: $("workspaceOpenmpHint"),
  workspaceRenderReserveHint: $("workspaceRenderReserveHint"),
  workspaceRenderAutoHint: $("workspaceRenderAutoHint"),
  settingsJobsUpdated: $("settingsJobsUpdated"),
  settingsJobsThreadsUsage: $("settingsJobsThreadsUsage"),
  settingsJobsList: $("settingsJobsList"),
  activeSceneCardScene: $("activeSceneCardScene"),
  activeSceneCardDescription: $("activeSceneCardDescription"),
  activeSceneCardCamera: $("activeSceneCardCamera"),
  activeSceneCardVariant: $("activeSceneCardVariant"),
  workspaceList: $("workspaceList"),
  sceneRefreshBtn: $("sceneRefreshBtn"),
  sceneActiveFile: $("sceneActiveFile"),
  sceneFileCount: $("sceneFileCount"),
  sceneSearch: $("sceneSearch"),
  sceneFileList: $("sceneFileList"),
  cameraActiveName: $("cameraActiveName"),
  cameraFileCount: $("cameraFileCount"),
  cameraFileList: $("cameraFileList"),
  variantActiveName: $("variantActiveName"),
  variantActiveDescription: $("variantActiveDescription"),
  variantFileCount: $("variantFileCount"),
  variantFileList: $("variantFileList"),
  sceneDependencyPill: $("sceneDependencyPill"),
  scene: $("scene"),
  camera: $("camera"),
  variant: $("variant"),
  integrator: $("integrator"),
  integratorControlsSection: $("integratorControlsSection"),
  integratorControls: $("integratorControls"),
  resolutionPreset: $("resolutionPreset"),
  resolutionPresetList: $("resolutionPresetList"),
  resolutionModeFilterAll: $("resolutionModeFilterAll"),
  resolutionModeFilterLandscape: $("resolutionModeFilterLandscape"),
  resolutionModeFilterPortrait: $("resolutionModeFilterPortrait"),
  resolutionModeFilterSquare: $("resolutionModeFilterSquare"),
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
  renderMode: $("renderMode"),
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
  postFiltersRecalcBtn: $("postFiltersRecalcBtn"),
  postFiltersEnabled: $("postFiltersEnabled"),
  postFiltersChain: $("postFiltersChain"),
  clearPreviewOnRender: $("clearPreviewOnRender"),
  renderBtn: $("renderBtn"),
  previewHeadline: $("previewHeadline"),
  status: $("status"),
  statusThreads: $("statusThreads"),
  statusPass: $("statusPass"),
  statusPercent: $("statusPercent"),
  sceneLoadState: $("sceneLoadState"),
  sceneLoadMessage: $("sceneLoadMessage"),
  sceneLoadJobId: $("sceneLoadJobId"),
  sceneLoadElapsed: $("sceneLoadElapsed"),
  previewTransferStats: $("previewTransferStats"),
  statsFrameRender: $("statsFrameRender"),
  statsDeltaBytes: $("statsDeltaBytes"),
  statsDeltaReqs: $("statsDeltaReqs"),
  statsFullBytes: $("statsFullBytes"),
  statsFullReqs: $("statsFullReqs"),
  tileHeatmapEnabled: $("tileHeatmapEnabled"),
  tileHeatmapBuckets: $("tileHeatmapBuckets"),
  tileHeatmapThroughput: $("tileHeatmapThroughput"),
  tileHeatmapEta: $("tileHeatmapEta"),
  tileHeatmapConfidence: $("tileHeatmapConfidence"),
  tileHeatmapBottleneck: $("tileHeatmapBottleneck"),
  renderTimer: $("renderTimer"),
  progressBar: $("progressBar"),
  progress: $("progress"),
  previewFrame: $("previewFrame"),
  previewEmpty: $("previewEmpty"),
  interactivePreviewHud: $("interactivePreviewHud"),
  interactivePreviewHudMode: $("interactivePreviewHudMode"),
  interactivePreviewHudSpeed: $("interactivePreviewHudSpeed"),
  interactivePreviewHudQuality: $("interactivePreviewHudQuality"),
  previewCanvas: $("previewCanvas"),
  preview: $("preview"),
  postPipelineGraph: $("postPipelineGraph"),
  postPipelineSummary: $("postPipelineSummary"),
  resetViewBtn: $("resetViewBtn"),
  interactivePreviewControls: $("interactivePreviewControls"),
  interactivePreviewSpeed: $("interactivePreviewSpeed"),
  interactivePreviewSpeedValue: $("interactivePreviewSpeedValue"),
  interactivePreviewSaveCameraBtn: $("interactivePreviewSaveCameraBtn"),
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
  ftueOverlay: $("ftueOverlay"),
  ftueDim: $("ftueDim"),
  ftueSpotlight: $("ftueSpotlight"),
  ftueDialog: $("ftueDialog"),
  ftueStepLabel: $("ftueStepLabel"),
  ftueTitle: $("ftueTitle"),
  ftueBody: $("ftueBody"),
  ftueBackBtn: $("ftueBackBtn"),
  ftueNextBtn: $("ftueNextBtn"),
  ftueSkipBtn: $("ftueSkipBtn"),
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
  logPollActiveMs: 3000,
  logPollBackgroundMs: 20000,
  textHistoryLimit: 200,
  visualHistoryLimit: 200,
  autoLoadEditor: true,
  autoScrollLogs: true,
  clearPreviewOnRender: false,
  tileHeatmapEnabled: true,
  previewSampling: "smooth",
  fontSizePreset: "default",
  fontScale: 1.0,
  darkPalette: "slate",
  lightPalette: "coastal",
};
let resolutionPresets = [];
let resolutionPresetModeFilter = "all";
let sceneDependencyByFile = new Map();
let sceneCatalog = [];
let sceneBrowserSelectedFile = "";
let cameraCatalog = [];
let cameraBrowserSelectedName = "";
let variantCatalog = [];
let variantBrowserSelectedName = "";
let lastBackendLogId = 0;
let backendLogWaitAbortController = null;
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
  ui: true,
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
  fullFrameRenderMs: 0,
  deltaReqs: 0,
  deltaBytes: 0,
  fullReqs: 0,
  fullBytes: 0,
};
const tileHeatmapState = {
  jobId: "",
  tiles: new Map(),
  progressSamples: [],
  throughputTilesPerSec: 0,
  etaMs: 0,
  etaConfidence: "low",
  bottleneckHint: "-",
  totalTilesEstimate: 0,
  buckets: { fast: 0, medium: 0, slow: 0 },
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
let postFilterStackEnabled = true;
let integratorCatalog = [];
let integratorById = new Map();
const integratorControlState = new Map();
const BACKEND_MODE_KEY = "xtracer-backend-mode";
const ACTIVE_TAB_KEY = "xtracer-active-tab";
const EDITOR_VIEW_MODE_KEY = "xtracer-editor-view-mode";
const WORKSPACE_VIEW_MODE_KEY = "xtracer-workspace-view-mode";
const WORKSPACE_SORT_MODE_KEY = "xtracer-workspace-sort-mode";
const LAST_SCENE_KEY = "xtracer-last-scene";
const LOG_FILTERS_KEY = "xtracer-log-filters";
const CLIENT_ID_KEY = "xtracer-client-id";
const FTUE_STATE_VERSION_KEY = "xtracer-ftue-version";
const FTUE_FORCE_NEXT_KEY = "xtracer-ftue-force-next";
const FTUE_VERSION = 1;
const SIDEBAR_VISIBILITY_CONFIG_URL = "/app/data/sidebar_cards.json";
const APP_CONFIG_URL = "/app/data/config.json";
const TAB_MODES = ["scene", "render", "visual", "workspaces", "logs", "settings"];
let sidebarCardVisibility = null;
let sidebarCardVisibilityRaw = "";
let api = null;
let backendMode = "server";
let clientId = "";
let activeWorkspaceId = "";
let workspaceViewMode = "cards";
let workspaceSortMode = "name";
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
  touchPoints: {},
  pinchActive: false,
  pinchLastCenterX: 0,
  pinchLastCenterY: 0,
  pinchLastDistance: 0,
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
const RENDER_MODE_NORMAL = "normal";
const RENDER_MODE_PROGRESSIVE = "progressive";
const RENDER_MODE_INTERACTIVE = "interactive";
let renderMode = RENDER_MODE_NORMAL;
let interactivePreviewEnabled = false;
let interactivePreviewLoopToken = 0;
let interactivePreviewLoopActive = false;
let interactivePreviewJobId = "";
let interactivePreviewDirty = false;
let interactivePreviewCameraSeq = 0;
let interactivePreviewLastInputMs = 0;
const INTERACTIVE_PREVIEW_SETTLE_MS = 420;
const INTERACTIVE_PREVIEW_ACTIVE_POLL_MS = 90;
const INTERACTIVE_PREVIEW_FLY_SPEED = 2.5;
const INTERACTIVE_PREVIEW_FLY_SHIFT_MULTIPLIER = 3.0;
const INTERACTIVE_PREVIEW_TARGET_FRAME_MS = 110;
let interactivePreviewAdaptiveMovingWidth = 64;
let interactivePreviewFlySpeedScale = 1.0;
let interactivePreviewHudMode = "LOOK";
let interactivePreviewHudQuality = "idle";
let interactivePreviewActiveMovingJob = false;
let interactivePreviewFlyTimer = 0;
let interactivePreviewFlyLastTickMs = 0;
const interactivePreviewKeyState = {
  w: false,
  a: false,
  s: false,
  d: false,
  q: false,
  e: false,
  shift: false,
};
const interactivePreviewCamera = {
  ready: false,
  type: "",
  sourceScene: "",
  sourceVariant: "",
  sourceCamera: "",
  position: [0, 0, 0],
  target: [0, 0, -1],
  up: [0, 1, 0],
  hfov: 60,
  pivot: [0, 0, 0],
  orbitYaw: 0,
  orbitPitch: 0,
  orbitDistance: 1,
};

function isSafeClientId(value) {
  return /^[A-Za-z0-9_.-]{1,96}$/.test(String(value || ""));
}

function fallbackClientId() {
  if (window.crypto && typeof window.crypto.randomUUID === "function") {
    return `client_${window.crypto.randomUUID()}`;
  }
  const ts = Date.now().toString(36);
  const rnd = Math.random().toString(36).slice(2, 14);
  return `client_${ts}_${rnd}`;
}

function getOrCreateClientIdForRequests() {
  const fromGlobal = String(clientId || "").trim();
  const persistCookie = (value) => {
    const id = String(value || "").trim();
    if (!isSafeClientId(id)) return;
    document.cookie = `client_id=${id}; Path=/; Max-Age=31536000; SameSite=Lax`;
  };
  if (isSafeClientId(fromGlobal)) {
    persistCookie(fromGlobal);
    return fromGlobal;
  }
  try {
    const stored = String(localStorage.getItem(CLIENT_ID_KEY) || "").trim();
    if (isSafeClientId(stored)) {
      clientId = stored;
      persistCookie(stored);
      return stored;
    }
    const created = fallbackClientId();
    localStorage.setItem(CLIENT_ID_KEY, created);
    clientId = created;
    persistCookie(created);
    return created;
  } catch (_) {
    return "";
  }
}

function withClientIdQuery(rawUrl, cid) {
  const client = String(cid || "").trim();
  if (!client) return rawUrl;
  try {
    const absolute = new URL(String(rawUrl), window.location.href);
    if (absolute.origin !== window.location.origin) return rawUrl;
    if (absolute.protocol !== "http:" && absolute.protocol !== "https:") return rawUrl;
    if (absolute.searchParams.has("client_id")) return rawUrl;
    absolute.searchParams.set("client_id", client);
    return absolute.toString();
  } catch (_) {
    return rawUrl;
  }
}

if (window && typeof window.fetch === "function") {
  const originalFetch = window.fetch.bind(window);
  window.fetch = function fetchWithClientId(input, init) {
    const cid = getOrCreateClientIdForRequests();
    if (!cid) return originalFetch(input, init);

    if (typeof input === "string" || input instanceof URL) {
      const nextUrl = withClientIdQuery(input, cid);
      return originalFetch(nextUrl, init);
    }

    if (input instanceof Request) {
      const nextUrl = withClientIdQuery(input.url, cid);
      if (nextUrl === input.url) return originalFetch(input, init);
      const nextReq = new Request(nextUrl, input);
      return originalFetch(nextReq, init);
    }

    return originalFetch(input, init);
  };
}
