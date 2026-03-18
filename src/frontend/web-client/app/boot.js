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
      async (sceneName) => {
        if (!hasBackendMethod(api, "getSceneRuntimeGraph")) throw new Error("runtime graph endpoint unavailable");
        return api.getSceneRuntimeGraph(sceneName);
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
    setSceneBrowserSelectedFile(el.scene.value);
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
    setCameraBrowserSelectedCamera(el.camera.value);
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
  if (el.tileHeatmapEnabled) {
    el.tileHeatmapEnabled.addEventListener("change", () => {
      uiOptions.tileHeatmapEnabled = !!el.tileHeatmapEnabled.checked;
      persistUIOptions();
      applyPreviewTransform();
      renderTileHeatmapStats();
      appendLog(`tile heatmap=${uiOptions.tileHeatmapEnabled ? "on" : "off"}`);
    });
  }

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
