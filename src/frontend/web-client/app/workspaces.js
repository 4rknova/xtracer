
function updateWorkspaceActiveHint() {
  if (!el.workspaceActiveHint) return;
  const id = String(activeWorkspaceId || "").trim();
  window.XTracerWidgets.renderStatHint(el.workspaceActiveHint, {
    label: "Active workspace",
    value: id || "-",
    className: "workspace-active-hint",
  });
}

function updateWorkspaceCountHint(count) {
  if (!el.workspaceCountHint) return;
  window.XTracerWidgets.renderStatHint(el.workspaceCountHint, {
    label: "Workspaces",
    value: Math.max(0, Number(count) || 0),
    className: "workspace-active-hint",
  });
}

function updateWorkspaceServerStatHint(node, label, value) {
  if (!node) return;
  window.XTracerWidgets.renderStatHint(node, {
    label,
    value: (value === null || value === undefined || value === "") ? "-" : String(value),
    className: "workspace-active-hint",
  });
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
  settingsJobsTotalRenderThreads = Number.isFinite(renderAutoThreads) && renderAutoThreads > 0
    ? Math.floor(renderAutoThreads)
    : 0;
  const computedThreadLimit = Number.isFinite(renderAutoThreads) && renderAutoThreads > 0
    ? Math.floor(renderAutoThreads)
    : (Number.isFinite(openmpThreads) && openmpThreads > 0
      ? Math.max(1, Math.floor(openmpThreads) - Math.max(0, Math.floor(renderReserveThreads) || 0))
      : 256);
  if (el.threads) {
    el.threads.max = String(computedThreadLimit);
    el.threads.dataset.autoThreadCount = String(computedThreadLimit);
    el.threads.title = `0 uses auto mode. Maximum manual value: ${computedThreadLimit}.`;
    const current = Math.max(0, Number.parseInt(String(el.threads.value || "0"), 10) || 0);
    if (current > computedThreadLimit) el.threads.value = String(computedThreadLimit);
  }
  if (el.threadsLabelText) {
    el.threadsLabelText.textContent = "Threads";
  }
  if (el.threadsLabelSubtext) {
    el.threadsLabelSubtext.textContent = `${computedThreadLimit} max`;
  }
  if (typeof syncTileSizeControlUi === "function") syncTileSizeControlUi();
  renderSettingsJobsThreadGraph();
}

let settingsJobsTotalRenderThreads = 0;
const settingsJobsAbortInFlight = new Set();
const settingsJobsMoveInFlight = new Set();
const SETTINGS_JOBS_GRAPH_WINDOW_MS = 60 * 1000;
let settingsJobsThreadSamples = [];

function resetSettingsJobsThreadGraph() {
  settingsJobsThreadSamples = [];
  renderSettingsJobsThreadGraph();
}

function pruneSettingsJobsThreadSamples(nowMs) {
  const now = Number.isFinite(nowMs) ? nowMs : Date.now();
  const minTs = now - SETTINGS_JOBS_GRAPH_WINDOW_MS;
  // Find the most recent sample that sits before the window start.  Its value
  // is what was current at the left edge, so clip it to minTs and use it as
  // an anchor instead of discarding it — otherwise the left portion of the
  // graph shows the wrong value when older samples roll out of the window.
  let anchorIdx = -1;
  for (let i = settingsJobsThreadSamples.length - 1; i >= 0; i--) {
    const s = settingsJobsThreadSamples[i];
    if (s && Number.isFinite(s.ts) && s.ts < minTs) { anchorIdx = i; break; }
  }
  if (anchorIdx >= 0) {
    settingsJobsThreadSamples[anchorIdx] = { ts: minTs, value: settingsJobsThreadSamples[anchorIdx].value };
    settingsJobsThreadSamples = settingsJobsThreadSamples.slice(anchorIdx);
  }
}

function getSettingsJobsOccupiedThreads(activeJobs) {
  const jobs = Array.isArray(activeJobs) ? activeJobs : [];
  return jobs.reduce((sum, job) => {
    const state = String((job && job.state) || "").toLowerCase();
    if (state !== "running") return sum;
    const threads = Math.max(0, Number((job && job.threads) || 0));
    return sum + (Number.isFinite(threads) ? Math.floor(threads) : 0);
  }, 0);
}

function recordSettingsJobsThreadUsage(occupied, nowMs) {
  const now = Number.isFinite(nowMs) ? nowMs : Date.now();
  const value = Math.max(0, Math.floor(Number(occupied) || 0));
  pruneSettingsJobsThreadSamples(now);
  const last = settingsJobsThreadSamples.length ? settingsJobsThreadSamples[settingsJobsThreadSamples.length - 1] : null;
  if (last && (now - last.ts) < 250) {
    last.ts = now;
    last.value = value;
    return;
  }
  settingsJobsThreadSamples.push({ ts: now, value });
  pruneSettingsJobsThreadSamples(now);
}

function createSettingsJobsGraphSvgNode(tag, attrs) {
  const ns = "http://www.w3.org/2000/svg";
  const node = document.createElementNS(ns, tag);
  const attributes = attrs || {};
  Object.keys(attributes).forEach((key) => {
    const value = attributes[key];
    if (value === undefined || value === null) return;
    node.setAttribute(key, String(value));
  });
  return node;
}

function buildSettingsJobsGraphPaths(samples, nowMs, maxScale, width, height) {
  const now = Number.isFinite(nowMs) ? nowMs : Date.now();
  const list = Array.isArray(samples) && samples.length ? samples : [{ ts: now, value: 0 }];
  const startTs = now - SETTINGS_JOBS_GRAPH_WINDOW_MS;
  const safeScale = Math.max(1, Number(maxScale) || 1);
  const xForTs = (ts) => {
    if (!Number.isFinite(ts) || ts <= startTs) return 0;
    if (ts >= now) return width;
    return ((ts - startTs) / SETTINGS_JOBS_GRAPH_WINDOW_MS) * width;
  };
  const yForValue = (value) => {
    const clamped = Math.max(0, Math.min(safeScale, Number(value) || 0));
    return height - ((clamped / safeScale) * height);
  };

  let prevX = 0;
  let prevY = yForValue(list[0].value);
  let line = `M 0 ${prevY.toFixed(2)}`;
  let area = `M 0 ${height.toFixed(2)} L 0 ${prevY.toFixed(2)}`;

  list.forEach((sample, index) => {
    let x = xForTs(sample.ts);
    const y = yForValue(sample.value);
    if (index === 0) {
      if (x > 0) {
        line += ` L ${x.toFixed(2)} ${prevY.toFixed(2)}`;
        area += ` L ${x.toFixed(2)} ${prevY.toFixed(2)}`;
      }
      prevX = x;
      prevY = y;
      return;
    }
    if (x < prevX) x = prevX;
    if (x > prevX) {
      line += ` L ${x.toFixed(2)} ${prevY.toFixed(2)}`;
      area += ` L ${x.toFixed(2)} ${prevY.toFixed(2)}`;
    }
    if (Math.abs(y - prevY) > 0.001) {
      line += ` L ${x.toFixed(2)} ${y.toFixed(2)}`;
      area += ` L ${x.toFixed(2)} ${y.toFixed(2)}`;
    }
    prevX = x;
    prevY = y;
  });

  if (prevX < width) {
    line += ` L ${width.toFixed(2)} ${prevY.toFixed(2)}`;
    area += ` L ${width.toFixed(2)} ${prevY.toFixed(2)}`;
  }

  area += ` L ${width.toFixed(2)} ${height.toFixed(2)} Z`;
  return {
    area,
    line,
    lastX: width,
    lastY: prevY,
  };
}

function renderSettingsJobsThreadGraph() {
  if (!el.settingsJobsThreadGraph) return;

  const now = Date.now();
  pruneSettingsJobsThreadSamples(now);
  const samples = settingsJobsThreadSamples.slice();
  const latest = samples.length ? Math.max(0, Math.floor(Number(samples[samples.length - 1].value) || 0)) : 0;
  const peak = samples.reduce((max, sample) => Math.max(max, Math.max(0, Math.floor(Number(sample && sample.value) || 0))), 0);
  const total = Math.max(0, Number(settingsJobsTotalRenderThreads) || 0);
  const scaleMax = Math.max(1, total, peak);
  const currentLabel = total > 0 ? `${latest} / ${total}` : `${latest} / -`;
  const widgets = window.XTracerWidgets || {};
  if (widgets.dom && typeof widgets.dom.clear === "function") widgets.dom.clear(el.settingsJobsThreadGraph);
  else el.settingsJobsThreadGraph.innerHTML = "";
  el.settingsJobsThreadGraph.setAttribute("aria-label", `Threads in use over the last minute. Current usage ${currentLabel}. Peak ${peak}.`);

  const header = document.createElement("div");
  header.className = "settings-jobs-graph-header";

  const titleGroup = document.createElement("div");
  titleGroup.className = "settings-jobs-graph-title-group";

  const title = document.createElement("p");
  title.className = "settings-jobs-graph-title";
  title.textContent = "Threads In Use";

  const subtitle = document.createElement("p");
  subtitle.className = "settings-jobs-graph-subtitle";
  subtitle.textContent = "Rolling 1 minute window";

  titleGroup.appendChild(title);
  titleGroup.appendChild(subtitle);

  const statGroup = document.createElement("div");
  statGroup.className = "settings-jobs-graph-stats";

  const peakNode = document.createElement("span");
  peakNode.className = "settings-jobs-graph-stat";
  peakNode.textContent = `peak ${peak}`;

  const currentNode = document.createElement("span");
  currentNode.className = "settings-jobs-graph-stat";
  currentNode.textContent = `now ${currentLabel}`;

  statGroup.appendChild(peakNode);
  statGroup.appendChild(currentNode);
  header.appendChild(titleGroup);
  header.appendChild(statGroup);
  el.settingsJobsThreadGraph.appendChild(header);

  const chart = document.createElement("div");
  chart.className = "settings-jobs-graph-chart";

  if (!samples.length) {
    const empty = document.createElement("p");
    empty.className = "settings-jobs-graph-empty";
    empty.textContent = "Waiting for the first jobs sample.";
    chart.appendChild(empty);
  } else {
    const width = 240;
    const height = 72;
    const svg = createSettingsJobsGraphSvgNode("svg", {
      class: "settings-jobs-graph-svg",
      viewBox: `0 0 ${width} ${height}`,
      preserveAspectRatio: "none",
      "aria-hidden": "true",
    });

    [0, height * 0.5, height].forEach((y) => {
      svg.appendChild(createSettingsJobsGraphSvgNode("line", {
        class: "settings-jobs-graph-grid-line",
        x1: 0,
        y1: y.toFixed(2),
        x2: width,
        y2: y.toFixed(2),
      }));
    });

    const paths = buildSettingsJobsGraphPaths(samples, now, scaleMax, width, height);
    svg.appendChild(createSettingsJobsGraphSvgNode("path", {
      class: "settings-jobs-graph-area",
      d: paths.area,
    }));
    svg.appendChild(createSettingsJobsGraphSvgNode("path", {
      class: "settings-jobs-graph-line",
      d: paths.line,
    }));
    svg.appendChild(createSettingsJobsGraphSvgNode("circle", {
      class: "settings-jobs-graph-dot",
      cx: paths.lastX.toFixed(2),
      cy: paths.lastY.toFixed(2),
      r: "3",
    }));

    chart.appendChild(svg);
  }

  el.settingsJobsThreadGraph.appendChild(chart);

  const footer = document.createElement("div");
  footer.className = "settings-jobs-graph-footer";

  const agoNode = document.createElement("span");
  agoNode.className = "settings-jobs-graph-footnote";
  agoNode.textContent = "60s ago";

  const scaleNode = document.createElement("span");
  scaleNode.className = "settings-jobs-graph-footnote";
  scaleNode.textContent = `scale 0-${scaleMax}`;

  const nowNode = document.createElement("span");
  nowNode.className = "settings-jobs-graph-footnote";
  nowNode.textContent = "now";

  footer.appendChild(agoNode);
  footer.appendChild(scaleNode);
  footer.appendChild(nowNode);
  el.settingsJobsThreadGraph.appendChild(footer);
}

function bindSettingsJobsCardLifecycle() {
  const card = document.getElementById("JobsControlsCard");
  if (!card || card._settingsJobsLifecycleBound) return;
  card._settingsJobsLifecycleBound = true;

  card.addEventListener("toggle", () => {
    if (card.open) {
      refreshSettingsJobsCard();
    } else {
      resetSettingsJobsThreadGraph();
    }
  });
  if (card.open) {
    refreshSettingsJobsCard();
  }
}

function notifyActiveJobsChanged() {
  if (activeTabMode === "workspaces" && hasBackendMethod(api, "getWorkspaces")) {
    refreshWorkspaces().catch((err) => appendLog(`workspace refresh error: ${err.message}`));
  }
  if (typeof refreshSettingsJobsCard === "function" && el.settingsJobsList) {
    refreshSettingsJobsCard();
  }
}

function parseJobSequence(jobId) {
  const id = String(jobId || "");
  const m = id.match(/^job_(\d+)$/);
  return m ? Number(m[1]) : 0;
}

function compareActiveJobsForSettings(a, b) {
  const stateA = String((a && a.state) || "").toLowerCase();
  const stateB = String((b && b.state) || "").toLowerCase();
  const priority = (state) => {
    if (state === "running") return 0;
    if (state === "queued") return 1;
    return 2;
  };
  const pa = priority(stateA);
  const pb = priority(stateB);
  if (pa !== pb) return pa - pb;
  const sa = parseJobSequence(a && a.id);
  const sb = parseJobSequence(b && b.id);
  if (sa !== sb) return sb - sa;
  return String((a && a.id) || "").localeCompare(String((b && b.id) || ""));
}

function formatJobElapsedMs(ms) {
  const elapsed = Math.max(0, Number(ms) || 0);
  if (!Number.isFinite(elapsed) || elapsed <= 0) return "-";
  return formatElapsed(elapsed);
}

function createSettingsJobActionIcon(kind) {
  if (window.XTracerWidgets && window.XTracerWidgets.dom && typeof window.XTracerWidgets.dom.svgIcon === "function") {
    if (kind === "up") return window.XTracerWidgets.dom.svgIcon("M8 12V4M8 4L5.4 6.6M8 4l2.6 2.6");
    if (kind === "down") return window.XTracerWidgets.dom.svgIcon("M8 4v8M8 12l-2.6-2.6M8 12l2.6-2.6");
    return window.XTracerWidgets.dom.svgIcon("M5 5l6 6M11 5L5 11");
  }
  const ns = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(ns, "svg");
  svg.setAttribute("viewBox", "0 0 16 16");
  svg.setAttribute("aria-hidden", "true");
  svg.classList.add("settings-job-action-icon");

  const path = document.createElementNS(ns, "path");
  path.setAttribute("fill", "none");
  path.setAttribute("stroke", "currentColor");
  path.setAttribute("stroke-width", "1.8");
  path.setAttribute("stroke-linecap", "round");
  path.setAttribute("stroke-linejoin", "round");

  if (kind === "up") path.setAttribute("d", "M8 12V4M8 4L5.4 6.6M8 4l2.6 2.6");
  else if (kind === "down") path.setAttribute("d", "M8 4v8M8 12l-2.6-2.6M8 12l2.6-2.6");
  else path.setAttribute("d", "M5 5l6 6M11 5L5 11");

  svg.appendChild(path);
  return svg;
}

function createSettingsJobStateTag(state) {
  if (window.XTracerWidgets && typeof window.XTracerWidgets.createTag === "function") {
    const tone = state === "running" ? "success" : (state === "queued" ? "warning" : "neutral");
    return window.XTracerWidgets.createTag({
      label: state,
      tone,
      className: `settings-job-state-pill state-${state}`,
    });
  }
  const node = document.createElement("span");
  node.className = `settings-job-state-pill state-${state}`;
  node.textContent = state;
  return node;
}

function createSettingsJobActionButton(kind, opts) {
  const options = opts || {};
  if (window.XTracerWidgets && typeof window.XTracerWidgets.createIconButton === "function") {
    return window.XTracerWidgets.createIconButton({
      title: options.title || "",
      label: options.ariaLabel || options.title || "",
      disabled: !!options.disabled,
      variant: "ghost",
      className: options.className || "",
      icon: createSettingsJobActionIcon(kind),
      onClick: options.onClick,
    });
  }
  const btn = document.createElement("button");
  btn.type = "button";
  btn.className = options.className || "";
  btn.setAttribute("aria-label", options.ariaLabel || options.title || "");
  if (options.title) btn.title = options.title;
  btn.disabled = !!options.disabled;
  btn.appendChild(createSettingsJobActionIcon(kind));
  if (typeof options.onClick === "function") btn.addEventListener("click", options.onClick);
  return btn;
}

function createSettingsJobProgress(progress) {
  if (window.XTracerWidgets && typeof window.XTracerWidgets.createProgressBar === "function") {
    return window.XTracerWidgets.createProgressBar({
      value: progress,
      className: "settings-job-progress",
    });
  }
  const progressBar = document.createElement("div");
  progressBar.className = "settings-job-progress";
  const progressFill = document.createElement("span");
  progressFill.style.width = `${(progress * 100).toFixed(1)}%`;
  progressBar.appendChild(progressFill);
  return progressBar;
}

function createWorkspaceStateBadge(label, className) {
  const text = String(label || "").trim() || "Unknown";
  const normalized = text.toLowerCase();
  if (window.XTracerWidgets && typeof window.XTracerWidgets.createTag === "function") {
    let tone = "neutral";
    if (normalized === "active" || normalized === "rendering") tone = "success";
    return window.XTracerWidgets.createTag({
      label: text,
      tone,
      className: className || "",
    });
  }
  const node = document.createElement("span");
  node.className = className || "";
  node.textContent = text;
  return node;
}

function createWorkspaceActionButton(label, opts) {
  const options = opts || {};
  if (window.XTracerWidgets && typeof window.XTracerWidgets.createButton === "function") {
    return window.XTracerWidgets.createButton({
      label,
      variant: options.variant || "secondary",
      disabled: !!options.disabled,
      className: options.className || "",
      onClick: options.onClick,
    });
  }
  const btn = document.createElement("button");
  btn.type = "button";
  btn.className = options.className || "";
  btn.textContent = label;
  btn.disabled = !!options.disabled;
  btn.setAttribute("aria-disabled", btn.disabled ? "true" : "false");
  if (typeof options.onClick === "function") btn.addEventListener("click", options.onClick);
  return btn;
}

function createWorkspaceEmptyState(title, message, className) {
  if (window.XTracerWidgets && typeof window.XTracerWidgets.createEmptyState === "function") {
    return window.XTracerWidgets.createEmptyState({
      title,
      message,
      className: className || "",
    });
  }
  const node = document.createElement("p");
  node.className = className || "";
  node.textContent = message || title || "";
  return node;
}

function renderSettingsJobsList(activeJobs) {
  if (!el.settingsJobsList) return;
  const widgets = window.XTracerWidgets || {};
  const jobs = Array.isArray(activeJobs) ? [...activeJobs] : [];
  el.settingsJobsList.innerHTML = "";
  el.settingsJobsList.hidden = false;
  if (!jobs.length) {
    el.settingsJobsList.hidden = true;
    return;
  }

  const queuedJobs = jobs.filter((job) => String((job && job.state) || "").toLowerCase() === "queued");
  const queuedIndexById = new Map();
  queuedJobs.forEach((job, i) => {
    const id = String((job && job.id) || "").trim();
    if (!id) return;
    queuedIndexById.set(id, i);
  });
  const queuedCount = queuedJobs.length;

  jobs.forEach((job) => {
    const id = String((job && job.id) || "").trim() || "-";
    const workspaceId = String((job && job.workspace_id) || "").trim() || "-";
    const state = String((job && job.state) || "").toLowerCase() || "unknown";
    const threads = Math.max(0, Number((job && job.threads) || 0));
    const progress = Math.max(0, Math.min(1, Number((job && job.progress) || 0)));
    const elapsed = formatJobElapsedMs(job && job.elapsed_ms);
    const scene = String((job && job.scene) || "").trim() || "-";
    const integrator = String((job && job.integrator) || "").trim() || "-";

    const statePill = createSettingsJobStateTag(state);
    const controlNodes = [];
    const queueIdx = queuedIndexById.has(id) ? queuedIndexById.get(id) : -1;
    const isMoving = settingsJobsMoveInFlight.has(id);
    if (state === "queued" && queueIdx >= 0) {
      const queueControls = document.createElement("div");
      queueControls.className = "settings-job-queue-controls";

      const upBtn = createSettingsJobActionButton("up", {
        className: "settings-job-queue-btn",
        ariaLabel: "Move job up",
        title: "Move up",
        disabled: isMoving || queueIdx <= 0,
        onClick: () => {
          moveSettingsJobQueue(id, "up").catch((err) => appendLog(`settings queue move up error: ${err.message}`));
        },
      });

      const downBtn = createSettingsJobActionButton("down", {
        className: "settings-job-queue-btn",
        ariaLabel: "Move job down",
        title: "Move down",
        disabled: isMoving || queueIdx >= (queuedCount - 1),
        onClick: () => {
          moveSettingsJobQueue(id, "down").catch((err) => appendLog(`settings queue move down error: ${err.message}`));
        },
      });

      queueControls.appendChild(upBtn);
      queueControls.appendChild(downBtn);
      controlNodes.push(queueControls);
    }
    if (state === "running" || state === "queued") {
      const abortBtn = createSettingsJobActionButton("abort", {
        className: "settings-job-abort-btn",
        ariaLabel: settingsJobsAbortInFlight.has(id) ? "Aborting" : "Abort job",
        title: settingsJobsAbortInFlight.has(id) ? "Aborting" : "Abort",
        disabled: settingsJobsAbortInFlight.has(id),
        onClick: () => {
          abortSettingsJob(id).catch((err) => appendLog(`settings abort error: ${err.message}`));
        },
      });
      controlNodes.push(abortBtn);
    }
    const threadsNode = document.createElement("span");
    threadsNode.className = "settings-job-meta-pill";
    threadsNode.textContent = threads === 1 ? "1 thread" : `${threads} threads`;
    const elapsedNode = document.createElement("span");
    elapsedNode.className = "settings-job-meta-pill";
    elapsedNode.textContent = elapsed;
    const progressNode = document.createElement("span");
    progressNode.className = "settings-job-meta-pill";
    progressNode.textContent = `${(progress * 100).toFixed(1)}%`;
    const progressBar = createSettingsJobProgress(progress);
    const item = widgets.createJobRow({
      id,
      state,
      pills: [statePill],
      workspaceLabel: `ws ${workspaceId}`,
      metrics: [threadsNode, elapsedNode, progressNode],
      progress: progressBar,
      subtext: `${scene} · ${integrator}`,
      controls: controlNodes,
    });
    item.dataset.jobId = id;
    el.settingsJobsList.appendChild(item);
  });
}


function refreshSettingsJobsCard() {
  if (!el.settingsJobsList) return Promise.resolve();
  const activeJobs = getActiveJobsFromCache();
  const occupied = getSettingsJobsOccupiedThreads(activeJobs);
  recordSettingsJobsThreadUsage(occupied);
  renderSettingsJobsList(activeJobs);
  renderSettingsJobsThreadGraph();
  if (el.settingsJobsUpdated) {
    const ts = new Date();
    const hh = String(ts.getHours()).padStart(2, "0");
    const mm = String(ts.getMinutes()).padStart(2, "0");
    const ss = String(ts.getSeconds()).padStart(2, "0");
    updateWorkspaceServerStatHint(el.settingsJobsUpdated, "Updated", `${hh}:${mm}:${ss}`);
  }
  if (el.settingsJobsThreadsUsage) {
    const total = Math.max(0, Number(settingsJobsTotalRenderThreads) || 0);
    const value = total > 0 ? `${occupied} / ${total}` : `${occupied} / -`;
    updateWorkspaceServerStatHint(el.settingsJobsThreadsUsage, "Threads In Use", value);
  }
  return Promise.resolve();
}

function isJobsControlsCardVisible() {
  const card = document.getElementById("JobsControlsCard");
  if (!card) return false;
  if (card.hidden) return false;
  if (!card.open) return false;
  return true;
}

async function moveSettingsJobQueue(jobId, direction) {
  const id = String(jobId || "").trim();
  const dir = String(direction || "").toLowerCase();
  if (!id) return;
  if (dir !== "up" && dir !== "down") return;
  const fn = (dir === "up") ? "moveJobQueueUp" : "moveJobQueueDown";
  if (!hasBackendMethod(api, fn)) return;
  const key = `${id}:${dir}`;
  if (settingsJobsMoveInFlight.has(id) || settingsJobsMoveInFlight.has(key)) return;
  settingsJobsMoveInFlight.add(id);
  settingsJobsMoveInFlight.add(key);
  try {
    if (dir === "up") await api.moveJobQueueUp(id);
    else await api.moveJobQueueDown(id);
    appendLog(`settings queue move ${dir} ${id}`);
  } catch (err) {
    appendLog(`settings queue move ${dir} failed for ${id}: ${err.message}`);
  } finally {
    settingsJobsMoveInFlight.delete(key);
    settingsJobsMoveInFlight.delete(id);
    notifyActiveJobsChanged();
  }
}

async function abortSettingsJob(jobId) {
  const id = String(jobId || "").trim();
  if (!id || !hasBackendMethod(api, "abortJob")) return;
  if (settingsJobsAbortInFlight.has(id)) return;
  settingsJobsAbortInFlight.add(id);
  try {
    appendLog(`settings abort requested for ${id}`);
    await api.abortJob(id);
    appendLog(`settings abort accepted for ${id}`);
  } catch (err) {
    appendLog(`settings abort failed for ${id}: ${err.message}`);
  } finally {
    settingsJobsAbortInFlight.delete(id);
    notifyActiveJobsChanged();
  }
}

function normalizeWorkspaceViewMode(value) {
  return String(value || "").toLowerCase() === "list" ? "list" : "cards";
}

function normalizeWorkspaceSortMode(value) {
  const mode = String(value || "").toLowerCase();
  if (mode === "updated") return "updated";
  if (mode === "scene") return "scene";
  return "name";
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

function setWorkspaceSortMode(mode, persist) {
  workspaceSortMode = normalizeWorkspaceSortMode(mode);
  if (el.workspaceSortNameBtn) {
    const active = workspaceSortMode === "name";
    el.workspaceSortNameBtn.classList.toggle("active", active);
    el.workspaceSortNameBtn.setAttribute("aria-pressed", active ? "true" : "false");
  }
  if (el.workspaceSortUpdatedBtn) {
    const active = workspaceSortMode === "updated";
    el.workspaceSortUpdatedBtn.classList.toggle("active", active);
    el.workspaceSortUpdatedBtn.setAttribute("aria-pressed", active ? "true" : "false");
  }
  if (el.workspaceSortSceneBtn) {
    const active = workspaceSortMode === "scene";
    el.workspaceSortSceneBtn.classList.toggle("active", active);
    el.workspaceSortSceneBtn.setAttribute("aria-pressed", active ? "true" : "false");
  }
  if (persist !== false) {
    localStorage.setItem(WORKSPACE_SORT_MODE_KEY, workspaceSortMode);
  }
}

function compareWorkspaceName(a, b) {
  const nameA = String((a && a.name) || (a && a.id) || "").trim().toLowerCase();
  const nameB = String((b && b.name) || (b && b.id) || "").trim().toLowerCase();
  const byName = nameA.localeCompare(nameB);
  if (byName !== 0) return byName;
  const idA = String((a && a.id) || "").trim().toLowerCase();
  const idB = String((b && b.id) || "").trim().toLowerCase();
  return idA.localeCompare(idB);
}

function compareWorkspaceUpdated(a, b) {
  const rawA = Number((a && a.updated_ms) || 0);
  const rawB = Number((b && b.updated_ms) || 0);
  const updatedA = Number.isFinite(rawA) ? rawA : 0;
  const updatedB = Number.isFinite(rawB) ? rawB : 0;
  if (updatedA !== updatedB) return updatedB - updatedA;
  return compareWorkspaceName(a, b);
}

function compareWorkspaceScene(a, b) {
  const sceneA = String((a && a.active_scene) || "").trim().toLowerCase();
  const sceneB = String((b && b.active_scene) || "").trim().toLowerCase();
  if (sceneA !== sceneB) return sceneA.localeCompare(sceneB);
  return compareWorkspaceName(a, b);
}

function sortWorkspaceItems(items) {
  const list = Array.isArray(items) ? [...items] : [];
  if (workspaceSortMode === "updated") {
    list.sort(compareWorkspaceUpdated);
    return list;
  }
  if (workspaceSortMode === "scene") {
    list.sort(compareWorkspaceScene);
    return list;
  }
  list.sort(compareWorkspaceName);
  return list;
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
  const hasOwned = !!(workspace && Object.prototype.hasOwnProperty.call(workspace, "is_owned_by_client"));
  const isMine = hasOwned
    ? !!workspace.is_owned_by_client
    : !!(workspace && workspace.is_active_for_client);
  if (isMine || (id && id === activeWorkspaceId)) return "Active";
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
      rdepth: String(el.rdepth && el.rdepth.value ? el.rdepth.value : "15"),
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
    preview: {
      render_mode: normalizeRenderMode(renderMode),
      interactive_speed: String((Number(interactivePreviewFlySpeedScale) || 1.0).toFixed(1)),
      interactive_moving_width: String(nearestInteractiveMovingWidth(interactivePreviewAdaptiveMovingWidth)),
    },
    post_filters: Array.isArray(postFilterChain)
      ? postFilterChain
        .map((entry) => normalizePostFilterEntry(entry))
        .filter((entry) => !!entry && !!entry.filter)
      : [],
    post_filters_enabled: !!postFilterStackEnabled,
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
      if (el.tileSize && integrator.tile_size !== undefined) {
        if (typeof setTileSizeControlValue === "function") setTileSizeControlValue(integrator.tile_size);
        else el.tileSize.value = String(integrator.tile_size);
      }
      if (el.tileOrder && integrator.tile_order !== undefined) el.tileOrder.value = String(integrator.tile_order);
      if (el.threads && integrator.threads !== undefined) el.threads.value = String(integrator.threads);
      renderIntegratorControls();
      if (typeof syncTileSizeControlUi === "function") syncTileSizeControlUi();
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

    const preview = cfg.preview && typeof cfg.preview === "object" ? cfg.preview : null;
    if (preview && typeof setRenderMode === "function") {
      let nextMode = normalizeRenderMode(preview.render_mode);
      if (preview.render_mode === undefined && preview.interactive !== undefined) {
        nextMode = preview.interactive ? RENDER_MODE_INTERACTIVE : RENDER_MODE_PROGRESSIVE;
      }
      setRenderMode(nextMode, { log: false }).catch(() => {});
    }
    if (preview && preview.interactive_speed !== undefined) {
      const speed = Math.max(0.2, Math.min(5.0, Number(preview.interactive_speed) || 1.0));
      interactivePreviewFlySpeedScale = speed;
      if (el.interactivePreviewSpeed) el.interactivePreviewSpeed.value = String(speed.toFixed(1));
      if (el.interactivePreviewSpeedValue) el.interactivePreviewSpeedValue.textContent = `${speed.toFixed(1)}x`;
      if (typeof renderInteractivePreviewHud === "function") renderInteractivePreviewHud();
    }
    if (preview && preview.interactive_moving_width !== undefined) {
      interactivePreviewAdaptiveMovingWidth = nearestInteractiveMovingWidth(preview.interactive_moving_width);
    }

    if (Array.isArray(cfg.post_filters)) {
      postFilterChain = cfg.post_filters
        .map((entry) => normalizePostFilterEntry(entry))
        .filter((entry) => !!entry && !!entry.filter);
    }
    if (cfg.post_filters_enabled !== undefined) {
      postFilterStackEnabled = !!cfg.post_filters_enabled;
    }
    if (el.postFiltersEnabled) el.postFiltersEnabled.checked = !!postFilterStackEnabled;
    updatePostFilterUiState();
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

function mapActiveJobsByWorkspace(activeJobs) {
  const byWorkspace = new Map();
  const list = Array.isArray(activeJobs) ? activeJobs : [];
  list.forEach((job) => {
    const workspaceId = String((job && job.workspace_id) || "").trim();
    const jobId = String((job && job.id) || "").trim();
    const state = String((job && job.state) || "").toLowerCase();
    if (!workspaceId || !jobId) return;
    if (state !== "queued" && state !== "running") return;
    if (!byWorkspace.has(workspaceId)) byWorkspace.set(workspaceId, jobId);
  });
  return byWorkspace;
}

function overlayWorkspaceActiveJobs(workspaces, activeJobs) {
  const list = Array.isArray(workspaces) ? workspaces : [];
  if (!Array.isArray(activeJobs)) return list.map((ws) => ({ ...ws }));
  const map = mapActiveJobsByWorkspace(activeJobs);
  return list.map((ws) => {
    const id = String((ws && ws.id) || "").trim();
    if (!id) return ws;
    const activeJobId = map.get(id) || "";
    return {
      ...ws,
      active_job_id: activeJobId,
    };
  });
}

async function applyActiveWorkspaceState(snapshot, options) {
  const opts = options || {};
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
    const skipSceneReload = currentScene === wsScene
      && String(opts.skipSceneReloadIfCurrent || "").trim() === wsScene;
    const sceneChanged = currentScene !== wsScene;
    const previousScene = currentScene;
    const previousVariant = selectedSceneVariantValue();
    const previousCamera = String(el.camera && el.camera.value ? el.camera.value : "").trim();
    try {
      if (sceneChanged) {
        el.scene.value = wsScene;
        setSceneBrowserSelectedFile(wsScene);
        localStorage.setItem(LAST_SCENE_KEY, wsScene);
        updateSceneDependencyPill(wsScene);
      }
      if (!skipSceneReload) {
        await loadVariants(wsScene);
        const variantName = selectedSceneVariantValue();
        await Promise.all([
          loadCameras(wsScene, variantName),
          loadSceneSource(wsScene),
        ]);
        if (visualEditor && editorViewMode === "visual" && sceneChanged) {
          try {
            await loadVisualSceneFromSelected();
          } catch (err) {
            appendLog(`visual load error: ${err.message}`);
          }
        }
      }
      if (editorViewMode === "graph") renderSceneGraphView();
    } catch (err) {
      appendLog(`workspace scene load error (${wsScene}): ${err.message}`);
      if (previousScene && hasSceneOption(previousScene)) {
        el.scene.value = previousScene;
        setSceneBrowserSelectedFile(previousScene);
        localStorage.setItem(LAST_SCENE_KEY, previousScene);
        updateSceneDependencyPill(previousScene);
        try {
          await loadVariants(previousScene, previousVariant);
          await loadCameras(previousScene, previousVariant);
          if (previousCamera && cameraCatalogHasName(previousCamera)) el.camera.value = previousCamera;
          await loadSceneSource(previousScene);
        } catch (rollbackErr) {
          appendLog(`workspace rollback error: ${rollbackErr.message}`);
        }
      }
    }
  }

  await restorePreviewForActiveWorkspace();
  const wsActiveJobId = String((ws && ws.active_job_id) || "").trim();
  if (wsActiveJobId) {
    resumeWorkspaceJobPolling(wsActiveJobId);
  }
}

function renderWorkspaceList(items) {
  if (!el.workspaceList) return;
  const widgets = window.XTracerWidgets || {};
  el.workspaceList.innerHTML = "";
  const list = Array.isArray(items) ? items : [];
  const canDeleteAny = list.length >= 1;
  if (list.length === 0) {
    const empty = createWorkspaceEmptyState("No workspaces", "No workspaces available.", "workspace-empty");
    el.workspaceList.appendChild(empty);
    return;
  }

  list.forEach((ws) => {
    const id = String((ws && ws.id) || "");
    const hasOwned = !!(ws && Object.prototype.hasOwnProperty.call(ws, "is_owned_by_client"));
    const isMine = hasOwned ? !!ws.is_owned_by_client : !!(ws && ws.is_active_for_client);
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

    const isRendering = !!activeJob;

    const head = document.createElement("header");
    head.className = "workspace-item-head";
    const title = document.createElement("h3");
    title.className = "workspace-item-title";
    title.textContent = String((ws && ws.name) || id || "Workspace");
    head.appendChild(title);
    const badgesEl = document.createElement("div");
    badgesEl.className = "workspace-item-badges";
    if (isMine) {
      const mineBadge = createWorkspaceStateBadge("This Client", "workspace-item-state");
      badgesEl.appendChild(mineBadge);
    }
    head.appendChild(badgesEl);

    const actions = document.createElement("div");
    actions.className = "workspace-item-actions";
    const useBtn = createWorkspaceActionButton(id === activeWorkspaceId ? "Active" : "Use", {
      variant: id === activeWorkspaceId ? "secondary" : "primary",
      className: "workspace-item-action-btn",
      disabled: !id || id === activeWorkspaceId,
      onClick: () => {
      switchActiveWorkspace(id).catch((err) => {
        appendLog(`workspace switch error: ${err.message}`);
      });
      },
    });
    actions.appendChild(useBtn);

    const deleteBtn = createWorkspaceActionButton("Delete", {
      variant: "ghost",
      className: "workspace-item-action-btn",
      disabled: !id || !canDeleteAny,
      onClick: () => {
        if (!id || !hasBackendMethod(api, "deleteWorkspace")) return;
        const widgets = window.XTracerWidgets;
        const doDelete = () => {
          api.deleteWorkspace(id)
            .then(() => refreshWorkspaces())
            .then(() => {
              appendLog(`deleted workspace: ${id}`);
              if (widgets && typeof widgets.showToast === "function") {
                widgets.showToast({ message: `Workspace "${id}" deleted`, tone: "success" });
              }
            })
            .catch((err) => {
              appendLog(`workspace delete error: ${err.message}`);
              if (widgets && typeof widgets.showToast === "function") {
                widgets.showToast({ message: `Delete failed: ${err.message}`, tone: "error" });
              }
            });
        };
        if (widgets && typeof widgets.showModal === "function") {
          widgets.showModal({
            title: "Delete Workspace",
            body: `Delete workspace "${id}"? This cannot be undone.`,
            confirmLabel: "Delete",
            danger: true,
            onConfirm: doDelete,
          });
        } else {
          if (window.confirm(`Delete workspace ${id}?`)) doDelete();
        }
      },
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
          const emptyPreview = createWorkspaceEmptyState("", "No render yet", "workspace-item-preview-empty");
          previewWrap.appendChild(emptyPreview);
        }
      };
      img.src = previewUrl;
      previewWrap.appendChild(img);
    } else if (!isRendering) {
      const emptyPreview = createWorkspaceEmptyState("", "No render yet", "workspace-item-preview-empty");
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
    addMeta("This Client", isMine ? "Yes" : "No");
    addMeta("Scene", scene || "-");
    addMeta("Clients", String(clients));
    addMeta("Drafts", String(drafts));
    addMeta("Job", activeJob || lastJob || "-");
    addMeta("TLAS Nodes", spatialNodes);
    addMeta("Finite/Infinite", `${spatialFinite}/${spatialInfinite}`);
    addMeta("TLAS Build", spatialBuildMs);
    addMeta("Updated", formatWorkspaceUpdated(ws && ws.updated_ms));

    if (workspaceViewMode === "list") {
      const previewCol = document.createElement("div");
      previewCol.className = "workspace-item-preview-col";
      previewCol.appendChild(previewWrap);

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
      addChip("This Client", isMine ? "Yes" : "No");
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
      const card = typeof widgets.createWorkspaceCard === "function"
        ? widgets.createWorkspaceCard({
          active: !!(id && id === activeWorkspaceId),
          rendering: isRendering,
          listMode: true,
          previewColumn: previewCol,
          main,
          actions,
        })
        : (() => {
          const legacyCard = document.createElement("article");
          legacyCard.className = "workspace-item workspace-item-list-compact";
          if (id && id === activeWorkspaceId) legacyCard.classList.add("is-active");
          if (isRendering) legacyCard.classList.add("is-rendering");
          legacyCard.appendChild(previewCol);
          legacyCard.appendChild(main);
          legacyCard.appendChild(actions);
          return legacyCard;
        })();
      el.workspaceList.appendChild(card);
      return;
    }

    const card = typeof widgets.createWorkspaceCard === "function"
      ? widgets.createWorkspaceCard({
        active: !!(id && id === activeWorkspaceId),
        rendering: isRendering,
        head,
        preview: previewWrap,
        meta,
        actions,
      })
      : (() => {
        const legacyCard = document.createElement("article");
        legacyCard.className = "workspace-item";
        if (id && id === activeWorkspaceId) legacyCard.classList.add("is-active");
        if (isRendering) legacyCard.classList.add("is-rendering");
        legacyCard.appendChild(head);
        legacyCard.appendChild(previewWrap);
        legacyCard.appendChild(meta);
        legacyCard.appendChild(actions);
        return legacyCard;
      })();
    el.workspaceList.appendChild(card);
  });
}

async function refreshWorkspaces() {
  if (!hasBackendMethod(api, "getWorkspaces")) return;
  const [payload, activeJobs] = await Promise.all([
    api.getWorkspaces(),
    Promise.resolve(getActiveJobsFromCache()),
  ]);
  workspaceSpatialIndexStats = payload && payload.spatial_index && typeof payload.spatial_index === "object"
    ? payload.spatial_index
    : null;
  const workspaceItems = overlayWorkspaceActiveJobs((payload && payload.workspaces) || [], activeJobs);
  const sortedWorkspaceItems = sortWorkspaceItems(workspaceItems);
  cacheWorkspaceSnapshots(sortedWorkspaceItems);
  const nextActive = String((payload && payload.active_workspace) || "").trim();
  let activeChanged = false;
  if (nextActive && nextActive !== activeWorkspaceId) {
    syncGlobalsToWorkspaceRuntime();
    activeWorkspaceId = nextActive;
    syncWorkspaceRuntimeToGlobals();
    activeChanged = true;
  }
  updateWorkspaceActiveHint();
  updateWorkspaceCountHint(Array.isArray(sortedWorkspaceItems) ? sortedWorkspaceItems.length : 0);
  renderWorkspaceList(sortedWorkspaceItems);
  if (activeChanged) {
    await applyActiveWorkspaceState(workspaceSnapshotById.get(activeWorkspaceId) || null);
    if (typeof syncRenderTabEnabled === "function") syncRenderTabEnabled();
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
