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
    api.getSceneGeometry(sceneName),
    hasBackendMethod(api, "getSceneRuntimeGraph")
      ? loadSceneRuntimeGraph(sceneName).catch(() => null)
      : Promise.resolve(null),
  ]);
  const geometryData = pair[0] || { meshes: {} };
  const runtimeData = pair[1] || null;
  await visualEditor.buildScene(sceneName, "", geometryData, runtimeData);
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
  uiOptions.tileHeatmapEnabled = localStorage.getItem("xtracer-tile-heatmap-enabled") !== "0";
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
  if (el.tileHeatmapEnabled) el.tileHeatmapEnabled.checked = !!uiOptions.tileHeatmapEnabled;
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
  localStorage.setItem("xtracer-tile-heatmap-enabled", uiOptions.tileHeatmapEnabled ? "1" : "0");
  localStorage.setItem("xtracer-preview-sampling", uiOptions.previewSampling);
  localStorage.setItem("xtracer-ui-font-size-preset", uiOptions.fontSizePreset);
  localStorage.setItem("xtracer-ui-font-scale", String(uiOptions.fontScale));
  localStorage.setItem("xtracer-dark-palette", uiOptions.darkPalette);
  localStorage.setItem("xtracer-light-palette", uiOptions.lightPalette);
  localStorage.setItem(LOG_FILTERS_KEY, JSON.stringify(logFilters));
}
