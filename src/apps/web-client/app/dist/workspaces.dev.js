"use strict";

function _slicedToArray(arr, i) { return _arrayWithHoles(arr) || _iterableToArrayLimit(arr, i) || _nonIterableRest(); }

function _nonIterableRest() { throw new TypeError("Invalid attempt to destructure non-iterable instance"); }

function _iterableToArrayLimit(arr, i) { if (!(Symbol.iterator in Object(arr) || Object.prototype.toString.call(arr) === "[object Arguments]")) { return; } var _arr = []; var _n = true; var _d = false; var _e = undefined; try { for (var _i = arr[Symbol.iterator](), _s; !(_n = (_s = _i.next()).done); _n = true) { _arr.push(_s.value); if (i && _arr.length === i) break; } } catch (err) { _d = true; _e = err; } finally { try { if (!_n && _i["return"] != null) _i["return"](); } finally { if (_d) throw _e; } } return _arr; }

function _arrayWithHoles(arr) { if (Array.isArray(arr)) return arr; }

function ownKeys(object, enumerableOnly) { var keys = Object.keys(object); if (Object.getOwnPropertySymbols) { var symbols = Object.getOwnPropertySymbols(object); if (enumerableOnly) symbols = symbols.filter(function (sym) { return Object.getOwnPropertyDescriptor(object, sym).enumerable; }); keys.push.apply(keys, symbols); } return keys; }

function _objectSpread(target) { for (var i = 1; i < arguments.length; i++) { var source = arguments[i] != null ? arguments[i] : {}; if (i % 2) { ownKeys(source, true).forEach(function (key) { _defineProperty(target, key, source[key]); }); } else if (Object.getOwnPropertyDescriptors) { Object.defineProperties(target, Object.getOwnPropertyDescriptors(source)); } else { ownKeys(source).forEach(function (key) { Object.defineProperty(target, key, Object.getOwnPropertyDescriptor(source, key)); }); } } return target; }

function _defineProperty(obj, key, value) { if (key in obj) { Object.defineProperty(obj, key, { value: value, enumerable: true, configurable: true, writable: true }); } else { obj[key] = value; } return obj; }

function _typeof(obj) { if (typeof Symbol === "function" && typeof Symbol.iterator === "symbol") { _typeof = function _typeof(obj) { return typeof obj; }; } else { _typeof = function _typeof(obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; }; } return _typeof(obj); }

function _toConsumableArray(arr) { return _arrayWithoutHoles(arr) || _iterableToArray(arr) || _nonIterableSpread(); }

function _nonIterableSpread() { throw new TypeError("Invalid attempt to spread non-iterable instance"); }

function _iterableToArray(iter) { if (Symbol.iterator in Object(iter) || Object.prototype.toString.call(iter) === "[object Arguments]") return Array.from(iter); }

function _arrayWithoutHoles(arr) { if (Array.isArray(arr)) { for (var i = 0, arr2 = new Array(arr.length); i < arr.length; i++) { arr2[i] = arr[i]; } return arr2; } }

function updateWorkspaceActiveHint() {
  if (!el.workspaceActiveHint) return;
  var id = String(activeWorkspaceId || "").trim();
  window.XTracerWidgets.renderStatHint(el.workspaceActiveHint, {
    label: "Active workspace",
    value: id || "-",
    className: "workspace-active-hint"
  });
}

function updateWorkspaceCountHint(count) {
  if (!el.workspaceCountHint) return;
  window.XTracerWidgets.renderStatHint(el.workspaceCountHint, {
    label: "Workspaces",
    value: Math.max(0, Number(count) || 0),
    className: "workspace-active-hint"
  });
}

function updateWorkspaceServerStatHint(node, label, value) {
  if (!node) return;
  window.XTracerWidgets.renderStatHint(node, {
    label: label,
    value: value === null || value === undefined || value === "" ? "-" : String(value),
    className: "workspace-active-hint"
  });
}

function updateWorkspaceServerStatsHints(data) {
  var maxConcurrent = Number(data && data.max_concurrent_renders);
  var logicalCores = Number(data && data.logical_cores);
  var openmpThreads = Number(data && data.openmp_max_threads);
  var renderReserveThreads = Number(data && data.render_reserve_threads);
  var renderAutoThreads = Number(data && data.render_auto_threads);
  updateWorkspaceServerStatHint(el.workspaceMaxConcurrentHint, "Max Concurrent Renders", Number.isFinite(maxConcurrent) && maxConcurrent > 0 ? Math.floor(maxConcurrent) : "-");
  updateWorkspaceServerStatHint(el.workspaceThreadsHint, "Hardware Threads", Number.isFinite(logicalCores) && logicalCores > 0 ? Math.floor(logicalCores) : "-");
  updateWorkspaceServerStatHint(el.workspaceOpenmpHint, "OpenMP Max Threads", Number.isFinite(openmpThreads) && openmpThreads > 0 ? Math.floor(openmpThreads) : "-");
  updateWorkspaceServerStatHint(el.workspaceRenderReserveHint, "Render Reserve Threads", Number.isFinite(renderReserveThreads) && renderReserveThreads >= 0 ? Math.floor(renderReserveThreads) : "-");
  updateWorkspaceServerStatHint(el.workspaceRenderAutoHint, "Render Auto Threads", Number.isFinite(renderAutoThreads) && renderAutoThreads > 0 ? Math.floor(renderAutoThreads) : "-");
  settingsJobsTotalRenderThreads = Number.isFinite(renderAutoThreads) && renderAutoThreads > 0 ? Math.floor(renderAutoThreads) : 0;
  var computedThreadLimit = Number.isFinite(renderAutoThreads) && renderAutoThreads > 0 ? Math.floor(renderAutoThreads) : Number.isFinite(openmpThreads) && openmpThreads > 0 ? Math.max(1, Math.floor(openmpThreads) - Math.max(0, Math.floor(renderReserveThreads) || 0)) : 256;

  if (el.threads) {
    el.threads.max = String(computedThreadLimit);
    el.threads.dataset.autoThreadCount = String(computedThreadLimit);
    el.threads.title = "0 uses auto mode. Maximum manual value: ".concat(computedThreadLimit, ".");
    var current = Math.max(0, Number.parseInt(String(el.threads.value || "0"), 10) || 0);
    if (current > computedThreadLimit) el.threads.value = String(computedThreadLimit);
  }

  if (el.threadsLabelText) {
    el.threadsLabelText.textContent = "Threads";
  }

  if (el.threadsLabelSubtext) {
    el.threadsLabelSubtext.textContent = "".concat(computedThreadLimit, " max");
  }

  if (typeof syncTileSizeControlUi === "function") syncTileSizeControlUi();
  renderSettingsJobsThreadGraph();
}

var settingsJobsTotalRenderThreads = 0;
var settingsJobsAbortInFlight = new Set();
var settingsJobsMoveInFlight = new Set();
var SETTINGS_JOBS_GRAPH_WINDOW_MS = 60 * 1000;
var settingsJobsThreadSamples = [];

function resetSettingsJobsThreadGraph() {
  settingsJobsThreadSamples = [];
  renderSettingsJobsThreadGraph();
}

function pruneSettingsJobsThreadSamples(nowMs) {
  var now = Number.isFinite(nowMs) ? nowMs : Date.now();
  var minTs = now - SETTINGS_JOBS_GRAPH_WINDOW_MS; // Find the most recent sample that sits before the window start.  Its value
  // is what was current at the left edge, so clip it to minTs and use it as
  // an anchor instead of discarding it — otherwise the left portion of the
  // graph shows the wrong value when older samples roll out of the window.

  var anchorIdx = -1;

  for (var i = settingsJobsThreadSamples.length - 1; i >= 0; i--) {
    var s = settingsJobsThreadSamples[i];

    if (s && Number.isFinite(s.ts) && s.ts < minTs) {
      anchorIdx = i;
      break;
    }
  }

  if (anchorIdx >= 0) {
    settingsJobsThreadSamples[anchorIdx] = {
      ts: minTs,
      value: settingsJobsThreadSamples[anchorIdx].value
    };
    settingsJobsThreadSamples = settingsJobsThreadSamples.slice(anchorIdx);
  }
}

function getSettingsJobsOccupiedThreads(activeJobs) {
  var jobs = Array.isArray(activeJobs) ? activeJobs : [];
  return jobs.reduce(function (sum, job) {
    var state = String(job && job.state || "").toLowerCase();
    if (state !== "running" && state !== "preparing") return sum;
    var threads = Math.max(0, Number(job && job.threads || 0));
    return sum + (Number.isFinite(threads) ? Math.floor(threads) : 0);
  }, 0);
}

function recordSettingsJobsThreadUsage(occupied, nowMs) {
  var now = Number.isFinite(nowMs) ? nowMs : Date.now();
  var value = Math.max(0, Math.floor(Number(occupied) || 0));
  pruneSettingsJobsThreadSamples(now);
  var last = settingsJobsThreadSamples.length ? settingsJobsThreadSamples[settingsJobsThreadSamples.length - 1] : null;

  if (last && now - last.ts < 250) {
    last.ts = now;
    last.value = value;
    return;
  }

  settingsJobsThreadSamples.push({
    ts: now,
    value: value
  });
  pruneSettingsJobsThreadSamples(now);
}

function createSettingsJobsGraphSvgNode(tag, attrs) {
  var ns = "http://www.w3.org/2000/svg";
  var node = document.createElementNS(ns, tag);
  var attributes = attrs || {};
  Object.keys(attributes).forEach(function (key) {
    var value = attributes[key];
    if (value === undefined || value === null) return;
    node.setAttribute(key, String(value));
  });
  return node;
}

function buildSettingsJobsGraphPaths(samples, nowMs, maxScale, width, height) {
  var now = Number.isFinite(nowMs) ? nowMs : Date.now();
  var list = Array.isArray(samples) && samples.length ? samples : [{
    ts: now,
    value: 0
  }];
  var startTs = now - SETTINGS_JOBS_GRAPH_WINDOW_MS;
  var safeScale = Math.max(1, Number(maxScale) || 1);

  var xForTs = function xForTs(ts) {
    if (!Number.isFinite(ts) || ts <= startTs) return 0;
    if (ts >= now) return width;
    return (ts - startTs) / SETTINGS_JOBS_GRAPH_WINDOW_MS * width;
  };

  var yForValue = function yForValue(value) {
    var clamped = Math.max(0, Math.min(safeScale, Number(value) || 0));
    return height - clamped / safeScale * height;
  };

  var prevX = 0;
  var prevY = yForValue(list[0].value);
  var line = "M 0 ".concat(prevY.toFixed(2));
  var area = "M 0 ".concat(height.toFixed(2), " L 0 ").concat(prevY.toFixed(2));
  list.forEach(function (sample, index) {
    var x = xForTs(sample.ts);
    var y = yForValue(sample.value);

    if (index === 0) {
      if (x > 0) {
        line += " L ".concat(x.toFixed(2), " ").concat(prevY.toFixed(2));
        area += " L ".concat(x.toFixed(2), " ").concat(prevY.toFixed(2));
      }

      prevX = x;
      prevY = y;
      return;
    }

    if (x < prevX) x = prevX;

    if (x > prevX) {
      line += " L ".concat(x.toFixed(2), " ").concat(prevY.toFixed(2));
      area += " L ".concat(x.toFixed(2), " ").concat(prevY.toFixed(2));
    }

    if (Math.abs(y - prevY) > 0.001) {
      line += " L ".concat(x.toFixed(2), " ").concat(y.toFixed(2));
      area += " L ".concat(x.toFixed(2), " ").concat(y.toFixed(2));
    }

    prevX = x;
    prevY = y;
  });

  if (prevX < width) {
    line += " L ".concat(width.toFixed(2), " ").concat(prevY.toFixed(2));
    area += " L ".concat(width.toFixed(2), " ").concat(prevY.toFixed(2));
  }

  area += " L ".concat(width.toFixed(2), " ").concat(height.toFixed(2), " Z");
  return {
    area: area,
    line: line,
    lastX: width,
    lastY: prevY
  };
}

function renderSettingsJobsThreadGraph() {
  if (!el.settingsJobsThreadGraph) return;
  var now = Date.now();
  pruneSettingsJobsThreadSamples(now);
  var samples = settingsJobsThreadSamples.slice();
  var latest = samples.length ? Math.max(0, Math.floor(Number(samples[samples.length - 1].value) || 0)) : 0;
  var peak = samples.reduce(function (max, sample) {
    return Math.max(max, Math.max(0, Math.floor(Number(sample && sample.value) || 0)));
  }, 0);
  var total = Math.max(0, Number(settingsJobsTotalRenderThreads) || 0);
  var scaleMax = Math.max(1, total, peak);
  var currentLabel = total > 0 ? "".concat(latest, " / ").concat(total) : "".concat(latest, " / -");
  var widgets = window.XTracerWidgets || {};
  if (widgets.dom && typeof widgets.dom.clear === "function") widgets.dom.clear(el.settingsJobsThreadGraph);else el.settingsJobsThreadGraph.innerHTML = "";
  el.settingsJobsThreadGraph.setAttribute("aria-label", "Threads in use over the last minute. Current usage ".concat(currentLabel, ". Peak ").concat(peak, "."));
  var header = document.createElement("div");
  header.className = "settings-jobs-graph-header";
  var titleGroup = document.createElement("div");
  titleGroup.className = "settings-jobs-graph-title-group";
  var title = document.createElement("p");
  title.className = "settings-jobs-graph-title";
  title.textContent = "Threads In Use";
  var subtitle = document.createElement("p");
  subtitle.className = "settings-jobs-graph-subtitle";
  subtitle.textContent = "Rolling 1 minute window";
  titleGroup.appendChild(title);
  titleGroup.appendChild(subtitle);
  var statGroup = document.createElement("div");
  statGroup.className = "settings-jobs-graph-stats";
  var peakNode = document.createElement("span");
  peakNode.className = "settings-jobs-graph-stat";
  peakNode.textContent = "peak ".concat(peak);
  var currentNode = document.createElement("span");
  currentNode.className = "settings-jobs-graph-stat";
  currentNode.textContent = "now ".concat(currentLabel);
  statGroup.appendChild(peakNode);
  statGroup.appendChild(currentNode);
  header.appendChild(titleGroup);
  header.appendChild(statGroup);
  el.settingsJobsThreadGraph.appendChild(header);
  var chart = document.createElement("div");
  chart.className = "settings-jobs-graph-chart";

  if (!samples.length) {
    var empty = document.createElement("p");
    empty.className = "settings-jobs-graph-empty";
    empty.textContent = "Waiting for the first jobs sample.";
    chart.appendChild(empty);
  } else {
    var width = 240;
    var height = 72;
    var svg = createSettingsJobsGraphSvgNode("svg", {
      "class": "settings-jobs-graph-svg",
      viewBox: "0 0 ".concat(width, " ").concat(height),
      preserveAspectRatio: "none",
      "aria-hidden": "true"
    });
    [0, height * 0.5, height].forEach(function (y) {
      svg.appendChild(createSettingsJobsGraphSvgNode("line", {
        "class": "settings-jobs-graph-grid-line",
        x1: 0,
        y1: y.toFixed(2),
        x2: width,
        y2: y.toFixed(2)
      }));
    });
    var paths = buildSettingsJobsGraphPaths(samples, now, scaleMax, width, height);
    svg.appendChild(createSettingsJobsGraphSvgNode("path", {
      "class": "settings-jobs-graph-area",
      d: paths.area
    }));
    svg.appendChild(createSettingsJobsGraphSvgNode("path", {
      "class": "settings-jobs-graph-line",
      d: paths.line
    }));
    svg.appendChild(createSettingsJobsGraphSvgNode("circle", {
      "class": "settings-jobs-graph-dot",
      cx: paths.lastX.toFixed(2),
      cy: paths.lastY.toFixed(2),
      r: "3"
    }));
    chart.appendChild(svg);
  }

  el.settingsJobsThreadGraph.appendChild(chart);
  var footer = document.createElement("div");
  footer.className = "settings-jobs-graph-footer";
  var agoNode = document.createElement("span");
  agoNode.className = "settings-jobs-graph-footnote";
  agoNode.textContent = "60s ago";
  var scaleNode = document.createElement("span");
  scaleNode.className = "settings-jobs-graph-footnote";
  scaleNode.textContent = "scale 0-".concat(scaleMax);
  var nowNode = document.createElement("span");
  nowNode.className = "settings-jobs-graph-footnote";
  nowNode.textContent = "now";
  footer.appendChild(agoNode);
  footer.appendChild(scaleNode);
  footer.appendChild(nowNode);
  el.settingsJobsThreadGraph.appendChild(footer);
}

function bindSettingsJobsCardLifecycle() {// Jobs panel is now a standalone modal; lifecycle handled by openJobsModal/closeJobsModal
}

function updateJobsBadge() {
  var jobs = getActiveJobsFromCache();
  var count = jobs.length;
  [el.topbarJobsBtn, el.bnTabJobs].filter(Boolean).forEach(function (btn) {
    var badge = btn.querySelector(".topbar-btn-badge");
    if (!badge) return;

    if (count > 0) {
      badge.textContent = count > 99 ? "99+" : String(count);
      badge.hidden = false;
    } else {
      badge.textContent = "";
      badge.hidden = true;
    }
  });
}

function notifyActiveJobsChanged() {
  if (activeTabMode === "workspaces" && hasBackendMethod(api, "getWorkspaces")) {
    var activeJobs = getActiveJobsFromCache();
    var cached = Array.from(workspaceSnapshotById.values());
    var items = sortWorkspaceItems(overlayWorkspaceActiveJobs(cached, activeJobs));
    renderWorkspaceList(items);
  }

  if (typeof refreshSettingsJobsCard === "function" && el.settingsJobsList) {
    refreshSettingsJobsCard();
  }

  updateJobsBadge();
}

function parseJobSequence(jobId) {
  var id = String(jobId || "");
  var m = id.match(/^job_(\d+)$/);
  return m ? Number(m[1]) : 0;
}

function compareActiveJobsForSettings(a, b) {
  var stateA = String(a && a.state || "").toLowerCase();
  var stateB = String(b && b.state || "").toLowerCase();

  var priority = function priority(state) {
    if (state === "running") return 0;
    if (state === "aborting") return 1;
    if (state === "preparing") return 2;
    if (state === "queued") return 3;
    return 4;
  };

  var pa = priority(stateA);
  var pb = priority(stateB);
  if (pa !== pb) return pa - pb;
  var sa = parseJobSequence(a && a.id);
  var sb = parseJobSequence(b && b.id);
  if (sa !== sb) return sb - sa;
  return String(a && a.id || "").localeCompare(String(b && b.id || ""));
}

function formatJobElapsedMs(ms) {
  var elapsed = Math.max(0, Number(ms) || 0);
  if (!Number.isFinite(elapsed) || elapsed <= 0) return "-";
  return formatElapsed(elapsed);
}

function createSettingsJobActionIcon(kind) {
  if (window.XTracerWidgets && window.XTracerWidgets.dom && typeof window.XTracerWidgets.dom.svgIcon === "function") {
    if (kind === "up") return window.XTracerWidgets.dom.svgIcon("M8 12V4M8 4L5.4 6.6M8 4l2.6 2.6");
    if (kind === "down") return window.XTracerWidgets.dom.svgIcon("M8 4v8M8 12l-2.6-2.6M8 12l2.6-2.6");
    return window.XTracerWidgets.dom.svgIcon("M5 5l6 6M11 5L5 11");
  }

  var ns = "http://www.w3.org/2000/svg";
  var svg = document.createElementNS(ns, "svg");
  svg.setAttribute("viewBox", "0 0 16 16");
  svg.setAttribute("aria-hidden", "true");
  svg.classList.add("settings-job-action-icon");
  var path = document.createElementNS(ns, "path");
  path.setAttribute("fill", "none");
  path.setAttribute("stroke", "currentColor");
  path.setAttribute("stroke-width", "1.8");
  path.setAttribute("stroke-linecap", "round");
  path.setAttribute("stroke-linejoin", "round");
  if (kind === "up") path.setAttribute("d", "M8 12V4M8 4L5.4 6.6M8 4l2.6 2.6");else if (kind === "down") path.setAttribute("d", "M8 4v8M8 12l-2.6-2.6M8 12l2.6-2.6");else path.setAttribute("d", "M5 5l6 6M11 5L5 11");
  svg.appendChild(path);
  return svg;
}

function createSettingsJobStateTag(state) {
  if (window.XTracerWidgets && typeof window.XTracerWidgets.createTag === "function") {
    var tone = state === "running" ? "success" : state === "aborting" ? "warning" : state === "preparing" ? "info" : state === "queued" ? "warning" : "neutral";
    return window.XTracerWidgets.createTag({
      label: state,
      tone: tone,
      className: "settings-job-state-pill state-".concat(state)
    });
  }

  var node = document.createElement("span");
  node.className = "settings-job-state-pill state-".concat(state);
  node.textContent = state;
  return node;
}

function createSettingsJobActionButton(kind, opts) {
  var options = opts || {};

  if (window.XTracerWidgets && typeof window.XTracerWidgets.createIconButton === "function") {
    return window.XTracerWidgets.createIconButton({
      title: options.title || "",
      label: options.ariaLabel || options.title || "",
      disabled: !!options.disabled,
      variant: "ghost",
      className: options.className || "",
      icon: createSettingsJobActionIcon(kind),
      onClick: options.onClick
    });
  }

  var btn = document.createElement("button");
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
      className: "settings-job-progress"
    });
  }

  var progressBar = document.createElement("div");
  progressBar.className = "settings-job-progress";
  var progressFill = document.createElement("span");
  progressFill.style.width = "".concat((progress * 100).toFixed(1), "%");
  progressBar.appendChild(progressFill);
  return progressBar;
}

function createWorkspaceStateBadge(label, className) {
  var text = String(label || "").trim() || "Unknown";
  var normalized = text.toLowerCase();

  if (window.XTracerWidgets && typeof window.XTracerWidgets.createTag === "function") {
    var tone = "neutral";
    if (normalized === "active" || normalized === "rendering") tone = "success";
    return window.XTracerWidgets.createTag({
      label: text,
      tone: tone,
      className: className || ""
    });
  }

  var node = document.createElement("span");
  node.className = className || "";
  node.textContent = text;
  return node;
}

function createWorkspaceActionButton(label, opts) {
  var options = opts || {};

  if (window.XTracerWidgets && typeof window.XTracerWidgets.createButton === "function") {
    return window.XTracerWidgets.createButton({
      label: label,
      variant: options.variant || "secondary",
      disabled: !!options.disabled,
      className: options.className || "",
      onClick: options.onClick
    });
  }

  var btn = document.createElement("button");
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
      title: title,
      message: message,
      className: className || ""
    });
  }

  var node = document.createElement("p");
  node.className = className || "";
  node.textContent = message || title || "";
  return node;
}

function renderSettingsJobsList(activeJobs) {
  if (!el.settingsJobsList) return;
  var widgets = window.XTracerWidgets || {};
  var jobs = Array.isArray(activeJobs) ? _toConsumableArray(activeJobs) : [];
  el.settingsJobsList.innerHTML = "";
  el.settingsJobsList.hidden = false;

  if (!jobs.length) {
    el.settingsJobsList.hidden = true;
    return;
  }

  var queuedJobs = jobs.filter(function (job) {
    return String(job && job.state || "").toLowerCase() === "queued";
  });
  var queuedIndexById = new Map();
  queuedJobs.forEach(function (job, i) {
    var id = String(job && job.id || "").trim();
    if (!id) return;
    queuedIndexById.set(id, i);
  });
  var queuedCount = queuedJobs.length;
  jobs.forEach(function (job) {
    var id = String(job && job.id || "").trim() || "-";
    var workspaceId = String(job && job.workspace_id || "").trim() || "-";
    var state = String(job && job.state || "").toLowerCase() || "unknown";
    var threads = Math.max(0, Number(job && job.threads || 0));
    var progress = Math.max(0, Math.min(1, Number(job && job.progress || 0)));
    var elapsed = formatJobElapsedMs(job && job.elapsed_ms);
    var scene = String(job && job.scene || "").trim() || "-";
    var integrator = String(job && job.integrator || "").trim() || "-";
    var statePill = createSettingsJobStateTag(state);
    var controlNodes = [];
    var queueIdx = queuedIndexById.has(id) ? queuedIndexById.get(id) : -1;
    var isMoving = settingsJobsMoveInFlight.has(id);

    if (state === "queued" && queueIdx >= 0) {
      var queueControls = document.createElement("div");
      queueControls.className = "settings-job-queue-controls";
      var upBtn = createSettingsJobActionButton("up", {
        className: "settings-job-queue-btn",
        ariaLabel: "Move job up",
        title: "Move up",
        disabled: isMoving || queueIdx <= 0,
        onClick: function onClick() {
          moveSettingsJobQueue(id, "up")["catch"](function (err) {
            return appendLog("settings queue move up error: ".concat(err.message));
          });
        }
      });
      var downBtn = createSettingsJobActionButton("down", {
        className: "settings-job-queue-btn",
        ariaLabel: "Move job down",
        title: "Move down",
        disabled: isMoving || queueIdx >= queuedCount - 1,
        onClick: function onClick() {
          moveSettingsJobQueue(id, "down")["catch"](function (err) {
            return appendLog("settings queue move down error: ".concat(err.message));
          });
        }
      });
      queueControls.appendChild(upBtn);
      queueControls.appendChild(downBtn);
      controlNodes.push(queueControls);
    }

    if (state === "running" || state === "preparing" || state === "queued") {
      var abortBtn = createSettingsJobActionButton("abort", {
        className: "settings-job-abort-btn",
        ariaLabel: settingsJobsAbortInFlight.has(id) ? "Aborting" : "Abort job",
        title: settingsJobsAbortInFlight.has(id) ? "Aborting" : "Abort",
        disabled: settingsJobsAbortInFlight.has(id),
        onClick: function onClick() {
          abortSettingsJob(id, workspaceId)["catch"](function (err) {
            return appendLog("settings abort error: ".concat(err.message));
          });
        }
      });
      controlNodes.push(abortBtn);
    }

    var threadsNode = document.createElement("span");
    threadsNode.className = "settings-job-meta-pill";
    threadsNode.textContent = threads === 1 ? "1 thread" : "".concat(threads, " threads");
    var elapsedNode = document.createElement("span");
    elapsedNode.className = "settings-job-meta-pill";
    elapsedNode.textContent = elapsed;
    var progressNode = document.createElement("span");
    progressNode.className = "settings-job-meta-pill";
    progressNode.textContent = "".concat((progress * 100).toFixed(1), "%");
    var progressBar = createSettingsJobProgress(progress);
    var item = widgets.createJobRow({
      id: id,
      state: state,
      pills: [statePill],
      workspaceLabel: "ws ".concat(workspaceId),
      metrics: [threadsNode, elapsedNode, progressNode],
      progress: progressBar,
      subtext: "".concat(scene, " \xB7 ").concat(integrator),
      controls: controlNodes
    });
    item.dataset.jobId = id;
    el.settingsJobsList.appendChild(item);
  });
}

function refreshSettingsJobsCard() {
  if (!el.settingsJobsList) return Promise.resolve();
  var activeJobs = getActiveJobsFromCache();
  var occupied = getSettingsJobsOccupiedThreads(activeJobs);
  recordSettingsJobsThreadUsage(occupied);
  renderSettingsJobsList(activeJobs);
  renderSettingsJobsThreadGraph();

  if (el.settingsJobsUpdated) {
    var ts = new Date();
    var hh = String(ts.getHours()).padStart(2, "0");
    var mm = String(ts.getMinutes()).padStart(2, "0");
    var ss = String(ts.getSeconds()).padStart(2, "0");
    updateWorkspaceServerStatHint(el.settingsJobsUpdated, "Updated", "".concat(hh, ":").concat(mm, ":").concat(ss));
  }

  if (el.settingsJobsThreadsUsage) {
    var total = Math.max(0, Number(settingsJobsTotalRenderThreads) || 0);
    var value = total > 0 ? "".concat(occupied, " / ").concat(total) : "".concat(occupied, " / -");
    updateWorkspaceServerStatHint(el.settingsJobsThreadsUsage, "Threads In Use", value);
  }

  return Promise.resolve();
}

function isJobsControlsCardVisible() {
  var modal = document.getElementById("jobsModal");
  return !!(modal && !modal.hidden);
}

function moveSettingsJobQueue(jobId, direction) {
  var id, dir, fn, key;
  return regeneratorRuntime.async(function moveSettingsJobQueue$(_context) {
    while (1) {
      switch (_context.prev = _context.next) {
        case 0:
          id = String(jobId || "").trim();
          dir = String(direction || "").toLowerCase();

          if (id) {
            _context.next = 4;
            break;
          }

          return _context.abrupt("return");

        case 4:
          if (!(dir !== "up" && dir !== "down")) {
            _context.next = 6;
            break;
          }

          return _context.abrupt("return");

        case 6:
          fn = dir === "up" ? "moveJobQueueUp" : "moveJobQueueDown";

          if (hasBackendMethod(api, fn)) {
            _context.next = 9;
            break;
          }

          return _context.abrupt("return");

        case 9:
          key = "".concat(id, ":").concat(dir);

          if (!(settingsJobsMoveInFlight.has(id) || settingsJobsMoveInFlight.has(key))) {
            _context.next = 12;
            break;
          }

          return _context.abrupt("return");

        case 12:
          settingsJobsMoveInFlight.add(id);
          settingsJobsMoveInFlight.add(key);
          _context.prev = 14;

          if (!(dir === "up")) {
            _context.next = 20;
            break;
          }

          _context.next = 18;
          return regeneratorRuntime.awrap(api.moveJobQueueUp(id));

        case 18:
          _context.next = 22;
          break;

        case 20:
          _context.next = 22;
          return regeneratorRuntime.awrap(api.moveJobQueueDown(id));

        case 22:
          appendLog("settings queue move ".concat(dir, " ").concat(id));
          _context.next = 28;
          break;

        case 25:
          _context.prev = 25;
          _context.t0 = _context["catch"](14);
          appendLog("settings queue move ".concat(dir, " failed for ").concat(id, ": ").concat(_context.t0.message));

        case 28:
          _context.prev = 28;
          settingsJobsMoveInFlight["delete"](key);
          settingsJobsMoveInFlight["delete"](id);
          notifyActiveJobsChanged();
          return _context.finish(28);

        case 33:
        case "end":
          return _context.stop();
      }
    }
  }, null, null, [[14, 25, 28, 33]]);
}

function abortSettingsJob(jobId, jobWorkspaceId) {
  var id;
  return regeneratorRuntime.async(function abortSettingsJob$(_context2) {
    while (1) {
      switch (_context2.prev = _context2.next) {
        case 0:
          id = String(jobId || "").trim();

          if (!(!id || !hasBackendMethod(api, "abortJob"))) {
            _context2.next = 3;
            break;
          }

          return _context2.abrupt("return");

        case 3:
          if (!settingsJobsAbortInFlight.has(id)) {
            _context2.next = 5;
            break;
          }

          return _context2.abrupt("return");

        case 5:
          settingsJobsAbortInFlight.add(id);
          _context2.prev = 6;
          appendLog("settings abort requested for ".concat(id));
          _context2.next = 10;
          return regeneratorRuntime.awrap(api.abortJob(id, jobWorkspaceId));

        case 10:
          appendLog("settings abort accepted for ".concat(id));
          _context2.next = 16;
          break;

        case 13:
          _context2.prev = 13;
          _context2.t0 = _context2["catch"](6);
          appendLog("settings abort failed for ".concat(id, ": ").concat(_context2.t0.message));

        case 16:
          _context2.prev = 16;
          settingsJobsAbortInFlight["delete"](id);
          notifyActiveJobsChanged();
          return _context2.finish(16);

        case 20:
        case "end":
          return _context2.stop();
      }
    }
  }, null, null, [[6, 13, 16, 20]]);
}

function normalizeWorkspaceViewMode(value) {
  return String(value || "").toLowerCase() === "list" ? "list" : "cards";
}

function normalizeWorkspaceSortMode(value) {
  var mode = String(value || "").toLowerCase();
  if (mode === "updated") return "updated";
  if (mode === "scene") return "scene";
  return "name";
}

function setWorkspaceViewMode(mode, persist) {
  workspaceViewMode = normalizeWorkspaceViewMode(mode);
  if (el.workspaceViewMode) el.workspaceViewMode.value = workspaceViewMode;

  if (el.workspaceViewCardsBtn) {
    var active = workspaceViewMode === "cards";
    el.workspaceViewCardsBtn.classList.toggle("active", active);
    el.workspaceViewCardsBtn.setAttribute("aria-pressed", active ? "true" : "false");
  }

  if (el.workspaceViewListBtn) {
    var _active = workspaceViewMode === "list";

    el.workspaceViewListBtn.classList.toggle("active", _active);
    el.workspaceViewListBtn.setAttribute("aria-pressed", _active ? "true" : "false");
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
    var active = workspaceSortMode === "name";
    el.workspaceSortNameBtn.classList.toggle("active", active);
    el.workspaceSortNameBtn.setAttribute("aria-pressed", active ? "true" : "false");
  }

  if (el.workspaceSortUpdatedBtn) {
    var _active2 = workspaceSortMode === "updated";

    el.workspaceSortUpdatedBtn.classList.toggle("active", _active2);
    el.workspaceSortUpdatedBtn.setAttribute("aria-pressed", _active2 ? "true" : "false");
  }

  if (el.workspaceSortSceneBtn) {
    var _active3 = workspaceSortMode === "scene";

    el.workspaceSortSceneBtn.classList.toggle("active", _active3);
    el.workspaceSortSceneBtn.setAttribute("aria-pressed", _active3 ? "true" : "false");
  }

  if (persist !== false) {
    localStorage.setItem(WORKSPACE_SORT_MODE_KEY, workspaceSortMode);
  }
}

function compareWorkspaceName(a, b) {
  var nameA = String(a && a.name || a && a.id || "").trim().toLowerCase();
  var nameB = String(b && b.name || b && b.id || "").trim().toLowerCase();
  var byName = nameA.localeCompare(nameB);
  if (byName !== 0) return byName;
  var idA = String(a && a.id || "").trim().toLowerCase();
  var idB = String(b && b.id || "").trim().toLowerCase();
  return idA.localeCompare(idB);
}

function compareWorkspaceUpdated(a, b) {
  var rawA = Number(a && a.updated_ms || 0);
  var rawB = Number(b && b.updated_ms || 0);
  var updatedA = Number.isFinite(rawA) ? rawA : 0;
  var updatedB = Number.isFinite(rawB) ? rawB : 0;
  if (updatedA !== updatedB) return updatedB - updatedA;
  return compareWorkspaceName(a, b);
}

function compareWorkspaceScene(a, b) {
  var sceneA = String(a && a.active_scene || "").trim().toLowerCase();
  var sceneB = String(b && b.active_scene || "").trim().toLowerCase();
  if (sceneA !== sceneB) return sceneA.localeCompare(sceneB);
  return compareWorkspaceName(a, b);
}

function sortWorkspaceItems(items) {
  var list = Array.isArray(items) ? _toConsumableArray(items) : [];

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
  var ms = Number(updatedMs || 0);
  if (!Number.isFinite(ms) || ms <= 0) return "-";
  var d = new Date(ms);
  if (Number.isNaN(d.getTime())) return "-";
  return d.toLocaleString();
}

function buildWorkspacePreviewUrl(lastJobId) {
  var jobId = String(lastJobId || "").trim();
  if (!jobId) return "";
  return "/api/jobs/".concat(encodeURIComponent(jobId), "/image?final=1&tm=aces");
}

var workspaceThumbCache = new Map(); // url → blob URL

var WORKSPACE_THUMB_CACHE_MAX = 20;

function loadWorkspacePreviewCanvas(url, _container, altLabel) {
  var makeImg, res, buf, w, h, canvas, ctx, blobUrl, firstKey;
  return regeneratorRuntime.async(function loadWorkspacePreviewCanvas$(_context3) {
    while (1) {
      switch (_context3.prev = _context3.next) {
        case 0:
          makeImg = function makeImg(blobUrl) {
            var img = document.createElement("img");
            img.alt = altLabel || "render preview";
            img.src = blobUrl;
            return img;
          };

          if (!workspaceThumbCache.has(url)) {
            _context3.next = 3;
            break;
          }

          return _context3.abrupt("return", makeImg(workspaceThumbCache.get(url)));

        case 3:
          _context3.prev = 3;
          _context3.next = 6;
          return regeneratorRuntime.awrap(fetch(url, {
            cache: "no-store"
          }));

        case 6:
          res = _context3.sent;

          if (res.ok) {
            _context3.next = 9;
            break;
          }

          return _context3.abrupt("return", null);

        case 9:
          _context3.next = 11;
          return regeneratorRuntime.awrap(res.arrayBuffer());

        case 11:
          buf = _context3.sent;
          w = parseInt(res.headers.get("X-XTracer-Width") || "0", 10);
          h = parseInt(res.headers.get("X-XTracer-Height") || "0", 10);

          if (!(!w || !h || buf.byteLength !== w * h * 4)) {
            _context3.next = 16;
            break;
          }

          return _context3.abrupt("return", null);

        case 16:
          canvas = document.createElement("canvas");
          canvas.width = w;
          canvas.height = h;
          ctx = canvas.getContext("2d");

          if (ctx) {
            _context3.next = 22;
            break;
          }

          return _context3.abrupt("return", null);

        case 22:
          ctx.putImageData(new ImageData(new Uint8ClampedArray(buf), w, h), 0, 0);
          _context3.next = 25;
          return regeneratorRuntime.awrap(new Promise(function (resolve) {
            canvas.toBlob(function (blob) {
              return resolve(blob ? URL.createObjectURL(blob) : null);
            }, "image/png");
          }));

        case 25:
          blobUrl = _context3.sent;

          if (blobUrl) {
            _context3.next = 28;
            break;
          }

          return _context3.abrupt("return", null);

        case 28:
          if (workspaceThumbCache.size >= WORKSPACE_THUMB_CACHE_MAX) {
            firstKey = workspaceThumbCache.keys().next().value;
            URL.revokeObjectURL(workspaceThumbCache.get(firstKey));
            workspaceThumbCache["delete"](firstKey);
          }

          workspaceThumbCache.set(url, blobUrl);
          return _context3.abrupt("return", makeImg(blobUrl));

        case 33:
          _context3.prev = 33;
          _context3.t0 = _context3["catch"](3);
          return _context3.abrupt("return", null);

        case 36:
        case "end":
          return _context3.stop();
      }
    }
  }, null, null, [[3, 33]]);
}

function workspaceStateLabel(workspace) {
  var id = String(workspace && workspace.id || "");
  var activeJob = String(workspace && workspace.active_job_id || "");
  if (activeJob) return "Rendering";
  var hasOwned = !!(workspace && Object.prototype.hasOwnProperty.call(workspace, "is_owned_by_client"));
  var isMine = hasOwned ? !!workspace.is_owned_by_client : !!(workspace && workspace.is_active_for_client);
  if (isMine || id && id === activeWorkspaceId) return "Active";
  return "Idle";
}

function serializeIntegratorControlState() {
  var out = {};
  integratorControlState.forEach(function (value, key) {
    if (!key || !value || _typeof(value) !== "object") return;
    out[key] = _objectSpread({}, value);
  });
  return out;
}

function workspaceSettingsPayload() {
  return {
    quality: {
      samples: String(el.samples && el.samples.value ? el.samples.value : "1"),
      aa: String(el.aa && el.aa.value ? el.aa.value : "1"),
      sample_distribution: String(el.sampleDistribution && el.sampleDistribution.value ? el.sampleDistribution.value : "grid"),
      rdepth: String(el.rdepth && el.rdepth.value ? el.rdepth.value : "15")
    },
    frame: {
      width: String(el.width && el.width.value ? el.width.value : "500"),
      height: String(el.height && el.height.value ? el.height.value : "500")
    },
    integrator: {
      id: String(el.integrator && el.integrator.value ? el.integrator.value : ""),
      tile_size: String(el.tileSize && el.tileSize.value ? el.tileSize.value : "32"),
      tile_order: String(el.tileOrder && el.tileOrder.value ? el.tileOrder.value : "random"),
      threads: String(el.threads && el.threads.value ? el.threads.value : "0"),
      controls_by_integrator: serializeIntegratorControlState()
    },
    tone_mapping: {
      operator: String(el.toneMapping && el.toneMapping.value ? el.toneMapping.value : "aces"),
      exposure: String(el.toneMappingExposure && el.toneMappingExposure.value ? el.toneMappingExposure.value : "1.0"),
      white_point: String(el.toneMappingWhitePoint && el.toneMappingWhitePoint.value ? el.toneMappingWhitePoint.value : "1.0"),
      mantiuk_contrast: String(el.toneMappingMantiukContrast && el.toneMappingMantiukContrast.value ? el.toneMappingMantiukContrast.value : "0.1"),
      mantiuk_saturation: String(el.toneMappingMantiukSaturation && el.toneMappingMantiukSaturation.value ? el.toneMappingMantiukSaturation.value : "0.8"),
      mantiuk_detail: String(el.toneMappingMantiukDetail && el.toneMappingMantiukDetail.value ? el.toneMappingMantiukDetail.value : "1.0")
    },
    preview: {
      render_mode: normalizeRenderMode(renderMode),
      interactive_speed: String((Number(interactivePreviewFlySpeedScale) || 1.0).toFixed(1)),
      interactive_moving_width: String(nearestInteractiveMovingWidth(interactivePreviewAdaptiveMovingWidth))
    },
    post_filters: Array.isArray(postFilterChain) ? postFilterChain.map(function (entry) {
      return normalizePostFilterEntry(entry);
    }).filter(function (entry) {
      return !!entry && !!entry.filter;
    }) : [],
    post_filters_enabled: !!postFilterStackEnabled,
    scene: String(el.scene && el.scene.value ? el.scene.value : ""),
    scene_variant: String(el.variant && el.variant.value !== undefined ? el.variant.value : "")
  };
}

function queueWorkspaceSettingsSave() {
  if (suppressWorkspaceSettingsSave) return;
  if (!activeWorkspaceId) return;
  if (!hasBackendMethod(api, "saveWorkspaceSettings")) return;
  var savedForWorkspace = activeWorkspaceId;

  if (workspaceSettingsSaveTimer) {
    clearTimeout(workspaceSettingsSaveTimer);
    workspaceSettingsSaveTimer = null;
  }

  workspaceSettingsSaveTimer = setTimeout(function () {
    workspaceSettingsSaveTimer = null;
    if (savedForWorkspace !== activeWorkspaceId) return;
    var json = "{}";

    try {
      json = JSON.stringify(workspaceSettingsPayload());
    } catch (_) {
      return;
    }

    api.saveWorkspaceSettings(json)["catch"](function (err) {
      return appendLog("workspace settings save failed: ".concat(err.message));
    });
  }, 250);
}

function applyWorkspaceSettings(settings) {
  var cfg = settings && _typeof(settings) === "object" ? settings : null;
  if (!cfg) return;
  suppressWorkspaceSettingsSave = true;

  try {
    var quality = cfg.quality && _typeof(cfg.quality) === "object" ? cfg.quality : null;

    if (quality) {
      if (el.samples && quality.samples !== undefined) el.samples.value = String(quality.samples);
      if (el.aa && quality.aa !== undefined) el.aa.value = String(quality.aa);
      if (el.sampleDistribution && quality.sample_distribution !== undefined) el.sampleDistribution.value = String(quality.sample_distribution);
      if (el.rdepth && quality.rdepth !== undefined) el.rdepth.value = String(quality.rdepth);
      syncSamplesPresetUi();
      syncAaPresetUi();
    }

    var frame = cfg.frame && _typeof(cfg.frame) === "object" ? cfg.frame : null;

    if (frame) {
      if (el.width && frame.width !== undefined) el.width.value = String(frame.width);
      if (el.height && frame.height !== undefined) el.height.value = String(frame.height);
      syncResolutionPresetFromInputs();
      updatePreviewSizing();
      syncVisualFrameAspect();
    }

    var integrator = cfg.integrator && _typeof(cfg.integrator) === "object" ? cfg.integrator : null;

    if (integrator) {
      integratorControlState.clear();
      var byInt = integrator.controls_by_integrator && _typeof(integrator.controls_by_integrator) === "object" ? integrator.controls_by_integrator : {};
      Object.keys(byInt).forEach(function (key) {
        var value = byInt[key];
        if (!key || !value || _typeof(value) !== "object") return;
        integratorControlState.set(key, _objectSpread({}, value));
      });

      if (el.integrator && integrator.id && integratorById.has(String(integrator.id))) {
        el.integrator.value = String(integrator.id);
      }

      if (el.tileSize && integrator.tile_size !== undefined) {
        if (typeof setTileSizeControlValue === "function") setTileSizeControlValue(integrator.tile_size);else el.tileSize.value = String(integrator.tile_size);
      }

      if (el.tileOrder && integrator.tile_order !== undefined) el.tileOrder.value = String(integrator.tile_order);
      if (el.threads && integrator.threads !== undefined) el.threads.value = String(integrator.threads);
      renderIntegratorControls();
      if (typeof syncTileSizeControlUi === "function") syncTileSizeControlUi();
    }

    var tm = cfg.tone_mapping && _typeof(cfg.tone_mapping) === "object" ? cfg.tone_mapping : null;

    if (tm) {
      if (el.toneMapping && tm.operator !== undefined) el.toneMapping.value = String(tm.operator);
      if (el.toneMappingExposure && tm.exposure !== undefined) el.toneMappingExposure.value = String(tm.exposure);
      if (el.toneMappingWhitePoint && tm.white_point !== undefined) el.toneMappingWhitePoint.value = String(tm.white_point);
      if (el.toneMappingMantiukContrast && tm.mantiuk_contrast !== undefined) el.toneMappingMantiukContrast.value = String(tm.mantiuk_contrast);
      if (el.toneMappingMantiukSaturation && tm.mantiuk_saturation !== undefined) el.toneMappingMantiukSaturation.value = String(tm.mantiuk_saturation);
      if (el.toneMappingMantiukDetail && tm.mantiuk_detail !== undefined) el.toneMappingMantiukDetail.value = String(tm.mantiuk_detail);
      updateToneMappingControlState();
    }

    var preview = cfg.preview && _typeof(cfg.preview) === "object" ? cfg.preview : null;

    if (preview && typeof setRenderMode === "function") {
      var nextMode = normalizeRenderMode(preview.render_mode);

      if (preview.render_mode === undefined && preview.interactive !== undefined) {
        nextMode = preview.interactive ? RENDER_MODE_INTERACTIVE : RENDER_MODE_PROGRESSIVE;
      }

      setRenderMode(nextMode, {
        log: false
      })["catch"](function () {});
    }

    if (preview && preview.interactive_speed !== undefined) {
      var speed = Math.max(0.2, Math.min(5.0, Number(preview.interactive_speed) || 1.0));
      interactivePreviewFlySpeedScale = speed;
      if (el.interactivePreviewSpeed) el.interactivePreviewSpeed.value = String(speed.toFixed(1));
      if (el.interactivePreviewSpeedValue) el.interactivePreviewSpeedValue.textContent = "".concat(speed.toFixed(1), "x");
      if (typeof renderInteractivePreviewHud === "function") renderInteractivePreviewHud();
    }

    if (preview && preview.interactive_moving_width !== undefined) {
      interactivePreviewAdaptiveMovingWidth = nearestInteractiveMovingWidth(preview.interactive_moving_width);
    }

    if (Array.isArray(cfg.post_filters)) {
      postFilterChain = cfg.post_filters.map(function (entry) {
        return normalizePostFilterEntry(entry);
      }).filter(function (entry) {
        return !!entry && !!entry.filter;
      });
    }

    if (cfg.post_filters_enabled !== undefined) {
      postFilterStackEnabled = !!cfg.post_filters_enabled;
    }

    if (el.postFiltersEnabled) el.postFiltersEnabled.checked = !!postFilterStackEnabled;
    updatePostFilterUiState();

    if (cfg.scene_variant !== undefined) {
      var wsRuntime = workspaceRuntimeState(activeWorkspaceId);
      if (wsRuntime) wsRuntime.activeVariant = String(cfg.scene_variant || "").trim();
    }
  } finally {
    suppressWorkspaceSettingsSave = false;
  }
}

function parseWorkspaceSettingsFromSnapshot(workspace) {
  var raw = String(workspace && workspace.settings_json || "").trim();
  if (!raw) return null;

  try {
    var parsed = JSON.parse(raw);
    return parsed && _typeof(parsed) === "object" ? parsed : null;
  } catch (_) {
    return null;
  }
}

function hasSceneOption(scene) {
  var name = String(scene || "").trim();
  if (!name || !el.scene) return false;

  for (var i = 0; i < el.scene.options.length; ++i) {
    if (String(el.scene.options[i].value || "") === name) return true;
  }

  return false;
}

function cacheWorkspaceSnapshots(items) {
  workspaceSnapshotById.clear();
  var list = Array.isArray(items) ? items : [];
  list.forEach(function (ws) {
    var id = String(ws && ws.id || "").trim();
    if (!id) return;
    workspaceSnapshotById.set(id, ws);
    var runtime = workspaceRuntimeState(id);
    if (!runtime) return;
    runtime.activeJobId = String(ws && ws.active_job_id || "").trim();
    runtime.lastCompletedJobId = String(ws && ws.last_job_id || "").trim();
    var scene = String(ws && ws.active_scene || "").trim();
    if (scene) runtime.lastCompletedJobScene = scene;
  });
}

function mapActiveJobsByWorkspace(activeJobs) {
  var byWorkspace = new Map();
  var list = Array.isArray(activeJobs) ? activeJobs : [];
  list.forEach(function (job) {
    var workspaceId = String(job && job.workspace_id || "").trim();
    var jobId = String(job && job.id || "").trim();
    var state = String(job && job.state || "").toLowerCase();
    if (!workspaceId || !jobId) return;
    if (state !== "queued" && state !== "preparing" && state !== "running") return;
    if (!byWorkspace.has(workspaceId)) byWorkspace.set(workspaceId, jobId);
  });
  return byWorkspace;
}

function overlayWorkspaceActiveJobs(workspaces, activeJobs) {
  var list = Array.isArray(workspaces) ? workspaces : [];
  if (!Array.isArray(activeJobs)) return list.map(function (ws) {
    return _objectSpread({}, ws);
  });
  var map = mapActiveJobsByWorkspace(activeJobs);
  return list.map(function (ws) {
    var id = String(ws && ws.id || "").trim();
    if (!id) return ws;
    var activeJobId = map.get(id) || "";
    return _objectSpread({}, ws, {
      active_job_id: activeJobId
    });
  });
}

function applyActiveWorkspaceState(snapshot, options) {
  var opts, ws, settings, wsScene, sceneFromSettings, effectiveScene, currentScene, sceneChanged, skipSceneReload, previousScene, previousVariant, previousCamera, wsVariant, sourceData, variantName, _wsVariant, _currentScene, _sourceData, _variantName, wsActiveJobId;

  return regeneratorRuntime.async(function applyActiveWorkspaceState$(_context4) {
    while (1) {
      switch (_context4.prev = _context4.next) {
        case 0:
          opts = options || {};
          cancelActivePollingUi();
          ws = snapshot || workspaceSnapshotById.get(activeWorkspaceId) || null;

          if (ws) {
            _context4.next = 7;
            break;
          }

          _context4.next = 6;
          return regeneratorRuntime.awrap(restorePreviewForActiveWorkspace());

        case 6:
          return _context4.abrupt("return");

        case 7:
          settings = parseWorkspaceSettingsFromSnapshot(ws);
          if (settings) applyWorkspaceSettings(settings);
          wsScene = String(ws && ws.active_scene || "").trim();
          sceneFromSettings = settings ? String(settings.scene || "").trim() : "";
          effectiveScene = wsScene || (hasSceneOption(sceneFromSettings) ? sceneFromSettings : "");

          if (!(effectiveScene && hasSceneOption(effectiveScene))) {
            _context4.next = 58;
            break;
          }

          currentScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
          sceneChanged = currentScene !== effectiveScene;
          skipSceneReload = !sceneChanged && String(opts.skipSceneReloadIfCurrent || "").trim() === effectiveScene;
          previousScene = currentScene;
          previousVariant = selectedSceneVariantValue();
          previousCamera = String(el.camera && el.camera.value ? el.camera.value : "").trim();
          _context4.prev = 19;

          if (sceneChanged) {
            el.scene.value = effectiveScene;
            setSceneBrowserSelectedFile(effectiveScene);
            localStorage.setItem(LAST_SCENE_KEY, effectiveScene);
            updateSceneDependencyPill(effectiveScene);
          }

          if (skipSceneReload) {
            _context4.next = 32;
            break;
          }

          wsVariant = String(ws && ws.active_variant || "").trim() || String((workspaceRuntimeState(activeWorkspaceId) || {}).activeVariant || "").trim();
          _context4.next = 25;
          return regeneratorRuntime.awrap(api.getSceneSource(effectiveScene));

        case 25:
          sourceData = _context4.sent;
          _context4.next = 28;
          return regeneratorRuntime.awrap(loadVariants(effectiveScene, wsVariant, sourceData && sourceData.source ? sourceData.source : ""));

        case 28:
          variantName = selectedSceneVariantValue();
          _context4.next = 31;
          return regeneratorRuntime.awrap(Promise.all([loadCameras(effectiveScene, variantName), loadSceneSource(effectiveScene, sourceData)]));

        case 31:
          if (visualEditor && editorViewMode === "visual") {
            loadVisualSceneFromSelected()["catch"](function (err) {
              return appendLog("visual load error: ".concat(err.message));
            });
          }

        case 32:
          if (editorViewMode === "graph") renderSceneGraphView();
          _context4.next = 56;
          break;

        case 35:
          _context4.prev = 35;
          _context4.t0 = _context4["catch"](19);
          appendLog("workspace scene load error (".concat(effectiveScene, "): ").concat(_context4.t0.message));

          if (!(previousScene && hasSceneOption(previousScene))) {
            _context4.next = 56;
            break;
          }

          el.scene.value = previousScene;
          setSceneBrowserSelectedFile(previousScene);
          localStorage.setItem(LAST_SCENE_KEY, previousScene);
          updateSceneDependencyPill(previousScene);
          _context4.prev = 43;
          _context4.next = 46;
          return regeneratorRuntime.awrap(loadVariants(previousScene, previousVariant));

        case 46:
          _context4.next = 48;
          return regeneratorRuntime.awrap(loadCameras(previousScene, previousVariant));

        case 48:
          if (previousCamera && cameraCatalogHasName(previousCamera)) el.camera.value = previousCamera;
          _context4.next = 51;
          return regeneratorRuntime.awrap(loadSceneSource(previousScene));

        case 51:
          _context4.next = 56;
          break;

        case 53:
          _context4.prev = 53;
          _context4.t1 = _context4["catch"](43);
          appendLog("workspace rollback error: ".concat(_context4.t1.message));

        case 56:
          _context4.next = 76;
          break;

        case 58:
          if (!(el.scene && el.scene.value)) {
            _context4.next = 76;
            break;
          }

          // No saved scene for this workspace yet; the picker still shows a scene from the previous
          // workspace. Reload variants using this workspace's saved variant (usually "" for a new
          // workspace) so the variant picker is never left showing a stale value from another workspace.
          _wsVariant = String(ws && ws.active_variant || "").trim() || String((workspaceRuntimeState(activeWorkspaceId) || {}).activeVariant || "").trim();
          _context4.prev = 60;
          _currentScene = String(el.scene.value).trim();

          if (!(_currentScene && hasSceneOption(_currentScene))) {
            _context4.next = 71;
            break;
          }

          _context4.next = 65;
          return regeneratorRuntime.awrap(api.getSceneSource(_currentScene));

        case 65:
          _sourceData = _context4.sent;
          _context4.next = 68;
          return regeneratorRuntime.awrap(loadVariants(_currentScene, _wsVariant, _sourceData && _sourceData.source ? _sourceData.source : ""));

        case 68:
          _variantName = selectedSceneVariantValue();
          _context4.next = 71;
          return regeneratorRuntime.awrap(loadCameras(_currentScene, _variantName));

        case 71:
          _context4.next = 76;
          break;

        case 73:
          _context4.prev = 73;
          _context4.t2 = _context4["catch"](60);
          appendLog("workspace variant reset error: ".concat(_context4.t2.message));

        case 76:
          _context4.next = 78;
          return regeneratorRuntime.awrap(restorePreviewForActiveWorkspace());

        case 78:
          wsActiveJobId = String(ws && ws.active_job_id || "").trim();

          if (wsActiveJobId) {
            resumeWorkspaceJobPolling(wsActiveJobId);
          }

        case 80:
        case "end":
          return _context4.stop();
      }
    }
  }, null, null, [[19, 35], [43, 53], [60, 73]]);
}

function renderWorkspaceList(items) {
  if (!el.workspaceList) return;
  var widgets = window.XTracerWidgets || {};
  el.workspaceList.innerHTML = "";
  var list = Array.isArray(items) ? items : [];
  var canDeleteAny = list.length >= 1;

  if (list.length === 0) {
    var empty = createWorkspaceEmptyState("No workspaces", "Create a new workspace.", "workspace-empty");
    el.workspaceList.appendChild(empty);
    return;
  }

  list.forEach(function (ws) {
    var id = String(ws && ws.id || "");
    var hasOwned = !!(ws && Object.prototype.hasOwnProperty.call(ws, "is_owned_by_client"));
    var isMine = hasOwned ? !!ws.is_owned_by_client : !!(ws && ws.is_active_for_client);
    var scene = String(ws && ws.active_scene || "").trim();
    var variant = String(ws && ws.active_variant || "").trim();
    var activeJob = String(ws && ws.active_job_id || "").trim();
    var lastJob = String(ws && ws.last_job_id || "").trim();
    var clients = Number(ws && ws.client_count || 0);
    var drafts = Number(ws && ws.draft_count || 0);
    var spatial = workspaceSpatialIndexStats && (typeof workspaceSpatialIndexStats === "undefined" ? "undefined" : _typeof(workspaceSpatialIndexStats)) === "object" ? workspaceSpatialIndexStats : null;
    var spatialNodes = spatial && Number.isFinite(Number(spatial.tlas_nodes)) ? String(Number(spatial.tlas_nodes)) : "-";
    var spatialFinite = spatial && Number.isFinite(Number(spatial.finite_objects)) ? String(Number(spatial.finite_objects)) : "-";
    var spatialInfinite = spatial && Number.isFinite(Number(spatial.infinite_objects)) ? String(Number(spatial.infinite_objects)) : "-";
    var spatialBuildMs = spatial && Number.isFinite(Number(spatial.build_ms)) ? "".concat(Math.max(0, Number(spatial.build_ms)).toFixed(0), " ms") : "-";
    var isRendering = !!activeJob;
    var head = document.createElement("header");
    head.className = "workspace-item-head";
    var title = document.createElement("h3");
    title.className = "workspace-item-title";
    title.textContent = String(ws && ws.name || id || "Workspace");
    head.appendChild(title);
    var badgesEl = document.createElement("div");
    badgesEl.className = "workspace-item-badges";

    if (isMine) {
      var mineBadge = createWorkspaceStateBadge("This Client", "workspace-item-state");
      badgesEl.appendChild(mineBadge);
    }

    head.appendChild(badgesEl);
    var actions = document.createElement("div");
    actions.className = "workspace-item-actions";
    var useBtn = createWorkspaceActionButton(id === activeWorkspaceId ? "Active" : "Use", {
      variant: id === activeWorkspaceId ? "secondary" : "primary",
      className: "workspace-item-action-btn",
      disabled: !id || id === activeWorkspaceId,
      onClick: function onClick() {
        switchActiveWorkspace(id)["catch"](function (err) {
          appendLog("workspace switch error: ".concat(err.message));
        });
      }
    });
    actions.appendChild(useBtn);
    var deleteBtn = createWorkspaceActionButton("Delete", {
      variant: "ghost",
      className: "workspace-item-action-btn",
      disabled: !id || !canDeleteAny,
      onClick: function onClick() {
        if (!id || !hasBackendMethod(api, "deleteWorkspace")) return;
        var widgets = window.XTracerWidgets;

        var doDelete = function doDelete() {
          api.deleteWorkspace(id).then(function () {
            return refreshWorkspaces();
          }).then(function () {
            appendLog("deleted workspace: ".concat(id));

            if (widgets && typeof widgets.showToast === "function") {
              widgets.showToast({
                message: "Workspace \"".concat(id, "\" deleted"),
                tone: "success"
              });
            }
          })["catch"](function (err) {
            appendLog("workspace delete error: ".concat(err.message));

            if (widgets && typeof widgets.showToast === "function") {
              widgets.showToast({
                message: "Delete failed: ".concat(err.message),
                tone: "error"
              });
            }
          });
        };

        if (widgets && typeof widgets.showModal === "function") {
          widgets.showModal({
            title: "Delete Workspace",
            body: "Delete workspace \"".concat(id, "\"? This cannot be undone."),
            confirmLabel: "Delete",
            danger: true,
            onConfirm: doDelete
          });
        } else {
          if (window.confirm("Delete workspace ".concat(id, "?"))) doDelete();
        }
      }
    });
    actions.appendChild(deleteBtn);
    var previewWrap = document.createElement("div");
    previewWrap.className = "workspace-item-preview"; // Avoid requesting the active job's final image while rendering; it often 404s
    // until completion and can show broken-image placeholders in the card.

    var previewJob = lastJob;
    var previewUrl = buildWorkspacePreviewUrl(previewJob);

    if (previewUrl) {
      loadWorkspacePreviewCanvas(previewUrl, previewWrap, "".concat(title.textContent, " render preview")).then(function (canvas) {
        if (canvas) {
          previewWrap.appendChild(canvas);
        } else if (!isRendering && !previewWrap.querySelector(".workspace-item-preview-empty")) {
          var emptyPreview = createWorkspaceEmptyState("", "No render yet", "workspace-item-preview-empty");
          previewWrap.appendChild(emptyPreview);
        }
      });
    } else if (!isRendering) {
      var emptyPreview = createWorkspaceEmptyState("", "No render yet", "workspace-item-preview-empty");
      previewWrap.appendChild(emptyPreview);
    }

    if (isRendering) {
      var loading = document.createElement("div");
      loading.className = "workspace-item-preview-loading";
      loading.setAttribute("aria-label", "Rendering");
      loading.innerHTML = '<span class="workspace-item-preview-spinner" aria-hidden="true"></span>';
      previewWrap.appendChild(loading);
    }

    var meta = document.createElement("dl");
    meta.className = "workspace-item-meta";

    var addMeta = function addMeta(label, value) {
      var row = document.createElement("div");
      row.className = "workspace-item-meta-row";
      row.setAttribute("data-meta-key", String(label || "").toLowerCase());
      var dt = document.createElement("dt");
      dt.textContent = label;
      var dd = document.createElement("dd");
      dd.textContent = value;
      row.appendChild(dt);
      row.appendChild(dd);
      meta.appendChild(row);
    };

    addMeta("ID", id || "-");
    addMeta("This Client", isMine ? "Yes" : "No");
    addMeta("Scene", scene || "-");
    addMeta("Variant", variant || "-");
    addMeta("Clients", String(clients));
    addMeta("Drafts", String(drafts));
    addMeta("Job", activeJob || lastJob || "-");
    addMeta("TLAS Nodes", spatialNodes);
    addMeta("Finite/Infinite", "".concat(spatialFinite, "/").concat(spatialInfinite));
    addMeta("TLAS Build", spatialBuildMs);
    addMeta("Updated", formatWorkspaceUpdated(ws && ws.updated_ms));

    if (workspaceViewMode === "list") {
      var previewCol = document.createElement("div");
      previewCol.className = "workspace-item-preview-col";
      previewCol.appendChild(previewWrap);
      var main = document.createElement("div");
      main.className = "workspace-item-list-main";
      var sceneLine = document.createElement("p");
      sceneLine.className = "workspace-item-list-scene";
      sceneLine.textContent = variant ? "".concat(scene || "-", " / ").concat(variant) : scene || "-";
      var metaStrip = document.createElement("div");
      metaStrip.className = "workspace-item-meta-strip";

      var addChip = function addChip(label, value) {
        var chip = document.createElement("span");
        chip.className = "workspace-item-meta-chip";
        chip.setAttribute("data-meta-key", String(label || "").toLowerCase());
        var key = document.createElement("span");
        key.className = "workspace-item-meta-chip-key";
        key.textContent = label;
        var val = document.createElement("span");
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
      addChip("Obj", "".concat(spatialFinite, "/").concat(spatialInfinite));
      addChip("Build", spatialBuildMs);
      addChip("Updated", formatWorkspaceUpdated(ws && ws.updated_ms));
      main.appendChild(head);
      main.appendChild(sceneLine);
      main.appendChild(metaStrip);

      var _card = typeof widgets.createWorkspaceCard === "function" ? widgets.createWorkspaceCard({
        active: !!(id && id === activeWorkspaceId),
        rendering: isRendering,
        listMode: true,
        previewColumn: previewCol,
        main: main,
        actions: actions
      }) : function () {
        var legacyCard = document.createElement("article");
        legacyCard.className = "workspace-item workspace-item-list-compact";
        if (id && id === activeWorkspaceId) legacyCard.classList.add("is-active");
        if (isRendering) legacyCard.classList.add("is-rendering");
        legacyCard.appendChild(previewCol);
        legacyCard.appendChild(main);
        legacyCard.appendChild(actions);
        return legacyCard;
      }();

      el.workspaceList.appendChild(_card);
      return;
    }

    var card = typeof widgets.createWorkspaceCard === "function" ? widgets.createWorkspaceCard({
      active: !!(id && id === activeWorkspaceId),
      rendering: isRendering,
      head: head,
      preview: previewWrap,
      meta: meta,
      actions: actions
    }) : function () {
      var legacyCard = document.createElement("article");
      legacyCard.className = "workspace-item";
      if (id && id === activeWorkspaceId) legacyCard.classList.add("is-active");
      if (isRendering) legacyCard.classList.add("is-rendering");
      legacyCard.appendChild(head);
      legacyCard.appendChild(previewWrap);
      legacyCard.appendChild(meta);
      legacyCard.appendChild(actions);
      return legacyCard;
    }();
    el.workspaceList.appendChild(card);
  });
}

function refreshWorkspaces() {
  var genBeforeFetch, _ref, _ref2, payload, activeJobs, workspaceItems, sortedWorkspaceItems, nextActive, activeChanged, wasEmpty;

  return regeneratorRuntime.async(function refreshWorkspaces$(_context5) {
    while (1) {
      switch (_context5.prev = _context5.next) {
        case 0:
          if (hasBackendMethod(api, "getWorkspaces")) {
            _context5.next = 2;
            break;
          }

          return _context5.abrupt("return");

        case 2:
          genBeforeFetch = workspaceSwitchGen;
          _context5.next = 5;
          return regeneratorRuntime.awrap(Promise.all([api.getWorkspaces(), Promise.resolve(getActiveJobsFromCache())]));

        case 5:
          _ref = _context5.sent;
          _ref2 = _slicedToArray(_ref, 2);
          payload = _ref2[0];
          activeJobs = _ref2[1];
          workspaceSpatialIndexStats = payload && payload.spatial_index && _typeof(payload.spatial_index) === "object" ? payload.spatial_index : null;
          workspaceItems = overlayWorkspaceActiveJobs(payload && payload.workspaces || [], activeJobs);
          sortedWorkspaceItems = sortWorkspaceItems(workspaceItems);
          cacheWorkspaceSnapshots(sortedWorkspaceItems);
          nextActive = String(payload && payload.active_workspace || "").trim();
          activeChanged = false; // Skip server-reported active workspace if an explicit switch happened during fetch —
          // the server response is stale relative to what the client just requested.

          if (nextActive && nextActive !== activeWorkspaceId && genBeforeFetch === workspaceSwitchGen) {
            wasEmpty = !activeWorkspaceId;
            syncGlobalsToWorkspaceRuntime();
            activeWorkspaceId = nextActive;
            syncWorkspaceRuntimeToGlobals();
            if (!wasEmpty) activeChanged = true;
          }

          updateWorkspaceActiveHint();
          updateWorkspaceCountHint(Array.isArray(sortedWorkspaceItems) ? sortedWorkspaceItems.length : 0);
          renderWorkspaceList(sortedWorkspaceItems);

          if (!activeChanged) {
            _context5.next = 23;
            break;
          }

          _context5.next = 22;
          return regeneratorRuntime.awrap(applyActiveWorkspaceState(workspaceSnapshotById.get(activeWorkspaceId) || null));

        case 22:
          if (typeof syncRenderTabEnabled === "function") syncRenderTabEnabled();

        case 23:
        case "end":
          return _context5.stop();
      }
    }
  });
}

function queueWorkspaceDraftSave() {
  if (!hasBackendMethod(api, "saveWorkspaceSceneDraft")) return;

  if (workspaceDraftSaveTimer) {
    clearTimeout(workspaceDraftSaveTimer);
    workspaceDraftSaveTimer = null;
  }

  var scene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!scene || !is_scene_name_safe_runtime(scene)) return;
  workspaceDraftSaveTimer = setTimeout(function () {
    workspaceDraftSaveTimer = null;
    api.saveWorkspaceSceneDraft(scene, el.sceneSource ? el.sceneSource.value || "" : "")["catch"](function (err) {
      return appendLog("workspace draft save failed: ".concat(err.message));
    });
  }, 450);
}

function is_scene_name_safe_runtime(scene) {
  return /^[A-Za-z0-9_.-]+\.scn$/.test(String(scene || ""));
}

function switchActiveWorkspace(workspaceId) {
  var nextId, switchGen, snapshot;
  return regeneratorRuntime.async(function switchActiveWorkspace$(_context6) {
    while (1) {
      switch (_context6.prev = _context6.next) {
        case 0:
          nextId = String(workspaceId || "").trim();

          if (!(!nextId || nextId === activeWorkspaceId)) {
            _context6.next = 3;
            break;
          }

          return _context6.abrupt("return");

        case 3:
          if (hasBackendMethod(api, "setActiveWorkspace")) {
            _context6.next = 5;
            break;
          }

          return _context6.abrupt("return");

        case 5:
          switchGen = ++workspaceSwitchGen;
          _context6.next = 8;
          return regeneratorRuntime.awrap(api.setActiveWorkspace(nextId));

        case 8:
          snapshot = _context6.sent;

          if (!(switchGen !== workspaceSwitchGen)) {
            _context6.next = 11;
            break;
          }

          return _context6.abrupt("return");

        case 11:
          // superseded by a newer switch
          syncGlobalsToWorkspaceRuntime();
          activeWorkspaceId = nextId;
          syncWorkspaceRuntimeToGlobals();

          if (snapshot && snapshot.id) {
            workspaceSnapshotById.set(nextId, snapshot);
          }

          updateWorkspaceActiveHint();
          _context6.next = 18;
          return regeneratorRuntime.awrap(applyActiveWorkspaceState(snapshot || workspaceSnapshotById.get(nextId) || null));

        case 18:
          if (typeof syncRenderTabEnabled === "function") syncRenderTabEnabled();
          refreshWorkspaces()["catch"](function (err) {
            return appendLog("workspace refresh error: ".concat(err.message));
          });
          appendLog("workspace active=".concat(nextId));

        case 21:
        case "end":
          return _context6.stop();
      }
    }
  });
}