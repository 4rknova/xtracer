function extractSceneTitle(source) {
  const m = /^\s*title\s*=\s*(.+)$/im.exec(source || "");
  if (!m) return "";
  let title = (m[1] || "").trim();
  if (!title) return "";
  if ((title[0] === "\"" && title[title.length - 1] === "\"")
    || (title[0] === "'" && title[title.length - 1] === "'")) {
    title = title.slice(1, -1).trim();
  }
  return title;
}

function sceneDependsOnExternalFiles(source) {
  const text = source || "";
  if (!text) return false;

  const lines = text.split("\n");
  for (let i = 0; i < lines.length; i += 1) {
    const raw = lines[i] || "";
    const line = raw.replace(/#.*/, "").trim();
    if (!line) continue;

    // Explicit scene path aliases, e.g. path_data = data or path_mesh = <data>/mesh
    const pathMatch = /^path_[A-Za-z0-9_]*\s*=\s*(.+)$/i.exec(line);
    if (pathMatch) {
      const v = (pathMatch[1] || "").trim();
      if (/[\\/]/.test(v) || /<[^>]+>/.test(v)) return true;
      continue;
    }

    // Any non-procedural source assignment is a filesystem dependency.
    const srcMatch = /^source\s*=\s*(.+)$/i.exec(line);
    if (srcMatch) {
      const v = (srcMatch[1] || "").trim();
      if (!/^gen\s*\(/i.test(v) && v.length > 0) return true;
    }
  }

  return false;
}

async function buildSceneLabels(sceneFiles) {
  const items = await Promise.all((sceneFiles || []).map(async (sceneFile) => {
    try {
      const data = await api.getSceneSource(sceneFile);
      const source = (data && data.source) || "";
      const title = extractSceneTitle(source);
      const dependsExternal = sceneDependsOnExternalFiles(source);
      return { sceneFile, label: title || sceneFile, dependsExternal };
    } catch (_) {
      return { sceneFile, label: sceneFile, dependsExternal: false };
    }
  }));

  return items;
}

function updateSceneDependencyPill(sceneFile) {
  const pill = el.sceneDependencyPill;
  if (!pill) return;
  const dependsExternal = !!sceneDependencyByFile.get(sceneFile || "");
  pill.hidden = !dependsExternal;
  pill.textContent = "EXT";
  pill.classList.toggle("scene-kind-ext", dependsExternal);
  pill.classList.toggle("scene-kind-self", !dependsExternal);
}

function presetId(index) {
  return String(index).padStart(2, "0");
}

function formatResolutionPresetLabel(index, preset, descWidth) {
  const baseDesc = String((preset && preset.description) || "").trim() || `Preset ${presetId(index)}`;
  const paddedDesc = baseDesc.padEnd(Math.max(1, descWidth), " ");
  const sizeLabel = `${preset.width}x${preset.height}`;
  return `${paddedDesc}   ${sizeLabel}`;
}

function syncResolutionPresetFromInputs() {
  const { width, height } = currentRenderSize();
  const index = resolutionPresets.findIndex((p) => p.width === width && p.height === height);
  el.resolutionPreset.value = index >= 0 ? String(index) : "custom";
}

function syncVisualFrameAspect() {
  if (!visualEditor || !visualEditor.setFrameAspect) return;
  const { width, height } = currentRenderSize();
  visualEditor.setFrameAspect(width, height);
}

async function loadResolutionPresets() {
  const presets = await api.getResolutionPresets();
  resolutionPresets = (presets || []).map((p) => ({
    width: parseInt(p.width, 10),
    height: parseInt(p.height, 10),
    description: p.description || "",
  })).filter((p) => Number.isFinite(p.width) && Number.isFinite(p.height) && p.width > 0 && p.height > 0);

  el.resolutionPreset.innerHTML = "";
  addOption(el.resolutionPreset, "custom", "Custom");
  const descWidth = resolutionPresets.reduce((max, preset, index) => {
    const desc = String((preset && preset.description) || "").trim() || `Preset ${presetId(index)}`;
    return Math.max(max, desc.length);
  }, 0);
  resolutionPresets.forEach((preset, index) => {
    addOption(
      el.resolutionPreset,
      String(index),
      formatResolutionPresetLabel(index, preset, descWidth),
    );
  });
  syncResolutionPresetFromInputs();
}


async function loadScenes() {
  const scenes = await api.getScenes();
  const sceneItems = await buildSceneLabels(scenes);
  const prev = el.scene.value;
  const saved = String(localStorage.getItem(LAST_SCENE_KEY) || "").trim();
  el.scene.innerHTML = "";
  sceneDependencyByFile = new Map(sceneItems.map((item) => [item.sceneFile, !!item.dependsExternal]));
  sceneItems.forEach((item) => addOption(el.scene, item.sceneFile, item.label));
  const preferred = prev || saved;
  if (preferred) el.scene.value = preferred;
  if (!el.scene.value && el.scene.options.length > 0) el.scene.selectedIndex = 0;
  updateSceneDependencyPill(el.scene.value);
  if (el.scene.value) localStorage.setItem(LAST_SCENE_KEY, el.scene.value);
  else localStorage.removeItem(LAST_SCENE_KEY);
}


async function loadCameras(scene) {
  el.camera.innerHTML = "";
  addOption(el.camera, "", "Auto (first camera)");
  const info = await api.getCameras(scene);
  const cameras = (info && info.cameras) || [];
  const defaultCamera = (info && info.defaultCamera) || "";
  cameras.forEach((name) => addOption(el.camera, name, name));

  if (defaultCamera && cameras.includes(defaultCamera)) {
    el.camera.value = defaultCamera;
  } else {
    el.camera.value = "";
  }

  syncVisualCameraFromRenderSelection();
}

async function loadIntegrators() {
  const integrators = await api.getIntegrators();
  integratorCatalog = Array.isArray(integrators) ? integrators : [];
  integratorById = new Map(integratorCatalog.map((it) => [it.id, it]));
  const prev = el.integrator.value;
  el.integrator.innerHTML = "";
  integratorCatalog.forEach((it) => addOption(el.integrator, it.id, it.label));
  if (prev && integratorCatalog.some((it) => it.id === prev)) {
    el.integrator.value = prev;
  } else if (integratorCatalog.some((it) => it.id === "pathtracer_mis")) {
    el.integrator.value = "pathtracer_mis";
  } else if (integratorCatalog.some((it) => it.id === "pathtracer_is")) {
    el.integrator.value = "pathtracer_is";
  } else if (!el.integrator.value && el.integrator.options.length > 0) {
    el.integrator.selectedIndex = 0;
  }
  renderIntegratorControls();
}

async function loadSceneSource(scene) {
  if (!scene) {
    el.sceneSource.value = "";
    updateEditorMetrics();
    refreshSceneEditControls();
    renderSceneGraphView();
    resetSceneHistoriesFromCurrentSource();
    return;
  }
  const data = await api.getSceneSource(scene);
  el.sceneName.value = data.scene || scene;
  el.sceneSource.value = data.source || "";
  updateEditorMetrics();
  refreshSceneEditControls();
  syncEditorScroll();
  renderSceneGraphView();
  resetSceneHistoriesFromCurrentSource();
}

async function loadSceneRuntimeGraph(scene) {
  const sceneName = String(scene || "").trim();
  if (!sceneName || !hasBackendMethod(api, "getSceneRuntimeGraph")) return null;
  const data = await api.getSceneRuntimeGraph(sceneName);
  runtimeGraphByScene.set(sceneName, data || { cameras: [], objects: [], surfaces: [], materials: [] });
  if (String(el.scene && el.scene.value ? el.scene.value : "").trim() === sceneName) {
    renderSceneGraphView();
  }
  return runtimeGraphByScene.get(sceneName) || null;
}

function renderThirdPartyLicenses(rawItems) {
  const items = Array.isArray(rawItems) && rawItems.length
    ? rawItems
    : DEFAULT_THIRD_PARTY_LICENSES;
  if (!el.aboutThirdPartyList) return;
  el.aboutThirdPartyList.replaceChildren();

  items.forEach((item) => {
    const name = item && item.name ? String(item.name) : "Unknown";
    const license = item && item.license ? String(item.license) : "Unknown";
    const url = item && item.url ? String(item.url) : "";

    const row = document.createElement("div");
    row.className = "about-third-party-row";

    const nameNode = document.createElement("span");
    nameNode.className = "about-third-party-name";
    nameNode.textContent = name;
    row.appendChild(nameNode);

    const licenseNode = document.createElement("code");
    licenseNode.className = "about-third-party-license";
    licenseNode.textContent = license;
    row.appendChild(licenseNode);

    if (url) {
      const link = document.createElement("a");
      link.className = "about-third-party-link";
      link.href = url;
      link.target = "_blank";
      link.rel = "noopener noreferrer";
      link.textContent = url;
      row.appendChild(link);
    }

    el.aboutThirdPartyList.appendChild(row);
  });
}

async function loadAbout() {
  const data = await api.getAbout();
  const rawVersion = (data.version || "").trim();
  const hideStandaloneVersion = !rawVersion || rawVersion.toLowerCase() === "standalone";
  const version = hideStandaloneVersion ? "" : rawVersion;
  el.aboutVersion.textContent = version;
  el.aboutVersionRow.hidden = hideStandaloneVersion;
  const homepage = data.homepage || "https://www.4rknova.com";
  el.aboutHomepage.href = homepage;
  el.aboutHomepage.textContent = homepage;
  const repository = data.website || "https://github.com/4rknova/xtracer";
  el.aboutWebsite.href = repository;
  el.aboutWebsite.textContent = repository;
  el.aboutCopyright.textContent = data.copyright || "unknown";
  el.aboutLicense.textContent = data.license || "Unavailable";
  const backendLabel = data.backend || (backendMode === "wasm" ? "xtracer_wasm_adapter" : "xtracer_web");
  const isWasmBackend = String(backendLabel).toLowerCase().indexOf("wasm") >= 0;
  el.aboutBuildPill.hidden = !isWasmBackend;
  el.aboutBackend.textContent = backendLabel;
  el.aboutDefaultUrl.textContent = data.default_url || window.location.origin;
  el.aboutSceneDir.textContent = data.scene_dir || (backendMode === "wasm" ? "scenes/" : "scene/");
  el.aboutStaticAssets.textContent = data.static_assets || "/";
  updateWorkspaceServerStatsHints(data);
  renderThirdPartyLicenses(data.third_party_licenses);
}

async function loadEmptySceneTemplate() {
  if (hasBackendMethod(api, "getEmptySceneTemplate")) {
    return api.getEmptySceneTemplate();
  }
  const data = await getJSON("/api/scenes/template/empty");
  return data.source || "";
}
