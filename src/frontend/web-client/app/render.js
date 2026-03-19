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
  const rdepth = sanitizeIntField(el.rdepth, 10, 1, 4096);
  const tileSize = sanitizeIntField(el.tileSize, 32, 8, 1024);
  const threads = sanitizeIntField(el.threads, 0, 0, 256);

  return api.startRender({
    scene: el.scene.value,
    variant: selectedSceneVariantValue(),
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
          setSceneBrowserSelectedFile(scene);
          localStorage.setItem(LAST_SCENE_KEY, scene);
          updateSceneDependencyPill(scene);
          return loadVariants(scene)
            .then(() => {
              const tasks = [loadCameras(scene)];
              if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(scene));
              if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(scene));
              if (visualEditor) tasks.push(loadVisualSceneFromSelected());
              return Promise.all(tasks);
            });
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
      const elapsedMsRaw = Math.max(0, Number(data.elapsed_ms) || 0);
      const elapsedMs = elapsedMsRaw > 0
        ? elapsedMsRaw
        : (renderStartMs > 0 ? (Date.now() - renderStartMs) : 0);
      recordFullFrameRenderTime(elapsedMs);
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
      setStatus(`done in ${Math.round(elapsedMs)} ms`);
      appendLog(`job ${jobId} finished in ${Math.round(elapsedMs)} ms`);
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
