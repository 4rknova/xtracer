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
  tileHeatmapEnabled: true,
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

