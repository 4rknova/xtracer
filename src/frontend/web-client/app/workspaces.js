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
