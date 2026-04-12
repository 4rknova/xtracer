function upgradeLegacyStatMarkup() {
  const widgets = window.XTracerWidgets || {};
  if (typeof widgets.renderStatHint !== "function") return;
  document.querySelectorAll(".workspace-active-hint").forEach((node) => {
    if (!(node instanceof HTMLElement)) return;
    const labelNode = node.querySelector(".workspace-active-label, .xui-stat__label");
    const valueNode = node.querySelector(".workspace-active-value, .xui-stat__value");
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
      syncVisualFrameAspect();
      if (el.visualProjection && visualEditor.setProjectionMode) {
        visualEditor.setProjectionMode(el.visualProjection.value || "perspective");
      }
      if (el.visualSceneScale && visualEditor.setSceneScaleMultiplier) {
        visualEditor.setSceneScaleMultiplier(uiOptions.visualSceneScale, false);
      }
      if (el.visualShowGlobalBvh && visualEditor.setGlobalBvhVisible) {
        visualEditor.setGlobalBvhVisible(!!el.visualShowGlobalBvh.checked);
      }
      if (el.visualShowMeshBvh && visualEditor.setMeshBvhVisible) {
        visualEditor.setMeshBvhVisible(!!el.visualShowMeshBvh.checked);
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
    el.visualShowGrid.addEventListener("change", () => {
      if (visualEditor && visualEditor.setGridVisible) visualEditor.setGridVisible(!!el.visualShowGrid.checked);
      appendLog(`visual grid=${el.visualShowGrid.checked ? "on" : "off"}`);
    });
    if (visualEditor && visualEditor.setGridVisible) visualEditor.setGridVisible(!!el.visualShowGrid.checked);
  }
  if (el.visualShowGlobalBvh) {
    el.visualShowGlobalBvh.addEventListener("change", () => {
      if (visualEditor && visualEditor.setGlobalBvhVisible) visualEditor.setGlobalBvhVisible(!!el.visualShowGlobalBvh.checked);
      appendLog(`visual global bvh=${el.visualShowGlobalBvh.checked ? "on" : "off"}`);
    });
    if (visualEditor && visualEditor.setGlobalBvhVisible) visualEditor.setGlobalBvhVisible(!!el.visualShowGlobalBvh.checked);
  }
  if (el.visualShowMeshBvh) {
    el.visualShowMeshBvh.addEventListener("change", () => {
      if (visualEditor && visualEditor.setMeshBvhVisible) visualEditor.setMeshBvhVisible(!!el.visualShowMeshBvh.checked);
      appendLog(`visual mesh bvh=${el.visualShowMeshBvh.checked ? "on" : "off"}`);
    });
    if (visualEditor && visualEditor.setMeshBvhVisible) visualEditor.setMeshBvhVisible(!!el.visualShowMeshBvh.checked);
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
  });

  el.pollInterval.addEventListener("change", () => {
    const v = parseInt(el.pollInterval.value || "300", 10);
    uiOptions.pollMs = Number.isFinite(v) ? Math.max(100, Math.min(10000, v)) : 300;
    el.pollInterval.value = String(uiOptions.pollMs);
    persistUIOptions();
    appendLog(`render poll interval=${uiOptions.pollMs}ms`);
  });

  if (el.logPollActiveInterval) {
    el.logPollActiveInterval.addEventListener("change", () => {
      uiOptions.logPollActiveMs = clampLogPollMs(el.logPollActiveInterval.value || "3000", 3000, 1000, 60000);
      el.logPollActiveInterval.value = String(uiOptions.logPollActiveMs);
      persistUIOptions();
      appendLog(`logs wait interval (logs tab)=${uiOptions.logPollActiveMs}ms`);
    });
  }

  if (el.logPollBackgroundInterval) {
    el.logPollBackgroundInterval.addEventListener("change", () => {
      uiOptions.logPollBackgroundMs = clampLogPollMs(el.logPollBackgroundInterval.value || "20000", 20000, 1000, 120000);
      el.logPollBackgroundInterval.value = String(uiOptions.logPollBackgroundMs);
      persistUIOptions();
      appendLog(`logs wait interval (background)=${uiOptions.logPollBackgroundMs}ms`);
    });
  }

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
  if (el.tabGallery) el.tabGallery.addEventListener("click", () => setActiveTab("gallery"));
  el.tabSettings.addEventListener("click", () => setActiveTab("settings"));
  if (el.tabAbout) el.tabAbout.addEventListener("click", () => setActiveTab("about"));
  el.tabLogs.addEventListener("click", () => setActiveTab("logs"));
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

  setInterval(() => {
    if (activeTabMode !== "workspaces") return;
    if (!hasBackendMethod(api, "getWorkspaces")) return;
    refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
  }, 2000);

  setInterval(() => {
    if (typeof refreshSettingsJobsCard !== "function") return;
    if (typeof isJobsControlsCardVisible === "function" && !isJobsControlsCardVisible()) return;
    refreshSettingsJobsCard().catch((err) => appendLog(`settings jobs refresh error: ${err.message}`));
  }, 1000);

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
