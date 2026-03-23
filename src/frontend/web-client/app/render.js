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

async function startRender(extraParams) {
  const width = sanitizeIntField(el.width, 500, 32, 8192);
  const height = sanitizeIntField(el.height, 500, 32, 8192);
  const samples = sanitizeIntField(el.samples, 1, 1, 1024);
  const aa = sanitizeIntField(el.aa, 1, 1, 16);
  const rdepth = sanitizeIntField(el.rdepth, 10, 1, 4096);
  const tileSize = sanitizeIntField(el.tileSize, 32, 8, 1024);
  const threads = sanitizeIntField(el.threads, 0, 0, 256);

  const extra = (extraParams && typeof extraParams === "object") ? { ...extraParams } : {};
  const skipIntegratorOptions = !!extra.__skipIntegratorOptions;
  const skipPostFilters = !!extra.__skipPostFilters;
  delete extra.__skipIntegratorOptions;
  delete extra.__skipPostFilters;

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
    render_mode: normalizeRenderMode(renderMode),
    ...(skipPostFilters ? {} : { post_filters: gatherPostFilterParams() }),
    ...(skipIntegratorOptions ? {} : gatherIntegratorOptionParams()),
    ...extra,
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
    const jobRenderMode = String(data.render_mode || "").toLowerCase();
    const passCurrent = Number(data.pass_current) || 0;
    const passTotal = Number(data.pass_total) || 0;
    updateActivePreviewTilesFromJob(data);
    setProgress(progress);
    setStatusThreads(threads);
    setStatusPass(passCurrent, passTotal, jobRenderMode);
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
    const isInteractiveMovingJob = interactivePreviewEnabled
      && !!interactivePreviewActiveMovingJob
      && String(interactivePreviewJobId || "").trim() === String(jobId || "").trim();
    let updatedByDelta = false;
    let deltaTilesDone = 0;
    let deltaTilesTotal = 0;
    if (!abortPending) {
      if (!isInteractiveMovingJob && (state === "queued" || state === "running" || state === "done")) {
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
      return { state: "done", elapsedMs };
    }

    if (state === "aborted") {
      if (String(abortRequestedJobId || "") === String(jobId || "")) abortRequestedJobId = "";
      clearActivePreviewTiles();
      resetProgressiveDeltaState("");
      progressiveDeltaEnabled = true;
      applyPreviewTransform();
      setStatus("aborted");
      appendLog(`job ${jobId} aborted`);
      return { state: "aborted", elapsedMs };
    }

    if (state === "error") {
      if (String(abortRequestedJobId || "") === String(jobId || "")) abortRequestedJobId = "";
      clearActivePreviewTiles();
      resetProgressiveDeltaState("");
      progressiveDeltaEnabled = true;
      applyPreviewTransform();
      throw new Error(data.error || "render failed");
    }

    const isInteractiveJob = interactivePreviewEnabled
      && String(interactivePreviewJobId || "").trim()
      && String(interactivePreviewJobId || "").trim() === String(jobId || "").trim();
    const delayMs = isInteractiveJob
      ? INTERACTIVE_PREVIEW_ACTIVE_POLL_MS
      : uiOptions.pollMs;
    await new Promise((r) => setTimeout(r, delayMs));
  }
}

function interactiveTargetDimensions() {
  const width = Math.max(32, Number.parseInt(String(el.width && el.width.value ? el.width.value : "500"), 10) || 500);
  const height = Math.max(32, Number.parseInt(String(el.height && el.height.value ? el.height.value : "500"), 10) || 500);
  return { width, height };
}

function interactiveMovingWidthLevels(targetWidth) {
  const fallbackWidth = interactiveTargetDimensions().width;
  const width = Math.max(32, Number(targetWidth) || fallbackWidth);
  const ratios = [0.1, 0.125, 0.16, 0.2, 0.28, 0.4, 0.56, 0.75, 1.0];
  const levels = [];
  for (let i = 0; i < ratios.length; i += 1) {
    const scaled = Math.round(width * ratios[i]);
    const clamped = Math.max(32, Math.min(width, scaled));
    if (!levels.length || levels[levels.length - 1] !== clamped) levels.push(clamped);
  }
  if (!levels.length) levels.push(width);
  if (levels[levels.length - 1] !== width) levels.push(width);
  return levels;
}

function nearestInteractiveMovingWidth(v, targetWidth) {
  const levels = interactiveMovingWidthLevels(targetWidth);
  const value = Math.max(levels[0], Math.min(levels[levels.length - 1], Number(v) || levels[0]));
  let best = levels[0];
  let bestErr = Math.abs(best - value);
  for (let i = 1; i < levels.length; i += 1) {
    const err = Math.abs(levels[i] - value);
    if (err < bestErr) {
      best = levels[i];
      bestErr = err;
    }
  }
  return best;
}

function interactiveAdaptiveAdjustAfterFrame(stageWidth, elapsedMs) {
  if (!Number.isFinite(elapsedMs) || elapsedMs <= 1) return;
  const target = interactiveTargetDimensions();
  const levels = interactiveMovingWidthLevels(target.width);
  let idx = levels.indexOf(nearestInteractiveMovingWidth(stageWidth, target.width));
  if (idx < 0) idx = levels.indexOf(nearestInteractiveMovingWidth(interactivePreviewAdaptiveMovingWidth, target.width));
  if (idx < 0) idx = 0;
  const targetMs = Math.max(40, Number(INTERACTIVE_PREVIEW_TARGET_FRAME_MS) || 110);
  if (elapsedMs > targetMs * 1.5 && idx > 0) {
    idx -= 1;
  } else if (elapsedMs < targetMs * 0.65 && idx + 1 < levels.length) {
    idx += 1;
  }
  interactivePreviewAdaptiveMovingWidth = levels[idx];
}

function interactivePreviewIsMoving() {
  const sinceInput = Date.now() - (Number(interactivePreviewLastInputMs) || 0);
  if (sinceInput < INTERACTIVE_PREVIEW_SETTLE_MS) return true;
  if (!interactivePreviewKeyState) return false;
  return !!(interactivePreviewKeyState.w
    || interactivePreviewKeyState.a
    || interactivePreviewKeyState.s
    || interactivePreviewKeyState.d
    || interactivePreviewKeyState.q
    || interactivePreviewKeyState.e);
}

function interactiveResolutionStages(settleMode) {
  const target = interactiveTargetDimensions();
  const width = target.width;
  const height = target.height;
  const samples = Math.max(1, Number.parseInt(String(el.samples && el.samples.value ? el.samples.value : "1"), 10) || 1);
  const aa = Math.max(1, Number.parseInt(String(el.aa && el.aa.value ? el.aa.value : "1"), 10) || 1);
  const rdepth = Math.max(1, Number.parseInt(String(el.rdepth && el.rdepth.value ? el.rdepth.value : "10"), 10) || 10);
  const pick = (targetW) => {
    const w = Math.max(32, Math.min(width, targetW));
    const h = Math.max(32, Math.round((height * w) / Math.max(1, width)));
    return { width: w, height: h };
  };
  if (!settleMode) {
    const navDims = pick(Math.round(width * 0.01));
    return [{
      width: navDims.width,
      height: navDims.height,
      samples: "1",
      aa: "1",
      rdepth: String(Math.min(rdepth, 3)),
      tile_size: "8",
      tile_order: "random",
      integrator: "raytracer",
      moving: true,
    }];
  }
  const halfDims = pick(Math.round(width * 0.5));
  const fullDims = pick(width);
  const out = [{
    width: halfDims.width,
    height: halfDims.height,
    samples: "1",
    aa: "1",
    rdepth: String(Math.min(rdepth, 3)),
    tile_size: String(Math.max(8, Math.min(32, Number.parseInt(String(el.tileSize && el.tileSize.value ? el.tileSize.value : "32"), 10) || 32))),
    tile_order: "random",
    integrator: String(el.integrator && el.integrator.value ? el.integrator.value : "pathtracer_mis"),
    moving: false,
  }, {
    width: fullDims.width,
    height: fullDims.height,
    samples: "1",
    aa: "1",
    rdepth: String(Math.min(rdepth, 3)),
    tile_size: String(Math.max(8, Math.min(32, Number.parseInt(String(el.tileSize && el.tileSize.value ? el.tileSize.value : "32"), 10) || 32))),
    tile_order: "random",
    integrator: String(el.integrator && el.integrator.value ? el.integrator.value : "pathtracer_mis"),
    moving: false,
  }, {
    width: fullDims.width,
    height: fullDims.height,
    samples: String(samples),
    aa: String(aa),
    rdepth: String(rdepth),
    tile_size: String(Math.max(8, Math.min(32, Number.parseInt(String(el.tileSize && el.tileSize.value ? el.tileSize.value : "32"), 10) || 32))),
    tile_order: "random",
    integrator: String(el.integrator && el.integrator.value ? el.integrator.value : "pathtracer_mis"),
    moving: false,
  }];
  return out;
}

async function abortInteractivePreviewJob() {
  const jobId = String(interactivePreviewJobId || activeJobId || "").trim();
  if (!jobId || !hasBackendMethod(api, "abortJob")) return;
  try {
    await abortRenderJob(jobId);
  } catch (_) {
    // Best effort cancellation only.
  }
}

async function runInteractiveStage(stage, seq, loopToken, settleMode) {
  if (!interactivePreviewEnabled) return false;
  if (loopToken !== interactivePreviewLoopToken) return false;
  if (seq !== interactivePreviewCameraSeq) return false;
  if (activeTabMode !== "render") return false;

  const cameraParams = (typeof interactivePreviewCameraRequestParams === "function")
    ? interactivePreviewCameraRequestParams()
    : null;
  if (!cameraParams) return false;

  const pollToken = beginPollSession();
  setRenderActive(true);
  setProgress(0);
  setStatusThreads(0);
  setStatus(`interactive ${stage.width}x${stage.height}${stage.moving ? " nav" : ""}`);
  interactivePreviewHudQuality = `${stage.width}x${stage.height} ${stage.moving ? "nav(raytracer)" : (Number(stage.samples) > 1 || Number(stage.aa) > 1 ? "refine" : "fast")}`;
  interactivePreviewActiveMovingJob = !!stage.moving;
  if (typeof renderInteractivePreviewHud === "function") renderInteractivePreviewHud();
  clearActivePreviewTiles();
  applyPreviewTransform();

  let jobId = "";
  try {
    jobId = await startRender({
      width: String(stage.width),
      height: String(stage.height),
      samples: stage.samples,
      aa: stage.aa,
      rdepth: stage.rdepth,
      tile_size: stage.tile_size,
      tile_order: stage.tile_order,
      integrator: stage.integrator,
      render_mode: RENDER_MODE_NORMAL,
      __skipIntegratorOptions: !!stage.moving,
      __skipPostFilters: !!stage.moving,
      ...cameraParams,
    });
  } catch (err) {
    setRenderActive(false);
    updateRenderActionButton();
    setStatus(`interactive render error: ${err.message}`);
    appendLog(`interactive render rejected: ${err.message}`);
    return false;
  }
  if (loopToken !== interactivePreviewLoopToken || seq !== interactivePreviewCameraSeq) {
    interactivePreviewJobId = String(jobId || "").trim();
    await abortInteractivePreviewJob();
    return false;
  }

  interactivePreviewJobId = String(jobId || "").trim();
  activeJobId = interactivePreviewJobId;
  syncGlobalsToWorkspaceRuntime();
  updateRenderActionButton();
  await pollJob(jobId, pollToken);
  if (activeJobId === interactivePreviewJobId) {
    activeJobId = "";
    syncGlobalsToWorkspaceRuntime();
  }
  interactivePreviewJobId = "";
  interactivePreviewActiveMovingJob = false;
  return true;
}

async function runInteractivePreviewLoop(loopToken) {
  while (interactivePreviewEnabled && loopToken === interactivePreviewLoopToken) {
    if (activeTabMode !== "render") break;
    if (!interactivePreviewDirty) {
      await new Promise((r) => setTimeout(r, 50));
      continue;
    }
    interactivePreviewDirty = false;
    const seq = interactivePreviewCameraSeq;
    const settleMode = !interactivePreviewIsMoving();
    interactivePreviewHudQuality = settleMode ? "settling" : "moving";
    if (typeof renderInteractivePreviewHud === "function") renderInteractivePreviewHud();
    const stages = interactiveResolutionStages(settleMode);
    for (let i = 0; i < stages.length; i += 1) {
      if (!interactivePreviewEnabled || loopToken !== interactivePreviewLoopToken) return;
      if (seq !== interactivePreviewCameraSeq) break;
      const ok = await runInteractiveStage(stages[i], seq, loopToken, settleMode);
      if (!ok) break;
      if (seq !== interactivePreviewCameraSeq) break;
      if (!settleMode && interactivePreviewIsMoving()) {
        // Keep latency tight while user is actively moving.
        break;
      }
    }
    if (!settleMode && seq === interactivePreviewCameraSeq && interactivePreviewEnabled) {
      interactivePreviewDirty = true;
    }
  }
}

function startInteractivePreviewLoop() {
  if (interactivePreviewLoopActive) return;
  if (!interactivePreviewEnabled) return;
  if (activeTabMode !== "render") return;
  interactivePreviewLoopActive = true;
  const token = ++interactivePreviewLoopToken;
  runInteractivePreviewLoop(token)
    .catch((err) => appendLog(`interactive loop error: ${err.message}`))
    .finally(() => {
      if (token !== interactivePreviewLoopToken) return;
      interactivePreviewLoopActive = false;
      interactivePreviewJobId = "";
      interactivePreviewActiveMovingJob = false;
      activeJobId = "";
      syncGlobalsToWorkspaceRuntime();
      setRenderActive(false);
      updateRenderActionButton();
      if (interactivePreviewEnabled && interactivePreviewDirty && activeTabMode === "render") {
        startInteractivePreviewLoop();
      }
    });
}

async function stopInteractivePreviewLoop(abortJob) {
  interactivePreviewLoopToken += 1;
  interactivePreviewDirty = false;
  interactivePreviewLoopActive = false;
  interactivePreviewActiveMovingJob = false;
  if (abortJob) await abortInteractivePreviewJob();
  interactivePreviewJobId = "";
  if (!activeJobId || String(activeJobId).trim() === "") {
    setRenderActive(false);
    updateRenderActionButton();
  }
  interactivePreviewHudQuality = "idle";
  if (typeof renderInteractivePreviewHud === "function") renderInteractivePreviewHud();
}

function requestInteractivePreviewRender() {
  if (!interactivePreviewEnabled) return;
  if (activeTabMode !== "render") return;
  if (typeof interactivePreviewCameraRequestParams === "function" && !interactivePreviewCameraRequestParams()) return;
  interactivePreviewDirty = true;
  const movingNow = interactivePreviewIsMoving();
  if (interactivePreviewJobId && !movingNow) {
    abortInteractivePreviewJob().catch(() => {});
  }
  startInteractivePreviewLoop();
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
    if (interactivePreviewEnabled) {
      await stopInteractivePreviewLoop(false);
    }
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
  if (isInteractiveRenderMode()) {
    if (typeof requestInteractivePreviewRender === "function") {
      requestInteractivePreviewRender();
      setStatus("interactive mode active");
      appendLog("interactive render loop requested");
    }
    return;
  }
  if (interactivePreviewEnabled) {
    await stopInteractivePreviewLoop(true);
  }
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
  appendLog(`submit render scene=${el.scene.value} integrator=${el.integrator.value} mode=${normalizeRenderMode(renderMode)} tile_order=${el.tileOrder.value}`);
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
