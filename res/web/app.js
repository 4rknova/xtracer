const $ = (id) => document.getElementById(id);

const el = {
  tabRender: $("tabRender"),
  tabEditor: $("tabEditor"),
  tabSettings: $("tabSettings"),
  tabLogs: $("tabLogs"),
  tabAbout: $("tabAbout"),
  paneRender: $("paneRender"),
  paneEditor: $("paneEditor"),
  paneSettings: $("paneSettings"),
  paneLogs: $("paneLogs"),
  paneAbout: $("paneAbout"),
  theme: $("theme"),
  pollInterval: $("pollInterval"),
  autoLoadEditor: $("autoLoadEditor"),
  clearLogsBtn: $("clearLogsBtn"),
  logOutput: $("logOutput"),
  aboutSummary: $("aboutSummary"),
  aboutVersion: $("aboutVersion"),
  aboutAuthor: $("aboutAuthor"),
  aboutEmail: $("aboutEmail"),
  aboutCopyright: $("aboutCopyright"),
  aboutLicense: $("aboutLicense"),
  scene: $("scene"),
  camera: $("camera"),
  integrator: $("integrator"),
  width: $("width"),
  height: $("height"),
  samples: $("samples"),
  aa: $("aa"),
  tileSize: $("tile_size"),
  threads: $("threads"),
  renderBtn: $("renderBtn"),
  status: $("status"),
  progress: $("progress"),
  previewFrame: $("previewFrame"),
  previewEmpty: $("previewEmpty"),
  preview: $("preview"),
  download: $("download"),
  sceneName: $("sceneName"),
  lineNumbers: $("lineNumbers"),
  lineCount: $("lineCount"),
  charCount: $("charCount"),
  sceneSource: $("sceneSource"),
  loadSceneBtn: $("loadSceneBtn"),
  newSceneBtn: $("newSceneBtn"),
  saveSceneBtn: $("saveSceneBtn"),
};

const uiOptions = {
  pollMs: 300,
  autoLoadEditor: true,
};
let lastBackendLogId = 0;
let previewObjectUrl = "";

function nowStamp() {
  const d = new Date();
  return d.toLocaleTimeString();
}

function appendLog(message) {
  if (!message) return;
  el.logOutput.textContent += `[UI ${nowStamp()}] ${message}\n`;
  el.logOutput.scrollTop = el.logOutput.scrollHeight;
}

function appendBackendLog(entry) {
  const id = entry.id || 0;
  const ts = entry.ts || "";
  const level = (entry.level || "info").toUpperCase();
  const msg = entry.message || "";
  el.logOutput.textContent += `#${id} ${ts} ${level} ${msg}\n`;
  el.logOutput.scrollTop = el.logOutput.scrollHeight;
}

function setStatus(text) {
  el.status.textContent = text;
}

async function getJSON(url) {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}

async function pollBackendLogs() {
  try {
    const data = await getJSON(`/api/logs?since=${lastBackendLogId}`);
    const entries = data.entries || [];
    for (let i = 0; i < entries.length; i += 1) {
      appendBackendLog(entries[i]);
      if ((entries[i].id || 0) > lastBackendLogId) lastBackendLogId = entries[i].id;
    }
  } catch (err) {
    appendLog(`backend logs unavailable: ${err.message}`);
  } finally {
    setTimeout(pollBackendLogs, Math.max(500, uiOptions.pollMs));
  }
}

function setProgress(value) {
  const p = Math.max(0, Math.min(1, value || 0));
  el.progress.style.width = `${(p * 100).toFixed(1)}%`;
}

function currentRenderSize() {
  const w = parseInt(el.width.value || "0", 10);
  const h = parseInt(el.height.value || "0", 10);
  const width = Number.isFinite(w) ? Math.max(32, Math.min(8192, w)) : 640;
  const height = Number.isFinite(h) ? Math.max(32, Math.min(8192, h)) : 480;
  return { width, height };
}

function updatePreviewSizing() {
  const { width, height } = currentRenderSize();
  el.previewFrame.style.aspectRatio = `${width} / ${height}`;
}

function editorLineCount(text) {
  if (!text) return 1;
  return text.split("\n").length;
}

function updateEditorMetrics() {
  const text = el.sceneSource.value || "";
  const lines = editorLineCount(text);
  const chars = text.length;

  const gutter = Array.from({ length: lines }, (_, i) => String(i + 1)).join("\n");
  el.lineNumbers.textContent = gutter;
  el.lineCount.textContent = `${lines} ${lines === 1 ? "line" : "lines"}`;
  el.charCount.textContent = `${chars} ${chars === 1 ? "char" : "chars"}`;
}

function syncEditorScroll() {
  el.lineNumbers.scrollTop = el.sceneSource.scrollTop;
}

function setPreviewEmptyState(isEmpty) {
  el.previewFrame.classList.toggle("is-empty", isEmpty);
  if (isEmpty) {
    if (previewObjectUrl) {
      URL.revokeObjectURL(previewObjectUrl);
      previewObjectUrl = "";
    }
    el.preview.removeAttribute("src");
    el.download.removeAttribute("href");
    el.download.setAttribute("aria-disabled", "true");
    el.download.classList.add("is-disabled");
    return;
  }
  el.download.setAttribute("aria-disabled", "false");
  el.download.classList.remove("is-disabled");
}

async function refreshProgressivePreview(jobId) {
  const res = await fetch(`/api/jobs/${jobId}/image?partial=1&t=${Date.now()}`);
  if (!res.ok) return false;
  const blob = await res.blob();
  if (!blob || blob.size === 0) return false;

  const url = URL.createObjectURL(blob);
  if (previewObjectUrl) URL.revokeObjectURL(previewObjectUrl);
  previewObjectUrl = url;
  el.preview.src = previewObjectUrl;
  setPreviewEmptyState(false);
  return true;
}

function addOption(select, value, label) {
  const opt = document.createElement("option");
  opt.value = value;
  opt.textContent = label || value;
  select.appendChild(opt);
}

function setActiveTab(mode) {
  const isRender = mode === "render";
  const isEditor = mode === "editor";
  const isSettings = mode === "settings";
  const isLogs = mode === "logs";
  const isAbout = mode === "about";
  el.tabRender.classList.toggle("active", isRender);
  el.tabEditor.classList.toggle("active", isEditor);
  el.tabSettings.classList.toggle("active", isSettings);
  el.tabLogs.classList.toggle("active", isLogs);
  el.tabAbout.classList.toggle("active", isAbout);
  el.paneRender.classList.toggle("active", isRender);
  el.paneEditor.classList.toggle("active", isEditor);
  el.paneSettings.classList.toggle("active", isSettings);
  el.paneLogs.classList.toggle("active", isLogs);
  el.paneAbout.classList.toggle("active", isAbout);
}

function applyTheme(mode) {
  const root = document.documentElement;
  if (mode === "light" || mode === "dark") root.setAttribute("data-theme", mode);
  else root.setAttribute("data-theme", "system");
  localStorage.setItem("xtracer-theme", mode);
}

function loadUIOptions() {
  const poll = parseInt(localStorage.getItem("xtracer-poll-ms") || "300", 10);
  uiOptions.pollMs = Number.isFinite(poll) ? Math.max(100, Math.min(10000, poll)) : 300;
  uiOptions.autoLoadEditor = localStorage.getItem("xtracer-auto-load-editor") !== "0";
  el.pollInterval.value = String(uiOptions.pollMs);
  el.autoLoadEditor.checked = uiOptions.autoLoadEditor;
}

function persistUIOptions() {
  localStorage.setItem("xtracer-poll-ms", String(uiOptions.pollMs));
  localStorage.setItem("xtracer-auto-load-editor", uiOptions.autoLoadEditor ? "1" : "0");
}

async function loadScenes() {
  const data = await getJSON("/api/scenes");
  const prev = el.scene.value;
  el.scene.innerHTML = "";
  (data.scenes || []).forEach((name) => addOption(el.scene, name, name));
  if (prev) el.scene.value = prev;
  if (!el.scene.value && el.scene.options.length > 0) el.scene.selectedIndex = 0;
}

async function loadCameras(scene) {
  el.camera.innerHTML = "";
  addOption(el.camera, "", "Auto (first camera)");
  if (!scene) return;
  const data = await getJSON(`/api/scenes/${encodeURIComponent(scene)}/cameras`);
  (data.cameras || []).forEach((name) => addOption(el.camera, name, name));
}

async function loadIntegrators() {
  const data = await getJSON("/api/integrators");
  el.integrator.innerHTML = "";
  (data.integrators || []).forEach((it) => addOption(el.integrator, it.id, it.label));
}

async function loadSceneSource(scene) {
  if (!scene) {
    el.sceneSource.value = "";
    updateEditorMetrics();
    return;
  }
  const data = await getJSON(`/api/scenes/${encodeURIComponent(scene)}/source`);
  el.sceneName.value = data.scene || scene;
  el.sceneSource.value = data.source || "";
  updateEditorMetrics();
  syncEditorScroll();
}

async function loadAbout() {
  const data = await getJSON("/api/about");
  if (data.name) el.aboutSummary.textContent = `${data.name} is a local browser frontend for the xtracer rendering framework.`;
  el.aboutVersion.textContent = data.version || "unknown";
  el.aboutAuthor.textContent = data.author_name || "unknown";
  el.aboutEmail.textContent = data.author_email || "unknown";
  el.aboutCopyright.textContent = data.copyright || "unknown";
  el.aboutLicense.textContent = data.license || "Unavailable";
}

async function startRender() {
  const body = new URLSearchParams();
  body.set("scene", el.scene.value);
  body.set("integrator", el.integrator.value);
  if (el.camera.value) body.set("camera", el.camera.value);
  body.set("width", el.width.value);
  body.set("height", el.height.value);
  body.set("samples", el.samples.value);
  body.set("aa", el.aa.value);
  body.set("tile_size", el.tileSize.value);
  body.set("threads", el.threads.value);

  const res = await fetch("/api/render", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: body.toString(),
  });

  const data = await res.json();
  if (!res.ok) throw new Error(data.error || `HTTP ${res.status}`);
  return data.job_id;
}

async function saveScene() {
  const name = (el.sceneName.value || "").trim();
  const source = el.sceneSource.value || "";
  if (!name) throw new Error("scene name is required");

  const body = new URLSearchParams();
  body.set("name", name);
  body.set("source", source);

  let res = await fetch("/api/scenes/save", {
    method: "POST",
    headers: { "Content-Type": "application/x-www-form-urlencoded" },
    body: body.toString(),
  });

  if (res.status === 409) {
    if (!window.confirm("Scene exists. Overwrite it?")) return null;
    body.set("overwrite", "1");
    res = await fetch("/api/scenes/save", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: body.toString(),
    });
  }

  const data = await res.json();
  if (!res.ok) throw new Error(data.error || `HTTP ${res.status}`);
  return data.scene;
}

async function pollJob(jobId) {
  let lastState = "";
  while (true) {
    const data = await getJSON(`/api/jobs/${jobId}`);
    const state = data.state || "unknown";
    const progress = data.progress || 0;
    setProgress(progress);
    setStatus(`${state} ${(100 * progress).toFixed(1)}%`);

    if (state !== lastState) {
      appendLog(`job ${jobId} -> ${state}`);
      lastState = state;
    }

    await refreshProgressivePreview(jobId);

    if (state === "done") {
      const finalUrl = `/api/jobs/${jobId}/image?final=1`;
      if (previewObjectUrl) {
        URL.revokeObjectURL(previewObjectUrl);
        previewObjectUrl = "";
      }
      el.preview.src = `${finalUrl}&t=${Date.now()}`;
      el.download.href = finalUrl;
      setPreviewEmptyState(false);
      setStatus(`done in ${Math.round(data.elapsed_ms || 0)} ms`);
      appendLog(`job ${jobId} finished in ${Math.round(data.elapsed_ms || 0)} ms`);
      return;
    }

    if (state === "error") {
      throw new Error(data.error || "render failed");
    }

    await new Promise((r) => setTimeout(r, uiOptions.pollMs));
  }
}

async function handleRender() {
  el.renderBtn.disabled = true;
  setProgress(0);
  setStatus("submitting job...");
  appendLog(`submit render scene=${el.scene.value} integrator=${el.integrator.value}`);
  try {
    const jobId = await startRender();
    appendLog(`job accepted: ${jobId}`);
    await pollJob(jobId);
  } catch (err) {
    setStatus(`error: ${err.message}`);
    appendLog(`render error: ${err.message}`);
  } finally {
    el.renderBtn.disabled = false;
  }
}

async function boot() {
  const savedTheme = localStorage.getItem("xtracer-theme") || "system";
  el.theme.value = savedTheme;
  applyTheme(savedTheme);
  loadUIOptions();

  setStatus("loading...");
  appendLog("boot");
  await Promise.all([loadScenes(), loadIntegrators()]);
  await loadCameras(el.scene.value);
  await loadSceneSource(el.scene.value);
  await loadAbout();
  updatePreviewSizing();
  setPreviewEmptyState(true);
  if (!el.scene.value) setStatus("no scenes found in scene/ directory");
  else setStatus("idle");

  setActiveTab("render");
  el.renderBtn.addEventListener("click", handleRender);

  el.scene.addEventListener("change", () => {
    const tasks = [loadCameras(el.scene.value)];
    if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(el.scene.value));
    Promise.all(tasks)
      .then(() => appendLog(`scene changed: ${el.scene.value}`))
      .catch((err) => {
        setStatus(`error: ${err.message}`);
        appendLog(`scene change error: ${err.message}`);
      });
  });

  el.theme.addEventListener("change", () => {
    applyTheme(el.theme.value);
    appendLog(`theme=${el.theme.value}`);
  });

  const onSizeChanged = () => {
    updatePreviewSizing();
  };
  el.width.addEventListener("input", onSizeChanged);
  el.width.addEventListener("change", onSizeChanged);
  el.height.addEventListener("input", onSizeChanged);
  el.height.addEventListener("change", onSizeChanged);

  el.pollInterval.addEventListener("change", () => {
    const v = parseInt(el.pollInterval.value || "300", 10);
    uiOptions.pollMs = Number.isFinite(v) ? Math.max(100, Math.min(10000, v)) : 300;
    el.pollInterval.value = String(uiOptions.pollMs);
    persistUIOptions();
    appendLog(`poll interval=${uiOptions.pollMs}ms`);
  });

  el.autoLoadEditor.addEventListener("change", () => {
    uiOptions.autoLoadEditor = !!el.autoLoadEditor.checked;
    persistUIOptions();
    appendLog(`auto-load editor=${uiOptions.autoLoadEditor ? "on" : "off"}`);
  });

  el.tabRender.addEventListener("click", () => setActiveTab("render"));
  el.tabEditor.addEventListener("click", () => setActiveTab("editor"));
  el.tabSettings.addEventListener("click", () => setActiveTab("settings"));
  el.tabLogs.addEventListener("click", () => setActiveTab("logs"));
  el.tabAbout.addEventListener("click", () => setActiveTab("about"));

  el.loadSceneBtn.addEventListener("click", () => {
    loadSceneSource(el.scene.value)
      .then(() => {
        setStatus(`loaded ${el.scene.value}`);
        appendLog(`loaded source: ${el.scene.value}`);
      })
      .catch((err) => {
        setStatus(`error: ${err.message}`);
        appendLog(`load source error: ${err.message}`);
      });
  });

  el.newSceneBtn.addEventListener("click", () => {
    el.sceneName.value = "new_scene.scn";
    el.sceneSource.value = "";
    updateEditorMetrics();
    syncEditorScroll();
    setStatus("new scene initialized");
    appendLog("new scene template");
  });

  el.saveSceneBtn.addEventListener("click", () => {
    saveScene()
      .then((scene) => {
        if (!scene) return;
        loadScenes()
          .then(() => {
            el.scene.value = scene;
            const tasks = [loadCameras(scene)];
            if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(scene));
            return Promise.all(tasks);
          })
          .then(() => {
            setStatus(`saved ${scene}`);
            appendLog(`saved scene: ${scene}`);
          })
          .catch((err) => {
            setStatus(`error: ${err.message}`);
            appendLog(`post-save error: ${err.message}`);
          });
      })
      .catch((err) => {
        setStatus(`error: ${err.message}`);
        appendLog(`save error: ${err.message}`);
      });
  });

  el.clearLogsBtn.addEventListener("click", () => {
    el.logOutput.textContent = "";
  });

  el.sceneSource.addEventListener("input", updateEditorMetrics);
  el.sceneSource.addEventListener("scroll", syncEditorScroll);
  el.sceneSource.addEventListener("keyup", syncEditorScroll);
  el.sceneSource.addEventListener("click", syncEditorScroll);
  updateEditorMetrics();
  syncEditorScroll();

  pollBackendLogs();
}

boot().catch((err) => {
  setStatus(`error: ${err.message}`);
  appendLog(`boot error: ${err.message}`);
});
