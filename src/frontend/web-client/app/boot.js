function refreshVisualCameraEditorPanel() {
  if (!el.visualCameraEditor) return;
  const fmt = (v) => Number.isFinite(Number(v)) ? String(Math.round(Number(v) * 1e6) / 1e6) : "";

  // Read camera data from the scene source using the render camera selection
  const cameraId = String(el.camera && el.camera.value ? el.camera.value : "").trim();
  const sourceText = String(el.sceneSource && el.sceneSource.value ? el.sceneSource.value : "");
  if (!cameraId || !sourceText.trim()) {
    el.visualCameraEditor.hidden = true;
    return;
  }
  const model = parseSceneEditModel(sourceText);
  const cam = (model.cameras || []).find((c) => c.id === cameraId);
  if (!cam) {
    el.visualCameraEditor.hidden = true;
    return;
  }
  const position = readSceneVec3Prop(cam.body, "position");
  const target = readSceneVec3Prop(cam.body, "target");
  const flength = readSceneNumber(readSceneStringProp(cam.body, "flength"), null);
  const fov = readSceneNumber(readSceneStringProp(cam.body, "fov"), null);
  if (el.visualCamPosX) el.visualCamPosX.value = fmt(position[0]);
  if (el.visualCamPosY) el.visualCamPosY.value = fmt(position[1]);
  if (el.visualCamPosZ) el.visualCamPosZ.value = fmt(position[2]);
  if (el.visualCamTgtX) el.visualCamTgtX.value = fmt(target[0]);
  if (el.visualCamTgtY) el.visualCamTgtY.value = fmt(target[1]);
  if (el.visualCamTgtZ) el.visualCamTgtZ.value = fmt(target[2]);
  if (el.visualCamFLength) el.visualCamFLength.value = flength != null ? fmt(flength) : "";
  if (el.visualCamFov) el.visualCamFov.value = fov != null ? fmt(fov) : "";
  el.visualCameraEditor.hidden = false;
}

async function applyCameraEditorChanges() {
  if (!visualEditor) throw new Error("visual editor not ready");
  const snap = visualEditor.getActiveCameraSnapshot ? visualEditor.getActiveCameraSnapshot() : null;
  const cameraId = snap
    ? snap.name
    : String(el.camera && el.camera.value ? el.camera.value : "").trim();
  if (!cameraId) throw new Error("no active camera");

  const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!sceneName) throw new Error("no active scene");

  let sourceText = String(el.sceneSource && el.sceneSource.value ? el.sceneSource.value : "");
  if (!sourceText.trim()) {
    await loadSceneSource(sceneName);
    sourceText = String(el.sceneSource && el.sceneSource.value ? el.sceneSource.value : "");
  }
  if (!sourceText.trim()) throw new Error("scene source is empty");

  const parseNum = (inp) => { const v = Number(inp && inp.value); return Number.isFinite(v) ? v : null; };
  const posX = parseNum(el.visualCamPosX), posY = parseNum(el.visualCamPosY), posZ = parseNum(el.visualCamPosZ);
  const tgtX = parseNum(el.visualCamTgtX), tgtY = parseNum(el.visualCamTgtY), tgtZ = parseNum(el.visualCamTgtZ);
  const flength = parseNum(el.visualCamFLength);
  const fov = parseNum(el.visualCamFov);

  const params = {};
  if (posX !== null && posY !== null && posZ !== null) params.position = [posX, posY, posZ];
  if (tgtX !== null && tgtY !== null && tgtZ !== null) params.target = [tgtX, tgtY, tgtZ];
  if (flength !== null) params.flength = flength;
  if (fov !== null) params.fov = fov;

  const nextSource = updateCameraInSource(sourceText, cameraId, params);
  updateSceneSourceText(nextSource, { history: "visual" });
  await api.saveScene(sceneName, nextSource, true);
  appendLog(`camera ${cameraId} updated`);
  await loadVisualSceneFromSelected();
}

async function addCameraFromEditorToScene() {
  const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!sceneName) throw new Error("no active scene");

  let sourceText = String(el.sceneSource && el.sceneSource.value ? el.sceneSource.value : "");
  if (!sourceText.trim()) {
    await loadSceneSource(sceneName);
    sourceText = String(el.sceneSource && el.sceneSource.value ? el.sceneSource.value : "");
  }
  if (!sourceText.trim()) throw new Error("scene source is empty");

  const parseNum = (inp) => { const v = Number(inp && inp.value); return Number.isFinite(v) ? v : null; };
  const posX = parseNum(el.visualCamPosX), posY = parseNum(el.visualCamPosY), posZ = parseNum(el.visualCamPosZ);
  const tgtX = parseNum(el.visualCamTgtX), tgtY = parseNum(el.visualCamTgtY), tgtZ = parseNum(el.visualCamTgtZ);
  const flength = parseNum(el.visualCamFLength);
  const fov = parseNum(el.visualCamFov);

  const next = addInteractiveCameraToSceneSource(sourceText, {
    baseName: "camera",
    position: (posX !== null && posY !== null && posZ !== null) ? [posX, posY, posZ] : undefined,
    target: (tgtX !== null && tgtY !== null && tgtZ !== null) ? [tgtX, tgtY, tgtZ] : undefined,
    hfov: fov !== null ? fov : undefined,
  });

  if (flength !== null) {
    const model = parseSceneEditModel(next.source);
    const cam = (model.cameras || []).find((c) => c.id === next.cameraId);
    if (cam) {
      const entryIndent = `${geometryEntryIndent(next.source, cam.entryStart)}\t`;
      const body = upsertSceneScalarProp(next.source.slice(cam.bodyStart, cam.bodyEnd), "flength", flength, entryIndent);
      next.source = `${next.source.slice(0, cam.bodyStart)}${body}${next.source.slice(cam.bodyEnd)}`;
    }
  }

  updateSceneSourceText(next.source, { history: "visual" });
  await api.saveScene(sceneName, next.source, true);

  const variantName = selectedSceneVariantValue();
  await loadCameras(sceneName, variantName);
  if (cameraCatalogHasName(next.cameraId)) {
    el.camera.value = next.cameraId;
    setCameraBrowserSelectedCamera(next.cameraId);
    syncVisualCameraFromRenderSelection();
    lastStableSelection.camera = next.cameraId;
  }
  if (hasBackendMethod(api, "getSceneRuntimeGraph")) {
    await loadSceneRuntimeGraph(sceneName, variantName).catch(() => null);
  }
  await loadVisualSceneFromSelected();
  appendLog(`camera ${next.cameraId} created`);
}

function upgradeLegacyStatMarkup() {
  const widgets = window.XTracerWidgets || {};
  if (typeof widgets.renderStatHint !== "function") return;
  document.querySelectorAll(".workspace-active-hint").forEach((node) => {
    if (!(node instanceof HTMLElement)) return;
    const labelNode = node.querySelector(".workspace-active-label, .xui-stat__label");
    const valueNode = node.querySelector(".workspace-active-value, .xui-stat__value");
    if (valueNode && valueNode.id) return;
    let label = labelNode ? String(labelNode.textContent || "").trim() : "";
    let value = valueNode ? String(valueNode.textContent || "").trim() : "";
    if (!label) {
      const raw = String(node.textContent || "").trim();
      const split = raw.indexOf(":");
      if (split >= 0) {
        label = raw.slice(0, split).trim();
        value = raw.slice(split + 1).trim();
      } else {
        label = raw;
      }
    }
    widgets.renderStatHint(node, { label, value: value || "-" });
  });
}

function adoptStaticWidgetMarkup() {
  upgradeLegacyStatMarkup();
  if (window.XTracerWidgets && typeof window.XTracerWidgets.enhanceSelects === "function") {
    window.XTracerWidgets.enhanceSelects(document);
  }
}

async function boot() {
  ensureClientId();
  api = initializeBackendApi();
  adoptStaticWidgetMarkup();
  if (typeof bindSettingsJobsCardLifecycle === "function") bindSettingsJobsCardLifecycle();
  renderSceneLoadStatus();
  const hasWorkspaceApi = hasBackendMethod(api, "getWorkspaces");
  const startupPhases = [
    "Preparing layout",
    "Restoring appearance",
    ...(hasWorkspaceApi ? ["Syncing workspaces"] : []),
    "Loading scenes",
    "Loading integrators",
    "Loading post filters",
    "Loading presets",
    "Resolving startup scene",
    "Loading runtime details",
  ];
  resetStartupProgress(startupPhases.length, startupPhases);
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
  setWorkspaceSortMode(localStorage.getItem(WORKSPACE_SORT_MODE_KEY) || "name", false);
  pollBackendLogs();
  if (typeof startJobEventsWebSocket === "function") startJobEventsWebSocket();
  advanceStartupProgress(1);

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
    trackStartupRequest(loadPostFilters()),
    trackStartupRequest(loadResolutionPresets()),
  ]);
  const tryLoadSceneBundle = async (sceneName) => {
    const name = String(sceneName || "").trim();
    if (!name) return false;
    await loadVariants(name);
    const variantName = selectedSceneVariantValue();
    const tasks = [
      loadCameras(name, variantName),
      loadSceneSource(name),
      loadSceneRuntimeGraph(name, variantName).catch(() => null),
    ];
    await Promise.all(tasks);
    return true;
  };

  const activeWorkspaceScene = () => {
    const snapshot = activeWorkspaceId ? workspaceSnapshotById.get(activeWorkspaceId) : null;
    return String((snapshot && snapshot.active_scene) || "").trim();
  };

  const chooseStartupSceneCandidates = () => {
    const seen = new Set();
    const out = [];
    const push = (value) => {
      const name = String(value || "").trim();
      if (!name || seen.has(name) || !sceneCatalogHasFile(name)) return;
      seen.add(name);
      out.push(name);
    };
    push(activeWorkspaceScene());
    push(el.scene && el.scene.value ? el.scene.value : "");
    sceneCatalog.forEach((item) => push(item && item.sceneFile ? item.sceneFile : ""));
    return out;
  };

  let loadedStartupScene = "";
  await trackStartupRequest((async () => {
    const candidates = chooseStartupSceneCandidates();
    let loaded = false;
    let lastError = null;
    for (let i = 0; i < candidates.length; ++i) {
      const candidate = candidates[i];
      try {
        if (String(el.scene && el.scene.value ? el.scene.value : "").trim() !== candidate) {
          el.scene.value = candidate;
          setSceneBrowserSelectedFile(candidate);
          localStorage.setItem(LAST_SCENE_KEY, candidate);
          updateSceneDependencyPill(candidate);
        }
        await tryLoadSceneBundle(candidate);
        loadedStartupScene = candidate;
        loaded = true;
        break;
      } catch (err) {
        lastError = err;
        appendLog(`startup scene load error (${candidate}): ${err.message}`);
      }
    }
    if (!loaded && lastError) {
      setSceneLoadStatus("error", lastError.message || "Scene load failed.", "");
      setStatus(`error: ${lastError.message || "scene load failed"}`);
    }
  })());
  if (hasWorkspaceApi && activeWorkspaceId) {
    await applyActiveWorkspaceState(workspaceSnapshotById.get(activeWorkspaceId) || null, {
      skipSceneReloadIfCurrent: loadedStartupScene,
    })
      .catch((err) => appendLog(`workspace restore error: ${err.message}`));
  }
  if (!activeJobId && typeof restorePreviewForActiveWorkspace === "function") {
    await restorePreviewForActiveWorkspace()
      .catch((err) => appendLog(`startup preview restore error: ${err.message}`));
  }
  await trackStartupRequest(loadAbout());
  updatePreviewSizing();
  bindPreviewInteraction();
  bindGraphInteraction();
  applyPreviewSampling();
  const hasActiveRender = !!String(activeJobId || "").trim();
  const hasRestoredCompletedRender = !!String(lastCompletedJobId || "").trim();
  if (!hasActiveRender && !hasRestoredCompletedRender) {
    setPreviewEmptyState(true);
  }
  if (!hasActiveRender) {
    setRenderActive(false);
    if (!el.scene.value) {
      setStatus("no scenes found in scene/ directory");
      setSceneLoadStatus("idle", "No scenes found in scene/ directory.", "");
    } else {
      setStatus("idle");
      setSceneLoadStatus("idle", "Ready.", "");
    }
  } else {
    setSceneLoadStatus("idle", "Ready.", "");
  }

  if (typeof syncRenderTabEnabled === "function") syncRenderTabEnabled();
  setActiveTab(localStorage.getItem(ACTIVE_TAB_KEY) || "scene");
  if (el.visualViewport && window.SceneVisualEditor) {
    visualEditor = new window.SceneVisualEditor(
      el.visualViewport,
      el.visualSelectionTag || null,
      async (sceneName) => {
        if (!hasBackendMethod(api, "getSceneGeometry")) throw new Error("geometry endpoint unavailable");
        return api.getSceneGeometry(sceneName, selectedSceneVariantValue());
      },
      async (sceneName) => {
        if (!hasBackendMethod(api, "getSceneRuntimeGraph")) throw new Error("runtime graph endpoint unavailable");
        return api.getSceneRuntimeGraph(sceneName, selectedSceneVariantValue());
      },
      async (sceneName, relpath) => {
        if (!hasBackendMethod(api, "getSceneAssetText")) throw new Error("asset endpoint unavailable");
        return api.getSceneAssetText(sceneName, relpath);
      }
    );
    if (visualEditor.init()) {
      if (el.visualOverlayBtns) el.visualViewport.appendChild(el.visualOverlayBtns);
      if (el.visualCameraBar) el.visualViewport.appendChild(el.visualCameraBar);
      if (el.visualCameraEditorPanel) el.visualViewport.appendChild(el.visualCameraEditorPanel);
      if (el.visualScaleBar) el.visualViewport.appendChild(el.visualScaleBar);
      if (el.visualInfoBtn) el.visualViewport.appendChild(el.visualInfoBtn);
      if (el.visualControlsOverlay) el.visualViewport.appendChild(el.visualControlsOverlay);
      syncVisualFrameAspect();
      if (visualEditor.setProjectionMode) {
        const initMode = (el.visualProjectionIsometric && el.visualProjectionIsometric.getAttribute("aria-pressed") === "true") ? "isometric" : "perspective";
        visualEditor.setProjectionMode(initMode);
      }
      if (el.visualSceneScale && visualEditor.setSceneScaleMultiplier) {
        visualEditor.setSceneScaleMultiplier(uiOptions.visualSceneScale, false);
      }
      if (el.visualShowGlobalBvh && visualEditor.setGlobalBvhVisible) {
        visualEditor.setGlobalBvhVisible(el.visualShowGlobalBvh.getAttribute("aria-pressed") === "true");
      }
      if (el.visualShowMeshBvh && visualEditor.setMeshBvhVisible) {
        visualEditor.setMeshBvhVisible(el.visualShowMeshBvh.getAttribute("aria-pressed") === "true");
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
  if (el.visualOverlayBtns) {
    el.visualOverlayBtns.addEventListener("pointerdown", (evt) => evt.stopPropagation());
  }
  if (el.visualCameraBar) {
    el.visualCameraBar.addEventListener("pointerdown", (evt) => evt.stopPropagation());
  }
  if (el.visualCameraEditBtn && el.visualCameraEditorPanel) {
    el.visualCameraEditorPanel.addEventListener("pointerdown", (evt) => evt.stopPropagation());
    el.visualCameraEditBtn.addEventListener("click", () => {
      const open = !el.visualCameraEditorPanel.hidden;
      el.visualCameraEditorPanel.hidden = open;
      el.visualCameraEditBtn.setAttribute("aria-expanded", open ? "false" : "true");
      el.visualCameraEditBtn.classList.toggle("is-active", !open);
    });
  }
  if (el.visualScaleBar) {
    el.visualScaleBar.addEventListener("pointerdown", (evt) => evt.stopPropagation());
  }
  if (el.visualInfoBtn && el.visualControlsOverlay) {
    el.visualInfoBtn.addEventListener("pointerdown", (evt) => evt.stopPropagation());
    el.visualControlsOverlay.addEventListener("pointerdown", (evt) => evt.stopPropagation());
    el.visualInfoBtn.addEventListener("click", (evt) => {
      evt.stopPropagation();
      const open = !el.visualControlsOverlay.hidden;
      el.visualControlsOverlay.hidden = open;
      el.visualInfoBtn.setAttribute("aria-expanded", open ? "false" : "true");
      el.visualInfoBtn.classList.toggle("is-active", !open);
    });
    document.addEventListener("pointerdown", (evt) => {
      if (evt.target instanceof Element && evt.target.closest("#visualInfoBtn, #visualControlsOverlay")) return;
      if (!el.visualControlsOverlay.hidden) {
        el.visualControlsOverlay.hidden = true;
        el.visualInfoBtn.setAttribute("aria-expanded", "false");
        el.visualInfoBtn.classList.remove("is-active");
      }
    }, { capture: true });
  }
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
      refreshVisualCameraEditorPanel();
    });
  }
  if (el.visualCameraSnapBtn) {
    el.visualCameraSnapBtn.addEventListener("click", () => {
      if (!visualEditor || !visualEditor.getViewportPose) return;
      const pose = visualEditor.getViewportPose();
      if (!pose) return;
      const fmt = (v) => String(Math.round(v * 1e6) / 1e6);
      if (el.visualCamPosX) el.visualCamPosX.value = fmt(pose.position[0]);
      if (el.visualCamPosY) el.visualCamPosY.value = fmt(pose.position[1]);
      if (el.visualCamPosZ) el.visualCamPosZ.value = fmt(pose.position[2]);
      if (el.visualCamTgtX) el.visualCamTgtX.value = fmt(pose.target[0]);
      if (el.visualCamTgtY) el.visualCamTgtY.value = fmt(pose.target[1]);
      if (el.visualCamTgtZ) el.visualCamTgtZ.value = fmt(pose.target[2]);
    });
  }
  if (el.visualCameraApplyBtn) {
    el.visualCameraApplyBtn.addEventListener("click", () => {
      applyCameraEditorChanges().catch((err) => {
        appendLog(`camera apply error: ${err.message}`);
      });
    });
  }
  if (el.visualCameraNewBtn) {
    el.visualCameraNewBtn.addEventListener("click", () => {
      addCameraFromEditorToScene().catch((err) => {
        appendLog(`camera new error: ${err.message}`);
      });
    });
  }
  const setProjection = (mode) => {
    if (visualEditor && visualEditor.setProjectionMode) visualEditor.setProjectionMode(mode);
    const isPerspective = mode === "perspective";
    if (el.visualProjectionPerspective) {
      el.visualProjectionPerspective.setAttribute("aria-pressed", String(isPerspective));
      el.visualProjectionPerspective.classList.toggle("is-active", isPerspective);
    }
    if (el.visualProjectionIsometric) {
      el.visualProjectionIsometric.setAttribute("aria-pressed", String(!isPerspective));
      el.visualProjectionIsometric.classList.toggle("is-active", !isPerspective);
    }
    appendLog(`visual projection=${mode}`);
  };
  if (el.visualProjectionPerspective) {
    el.visualProjectionPerspective.addEventListener("click", () => setProjection("perspective"));
  }
  if (el.visualProjectionIsometric) {
    el.visualProjectionIsometric.addEventListener("click", () => setProjection("isometric"));
  }
  if (el.visualSceneScale) {
    el.visualSceneScale.addEventListener("change", () => {
      const next = clampVisualSceneScale(el.visualSceneScale.value || "1");
      uiOptions.visualSceneScale = next;
      el.visualSceneScale.value = String(next);
      persistUIOptions();
      if (visualEditor && visualEditor.setSceneScaleMultiplier) {
        visualEditor.setSceneScaleMultiplier(next, true);
      }
      appendLog(`visual scene scale=${next}`);
    });
  }
  if (el.visualShowGrid) {
    el.visualShowGrid.addEventListener("click", () => {
      const next = el.visualShowGrid.getAttribute("aria-pressed") !== "true";
      el.visualShowGrid.setAttribute("aria-pressed", String(next));
      el.visualShowGrid.classList.toggle("is-active", next);
      if (visualEditor && visualEditor.setGridVisible) visualEditor.setGridVisible(next);
      appendLog(`visual grid=${next ? "on" : "off"}`);
    });
    if (visualEditor && visualEditor.setGridVisible) visualEditor.setGridVisible(el.visualShowGrid.getAttribute("aria-pressed") === "true");
  }
  if (el.visualShowGlobalBvh) {
    el.visualShowGlobalBvh.addEventListener("click", () => {
      const next = el.visualShowGlobalBvh.getAttribute("aria-pressed") !== "true";
      el.visualShowGlobalBvh.setAttribute("aria-pressed", String(next));
      el.visualShowGlobalBvh.classList.toggle("is-active", next);
      if (visualEditor && visualEditor.setGlobalBvhVisible) visualEditor.setGlobalBvhVisible(next);
      appendLog(`visual global bvh=${next ? "on" : "off"}`);
    });
    if (visualEditor && visualEditor.setGlobalBvhVisible) visualEditor.setGlobalBvhVisible(el.visualShowGlobalBvh.getAttribute("aria-pressed") === "true");
  }
  if (el.visualShowMeshBvh) {
    el.visualShowMeshBvh.addEventListener("click", () => {
      const next = el.visualShowMeshBvh.getAttribute("aria-pressed") !== "true";
      el.visualShowMeshBvh.setAttribute("aria-pressed", String(next));
      el.visualShowMeshBvh.classList.toggle("is-active", next);
      if (visualEditor && visualEditor.setMeshBvhVisible) visualEditor.setMeshBvhVisible(next);
      appendLog(`visual mesh bvh=${next ? "on" : "off"}`);
    });
    if (visualEditor && visualEditor.setMeshBvhVisible) visualEditor.setMeshBvhVisible(el.visualShowMeshBvh.getAttribute("aria-pressed") === "true");
  }
  if (el.resetViewBtn) {
    el.resetViewBtn.addEventListener("click", () => {
      resetPreviewView();
      appendLog("preview view reset");
    });
  }
  if (el.interactivePreviewSaveCameraBtn) {
    el.interactivePreviewSaveCameraBtn.addEventListener("click", () => {
      saveInteractiveCameraToScene()
        .catch((err) => {
          setStatus(`error: ${err.message}`);
          setEditorOpStatus("error", `Save camera failed: ${err.message}`);
          appendLog(`interactive camera save error: ${err.message}`);
        });
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

  const lastStableSelection = {
    scene: String(el.scene && el.scene.value ? el.scene.value : "").trim(),
    variant: selectedSceneVariantValue(),
    camera: String(el.camera && el.camera.value ? el.camera.value : "").trim(),
  };

  const restoreStableSelection = async () => {
    const sceneName = String(lastStableSelection.scene || "").trim();
    if (!sceneName || !sceneCatalogHasFile(sceneName)) return;

    if (String(el.scene && el.scene.value ? el.scene.value : "").trim() !== sceneName) {
      el.scene.value = sceneName;
    }
    setSceneBrowserSelectedFile(sceneName);
    updateSceneDependencyPill(sceneName);
    localStorage.setItem(LAST_SCENE_KEY, sceneName);

    const variantName = normalizeVariantName(lastStableSelection.variant);
    await loadVariants(sceneName, variantName);
    const tasks = [loadCameras(sceneName, variantName)];
    if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(sceneName, variantName));
    if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(sceneName));
    await Promise.all(tasks);

    const cameraName = String(lastStableSelection.camera || "").trim();
    if (cameraName && cameraCatalogHasName(cameraName)) {
      el.camera.value = cameraName;
      setCameraBrowserSelectedCamera(cameraName);
      syncVisualCameraFromRenderSelection();
    }
  };

  const saveInteractiveCameraToScene = async () => {
    const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
    if (!sceneName) throw new Error("no active scene");
    if (!interactivePreviewCamera || !interactivePreviewCamera.ready) {
      throw new Error("interactive camera is not ready");
    }
    const sourceText = String(el.sceneSource && el.sceneSource.value ? el.sceneSource.value : "");
    if (!sourceText.trim()) throw new Error("scene source is empty");

    const baseLabel = String(interactivePreviewCamera.sourceCamera || "").trim() || "interactive_camera";
    const next = addInteractiveCameraToSceneSource(sourceText, {
      baseName: `interactive_${sanitizeSceneId(baseLabel, "camera")}`,
      position: interactivePreviewCamera.position,
      target: interactivePreviewCamera.target,
      up: interactivePreviewCamera.up,
      hfov: interactivePreviewCamera.hfov,
    });
    updateSceneSourceText(next.source, { history: "visual" });

    await api.saveScene(sceneName, next.source, true);
    await loadScenes();
    el.scene.value = sceneName;
    setSceneBrowserSelectedFile(sceneName);
    localStorage.setItem(LAST_SCENE_KEY, sceneName);
    updateSceneDependencyPill(sceneName);

    const variantName = selectedSceneVariantValue();
    await loadCameras(sceneName, variantName);
    if (cameraCatalogHasName(next.cameraId)) {
      el.camera.value = next.cameraId;
      setCameraBrowserSelectedCamera(next.cameraId);
      syncVisualCameraFromRenderSelection();
      lastStableSelection.camera = next.cameraId;
    }
    if (hasBackendMethod(api, "getSceneRuntimeGraph")) {
      await loadSceneRuntimeGraph(sceneName, variantName).catch(() => null);
    }
    if (visualEditor) await loadVisualSceneFromSelected().catch(() => null);
    if (interactivePreviewEnabled && typeof refreshInteractivePreviewCameraFromSelection === "function") {
      const ok = await refreshInteractivePreviewCameraFromSelection().catch(() => false);
      if (ok && typeof requestInteractivePreviewRender === "function") requestInteractivePreviewRender();
    }
    setStatus(`saved ${sceneName} (+camera ${next.cameraId})`);
    setEditorOpStatus("success", `Saved camera: ${next.cameraId}`);
    appendLog(`interactive camera saved: ${next.cameraId}`);
  };

  el.scene.addEventListener("change", () => {
    setSceneBrowserSelectedFile(el.scene.value);
    localStorage.setItem(LAST_SCENE_KEY, el.scene.value || "");
    updateSceneDependencyPill(el.scene.value);
    setSceneLoadStatus("loading", `Loading ${el.scene.value || "scene"}...`, "");
    loadVariants(el.scene.value)
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
      .then(() => appendLog(`scene changed: ${el.scene.value}`))
      .then(() => {
        lastStableSelection.scene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
        lastStableSelection.variant = selectedSceneVariantValue();
        lastStableSelection.camera = String(el.camera && el.camera.value ? el.camera.value : "").trim();
        setSceneLoadStatus("idle", `Loaded ${el.scene.value || "scene"}.`, "");
        if (typeof syncRenderTabEnabled === "function") syncRenderTabEnabled();
        if (interactivePreviewEnabled && typeof refreshInteractivePreviewCameraFromSelection === "function") {
          refreshInteractivePreviewCameraFromSelection()
            .then((ok) => {
              if (ok && typeof requestInteractivePreviewRender === "function") requestInteractivePreviewRender();
            })
            .catch(() => {});
        }
      })
      .catch((err) => {
        setSceneLoadStatus("error", err.message || "Scene change failed.", "");
        setStatus(`error: ${err.message}`);
        appendLog(`scene change error: ${err.message}`);
        restoreStableSelection().catch((restoreErr) => {
          appendLog(`scene recovery error: ${restoreErr.message}`);
        });
      });
  });

  if (el.variant) {
    el.variant.addEventListener("change", () => {
      setVariantBrowserSelectedVariant(el.variant.value);
      const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
      const variantName = selectedSceneVariantValue();
      setSceneLoadStatus("loading", `Loading ${sceneName || "scene"} (${variantName || "base"})...`, "");
      const tasks = [loadCameras(sceneName, variantName)];
      if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(sceneName, variantName));
      Promise.all(tasks)
        .then(() => {
          reconcileCameraCatalogFromRuntimeGraph(sceneName, variantName);
          if (visualEditor) return loadVisualSceneFromSelected();
          return null;
        })
        .then(() => {
          setSceneLoadStatus("idle", `Loaded ${sceneName || "scene"} (${variantName || "base"}).`, "");
          appendLog(`variant changed: ${variantName || "(base)"}`);
          lastStableSelection.scene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
          lastStableSelection.variant = selectedSceneVariantValue();
          lastStableSelection.camera = String(el.camera && el.camera.value ? el.camera.value : "").trim();
          if (interactivePreviewEnabled && typeof refreshInteractivePreviewCameraFromSelection === "function") {
            refreshInteractivePreviewCameraFromSelection()
              .then((ok) => {
                if (ok && typeof requestInteractivePreviewRender === "function") requestInteractivePreviewRender();
              })
              .catch(() => {});
          }
        })
        .catch((err) => {
          setSceneLoadStatus("error", err.message || "Variant change failed.", "");
          setStatus(`error: ${err.message}`);
          appendLog(`variant change error: ${err.message}`);
          restoreStableSelection().catch((restoreErr) => {
            appendLog(`variant recovery error: ${restoreErr.message}`);
          });
        });
    });
  }

  el.camera.addEventListener("change", () => {
    setCameraBrowserSelectedCamera(el.camera.value);
    syncVisualCameraFromRenderSelection();
    if (typeof refreshVisualCameraEditorPanel === "function") refreshVisualCameraEditorPanel();
    lastStableSelection.camera = String(el.camera && el.camera.value ? el.camera.value : "").trim();
    if (interactivePreviewEnabled && typeof refreshInteractivePreviewCameraFromSelection === "function") {
      refreshInteractivePreviewCameraFromSelection()
        .then((ok) => {
          if (ok && typeof requestInteractivePreviewRender === "function") requestInteractivePreviewRender();
        })
        .catch(() => {});
    }
  });

  el.theme.addEventListener("change", () => {
    applyTheme(el.theme.value);
    refreshPaletteOptions();
    persistUIOptions();
    appendLog(`theme=${el.theme.value}`);
  });

  if (el.themeToggle) {
    el.themeToggle.addEventListener("click", () => {
      const order = ["system", "light", "dark"];
      const current = String(el.theme && el.theme.value ? el.theme.value : localStorage.getItem("xtracer-theme") || "system").toLowerCase();
      const next = order[(Math.max(order.indexOf(current), 0) + 1) % order.length];
      if (el.theme) el.theme.value = next;
      applyTheme(next);
      refreshPaletteOptions();
      persistUIOptions();
      appendLog(`theme=${next}`);
    });
  }

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
        refreshThemeToggleButton();
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
      if (typeof syncTileSizeControlUi === "function") syncTileSizeControlUi();
      appendLog(`tile_size=${el.tileSize.value}`);
      queueWorkspaceSettingsSave();
    });
    if (typeof syncTileSizeControlUi === "function") syncTileSizeControlUi();
  }
  if (el.threads) {
    el.threads.addEventListener("change", () => {
      if (typeof syncTileSizeControlUi === "function") syncTileSizeControlUi();
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
  updatePostFilterUiState();
  if (typeof renderPostPipelineGraph === "function") renderPostPipelineGraph();
  if (el.postFiltersEnabled) {
    el.postFiltersEnabled.checked = !!postFilterStackEnabled;
    el.postFiltersEnabled.addEventListener("change", () => {
      postFilterStackEnabled = !!el.postFiltersEnabled.checked;
      updatePostFilterUiState();
      appendLog(`post_filter stack=${postFilterStackEnabled ? "on" : "off"}`);
      refreshPreviewForToneMapping();
      queueWorkspaceSettingsSave();
    });
  }
  if (el.postFilterAddBtn) {
    el.postFilterAddBtn.addEventListener("click", () => {
      addPostFilterToChain();
    });
  }
  if (el.postFiltersRecalcBtn) {
    el.postFiltersRecalcBtn.addEventListener("click", () => {
      appendLog("post_filter recalculate");
      refreshPreviewForToneMapping();
    });
  }

  const onSizeChanged = () => {
    syncResolutionPresetFromInputs();
    updatePreviewSizing();
    syncVisualFrameAspect();
    if (typeof syncTileSizeControlUi === "function") syncTileSizeControlUi();
    queueWorkspaceSettingsSave();
  };
  [
    { mode: "all", node: el.resolutionModeFilterAll },
    { mode: "square", node: el.resolutionModeFilterSquare },
    { mode: "portrait", node: el.resolutionModeFilterPortrait },
    { mode: "landscape", node: el.resolutionModeFilterLandscape },
  ].forEach((entry) => {
    if (!entry.node) return;
    entry.node.addEventListener("click", () => {
      if (typeof setResolutionPresetModeFilter === "function") {
        setResolutionPresetModeFilter(entry.mode);
      }
      if (typeof renderResolutionPresetList === "function") {
        renderResolutionPresetList();
      }
    });
  });
  el.resolutionPreset.addEventListener("change", () => {
    if (typeof renderResolutionPresetList === "function") renderResolutionPresetList();
    if (el.resolutionPreset.value === "custom") return;
    const index = parseInt(el.resolutionPreset.value, 10);
    if (!Number.isFinite(index) || index < 0 || index >= resolutionPresets.length) return;
    const preset = resolutionPresets[index];
    el.width.value = String(preset.width);
    el.height.value = String(preset.height);
    updatePreviewSizing();
    syncVisualFrameAspect();
    if (typeof syncTileSizeControlUi === "function") syncTileSizeControlUi();
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
    if (typeof syncMobileLogsViewport === "function") {
      requestAnimationFrame(() => {
        syncMobileLogsViewport();
      });
    }
    if (persistentSidebar && !isMobileTabMenuViewport()) {
      persistentSidebar.style.transform = "";
      persistentSidebar.style.transition = "";
      persistentSidebar.classList.remove("is-sheet-open");
      if (el.sheetBackdrop) el.sheetBackdrop.classList.remove("is-open");
    }
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
  if (el.renderMode) {
    el.renderMode.value = normalizeRenderMode(renderMode);
    el.renderMode.addEventListener("change", () => {
      const next = normalizeRenderMode(el.renderMode.value);
      if (typeof setRenderMode === "function") {
        setRenderMode(next, { log: true })
          .then(() => queueWorkspaceSettingsSave())
          .catch(() => {
            if (el.renderMode) el.renderMode.value = normalizeRenderMode(renderMode);
          });
      }
    });
  }
  if (el.interactivePreviewSpeed) {
    const clampInteractiveSpeed = (v) => Math.max(0.2, Math.min(5.0, Number(v) || 1.0));
    const applyInteractiveSpeed = (raw, logIt) => {
      const speed = clampInteractiveSpeed(raw);
      interactivePreviewFlySpeedScale = speed;
      el.interactivePreviewSpeed.value = String(speed.toFixed(1));
      if (el.interactivePreviewSpeedValue) el.interactivePreviewSpeedValue.textContent = `${speed.toFixed(1)}x`;
      if (typeof renderInteractivePreviewHud === "function") renderInteractivePreviewHud();
      if (logIt) appendLog(`interactive speed=${speed.toFixed(1)}x`);
    };
    applyInteractiveSpeed(interactivePreviewFlySpeedScale, false);
    el.interactivePreviewSpeed.addEventListener("input", () => {
      applyInteractiveSpeed(el.interactivePreviewSpeed.value, false);
    });
    el.interactivePreviewSpeed.addEventListener("change", () => {
      applyInteractiveSpeed(el.interactivePreviewSpeed.value, true);
      queueWorkspaceSettingsSave();
    });
  }
  if (typeof setRenderMode === "function") {
    setRenderMode(renderMode, { log: false }).catch(() => {});
  }
  if (el.tileHeatmapEnabled) {
    el.tileHeatmapEnabled.addEventListener("change", () => {
      uiOptions.tileHeatmapEnabled = !!el.tileHeatmapEnabled.checked;
      persistUIOptions();
      applyPreviewTransform();
      renderTileHeatmapStats();
      appendLog(`tile heatmap=${uiOptions.tileHeatmapEnabled ? "on" : "off"}`);
    });
  }

if (el.tabScene) el.tabScene.addEventListener("click", () => setActiveTab("scene"));
  el.tabRender.addEventListener("click", () => setActiveTab("render"));
  el.tabVisual.addEventListener("click", () => setActiveTab("visual"));
  if (el.tabWorkspaces) el.tabWorkspaces.addEventListener("click", () => setActiveTab("workspaces"));
  if (el.tabGallery) el.tabGallery.addEventListener("click", () => setActiveTab("gallery"));
  if (el.tabSettings) el.tabSettings.addEventListener("click", () => setActiveTab("settings"));
  if (el.tabAbout) el.tabAbout.addEventListener("click", () => setActiveTab("about"));
  if (el.tabLogs) el.tabLogs.addEventListener("click", () => setActiveTab("logs"));
  if (el.bnTabScene) el.bnTabScene.addEventListener("click", () => setActiveTab("scene"));
  if (el.bnTabRender) el.bnTabRender.addEventListener("click", () => setActiveTab("render"));
  if (el.bnTabWorkspaces) el.bnTabWorkspaces.addEventListener("click", () => setActiveTab("workspaces"));
  if (el.bnTabVisual) el.bnTabVisual.addEventListener("click", () => setActiveTab("visual"));
  if (el.bnTabGallery) el.bnTabGallery.addEventListener("click", () => setActiveTab("gallery"));
  if (el.bnTabLogs) el.bnTabLogs.addEventListener("click", () => setActiveTab("logs"));
  if (el.bnTabSettings) el.bnTabSettings.addEventListener("click", () => setActiveTab("settings"));
  if (el.bnTabAbout) el.bnTabAbout.addEventListener("click", () => setActiveTab("about"));

  const controlsFlyoutBackdrop = document.getElementById("controlsFlyoutBackdrop");
  const appShell = document.querySelector(".app-shell");

  function setControlsPanelOpen(open) {
    if (!appShell) return;
    appShell.classList.toggle("is-controls-open", open);
    if (controlsFlyoutBackdrop) controlsFlyoutBackdrop.classList.toggle("is-open", open);
    if (el.controlsPanelToggle) {
      el.controlsPanelToggle.setAttribute("aria-expanded", open ? "true" : "false");
      el.controlsPanelToggle.setAttribute("aria-label", open ? "Close controls" : "Open controls");
      el.controlsPanelToggle.setAttribute("title", open ? "Close controls" : "Open controls");
    }
  }

  if (el.controlsPanelToggle) {
    el.controlsPanelToggle.addEventListener("click", () => {
      const isOpen = appShell && appShell.classList.contains("is-controls-open");
      setControlsPanelOpen(!isOpen);
    });
  }

  if (el.topbarLogsBtn)   el.topbarLogsBtn.addEventListener("click",   () => setActiveTab("logs"));
  if (el.topbarConfigBtn) el.topbarConfigBtn.addEventListener("click", () => setActiveTab("settings"));
  if (el.topbarAboutBtn)  el.topbarAboutBtn.addEventListener("click",  () => setActiveTab("about"));

  if (controlsFlyoutBackdrop) {
    controlsFlyoutBackdrop.addEventListener("click", () => setControlsPanelOpen(false));
  }

  const persistentSidebar = document.querySelector(".persistent-sidebar");
  const SHEET_PEEK_PX = 28;
  const SHEET_EASE = "transform 0.32s cubic-bezier(0.4, 0, 0.2, 1)";

  function sheetPeekY() {
    // offsetHeight can be 0 before first paint; fall back to 72vh (CSS max-height)
    const h = persistentSidebar.offsetHeight || Math.round(window.innerHeight * 0.72);
    return h - SHEET_PEEK_PX;
  }
  function openControlsSheet() {
    if (!isMobileTabMenuViewport()) return;
    if (persistentSidebar && persistentSidebar.classList.contains("has-no-sidebar-cards")) return;
    if (persistentSidebar) {
      persistentSidebar.style.transition = SHEET_EASE;
      persistentSidebar.style.transform = "translateY(0)";
      persistentSidebar.classList.add("is-sheet-open");
    }
    if (el.sheetBackdrop) el.sheetBackdrop.classList.add("is-open");
  }
  function closeControlsSheet() {
    if (!isMobileTabMenuViewport()) return;
    if (persistentSidebar) {
      persistentSidebar.style.transition = SHEET_EASE;
      persistentSidebar.style.transform = `translateY(${sheetPeekY()}px)`;
      persistentSidebar.classList.remove("is-sheet-open");
    }
    if (el.sheetBackdrop) el.sheetBackdrop.classList.remove("is-open");
  }
  if (el.sheetBackdrop) el.sheetBackdrop.addEventListener("click", closeControlsSheet);
  document.addEventListener("sheet:close", () => {
    if (el.sheetBackdrop) el.sheetBackdrop.classList.remove("is-open");
    if (persistentSidebar) persistentSidebar.classList.remove("is-sheet-open");
    setControlsPanelOpen(false);
  });
  document.addEventListener("keydown", (event) => {
    if (event.key !== "Escape") return;
    if (el.sheetBackdrop && el.sheetBackdrop.classList.contains("is-open")) closeControlsSheet();
    if (appShell && appShell.classList.contains("is-controls-open")) setControlsPanelOpen(false);
  });

  // Tap the sheet handle to open when closed.
  const sheetHandle = document.querySelector(".sheet-handle");
  if (sheetHandle) {
    sheetHandle.addEventListener("click", () => {
      if (!persistentSidebar || persistentSidebar.classList.contains("is-sheet-open")) return;
      openControlsSheet();
    });
  }


  // Drag-to-open / drag-to-close bottom sheet.
  // Attach to the sidebar itself so the full visible peek strip is a hit target,
  // not just the 4px handle pill. Move/end listeners go on document so the
  // finger can travel outside the element without losing the gesture.
  if (persistentSidebar) {
    let dragging = false;
    let startTouchY = 0;
    let startTranslateY = 0;
    let lastTouchY = 0;
    let lastTouchTime = 0;
    let velocity = 0;

    function currentTranslateY() {
      const inline = persistentSidebar.style.transform;
      if (inline && inline !== "none") return new DOMMatrix(inline).m42;
      const t = getComputedStyle(persistentSidebar).transform;
      if (t && t !== "none") return new DOMMatrix(t).m42;
      return sheetPeekY();
    }

    persistentSidebar.addEventListener("touchstart", (e) => {
      if (!isMobileTabMenuViewport()) return;
      if (persistentSidebar.classList.contains("has-no-sidebar-cards")) return;
      const isOpen = persistentSidebar.classList.contains("is-sheet-open");
      // When open, only drag from the top 48px (handle zone) to close
      if (isOpen) {
        const rect = persistentSidebar.getBoundingClientRect();
        if (e.touches[0].clientY > rect.top + 48) return;
      }
      dragging = true;
      startTouchY = e.touches[0].clientY;
      lastTouchY = startTouchY;
      lastTouchTime = Date.now();
      velocity = 0;
      startTranslateY = currentTranslateY();
      persistentSidebar.style.transition = "none";
    }, { passive: true });

    document.addEventListener("touchmove", (e) => {
      if (!dragging) return;
      const y = e.touches[0].clientY;
      const now = Date.now();
      const dt = now - lastTouchTime;
      if (dt > 0) velocity = (y - lastTouchY) / dt;
      lastTouchY = y;
      lastTouchTime = now;
      const raw = startTranslateY + (y - startTouchY);
      persistentSidebar.style.transform = `translateY(${Math.max(0, Math.min(sheetPeekY(), raw))}px)`;
      e.preventDefault();
    }, { passive: false });

    function onTouchEnd() {
      if (!dragging) return;
      dragging = false;
      const finalY = startTranslateY + (lastTouchY - startTouchY);
      const draggedUp = startTranslateY - finalY; // positive = moved toward open
      // Open on fast upward flick OR any 60px+ upward drag from closed position
      const open = velocity < -0.3 || draggedUp >= 60;
      if (open) openControlsSheet(); else closeControlsSheet();
    }

    document.addEventListener("touchend", onTouchEnd, { passive: true });
    document.addEventListener("touchcancel", onTouchEnd, { passive: true });
  }

  if (el.mainMenuToggle) {
    el.mainMenuToggle.addEventListener("click", () => {
      const topbar = document.querySelector(".topbar");
      const isOpen = !!(topbar && topbar.classList.contains("menu-open"));
      setMainMenuOpen(!isOpen);
    });
  }
  document.addEventListener("click", (event) => {
    if (!isMobileTabMenuViewport()) return;
    const topbar = document.querySelector(".topbar");
    if (!topbar || !topbar.classList.contains("menu-open")) return;
    const target = event.target instanceof Node ? event.target : null;
    if (target && topbar.contains(target)) return;
    setMainMenuOpen(false);
  });
  window.addEventListener("resize", () => {
    if (!isMobileTabMenuViewport()) setMainMenuOpen(false);
    if (typeof fitAboutLicenseText === "function") {
      requestAnimationFrame(() => {
        fitAboutLicenseText();
      });
    }
    if (typeof syncMobileLogsViewport === "function") {
      requestAnimationFrame(() => {
        syncMobileLogsViewport();
      });
    }
  });
  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape") setMainMenuOpen(false);
  });
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
  if (el.editorViewSamplersBtn) {
    el.editorViewSamplersBtn.addEventListener("click", () => setEditorViewMode("samplers"));
  }
  if (el.editorViewGeometryBtn) {
    el.editorViewGeometryBtn.addEventListener("click", () => setEditorViewMode("geometry"));
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
  if (el.workspaceSortNameBtn) {
    el.workspaceSortNameBtn.addEventListener("click", () => {
      setWorkspaceSortMode("name");
      refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
    });
  }
  if (el.workspaceSortUpdatedBtn) {
    el.workspaceSortUpdatedBtn.addEventListener("click", () => {
      setWorkspaceSortMode("updated");
      refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
    });
  }
  if (el.workspaceSortSceneBtn) {
    el.workspaceSortSceneBtn.addEventListener("click", () => {
      setWorkspaceSortMode("scene");
      refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
    });
  }
  if (el.workspaceCreateBtn) {
    el.workspaceCreateBtn.addEventListener("click", () => {
      if (!hasBackendMethod(api, "createWorkspace")) return;
      if (typeof showSceneSelectModal !== "function") return;
      const rawName = el.workspaceCreateName ? String(el.workspaceCreateName.value || "").trim() : "";
      showSceneSelectModal({
        onConfirm: (sceneName, variantName) => {
          api.createWorkspace(rawName)
            .then((data) => {
              const id = String((data && data.id) || "").trim();
              if (el.workspaceCreateName) el.workspaceCreateName.value = "";
              if (!id) return refreshWorkspaces();
              return switchActiveWorkspace(id).then(() => {
                if (variantName !== null) pendingVariantForNextSceneLoad = variantName;
                activateSceneFile(sceneName);
                if (typeof queueWorkspaceDraftSave === "function") queueWorkspaceDraftSave();
              });
            })
            .catch((err) => appendLog(`workspace create error: ${err.message}`));
        },
      });
    });
  }
  if (el.workspaceCreateName) {
    el.workspaceCreateName.addEventListener("keydown", (event) => {
      if (event.key !== "Enter") return;
      event.preventDefault();
      if (el.workspaceCreateBtn) el.workspaceCreateBtn.click();
    });
  }
  if (el.sceneRefreshBtn) {
    el.sceneRefreshBtn.addEventListener("click", () => {
      const currentScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
      refreshSceneCatalog(currentScene)
        .then(() => appendLog("scene list refreshed"))
        .catch((err) => {
          setSceneLoadStatus("error", err.message || "Scene refresh failed.", "");
          setStatus(`error: ${err.message}`);
          appendLog(`scene refresh error: ${err.message}`);
        });
    });
  }
  if (el.sceneSearch && typeof setSceneSearchQuery === "function") {
    el.sceneSearch.addEventListener("input", () => {
      setSceneSearchQuery(el.sceneSearch.value);
    });
    el.sceneSearch.addEventListener("keydown", (evt) => {
      if (evt.key !== "Escape") return;
      if (!el.sceneSearch.value) return;
      el.sceneSearch.value = "";
      setSceneSearchQuery("");
      evt.preventDefault();
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
  bindLogFilter(el.logFilterUi, "ui");

  el.sceneSource.addEventListener("input", () => {
    updateEditorMetrics();
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
  syncEditorScroll();
  initializeFtueTutorial();

  window.addEventListener("scene-load-warnings", (ev) => {
    const warnings = Array.isArray(ev.detail && ev.detail.warnings) ? ev.detail.warnings : [];
    warnings.forEach((msg) => {
      appendLog(`scene load warning: ${msg}`);
      if (widgets && typeof widgets.showToast === "function") {
        widgets.showToast({ message: String(msg), tone: "warning" });
      }
    });
  });
}

setTimeout(() => {
  // Safety valve: avoid a permanent loading overlay on unexpected stalls.
  dismissStartupScreen(false);
}, 15000);

boot()
  .then(() => {
    dismissStartupScreen(false);
    maybeStartFtueTutorial();
  })
  .catch((err) => {
    setStatus(`error: ${err.message}`);
    appendLog(`boot error: ${err.message}`);
    dismissStartupScreen(false);
  });
