let activeCardPanelId = null;

function updateTabCardsBarState() {
  document.querySelectorAll(".tab-card-btn").forEach((btn) => {
    const cardId = btn.dataset.cardId;
    const isActive = cardId === activeCardPanelId;
    btn.classList.toggle("is-active", isActive);
    btn.setAttribute("aria-pressed", isActive ? "true" : "false");
  });
}

function returnModalBodyToSource(container) {
  const modalBody = container && container.querySelector(".controls-modal-body");
  if (!modalBody || !activeCardPanelId) return;
  const portedBody = modalBody.firstElementChild;
  if (portedBody) {
    const sourceCard = container.querySelector(`#${activeCardPanelId}`);
    if (sourceCard) sourceCard.appendChild(portedBody);
  }
  modalBody.hidden = true;
}

function openCardPanel(cardId) {
  const container = document.querySelector(".panel-controls");
  returnModalBodyToSource(container);
  activeCardPanelId = cardId;
  if (container) {
    container.querySelectorAll("details.control-section").forEach((c) => { c.hidden = true; });
    const sourceCard = container.querySelector(`#${cardId}`);
    const modalBody = container.querySelector(".controls-modal-body");
    if (sourceCard && modalBody) {
      const cardBody = sourceCard.querySelector(".xui-card__body");
      if (cardBody) {
        modalBody.appendChild(cardBody);
        modalBody.hidden = false;
      }
    }
  }
  const defs = window.XTracerSidebarCards && typeof window.XTracerSidebarCards.getCardDefs === "function"
    ? window.XTracerSidebarCards.getCardDefs()
    : [];
  const def = defs.find((d) => d.id === cardId);
  const titleEl = document.getElementById("controlsModalTitle");
  if (titleEl && def) titleEl.textContent = def.title;
  updateTabCardsBarState();
  document.dispatchEvent(new CustomEvent("controls-panel-request", { detail: { open: true } }));
}

function closeCardPanel() {
  const container = document.querySelector(".panel-controls");
  returnModalBodyToSource(container);
  activeCardPanelId = null;
  updateTabCardsBarState();
  document.dispatchEvent(new CustomEvent("controls-panel-request", { detail: { open: false } }));
  applySidebarCardLayout(activeTabMode);
}

function clearActiveCardPanel() {
  if (activeCardPanelId === null) return;
  const container = document.querySelector(".panel-controls");
  returnModalBodyToSource(container);
  activeCardPanelId = null;
  updateTabCardsBarState();
  applySidebarCardLayout(activeTabMode);
}

document.addEventListener("card-panel-closed", clearActiveCardPanel);

function buildTabCardIconSvg(pathData) {
  const ns = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(ns, "svg");
  svg.setAttribute("viewBox", "0 0 24 24");
  svg.setAttribute("aria-hidden", "true");
  svg.setAttribute("fill", "none");
  svg.setAttribute("stroke", "currentColor");
  svg.setAttribute("stroke-width", "1.75");
  svg.setAttribute("stroke-linecap", "round");
  svg.setAttribute("stroke-linejoin", "round");
  const segments = String(pathData || "").split(/\s+(?=[A-Z])/);
  segments.forEach((seg) => {
    const path = document.createElementNS(ns, "path");
    path.setAttribute("d", seg.trim());
    svg.appendChild(path);
  });
  return svg;
}

function renderTabCardsBar() {
  const mode = activeTabMode;
  if (!sidebarCardVisibility) return;

  let visibleIds = sidebarCardVisibility[mode] || [];
  if (mode === "visual" && sidebarCardVisibility.visual_by_editor) {
    const byEditor = sidebarCardVisibility.visual_by_editor;
    visibleIds = byEditor[editorViewMode] || byEditor.visual || visibleIds;
  }

  const defs = window.XTracerSidebarCards && typeof window.XTracerSidebarCards.getCardDefs === "function"
    ? window.XTracerSidebarCards.getCardDefs()
    : [];

  const paneId = `pane${mode.charAt(0).toUpperCase()}${mode.slice(1)}`;
  const pane = document.getElementById(paneId);
  if (!pane) return;

  pane.querySelectorAll(".tab-cards-bar").forEach((bar) => bar.remove());

  if (!visibleIds.length) return;

  const bar = document.createElement("div");
  bar.className = "tab-cards-bar";
  bar.setAttribute("role", "toolbar");
  bar.setAttribute("aria-label", "Control panels");

  visibleIds.forEach((cardId) => {
    const def = defs.find((d) => d.id === cardId);
    if (!def) return;
    const btn = document.createElement("button");
    btn.type = "button";
    btn.className = "tab-card-btn";
    btn.dataset.cardId = cardId;
    btn.title = def.title;
    btn.setAttribute("aria-label", def.title);
    btn.setAttribute("aria-pressed", cardId === activeCardPanelId ? "true" : "false");
    if (cardId === activeCardPanelId) btn.classList.add("is-active");
    btn.appendChild(buildTabCardIconSvg(def.icon || "M4 6h16M4 12h16M4 18h16"));
    btn.addEventListener("click", () => {
      if (activeCardPanelId === cardId) closeCardPanel();
      else openCardPanel(cardId);
    });
    bar.appendChild(btn);
  });

  if (mode === "visual") {
    const actions = pane.querySelector(".editor-view-actions");
    if (actions) {
      bar.classList.add("tab-cards-bar--inline");
      actions.insertBefore(bar, actions.firstChild);
      return;
    }
  }

  if (mode === "render") {
    const toolbarControls = pane.querySelector("#renderPreviewToolbarControls");
    if (toolbarControls) {
      bar.classList.add("tab-cards-bar--inline", "tab-cards-bar--flush-right");
      toolbarControls.appendChild(bar);
      return;
    }
  }

  pane.insertBefore(bar, pane.firstChild);
}

function mountTabUtilActions() {
  const bar = document.getElementById("tabUtilActions");
  if (!bar) return;
  const mode = activeTabMode;
  let target = null;
  if (mode === "render") {
    target = document.getElementById("renderPreviewToolbarControls");
  } else if (mode === "visual") {
    const pane = document.getElementById("paneVisual");
    target = pane && pane.querySelector(".editor-view-actions");
  } else if (mode === "workspaces") {
    const pane = document.getElementById("paneWorkspaces");
    target = pane && pane.querySelector(".workspace-toolbar-actions");
  } else if (mode === "gallery") {
    const pane = document.getElementById("paneGallery");
    target = pane && pane.querySelector(".gallery-toolbar");
  } else if (mode === "settings") {
    target = document.getElementById("paneSettingsHead");
  } else if (mode === "about") {
    const pane = document.getElementById("paneAbout");
    target = pane && pane.querySelector(".about-tab-bar");
  }
  if (!target) return;
  if (bar.parentNode !== target) target.appendChild(bar);
  bar.hidden = false;
}

function normalizeTabMode(mode) {
  const raw = String(mode || "").toLowerCase();
  if (raw === "editor") return "visual";
  if (raw === "workspace" || raw === "scene" || raw === "scene_setup" || raw === "scenesetup") return "workspaces";
  if (raw === "render" || raw === "visual" || raw === "workspaces" || raw === "gallery" || raw === "settings" || raw === "about") {
    return raw;
  }
  return "workspaces";
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
    } else if (rawConfig && mode === "about" && Array.isArray(rawConfig.about)) {
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
  if (raw === "visual" || raw === "graph" || raw === "text" || raw === "samplers" || raw === "geometry" || raw === "materials") return raw;
  return "visual";
}

function syncAaPresetUi() {
  const current = String(el.aa && el.aa.value ? el.aa.value : "");
  document.querySelectorAll(".aa-pill[data-aa]").forEach((btn) => {
    const value = String(btn.getAttribute("data-aa") || "");
    btn.classList.toggle("active", value === current);
  });
}

function syncSamplesPresetUi() {
  const current = String(el.samples && el.samples.value ? el.samples.value : "");
  document.querySelectorAll(".samples-pill").forEach((btn) => {
    const value = String(btn.getAttribute("data-samples") || "");
    btn.classList.toggle("active", value === current);
  });
}


function setEditorViewMode(mode, persist) {
  const nextMode = normalizeEditorViewMode(mode);
  const isVisual = nextMode === "visual";
  const isGraph = nextMode === "graph";
  const isText = nextMode === "text";
  const isSamplers = nextMode === "samplers";
  const isGeometry = nextMode === "geometry";
  const isMaterials = nextMode === "materials";
  editorViewMode = nextMode;

  if (el.visualPanel) el.visualPanel.hidden = !isVisual;
  if (el.graphPanel) el.graphPanel.hidden = !isGraph;
  if (el.textEditorPanel) el.textEditorPanel.hidden = !isText;
  if (el.graphResetLayoutBtn) el.graphResetLayoutBtn.hidden = !isGraph;
  if (el.samplersPanel) {
    el.samplersPanel.hidden = !isSamplers;
    if (isSamplers) {
      const iframe = el.samplersPanel.querySelector("iframe");
      if (iframe && !iframe.dataset.loaded) { iframe.src = "/samplers.html"; iframe.dataset.loaded = "1"; }
    }
  }
  if (el.geometryPanel) {
    el.geometryPanel.hidden = !isGeometry;
    if (isGeometry) {
      const iframe = el.geometryPanel.querySelector("iframe");
      if (iframe && !iframe.dataset.loaded) { iframe.src = "/geometry.html"; iframe.dataset.loaded = "1"; }
    }
  }
  if (el.materialsPanel) {
    el.materialsPanel.hidden = !isMaterials;
    if (isMaterials) {
      const iframe = el.materialsPanel.querySelector("iframe");
      if (iframe && !iframe.dataset.loaded) { iframe.src = "/materials.html"; iframe.dataset.loaded = "1"; }
    }
  }

  const setActive = (node, state) => {
    if (!node) return;
    node.classList.toggle("active", state);
    node.setAttribute("aria-selected", state ? "true" : "false");
    node.setAttribute("aria-pressed", state ? "true" : "false");
  };
  setActive(el.editorView3dBtn, isVisual);
  setActive(el.editorViewGraphBtn, isGraph);
  setActive(el.editorViewTextBtn, isText);
  setActive(el.editorViewSamplersBtn, isSamplers);
  setActive(el.editorViewGeometryBtn, isGeometry);
  setActive(el.editorViewMaterialsBtn, isMaterials);

  if (persist !== false) localStorage.setItem(EDITOR_VIEW_MODE_KEY, nextMode);
  if (activeTabMode === "visual") {
    closeCardPanel();
    applySidebarCardLayout("visual");
    requestAnimationFrame(renderTabCardsBar);
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

function getSidebarVisibleCards(container) {
  if (!container) return [];
  return Array.from(container.querySelectorAll("details.control-section"))
    .filter((card) => !card.hidden && !card.classList.contains("is-visibility-hidden"));
}

function getSidebarCardTitle(card) {
  if (!card) return "";
  const titleNode = card.querySelector(".control-card-title");
  const raw = titleNode ? titleNode.textContent : card.id;
  return String(raw || "").trim();
}

let _jobsModalCloseListener = null;
let _jobsModalCloseTimer = null;

function openJobsModal() {
  if (!el.jobsModal) return;
  const panel = el.jobsModal.querySelector(".jobs-modal-panel");
  if (_jobsModalCloseListener && panel) {
    panel.removeEventListener("animationend", _jobsModalCloseListener);
    _jobsModalCloseListener = null;
  }
  if (_jobsModalCloseTimer) { clearTimeout(_jobsModalCloseTimer); _jobsModalCloseTimer = null; }
  el.jobsModal.classList.remove("jobs-modal-closing");
  el.jobsModal.hidden = false;
  document.body.classList.add("jobs-modal-open");
  if (el.topbarJobsBtn) el.topbarJobsBtn.setAttribute("aria-pressed", "true");
  if (el.bnTabJobs) { el.bnTabJobs.setAttribute("aria-pressed", "true"); el.bnTabJobs.classList.add("active"); }
  if (el.tabJobs) { el.tabJobs.setAttribute("aria-pressed", "true"); el.tabJobs.classList.add("active"); }
  if (typeof refreshSettingsJobsCard === "function") refreshSettingsJobsCard();
}

function closeJobsModal() {
  if (!el.jobsModal || el.jobsModal.hidden) return;
  el.jobsModal.classList.add("jobs-modal-closing");
  const panel = el.jobsModal.querySelector(".jobs-modal-panel");
  const done = () => {
    _jobsModalCloseListener = null;
    if (_jobsModalCloseTimer) { clearTimeout(_jobsModalCloseTimer); _jobsModalCloseTimer = null; }
    el.jobsModal.hidden = true;
    el.jobsModal.classList.remove("jobs-modal-closing");
    document.body.classList.remove("jobs-modal-open");
    if (el.topbarJobsBtn) el.topbarJobsBtn.setAttribute("aria-pressed", "false");
    if (el.bnTabJobs) { el.bnTabJobs.setAttribute("aria-pressed", "false"); el.bnTabJobs.classList.remove("active"); }
    if (el.tabJobs) { el.tabJobs.setAttribute("aria-pressed", "false"); el.tabJobs.classList.remove("active"); }
  };
  if (panel) {
    _jobsModalCloseListener = done;
    panel.addEventListener("animationend", done, { once: true });
    _jobsModalCloseTimer = setTimeout(done, 300);
  } else {
    done();
  }
}

let _logsModalCloseListener = null;
let _logsModalCloseTimer = null;

function openLogsModal() {
  if (!el.logsModal) return;
  const panel = el.logsModal.querySelector(".log-modal-panel");
  if (_logsModalCloseListener && panel) {
    panel.removeEventListener("animationend", _logsModalCloseListener);
    _logsModalCloseListener = null;
  }
  if (_logsModalCloseTimer) { clearTimeout(_logsModalCloseTimer); _logsModalCloseTimer = null; }
  el.logsModal.classList.remove("log-modal-closing");
  el.logsModal.hidden = false;
  document.body.classList.add("log-modal-open");
  if (el.topbarLogsBtn) el.topbarLogsBtn.setAttribute("aria-pressed", "true");
  if (el.bnTabLogs) { el.bnTabLogs.setAttribute("aria-pressed", "true"); el.bnTabLogs.classList.add("active"); }
  if (el.tabLogs) { el.tabLogs.setAttribute("aria-pressed", "true"); el.tabLogs.classList.add("active"); }
  if (typeof setLogsUnseenBadge === "function") setLogsUnseenBadge(false);
  if (uiOptions.autoScrollLogs || pendingLogScroll) scrollLogToBottom(true);
}

function closeLogsModal() {
  if (!el.logsModal || el.logsModal.hidden) return;
  el.logsModal.classList.add("log-modal-closing");
  const panel = el.logsModal.querySelector(".log-modal-panel");
  const done = () => {
    _logsModalCloseListener = null;
    if (_logsModalCloseTimer) { clearTimeout(_logsModalCloseTimer); _logsModalCloseTimer = null; }
    el.logsModal.hidden = true;
    el.logsModal.classList.remove("log-modal-closing");
    document.body.classList.remove("log-modal-open");
    if (el.topbarLogsBtn) el.topbarLogsBtn.setAttribute("aria-pressed", "false");
    if (el.bnTabLogs) { el.bnTabLogs.setAttribute("aria-pressed", "false"); el.bnTabLogs.classList.remove("active"); }
    if (el.tabLogs) { el.tabLogs.setAttribute("aria-pressed", "false"); el.tabLogs.classList.remove("active"); }
  };
  if (panel) {
    _logsModalCloseListener = done;
    panel.addEventListener("animationend", done, { once: true });
    _logsModalCloseTimer = setTimeout(done, 300);
  } else {
    done();
  }
}

function openRenderStatsModal() {
  if (!el.renderStatsModal) return;
  el.renderStatsModal.classList.remove("render-stats-modal-closing");
  el.renderStatsModal.hidden = false;
  if (el.renderStatsToggleBtn) el.renderStatsToggleBtn.setAttribute("aria-pressed", "true");
  if (el.renderStatsToggleBtn) el.renderStatsToggleBtn.classList.add("is-active");
}

function closeRenderStatsModal() {
  if (!el.renderStatsModal || el.renderStatsModal.hidden) return;
  el.renderStatsModal.classList.add("render-stats-modal-closing");
  const panel = el.renderStatsModal.querySelector(".render-stats-modal-panel");
  const done = () => {
    el.renderStatsModal.hidden = true;
    el.renderStatsModal.classList.remove("render-stats-modal-closing");
    if (el.renderStatsToggleBtn) el.renderStatsToggleBtn.setAttribute("aria-pressed", "false");
    if (el.renderStatsToggleBtn) el.renderStatsToggleBtn.classList.remove("is-active");
  };
  if (panel) {
    panel.addEventListener("animationend", done, { once: true });
  } else {
    done();
  }
}

function refreshMobileCardSwitcher() {
  const container = document.querySelector(".panel-controls");
  if (!container) return;
  const allCards = Array.from(container.querySelectorAll("details.control-section"));

  let switcher = container.querySelector(".mobile-card-switcher");
  if (!switcher) {
    switcher = document.createElement("div");
    switcher.className = "mobile-card-switcher";
    switcher.hidden = true;
    switcher.setAttribute("aria-label", "Control sections");
    container.insertBefore(switcher, container.firstChild);
  }

  if (!isMobileTabMenuViewport()) {
    switcher.hidden = true;
    switcher.innerHTML = "";
    allCards.forEach((card) => card.classList.remove("mobile-card-hidden"));
    scheduleMobileLogsViewportSync();
    return;
  }

  const cards = getSidebarVisibleCards(container);
  if (cards.length <= 1) {
    switcher.hidden = true;
    switcher.innerHTML = "";
    allCards.forEach((card) => card.classList.remove("mobile-card-hidden"));
    delete container.dataset.mobileSelectedCardId;
    scheduleMobileLogsViewportSync();
    return;
  }

  const preferredId = container.dataset.mobileSelectedCardId || "";
  const selectedCard = cards.find((card) => card.id === preferredId)
    || cards.find((card) => card.open)
    || cards[0];
  container.dataset.mobileSelectedCardId = selectedCard ? selectedCard.id : "";

  cards.forEach((card) => {
    card.classList.toggle("mobile-card-hidden", card !== selectedCard);
  });
  switcher.hidden = false;
  switcher.innerHTML = "";
  cards.forEach((card) => {
    const isActive = card === selectedCard;
    const chip = window.XTracerWidgets.createPill({
      label: getSidebarCardTitle(card),
      active: isActive,
      pressable: true,
      className: "mobile-card-chip",
    });
    chip.addEventListener("click", () => {
      if (!isMobileTabMenuViewport()) return;
      container.dataset.mobileSelectedCardId = card.id;
      if (!card.open) card.open = true;
      refreshMobileCardSwitcher();
      requestAnimationFrame(() => {
        card.scrollIntoView({ block: "start", inline: "nearest", behavior: "smooth" });
      });
    });
    switcher.appendChild(chip);
  });

  scheduleMobileLogsViewportSync();
}

function applySidebarCardLayout(mode) {
  if (!sidebarCardVisibility || typeof sidebarCardVisibility !== "object") return;
  if (activeCardPanelId !== null) return;
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

  const sidebar = document.querySelector(".persistent-sidebar");
  const hadNoCards = sidebar && sidebar.classList.contains("has-no-sidebar-cards");
  const hasNoCards = visibleIds.length === 0;
  if (sidebar) sidebar.classList.toggle("has-no-sidebar-cards", hasNoCards);
  if (hasNoCards && !hadNoCards) {
    document.dispatchEvent(new CustomEvent("sheet:close"));
  }

  requestAnimationFrame(refreshMobileCardSwitcher);
}

function isMobileTabMenuViewport() {
  return !!(window.matchMedia && window.matchMedia("(max-width: 1099px)").matches);
}


function setMainMenuOpen(open) {
  const topbar = document.querySelector(".topbar");
  if (!topbar || !el.mainMenuToggle) return;
  const next = !!open;
  topbar.classList.toggle("menu-open", next);
  el.mainMenuToggle.setAttribute("aria-expanded", next ? "true" : "false");
}

function syncRenderTabEnabled() {
  const hasWorkspace = !!String(activeWorkspaceId || "").trim();
  const hasScene = !!String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const enabled = hasWorkspace && hasScene;
  [el.tabRender, el.bnTabRender].forEach((btn) => {
    if (!btn) return;
    btn.disabled = !enabled;
    btn.setAttribute("aria-disabled", enabled ? "false" : "true");
    btn.classList.toggle("is-tab-disabled", !enabled);
  });
  if (!enabled && activeTabMode === "render") {
    setActiveTab("workspaces");
  }
}

function setActiveTab(mode) {
  const nextMode = normalizeTabMode(mode);
  if (nextMode === "render") {
    const hasWorkspace = !!String(activeWorkspaceId || "").trim();
    const hasScene = !!String(el.scene && el.scene.value ? el.scene.value : "").trim();
    if (!hasWorkspace || !hasScene) {
      setActiveTab("workspaces");
      return;
    }
  }
  activeTabMode = nextMode;
  const isRender = nextMode === "render";
  const isVisual = nextMode === "visual";
  const isWorkspaces = nextMode === "workspaces";
  const isGallery = nextMode === "gallery";
  const isSettings = nextMode === "settings";
  const isAbout = nextMode === "about";
  const setActive = (node, state) => { if (node) node.classList.toggle("active", state); };
  setActive(el.tabRender, isRender);
  setActive(el.tabVisual, isVisual);
  setActive(el.tabWorkspaces, isWorkspaces);
  setActive(el.tabGallery, isGallery);
  setActive(el.tabSettings, isSettings);
  setActive(el.tabAbout, isAbout);
  setActive(el.topbarConfigBtn, isSettings);
  setActive(el.topbarAboutBtn,  isAbout);
  ["bnTabWorkspaces", "bnTabRender", "bnTabVisual", "bnTabGallery", "bnTabSettings", "bnTabAbout"].forEach((id) => {
    const btn = el[id];
    if (!btn) return;
    const isBtn = btn.dataset.mode === nextMode;
    btn.classList.toggle("active", isBtn);
    btn.setAttribute("aria-pressed", isBtn ? "true" : "false");
  });
  setActive(el.paneRender, isRender);
  setActive(el.paneVisual, isVisual);
  setActive(el.paneWorkspaces, isWorkspaces);
  setActive(el.paneGallery, isGallery);
  setActive(el.paneSettings, isSettings);
  setActive(el.paneAbout, isAbout);
  closeCardPanel();
  applySidebarCardLayout(nextMode);
  void refreshSidebarCardVisibilityConfig(nextMode);
  localStorage.setItem(ACTIVE_TAB_KEY, nextMode);
  requestAnimationFrame(renderTabCardsBar);
  requestAnimationFrame(mountTabUtilActions);
  if (isVisual && editorViewMode === "visual" && visualEditor) visualEditor.onShow();
  if (isVisual && editorViewMode === "graph") scheduleGraphRender();
  if (isWorkspaces && hasBackendMethod(api, "getWorkspaces")) {
    refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
  }
  if (isGallery && typeof refreshGallery === "function") {
    refreshGallery().catch((err) => appendLog(`gallery refresh error: ${err.message}`));
  }
  if (typeof refreshSettingsJobsCard === "function"
    && typeof isJobsControlsCardVisible === "function"
    && isJobsControlsCardVisible()) {
    refreshSettingsJobsCard();
  }
  if (isRender) {
    requestAnimationFrame(() => {
      updatePreviewSizing();
    });
    restorePreviewForActiveWorkspace().catch((err) => {
      appendLog(`preview restore error: ${err.message}`);
    });
    if (interactivePreviewEnabled && typeof requestInteractivePreviewRender === "function") {
      requestInteractivePreviewRender();
    }
    if (typeof renderInteractivePreviewHud === "function") renderInteractivePreviewHud();
  } else if (interactivePreviewEnabled && typeof stopInteractivePreviewLoop === "function") {
    stopInteractivePreviewLoop(true).catch(() => {});
    if (typeof renderInteractivePreviewHud === "function") renderInteractivePreviewHud();
  }
  if (isMobileTabMenuViewport()) setMainMenuOpen(false);
  requestAnimationFrame(() => {
    refreshMobileCardSwitcher();
    scheduleMobileLogsViewportSync();
  });
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

  window.addEventListener("resize", () => {
    refreshMobileCardSwitcher();
    scheduleMobileLogsViewportSync();
  });
  cards.forEach((card) => {
    card.addEventListener("toggle", () => {
      requestAnimationFrame(() => {
        refreshMobileCardSwitcher();
        scheduleMobileLogsViewportSync();
      });
    });
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
      animateOpen(card);
      requestAnimationFrame(() => {
        refreshMobileCardSwitcher();
        scheduleMobileLogsViewportSync();
      });
    });
  });
}

function getOrCreateVisualLoadingOverlay() {
  const viewport = el.visualViewport;
  if (!viewport) return null;
  let overlay = viewport.querySelector(".visual-loading-overlay");
  if (!overlay) {
    overlay = document.createElement("div");
    overlay.className = "visual-loading-overlay";
    overlay.setAttribute("role", "status");
    overlay.setAttribute("aria-label", "Loading 3D scene");
    const spinner = document.createElement("div");
    spinner.className = "visual-loading-spinner";
    spinner.setAttribute("aria-hidden", "true");
    const label = document.createElement("span");
    label.className = "visual-loading-label";
    overlay.appendChild(spinner);
    overlay.appendChild(label);
    overlay.hidden = true;
    viewport.appendChild(overlay);
  }
  return overlay;
}

async function loadVisualSceneFromSelected() {
  if (!visualEditor) return;
  const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const variantName = selectedSceneVariantValue();
  if (!sceneName) {
    visualEditor.setStatus("No scene selected.");
    return;
  }

  const overlay = getOrCreateVisualLoadingOverlay();
  if (overlay) {
    const label = overlay.querySelector(".visual-loading-label");
    if (label) label.textContent = sceneName + (variantName ? " (" + variantName + ")" : "");
    overlay.hidden = false;
  }
  visualEditor.setStatus("Loading " + sceneName + (variantName ? " (" + variantName + ")" : "") + " ...");
  try {
    const pair = await Promise.all([
      api.getSceneGeometry(sceneName, variantName),
      hasBackendMethod(api, "getSceneRuntimeGraph")
        ? loadSceneRuntimeGraph(sceneName, variantName).catch(() => null)
        : Promise.resolve(null),
    ]);
    const geometryData = pair[0] || { meshes: {} };
    const runtimeData = pair[1] || null;
    await visualEditor.buildScene(sceneName, "", geometryData, runtimeData);
    const baseRadius = Number(visualEditor.sceneBaseRadius);
    if (Number.isFinite(baseRadius) && baseRadius > 1e-6 && visualEditor.setSceneScaleMultiplier) {
      const autoScale = clampVisualSceneScale(5.0 / baseRadius);
      visualEditor.setSceneScaleMultiplier(autoScale, false);
      uiOptions.visualSceneScale = autoScale;
      if (el.visualSceneScale) el.visualSceneScale.value = String(autoScale);
    }
    visualLoadedSceneName = sceneName;
    refreshVisualCameraOptions();
    syncVisualCameraFromRenderSelection();
    if (typeof refreshVisualCameraEditorPanel === "function") refreshVisualCameraEditorPanel();
    refreshVisualPhotonOverlay().catch(() => {});
    appendLog("visual loaded: " + sceneName + (variantName ? " (" + variantName + ")" : ""));
  } finally {
    if (overlay) overlay.hidden = true;
  }
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
  if (typeof refreshVisualCameraEditorPanel === "function") refreshVisualCameraEditorPanel();
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
  refreshThemeToggleButton();
}

function refreshThemeToggleButton() {
  if (!el.themeToggle) return;
  const selected = String(el.theme && el.theme.value ? el.theme.value : localStorage.getItem("xtracer-theme") || "system").toLowerCase();
  const effective = effectiveThemeMode(selected);
  const label = selected === "system" ? `Theme: system (${effective})` : `Theme: ${selected}`;
  el.themeToggle.setAttribute("aria-label", label);
  el.themeToggle.setAttribute("title", label);
  el.themeToggle.dataset.themeMode = selected;
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

function clampVisualSceneScale(v) {
  const n = Number(v);
  if (!Number.isFinite(n)) return 1.0;
  return Math.max(0.01, Math.min(100, n));
}

function applyFontScale(scale) {
  uiOptions.fontScale = clampFontScale(scale);
  document.documentElement.style.fontSize = `${(uiOptions.fontScale * 100).toFixed(1)}%`;
}

function loadUIOptions() {
  uiOptions.textHistoryLimit = clampHistoryLimit(localStorage.getItem("xtracer-text-history-limit") || "200");
  uiOptions.visualHistoryLimit = clampHistoryLimit(localStorage.getItem("xtracer-visual-history-limit") || "200");
  uiOptions.visualSceneScale = clampVisualSceneScale(localStorage.getItem("xtracer-visual-scene-scale") || "1");
  uiOptions.autoLoadEditor = localStorage.getItem("xtracer-auto-load-editor") !== "0";
  uiOptions.autoScrollLogs = localStorage.getItem("xtracer-auto-scroll-logs") !== "0";
  uiOptions.clearPreviewOnRender = localStorage.getItem("xtracer-clear-preview-on-render") === "1";
  uiOptions.draftMode = localStorage.getItem("xtracer-draft-mode") === "1";
  uiOptions.tileHeatmapEnabled = localStorage.getItem("xtracer-tile-heatmap-enabled") !== "0";
  uiOptions.fontScale = 1.0;
  const previewSamplingRaw = String(localStorage.getItem("xtracer-preview-sampling") || "smooth").toLowerCase();
  const previewSampling = (previewSamplingRaw === "linear" || previewSamplingRaw === "bilinear")
    ? "smooth"
    : previewSamplingRaw;
  uiOptions.previewSampling = (previewSampling === "nearest" || previewSampling === "smooth")
    ? previewSampling
    : "smooth";
  uiOptions.darkPalette = normalizeDarkPalette(localStorage.getItem("xtracer-dark-palette") || "slate");
  uiOptions.lightPalette = normalizeLightPalette(localStorage.getItem("xtracer-light-palette") || "coastal");
  if (el.textHistorySize) el.textHistorySize.value = String(uiOptions.textHistoryLimit);
  if (el.visualHistorySize) el.visualHistorySize.value = String(uiOptions.visualHistoryLimit);
  if (el.visualSceneScale) el.visualSceneScale.value = String(uiOptions.visualSceneScale);
  el.autoLoadEditor.checked = uiOptions.autoLoadEditor;
  el.autoScrollLogs.checked = uiOptions.autoScrollLogs;
  el.clearPreviewOnRender.checked = uiOptions.clearPreviewOnRender;
  if (el.draftMode) el.draftMode.checked = uiOptions.draftMode;
  if (el.tileHeatmapEnabled) el.tileHeatmapEnabled.checked = !!uiOptions.tileHeatmapEnabled;
  if (el.previewSampling) el.previewSampling.value = uiOptions.previewSampling;
  if (typeof syncRenderPreviewSamplingSwitch === "function") syncRenderPreviewSamplingSwitch();
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
        if (typeof parsed.ui === "boolean") logFilters.ui = parsed.ui;
      }
    }
  } catch (_) {
    // keep defaults if local storage has invalid JSON
  }
}

function persistUIOptions() {
  localStorage.setItem("xtracer-text-history-limit", String(uiOptions.textHistoryLimit));
  localStorage.setItem("xtracer-visual-history-limit", String(uiOptions.visualHistoryLimit));
  localStorage.setItem("xtracer-visual-scene-scale", String(uiOptions.visualSceneScale));
  localStorage.setItem("xtracer-auto-load-editor", uiOptions.autoLoadEditor ? "1" : "0");
  localStorage.setItem("xtracer-auto-scroll-logs", uiOptions.autoScrollLogs ? "1" : "0");
  localStorage.setItem("xtracer-clear-preview-on-render", uiOptions.clearPreviewOnRender ? "1" : "0");
  localStorage.setItem("xtracer-draft-mode", uiOptions.draftMode ? "1" : "0");
  localStorage.setItem("xtracer-tile-heatmap-enabled", uiOptions.tileHeatmapEnabled ? "1" : "0");
  localStorage.setItem("xtracer-preview-sampling", uiOptions.previewSampling);
  localStorage.setItem("xtracer-dark-palette", uiOptions.darkPalette);
  localStorage.setItem("xtracer-light-palette", uiOptions.lightPalette);
  localStorage.setItem(LOG_FILTERS_KEY, JSON.stringify(logFilters));
}
