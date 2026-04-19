if (window.XTracerWidgets && typeof window.XTracerWidgets.renderMainTabs === "function") {
  window.XTracerWidgets.renderMainTabs(document.getElementById("mainTabs"));
}
if (window.XTracerWidgets && typeof window.XTracerWidgets.upgradePanelHeaders === "function") {
  window.XTracerWidgets.upgradePanelHeaders(document);
}
if (window.XTracerSidebarCards && typeof window.XTracerSidebarCards.renderSidebarCards === "function") {
  window.XTracerSidebarCards.renderSidebarCards(document.getElementById("sidebarCards"));
}
if (window.XTracerWidgets && typeof window.XTracerWidgets.createTabContainer === "function") {
  const aboutPanel = document.querySelector("#paneAbout .about-panel");
  const overviewGrid = aboutPanel && aboutPanel.querySelector(".about-grid");
  const devtoolsCard = aboutPanel && aboutPanel.querySelector(".about-devtools-card");
  const licenseCard = aboutPanel && aboutPanel.querySelector(".about-license-card");
  const thirdPartyCard = aboutPanel && aboutPanel.querySelector(".about-third-party-card");
  if (aboutPanel && overviewGrid && licenseCard && thirdPartyCard) {
    if (devtoolsCard) overviewGrid.appendChild(devtoolsCard);
    const tabs = window.XTracerWidgets.createTabContainer({
      className: "about-tabs",
      tabs: [
        { id: "overview", label: "Overview", content: overviewGrid },
        { id: "license", label: "License", content: licenseCard },
        { id: "third-party", label: "Third-Party", content: thirdPartyCard },
      ],
    });
    aboutPanel.replaceChildren(tabs);
  }
}
(function populateClientCard() {
  const browserEl = document.getElementById("aboutClientBrowser");
  const themeEl = document.getElementById("aboutClientTheme");
  const clientIdEl = document.getElementById("aboutClientId");

  if (browserEl) {
    const ua = navigator.userAgent;
    let browser = "Unknown";
    if (ua.indexOf("Edg/") !== -1) browser = "Edge";
    else if (ua.indexOf("OPR/") !== -1 || ua.indexOf("Opera") !== -1) browser = "Opera";
    else if (ua.indexOf("Firefox") !== -1) browser = "Firefox";
    else if (ua.indexOf("Chrome") !== -1) browser = "Chrome";
    else if (ua.indexOf("Safari") !== -1) browser = "Safari";
    const platform = (navigator.userAgentData && navigator.userAgentData.platform)
      || navigator.platform || "";
    browserEl.textContent = platform ? `${browser} · ${platform}` : browser;
  }

  if (themeEl) {
    const root = document.documentElement;
    const theme = root.getAttribute("data-theme") || "system";
    const darkPalette = root.getAttribute("data-dark-palette") || "";
    const lightPalette = root.getAttribute("data-light-palette") || "";
    const isDark = theme === "dark" || (theme === "system" && window.matchMedia("(prefers-color-scheme: dark)").matches);
    const palette = isDark ? darkPalette : lightPalette;
    themeEl.textContent = palette ? `${theme} · ${palette}` : theme;
  }

  if (clientIdEl) {
    try {
      const id = localStorage.getItem("xtracer-client-id") || "";
      clientIdEl.textContent = id ? (id.length > 30 ? id.slice(0, 28) + "\u2026" : id) : "-";
    } catch (_) {
      clientIdEl.textContent = "-";
    }
  }
})();

function createRenderToolbarSvgIcon(pathData, viewBox) {
  return window.XTracerWidgets.dom.svgIcon(pathData, viewBox || "0 0 24 24");
}

function decorateRenderExportButton(button) {
  if (!button) return;
  button.classList.remove("reset-view-btn", "xui-icon-button", "xui-icon-button--ghost", "xui-button", "xui-button--secondary");
  button.classList.add("render-toolbar-save-btn");
  if (!button.title || /^export\b/i.test(button.title)) button.title = "Save render";
  const ariaLabel = String(button.getAttribute("aria-label") || "");
  if (!ariaLabel || /^export\b/i.test(ariaLabel)) button.setAttribute("aria-label", "Save render");
  if (button.dataset.renderToolbarDecorated === "1") return;
  button.dataset.renderToolbarDecorated = "1";
  button.textContent = "";
  const label = document.createElement("span");
  label.className = "render-toolbar-save-btn__label";
  label.setAttribute("aria-hidden", "true");
  label.textContent = "save";
  button.appendChild(label);
  const sr = document.createElement("span");
  sr.className = "sr-only";
  sr.textContent = "Save render";
  button.appendChild(sr);
}

function syncRenderExportButtonDecor(button, format) {
  if (!button) return;
  const shell = button.closest(".render-toolbar-save-shell");
  if (shell && format) shell.dataset.exportFormat = String(format || "").toUpperCase();
  else if (shell) delete shell.dataset.exportFormat;
}

function syncRenderPreviewAuxPanel() {
  const panel = document.getElementById("renderPreviewAuxPanel");
  const controls = document.getElementById("interactivePreviewControls");
  if (!panel || !controls) return;
  panel.hidden = !!controls.hidden;
  if (typeof updatePreviewSizing === "function") updatePreviewSizing();
}

function setRenderPreviewSamplingValue(mode) {
  const select = document.getElementById("previewSampling");
  const next = String(mode || "").toLowerCase() === "nearest" ? "nearest" : "smooth";
  if (!select) return;
  const prev = String(select.value || "").toLowerCase();
  if (prev !== next) {
    select.value = next;
    select.dispatchEvent(new Event("input", { bubbles: true }));
    select.dispatchEvent(new Event("change", { bubbles: true }));
  } else {
    syncRenderPreviewSamplingSwitch();
  }
}

function syncRenderPreviewSamplingSwitch() {
  const select = document.getElementById("previewSampling");
  const switchNode = document.getElementById("renderPreviewSamplingSwitch");
  if (!select || !switchNode) return;
  const value = String(select.value || "").toLowerCase() === "nearest" ? "nearest" : "smooth";
  window.XTracerWidgets.syncSamplingSwitch(switchNode, value);
}

function createRenderPreviewSamplingSwitch(field) {
  const select = field ? field.querySelector("#previewSampling") : null;
  if (!select) return null;
  const label = field.querySelector(".xui-field__label");
  if (label) label.classList.add("sr-only");
  field.classList.add("render-preview-sampling-native");
  field.setAttribute("aria-hidden", "true");
  select.setAttribute("aria-hidden", "true");
  select.tabIndex = -1;

  const switchNode = document.createElement("div");
  switchNode.id = "renderPreviewSamplingSwitch";
  switchNode.className = "render-preview-sampling-switch";
  switchNode.setAttribute("role", "group");
  switchNode.setAttribute("aria-label", "Preview sampling");
  switchNode.appendChild(field);

  const samplingButtons = window.XTracerWidgets.createSamplingSwitch({
    value: String(select.value || "").toLowerCase(),
    onChange: setRenderPreviewSamplingValue,
  });
  while (samplingButtons.firstChild) switchNode.appendChild(samplingButtons.firstChild);

  if (select.dataset.renderPreviewSamplingBound !== "1") {
    select.dataset.renderPreviewSamplingBound = "1";
    select.addEventListener("change", syncRenderPreviewSamplingSwitch);
  }

  syncRenderPreviewSamplingSwitch();
  return switchNode;
}

function relocateRenderExportControls() {
  const toolbarDock = document.getElementById("renderPreviewToolbarControls");
  const viewDock = document.getElementById("renderPreviewViewControls");
  const auxDock = document.getElementById("renderPreviewAuxDock");
  const card = document.getElementById("exportControlsCard");
  const body = card ? card.querySelector(".control-section-body") : null;
  if (!toolbarDock || !auxDock || !body) return;

  const exportField = body.querySelector('label[for="exportFormat"]');
  const previewSamplingField = body.querySelector('label[for="previewSampling"]');
  const downloadBtn = body.querySelector("#download");
  const interactiveControls = body.querySelector("#interactivePreviewControls");

  if (exportField && downloadBtn) {
    const exportGroup = document.createElement("div");
    exportGroup.className = "render-toolbar-export-group";
    const exportShell = document.createElement("div");
    exportShell.className = "render-toolbar-save-shell";
    exportField.classList.add("render-toolbar-save-format");
    const exportLabel = exportField.querySelector(".xui-field__label");
    if (exportLabel) exportLabel.classList.add("sr-only");
    const exportSelect = exportField.querySelector("select");
    if (exportSelect) {
      exportSelect.classList.add("render-toolbar-save-select");
      exportSelect.setAttribute("aria-label", "Export format");
    }
    decorateRenderExportButton(downloadBtn);
    exportShell.appendChild(downloadBtn);
    exportShell.appendChild(exportField);
    exportGroup.appendChild(exportShell);
    toolbarDock.appendChild(exportGroup);
  }

  if (previewSamplingField) {
    const samplingSwitch = createRenderPreviewSamplingSwitch(previewSamplingField);
    const samplingDock = viewDock || toolbarDock;
    if (samplingSwitch && samplingDock) {
      samplingDock.appendChild(samplingSwitch);
      syncRenderPreviewSamplingSwitch();
    }
  }

  if (interactiveControls) auxDock.appendChild(interactiveControls);
  if (card) card.hidden = true;
  syncRenderPreviewAuxPanel();
}

if (window.XTracerWidgets && typeof window.XTracerWidgets.renderCreateField === "function") {
  window.XTracerWidgets.renderCreateField(document.getElementById("workspaceCreateField"), {
    inputId: "workspaceCreateName",
    buttonId: "workspaceCreateBtn",
    placeholder: "Workspace name",
    inputLabel: "Workspace name for new workspace",
    buttonTitle: "Create workspace",
    buttonLabel: "Create workspace",
  });
}
if (window.XTracerWidgets && typeof window.XTracerWidgets.createIconButton === "function") {
  const refreshMount = document.getElementById("workspaceRefreshBtnMount");
  if (refreshMount && refreshMount.parentNode) {
    const refreshBtn = window.XTracerWidgets.createIconButton({
      className: "workspace-refresh-btn",
      variant: "ghost",
      title: "Refresh workspace list",
      label: "Refresh workspace list",
      icon: window.XTracerWidgets.dom.svgIcon("M12 5a7 7 0 0 1 5.43 2.58M17.43 5v6h-6M12 19a7 7 0 1 1 5-12", "0 0 24 24"),
    });
    refreshBtn.id = "workspaceRefreshBtn";
    refreshMount.parentNode.replaceChild(refreshBtn, refreshMount);
  }
}

const $ = (id) => document.getElementById(id);

const el = {
  startupScreen: $("startupScreen"),
  startupLabel: $("startupLabel"),
  startupPercent: $("startupPercent"),
  startupProgressFill: $("startupProgressFill"),
  mainMenuToggle: $("mainMenuToggle"),
  mainTabs: $("mainTabs"),
  sidebarCards: $("sidebarCards"),
  themeToggle: $("themeToggle"),
  tabRender: $("tabRender"),
  tabVisual: $("tabVisual"),
  tabWorkspaces: $("tabWorkspaces"),
  tabGallery: $("tabGallery"),
  tabSettings: $("tabSettings"),
  tabAbout: $("tabAbout"),
  tabLogs: $("tabLogs"),
  sidebarRailToggle: $("sidebarRailToggle"),
  sheetBackdrop: $("sheetBackdrop"),
  controlsSheetFab: $("controlsSheetFab"),
  bnTabRender: $("bnTabRender"),
  bnTabWorkspaces: $("bnTabWorkspaces"),
  bnTabVisual: $("bnTabVisual"),
  bnTabGallery: $("bnTabGallery"),
  bnTabLogs: $("bnTabLogs"),
  bnTabSettings: $("bnTabSettings"),
  bnTabAbout: $("bnTabAbout"),
  paneRender: $("paneRender"),
  paneVisual: $("paneVisual"),
  paneWorkspaces: $("paneWorkspaces"),
  paneGallery: $("paneGallery"),
  paneSettings: $("paneSettings"),
  paneAbout: $("paneAbout"),
  paneLogs: $("paneLogs"),
  qualityControlsCard: $("qualityControlsCard"),
  exportControlsCard: $("exportControlsCard"),
  theme: $("theme"),
  darkPalette: $("darkPalette"),
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
  settingsJobsThreadGraph: $("settingsJobsThreadGraph"),
  settingsJobsUpdated: $("settingsJobsUpdated"),
  settingsJobsThreadsUsage: $("settingsJobsThreadsUsage"),
  settingsJobsList: $("settingsJobsList"),
  activeSceneCardScene: $("activeSceneCardScene"),
  activeSceneCardDescription: $("activeSceneCardDescription"),
  activeSceneCardSource: $("activeSceneCardSource"),
  activeSceneCardVariant: $("activeSceneCardVariant"),
  activeSceneCardVariantDescription: $("activeSceneCardVariantDescription"),
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
  threadsLabelText: $("threadsLabelText"),
  threadsLabelSubtext: $("threadsLabelSubtext"),
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
  visualSceneScale: $("visualSceneScale"),
  visualSelectionTag: $("visualSelectionTag"),
  visualShowGrid: $("visualShowGrid"),
  visualShowGlobalBvh: $("visualShowGlobalBvh"),
  visualShowMeshBvh: $("visualShowMeshBvh"),
  visualViewport: $("visualViewport"),
  visualPanel: $("visualPanel"),
  visualCameraEditor: $("visualCameraEditor"),
  visualCamPosX: $("visualCamPosX"),
  visualCamPosY: $("visualCamPosY"),
  visualCamPosZ: $("visualCamPosZ"),
  visualCamTgtX: $("visualCamTgtX"),
  visualCamTgtY: $("visualCamTgtY"),
  visualCamTgtZ: $("visualCamTgtZ"),
  visualCamFLength: $("visualCamFLength"),
  visualCamFov: $("visualCamFov"),
  visualCameraSnapBtn: $("visualCameraSnapBtn"),
  visualCameraApplyBtn: $("visualCameraApplyBtn"),
  visualCameraNewBtn: $("visualCameraNewBtn"),
  graphPanel: $("graphPanel"),
  graphCanvas: $("graphCanvas"),
  graphLegend: $("graphLegend"),
  graphResetLayoutBtn: $("graphResetLayoutBtn"),
  textEditorPanel: $("textEditorPanel"),
  editorView3dBtn: $("editorView3dBtn"),
  editorViewGraphBtn: $("editorViewGraphBtn"),
  editorViewTextBtn: $("editorViewTextBtn"),
  editorViewSamplersBtn: $("editorViewSamplersBtn"),
  editorViewGeometryBtn: $("editorViewGeometryBtn"),
  samplersPanel: $("samplersPanel"),
  geometryPanel: $("geometryPanel"),
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
  { name: "cgltf", description: "Single-file glTF 2.0 loader used for importing compact scene assets into the renderer.", used_in: "xtcore", license: "MIT", url: "https://github.com/jkuhlmann/cgltf" },
  { name: "TinyObjLoader", description: "Wavefront OBJ and MTL loader used by the mesh pipeline and scene import path.", used_in: "lib/nmesh, xtcore", license: "MIT", url: "https://github.com/tinyobjloader/tinyobjloader" },
  { name: "STB", description: "Collection of single-header image and utility libraries used for texture IO and image helpers.", used_in: "lib/nimg, xtcore, xtracer-web", license: "Public Domain / MIT", url: "https://github.com/nothings/stb" },
  { name: "TinyEXR", description: "OpenEXR reader and writer used for high-dynamic-range image support.", used_in: "lib/nimg", license: "BSD-3-Clause", url: "https://github.com/syoyo/tinyexr" },
  { name: "strpool", description: "String interning helper used to keep repeated identifiers compact in runtime data structures.", used_in: "xtcore, frontend/common, xtracer-web, xtracer-wasm", license: "MIT / Public Domain", url: "https://github.com/mattiasgustavsson/libs" },
  { name: "crow", description: "C++ HTTP and WebSocket server framework used by the web backend.", used_in: "xtracer-web", license: "BSD-3-Clause", url: "https://github.com/CrowCpp/Crow" },
  { name: "Three.js", description: "3D scene graph and rendering toolkit used by the web visualizer and interactive previews.", used_in: "xtracer-web", license: "MIT", url: "https://github.com/mrdoob/three.js" },
  { name: "ufbx", description: "FBX parser and evaluator used to read production-style geometry, transforms, and animation data.", used_in: "xtcore", license: "MIT", url: "https://github.com/ufbx/ufbx" },
];

const uiOptions = {
  textHistoryLimit: 200,
  visualHistoryLimit: 200,
  visualSceneScale: 1.0,
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
let cameraLoadToken = 0;
let runtimeGraphLoadToken = 0;
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
let exportRequestInFlight = false;
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
const TAB_MODES = ["workspaces", "render", "visual", "gallery", "logs", "settings", "about"];
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
let activeTabMode = "workspaces";
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
  panMode: "",
  pointerId: null,
  lastX: 0,
  lastY: 0,
};
const RENDER_MODE_DIRECT = "direct";
const RENDER_MODE_PROGRESSIVE = "progressive";
const RENDER_MODE_INCREMENTAL = "incremental";
const RENDER_MODE_INTERACTIVE = "interactive";
let renderMode = RENDER_MODE_PROGRESSIVE;
let interactivePreviewEnabled = false;
let interactivePreviewLoopToken = 0;
let interactivePreviewLoopActive = false;
let interactivePreviewJobId = "";
let interactivePreviewDirty = false;
let interactivePreviewCameraSeq = 0;
let interactivePreviewLastInputMs = 0;
const INTERACTIVE_PREVIEW_SETTLE_MS = 420;
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

relocateRenderExportControls();
