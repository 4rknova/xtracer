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

function updateRenderActionButton() {
  if (!el.renderBtn) return;
  const running = !!renderActive;
  if (abortRequestInFlight) {
    el.renderBtn.disabled = true;
    el.renderBtn.textContent = "Aborting...";
    return;
  }
  el.renderBtn.disabled = false;
  el.renderBtn.textContent = running ? "Abort" : "Render";
}

let abortRequestedJobId = "";
let abortRequestInFlight = false;

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

async function abortRenderJob(jobId) {
  if (!jobId) return;
  if (!hasBackendMethod(api, "abortJob")) throw new Error("abort endpoint unavailable");
  return api.abortJob(jobId);
}

async function resolveAbortJobId() {
  const localId = String(activeJobId || "").trim();
  if (localId) return localId;
  if (hasBackendMethod(api, "getActiveJobs")) {
    try {
      const activeJobs = await api.getActiveJobs();
      const workspaceId = String(activeWorkspaceId || "").trim();
      const forWorkspace = workspaceId
        ? activeJobs.find((job) => String((job && job.workspace_id) || "").trim() === workspaceId)
        : null;
      const selected = forWorkspace || (activeJobs.length > 0 ? activeJobs[0] : null);
      const state = String((selected && selected.state) || "").toLowerCase();
      const serverJobId = (state === "queued" || state === "running")
        ? String((selected && selected.id) || "").trim()
        : "";
      if (serverJobId) {
        activeJobId = serverJobId;
        syncGlobalsToWorkspaceRuntime();
        updateRenderActionButton();
        appendLog(`abort resolve server job=${serverJobId}`);
        return serverJobId;
      }
    } catch (err) {
      appendLog(`abort resolve server lookup failed: ${err.message}`);
    }
  }
  return "";
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
    const elapsedMs = Math.max(0, Number(data.elapsed_ms) || 0);
    const threads = Math.max(0, Number(data.threads) || 0);
    updateActivePreviewTilesFromJob(data);
    setProgress(progress);
    setStatusThreads(threads);
    const stateLabel = state === "running" ? "rendering" : state;
    if ((state === "queued" || state === "running") && elapsedMs > 0 && typeof formatElapsed === "function") {
      setStatus(`${stateLabel} ${(100 * progress).toFixed(1)}% (${formatElapsed(elapsedMs)})`);
    } else {
      setStatus(`${stateLabel} ${(100 * progress).toFixed(1)}%`);
    }
    applyPreviewTransform();

    if (state !== lastState) {
      appendLog(`job ${jobId} -> ${state}`);
      lastState = state;
    }

    const abortPending = String(abortRequestedJobId || "") === String(jobId || "");
    let updatedByDelta = false;
    let deltaTilesDone = 0;
    let deltaTilesTotal = 0;
    if (!abortPending) {
      if (state === "queued" || state === "running" || state === "done") {
        try {
          const info = await refreshProgressivePreviewDelta(jobId);
          if (info && typeof info === "object") {
            updatedByDelta = !!info.updated;
            deltaTilesDone = Number(info.tilesDone) || 0;
            deltaTilesTotal = Number(info.tilesTotal) || 0;
          }
        } catch (_) {
          updatedByDelta = false;
          deltaTilesDone = 0;
          deltaTilesTotal = 0;
        }
      }
      if (!updatedByDelta) {
        await refreshProgressivePreview(jobId);
      }
    }
    if (token !== undefined && token !== activePollToken) return;

    const deltaComplete = !hasBackendMethod(api, "getJobImageDelta")
      || deltaTilesTotal <= 0
      || deltaTilesDone >= deltaTilesTotal;
    if (state === "done" && !abortPending && !deltaComplete) {
      await new Promise((r) => setTimeout(r, uiOptions.pollMs));
      continue;
    }

    if (state === "done") {
      if (String(abortRequestedJobId || "") === String(jobId || "")) abortRequestedJobId = "";
      clearActivePreviewTiles();
      resetProgressiveDeltaState("");
      progressiveDeltaEnabled = true;
      const elapsedMs = Math.max(0, Number(data.elapsed_ms) || 0);
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

    if (state === "aborted") {
      if (String(abortRequestedJobId || "") === String(jobId || "")) abortRequestedJobId = "";
      clearActivePreviewTiles();
      resetProgressiveDeltaState("");
      progressiveDeltaEnabled = true;
      applyPreviewTransform();
      setStatus("aborted");
      appendLog(`job ${jobId} aborted`);
      return;
    }

    if (state === "error") {
      if (String(abortRequestedJobId || "") === String(jobId || "")) abortRequestedJobId = "";
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
  if (renderActive) {
    if (abortRequestInFlight) return;
    abortRequestInFlight = true;
    updateRenderActionButton();
    try {
      const jobId = await resolveAbortJobId();
      if (!jobId) throw new Error("no active job id available for abort");
      abortRequestedJobId = String(jobId || "").trim();
      // Stop render polling before issuing abort to avoid queuing abort behind
      // a flood of polling requests.
      beginPollSession();
      clearActivePreviewTiles();
      resetProgressiveDeltaState("");
      progressiveDeltaEnabled = false;
      applyPreviewTransform();
      appendLog(`abort requested for job ${jobId}`);
      const abortResult = await abortRenderJob(jobId);
      const serverState = abortResult && abortResult.state ? String(abortResult.state) : "";
      if (serverState) {
        appendLog(`abort accepted for job ${jobId} state=${serverState}`);
        setStatus(`aborting (${serverState})...`);
      } else {
        setStatus("aborting...");
      }
      // Stop all job polling immediately after abort request so we can observe
      // backend cancellation behavior without client-side polling noise.
      activeJobId = "";
      abortRequestedJobId = "";
      syncGlobalsToWorkspaceRuntime();
      cancelActivePollingUi();
    } catch (err) {
      setStatus(`error: ${err.message}`);
      appendLog(`abort error: ${err.message}`);
    } finally {
      abortRequestInFlight = false;
      updateRenderActionButton();
    }
    return;
  }

  if (activeTabMode !== "render") setActiveTab("render");
  const pollToken = beginPollSession();
  abortRequestedJobId = "";
  abortRequestInFlight = false;
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
  updateRenderActionButton();
  clearActivePreviewTiles();
  applyPreviewTransform();
  setProgress(0);
  setStatusThreads(0);
  setStatus("submitting job...");
  appendLog(`submit render scene=${el.scene.value} integrator=${el.integrator.value} tile_order=${el.tileOrder.value}`);
  let submittedJobId = "";
  let pollReachedTerminalState = false;
  try {
    const jobId = await startRender();
    if (pollToken !== activePollToken) return;
    submittedJobId = String(jobId || "").trim();
    activeJobId = jobId;
    updateRenderActionButton();
    syncGlobalsToWorkspaceRuntime();
    appendLog(`job accepted: ${jobId}`);
    await pollJob(jobId, pollToken);
    pollReachedTerminalState = true;
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

    const ownPollSession = (pollToken === activePollToken);
    let shouldClearActiveJob = pollReachedTerminalState;
    const candidateJobId = String(activeJobId || submittedJobId || "").trim();

    if (!shouldClearActiveJob && ownPollSession && candidateJobId && hasBackendMethod(api, "getJob")) {
      try {
        const snap = await api.getJob(candidateJobId);
        const serverState = String((snap && snap.state) || "").toLowerCase();
        if (serverState === "done" || serverState === "aborted" || serverState === "error") {
          shouldClearActiveJob = true;
        } else if (serverState === "queued" || serverState === "running") {
          activeJobId = candidateJobId;
          syncGlobalsToWorkspaceRuntime();
          setRenderActive(true);
          updateRenderActionButton();
          appendLog(`poll recovered: job ${candidateJobId} still ${serverState}`);
          if (typeof resumeWorkspaceJobPolling === "function") {
            resumeWorkspaceJobPolling(candidateJobId);
            return;
          }
        }
      } catch (recoverErr) {
        appendLog(`poll recover check failed: ${recoverErr.message}`);
      }
    }

    if (shouldClearActiveJob && ownPollSession) {
      activeJobId = "";
      updateRenderActionButton();
      syncGlobalsToWorkspaceRuntime();
    }
    if (!activeJobId) {
      setRenderActive(false);
      updateRenderActionButton();
    }
  }
}
