const APP_CONFIG_URL = "/app/data/config.json";
let appConfigLoadPromise = null;
let sceneSearchQuery = "";
let pendingVariantForNextSceneLoad = null;
let currentSceneSourceOrigin = "";

function normalizeSceneSourceOrigin(value) {
  const raw = String(value || "").trim().toLowerCase();
  if (raw === "workspace") return "workspace";
  return "disk";
}

function sceneSourceOriginLabel(value) {
  return normalizeSceneSourceOrigin(value) === "workspace" ? "Workspace Draft" : "Disk";
}

function normalizeSceneSearchQuery(value) {
  return String(value || "").trim().toLowerCase();
}

function normalizeConfiguredDefaultIntegratorId(config) {
  if (!config || typeof config !== "object" || Array.isArray(config)) return "";
  const defaults = config.defaults;
  if (!defaults || typeof defaults !== "object" || Array.isArray(defaults)) return "";
  const id = String(defaults.integrator || "").trim();
  return id;
}

async function loadAppConfig() {
  if (appConfigLoadPromise) return appConfigLoadPromise;
  appConfigLoadPromise = (async () => {
    try {
      const sep = APP_CONFIG_URL.includes("?") ? "&" : "?";
      const url = `${APP_CONFIG_URL}${sep}t=${Date.now()}`;
      const data = await getJSON(url);
      if (!data || typeof data !== "object" || Array.isArray(data)) return {};
      return data;
    } catch (_) {
      return {};
    }
  })();
  return appConfigLoadPromise;
}

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

function extractSceneDescription(source) {
  const m = /^\s*description\s*=\s*(.+)$/im.exec(source || "");
  if (!m) return "";
  let description = (m[1] || "").trim();
  if (!description) return "";
  if ((description[0] === "\"" && description[description.length - 1] === "\"")
    || (description[0] === "'" && description[description.length - 1] === "'")) {
    description = description.slice(1, -1).trim();
  }
  return description;
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

    const svgMatch = /^svg_source\s*=\s*(.+)$/i.exec(line);
    if (svgMatch) {
      const v = (svgMatch[1] || "").trim();
      if (v.length > 0) return true;
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

function countTopLevelSceneEntries(source, groupName) {
  const text = String(source || "");
  if (!text) return 0;
  const groupRange = findSceneGroupRange(text, groupName);
  if (!groupRange) return 0;
  return splitTopLevelSceneEntries(text, groupRange).length;
}

function buildSceneLabels(sceneFiles) {
  return (sceneFiles || []).map((sceneFile) => ({
    sceneFile,
    label: sceneFile,
    title: "",
    description: "",
    sourceOrigin: "disk",
    dependsExternal: false,
    variantCount: 0,
    cameraCount: 0,
    hasVariants: false,
  }));
}

async function enrichSceneCatalogEntry(sceneFile) {
  const name = String(sceneFile || "").trim();
  if (!name) return;
  try {
    const data = await api.getSceneSource(name);
    const source = (data && data.source) || "";
    const sourceOrigin = normalizeSceneSourceOrigin(data && data.source_origin ? data.source_origin : "");
    const title = extractSceneTitle(source);
    const description = extractSceneDescription(source);
    const dependsExternal = sceneDependsOnExternalFiles(source);
    const variantMeta = extractSceneVariantNames(source);
    const variantCount = Array.isArray(variantMeta && variantMeta.variants) ? variantMeta.variants.length : 0;
    const cameraCount = countTopLevelSceneEntries(source, "camera");
    const hasVariants = variantCount > 0;
    for (let i = 0; i < sceneCatalog.length; i += 1) {
      if (String(sceneCatalog[i] && sceneCatalog[i].sceneFile ? sceneCatalog[i].sceneFile : "") !== name) continue;
      sceneCatalog[i] = { ...sceneCatalog[i], label: title || name, title, description, sourceOrigin, dependsExternal, variantCount, cameraCount, hasVariants };
      break;
    }
    renderSceneFileBrowser();
  } catch (_) {
    // enrichment is best-effort
  }
}

function sceneCatalogHasFile(sceneFile) {
  const name = String(sceneFile || "").trim();
  if (!name) return false;
  return sceneCatalog.some((item) => String(item && item.sceneFile ? item.sceneFile : "").trim() === name);
}

function createSceneFileIcon() {
  const ns = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(ns, "svg");
  svg.setAttribute("viewBox", "0 0 24 28");
  svg.setAttribute("aria-hidden", "true");
  svg.classList.add("scene-file-icon");

  const body = document.createElementNS(ns, "path");
  body.setAttribute("d", "M6.2 1.5h7.8L19 6.5V24a2.4 2.4 0 0 1-2.4 2.4H6.2A2.2 2.2 0 0 1 4 24.2V3.7a2.2 2.2 0 0 1 2.2-2.2z");
  body.setAttribute("fill", "none");
  body.setAttribute("stroke", "currentColor");
  body.setAttribute("stroke-width", "1.35");
  body.setAttribute("stroke-linejoin", "round");
  body.setAttribute("stroke-linecap", "round");
  svg.appendChild(body);

  const foldFill = document.createElementNS(ns, "path");
  foldFill.setAttribute("d", "M14 1.5v4.9H19");
  foldFill.setAttribute("fill", "currentColor");
  foldFill.setAttribute("fill-opacity", "0.12");
  svg.appendChild(foldFill);

  const fold = document.createElementNS(ns, "path");
  fold.setAttribute("d", "M14 1.5v4.9H19");
  fold.setAttribute("fill", "none");
  fold.setAttribute("stroke", "currentColor");
  fold.setAttribute("stroke-width", "1.35");
  fold.setAttribute("stroke-linecap", "round");
  fold.setAttribute("stroke-linejoin", "round");
  svg.appendChild(fold);

  const divider = document.createElementNS(ns, "path");
  divider.setAttribute("d", "M7.2 17.9h9.6");
  divider.setAttribute("fill", "none");
  divider.setAttribute("stroke", "currentColor");
  divider.setAttribute("stroke-opacity", "0.35");
  divider.setAttribute("stroke-width", "1");
  divider.setAttribute("stroke-linecap", "round");
  svg.appendChild(divider);

  const text = document.createElementNS(ns, "text");
  text.setAttribute("x", "12");
  text.setAttribute("y", "12.4");
  text.setAttribute("text-anchor", "middle");
  text.setAttribute("dominant-baseline", "middle");
  text.textContent = ".ncf";
  svg.appendChild(text);

  return svg;
}

function normalizeCameraType(cameraType) {
  const raw = String(cameraType || "").trim().toLowerCase();
  if (raw.indexOf("cube") >= 0) return "cubemap";
  if (raw.indexOf("omni") >= 0 || raw.indexOf("stereo") >= 0 || raw.indexOf("ods") >= 0) return "ods";
  if (raw.indexOf("equirect") >= 0 || raw.indexOf("erp") >= 0) return "erp";
  if (raw.indexOf("tilt") >= 0 || raw.indexOf("shift") >= 0) return "tiltshift";
  if (raw.indexOf("perspective") >= 0 || raw.indexOf("thin") >= 0 || raw.indexOf("lens") >= 0) return "perspective";
  return "camera";
}

function cameraTypeLabel(cameraType) {
  const kind = normalizeCameraType(cameraType);
  if (kind === "perspective") return "Perspective";
  if (kind === "erp") return "Equirectangular";
  if (kind === "ods") return "Omni Stereo";
  if (kind === "cubemap") return "Cubemap";
  if (kind === "tiltshift") return "Tilt-Shift";
  return String(cameraType || "Camera").trim() || "Camera";
}

function createCameraIcon(cameraType) {
  const ns = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(ns, "svg");
  svg.setAttribute("viewBox", "0 0 24 16");
  svg.setAttribute("aria-hidden", "true");
  const kind = normalizeCameraType(cameraType);
  svg.classList.add("scene-file-icon", "camera-file-icon", `camera-file-icon--${kind}`);

  if (kind === "erp") {
    const globe = document.createElementNS(ns, "circle");
    globe.setAttribute("cx", "12");
    globe.setAttribute("cy", "8");
    globe.setAttribute("r", "6.2");
    globe.setAttribute("fill", "none");
    globe.setAttribute("stroke", "currentColor");
    globe.setAttribute("stroke-width", "1.5");
    svg.appendChild(globe);

    const meridian = document.createElementNS(ns, "path");
    meridian.setAttribute("d", "M12 1.8c2.2 1.8 3.5 3.9 3.5 6.2S14.2 12.4 12 14.2 8.5 10.3 8.5 8 9.8 3.6 12 1.8z");
    meridian.setAttribute("fill", "none");
    meridian.setAttribute("stroke", "currentColor");
    meridian.setAttribute("stroke-width", "1.2");
    svg.appendChild(meridian);

    const equator = document.createElementNS(ns, "path");
    equator.setAttribute("d", "M5.8 8h12.4");
    equator.setAttribute("fill", "none");
    equator.setAttribute("stroke", "currentColor");
    equator.setAttribute("stroke-width", "1.2");
    equator.setAttribute("stroke-linecap", "round");
    svg.appendChild(equator);
  } else if (kind === "cubemap") {
    const cube = document.createElementNS(ns, "path");
    cube.setAttribute("d", "M12 2.2l5.7 3.2v6.4L12 15l-5.7-3.2V5.4L12 2.2zm0 0v6.4m5.7-3.2L12 8.6 6.3 5.4");
    cube.setAttribute("fill", "none");
    cube.setAttribute("stroke", "currentColor");
    cube.setAttribute("stroke-width", "1.4");
    cube.setAttribute("stroke-linejoin", "round");
    cube.setAttribute("stroke-linecap", "round");
    svg.appendChild(cube);
  } else if (kind === "ods") {
    const left = document.createElementNS(ns, "circle");
    left.setAttribute("cx", "8.3");
    left.setAttribute("cy", "8");
    left.setAttribute("r", "3.3");
    left.setAttribute("fill", "none");
    left.setAttribute("stroke", "currentColor");
    left.setAttribute("stroke-width", "1.4");
    svg.appendChild(left);

    const right = document.createElementNS(ns, "circle");
    right.setAttribute("cx", "15.7");
    right.setAttribute("cy", "8");
    right.setAttribute("r", "3.3");
    right.setAttribute("fill", "none");
    right.setAttribute("stroke", "currentColor");
    right.setAttribute("stroke-width", "1.4");
    svg.appendChild(right);

    const bridge = document.createElementNS(ns, "path");
    bridge.setAttribute("d", "M11.6 8h0.8");
    bridge.setAttribute("fill", "none");
    bridge.setAttribute("stroke", "currentColor");
    bridge.setAttribute("stroke-width", "1.6");
    bridge.setAttribute("stroke-linecap", "round");
    svg.appendChild(bridge);
  } else {
    const body = document.createElementNS(ns, "rect");
    body.setAttribute("x", "1.5");
    body.setAttribute("y", "3.2");
    body.setAttribute("width", "21");
    body.setAttribute("height", "11.3");
    body.setAttribute("rx", "2");
    body.setAttribute("fill", "none");
    body.setAttribute("stroke", "currentColor");
    body.setAttribute("stroke-width", "1.5");
    svg.appendChild(body);

    const lens = document.createElementNS(ns, "circle");
    lens.setAttribute("cx", "12");
    lens.setAttribute("cy", "8.85");
    lens.setAttribute("r", "3.1");
    lens.setAttribute("fill", "none");
    lens.setAttribute("stroke", "currentColor");
    lens.setAttribute("stroke-width", "1.4");
    svg.appendChild(lens);

    const top = document.createElementNS(ns, "path");
    top.setAttribute("d", "M6.8 3.2l1.4-1.7h7.6l1.4 1.7");
    top.setAttribute("fill", "none");
    top.setAttribute("stroke", "currentColor");
    top.setAttribute("stroke-width", "1.4");
    top.setAttribute("stroke-linejoin", "round");
    svg.appendChild(top);
  }

  return svg;
}

function createVariantIcon(isBase) {
  const ns = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(ns, "svg");
  svg.setAttribute("viewBox", "0 0 24 20");
  svg.setAttribute("aria-hidden", "true");
  svg.classList.add("scene-file-icon", "variant-file-icon");
  if (isBase) svg.classList.add("variant-file-icon--base");

  const back = document.createElementNS(ns, "path");
  back.setAttribute("d", "M12 2.5l7 3.9v7.2l-7 3.9-7-3.9V6.4l7-3.9z");
  back.setAttribute("fill", "none");
  back.setAttribute("stroke", "currentColor");
  back.setAttribute("stroke-width", "1.4");
  back.setAttribute("stroke-linejoin", "round");
  svg.appendChild(back);

  if (!isBase) {
    const front = document.createElementNS(ns, "path");
    front.setAttribute("d", "M12 6.2l3.7 2.1v3.8L12 14.2l-3.7-2.1V8.3L12 6.2z");
    front.setAttribute("fill", "currentColor");
    front.setAttribute("fill-opacity", "0.34");
    svg.appendChild(front);
  }

  return svg;
}

function normalizeVariantName(variantName) {
  return String(variantName || "").trim();
}

function selectedSceneVariantValue() {
  if (!el.variant) return "";
  return normalizeVariantName(el.variant.value);
}

function runtimeGraphCacheKey(sceneName, variantName) {
  return `${normalizeVariantName(sceneName)}\n${normalizeVariantName(variantName)}`;
}

function getRuntimeGraphForScene(sceneName, variantName) {
  return runtimeGraphByScene.get(runtimeGraphCacheKey(sceneName, variantName !== undefined ? variantName : selectedSceneVariantValue())) || null;
}

function extractSceneVariantNames(source) {
  const text = String(source || "");
  if (!text) return { base: { label: "(base)", description: "" }, variants: [], hideBase: false };

  const groupRange = findSceneGroupRange(text, "variants");
  if (!groupRange) return { base: { label: "(base)", description: "" }, variants: [], hideBase: false };

  const unquote = (value) => {
    const s = String(value || "").trim();
    if (s.length >= 2 && ((s[0] === "\"" && s[s.length - 1] === "\"") || (s[0] === "'" && s[s.length - 1] === "'"))) {
      return s.slice(1, -1).trim();
    }
    return s;
  };

  const entries = splitTopLevelSceneEntries(text, groupRange);
  const variantsBody = text.slice(groupRange.bodyStart, groupRange.bodyEnd);
  const hideBaseRaw = (typeof readSceneStringProp === "function") ? readSceneStringProp(variantsBody, "hide_base") : "";
  const hideBaseText = unquote(hideBaseRaw).toLowerCase();
  const hideBase = (hideBaseText === "1" || hideBaseText === "true" || hideBaseText === "yes");
  const byId = new Map();
  entries.forEach((entry) => {
    const id = normalizeVariantName(entry && entry.id ? entry.id : "");
    if (!id || byId.has(id)) return;
    const labelRaw = (typeof readSceneStringProp === "function") ? readSceneStringProp(entry.body || "", "name") : "";
    const descRaw = (typeof readSceneStringProp === "function") ? readSceneStringProp(entry.body || "", "description") : "";
    byId.set(id, {
      id,
      label: unquote(labelRaw) || id,
      description: unquote(descRaw),
    });
  });

  const baseMeta = byId.get("base");
  if (baseMeta) byId.delete("base");

  const variants = Array.from(byId.values()).sort((a, b) => a.id.localeCompare(b.id));
  return {
    base: {
      label: (baseMeta && baseMeta.label) ? baseMeta.label : "(base)",
      description: (baseMeta && baseMeta.description) ? baseMeta.description : "",
    },
    variants,
    hideBase,
  };
}

function updateSceneFileCount() {
  if (!el.sceneFileCount) return;
  const visibleCount = getFilteredSceneCatalog().length;
  const totalCount = sceneCatalog.length;
  if (sceneSearchQuery && visibleCount !== totalCount) {
    el.sceneFileCount.textContent = `${visibleCount}/${totalCount}`;
    return;
  }
  el.sceneFileCount.textContent = String(totalCount);
}

function sceneItemSearchText(item) {
  if (!item || typeof item !== "object") return "";
  const sceneFile = String(item.sceneFile || "");
  const title = String(item.title || "");
  const description = String(item.description || "");
  return `${sceneFile}\n${title}\n${description}`.toLowerCase();
}

function getFilteredSceneCatalog() {
  if (!sceneSearchQuery) return sceneCatalog.slice();
  return sceneCatalog.filter((item) => sceneItemSearchText(item).indexOf(sceneSearchQuery) >= 0);
}

function setSceneSearchQuery(value) {
  const next = normalizeSceneSearchQuery(value);
  if (sceneSearchQuery === next) return;
  sceneSearchQuery = next;
  renderSceneFileBrowser();
}

async function refreshSceneCatalog(sceneFile) {
  const requestedScene = String(sceneFile || "").trim();
  const previousScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const targetHint = requestedScene || previousScene || "scene";
  setSceneLoadStatus("loading", `Refreshing ${targetHint}...`, "");

  await loadScenes();

  const resolvedScene = sceneCatalogHasFile(requestedScene)
    ? requestedScene
    : String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!resolvedScene) {
    setStatus("no scenes found in scene/ directory");
    setSceneLoadStatus("idle", "No scenes found in scene/ directory.", "");
    appendLog("scene refresh finished (no scenes available)");
    return;
  }

  if (String(el.scene && el.scene.value ? el.scene.value : "").trim() !== resolvedScene) {
    el.scene.value = resolvedScene;
  }
  setSceneBrowserSelectedFile(resolvedScene);
  localStorage.setItem(LAST_SCENE_KEY, resolvedScene);
  updateSceneDependencyPill(resolvedScene);

  const preferredVariant = (resolvedScene === previousScene) ? selectedSceneVariantValue() : "";
  await loadVariants(resolvedScene, preferredVariant);
  const variantName = selectedSceneVariantValue();

  const tasks = [loadCameras(resolvedScene, variantName)];
  if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(resolvedScene, variantName));
  if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(resolvedScene));
  await Promise.all(tasks);

  if (visualEditor) await loadVisualSceneFromSelected();

  const variantSuffix = variantName ? ` (${variantName})` : "";
  setStatus(`refreshed ${resolvedScene}${variantSuffix}`);
  setSceneLoadStatus("idle", `Refreshed ${resolvedScene}${variantSuffix}.`, "");
  appendLog(`scene refreshed: ${resolvedScene}${variantSuffix}`);
}

async function reloadSceneFile(sceneFile) {
  const activeScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const requestedScene = String(sceneFile || "").trim();
  if (!activeScene || !requestedScene || requestedScene !== activeScene) return;

  const variantBeforeReload = selectedSceneVariantValue();
  const targetHint = variantBeforeReload ? `${activeScene} (${variantBeforeReload})` : activeScene;
  setSceneLoadStatus("loading", `Reloading ${targetHint}...`, "");

  await loadVariants(activeScene, variantBeforeReload);
  const variantName = selectedSceneVariantValue();
  const tasks = [loadCameras(activeScene, variantName)];
  if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(activeScene, variantName));
  if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(activeScene));
  await Promise.all(tasks);

  if (visualEditor) await loadVisualSceneFromSelected();

  const variantSuffix = variantName ? ` (${variantName})` : "";
  setStatus(`reloaded ${activeScene}${variantSuffix}`);
  setSceneLoadStatus("idle", `Reloaded ${activeScene}${variantSuffix}.`, "");
  appendLog(`scene reloaded: ${activeScene}${variantSuffix}`);
}

function ensureSceneContextMenu() {
  if (document.getElementById("sceneFileContextMenu")) return;
  const widgets = window.XTracerWidgets || {};
  const menu = (typeof widgets.createMenu === "function" && typeof widgets.createMenuItem === "function")
    ? widgets.createMenu({
        id: "sceneFileContextMenu",
        className: "scene-file-context-menu",
        label: "Scene actions",
        children: [
          widgets.createMenuItem({ id: "sceneCtxSetActive", label: "Set Active" }),
          widgets.createMenuItem({ id: "sceneCtxRefetch", label: "Refetch" }),
          widgets.createMenuItem({ id: "sceneCtxReload", label: "Reload" }),
          widgets.createMenuDivider ? widgets.createMenuDivider() : null,
          widgets.createMenuItem({ id: "sceneCtxDelete", label: "Delete", danger: true }),
        ],
      })
    : document.createElement("div");
  menu.id = "sceneFileContextMenu";
  if (!menu.classList.contains("scene-file-context-menu")) menu.className = "scene-file-context-menu";
  menu.hidden = true;
  if (!menu.children.length) {
    menu.innerHTML = ""
      + "<button id=\"sceneCtxSetActive\" type=\"button\">Set Active</button>"
      + "<button id=\"sceneCtxRefetch\" type=\"button\">Refetch</button>"
      + "<button id=\"sceneCtxReload\" type=\"button\">Reload</button>"
      + "<button id=\"sceneCtxDelete\" type=\"button\" class=\"danger\">Delete</button>";
  }
  document.body.appendChild(menu);

  const close = () => {
    menu.hidden = true;
    delete menu.dataset.scene;
  };

  const setActiveBtn = menu.querySelector("#sceneCtxSetActive");
  const refetchBtn = menu.querySelector("#sceneCtxRefetch");
  const reloadBtn = menu.querySelector("#sceneCtxReload");
  const deleteBtn = menu.querySelector("#sceneCtxDelete");
  if (setActiveBtn) {
    setActiveBtn.addEventListener("click", () => {
      const sceneFile = String(menu.dataset.scene || "").trim();
      close();
      if (!sceneFile) return;
      activateSceneFile(sceneFile);
    });
  }
  if (refetchBtn) {
    refetchBtn.addEventListener("click", () => {
      const sceneFile = String(menu.dataset.scene || "").trim();
      close();
      if (!sceneFile) return;
      refetchSceneFile(sceneFile).catch((err) => {
        setStatus(`error: ${err.message}`);
        setSceneLoadStatus("error", err.message || "Scene refetch failed.", "");
        appendLog(`scene refetch error: ${err.message}`);
      });
    });
  }
  if (reloadBtn) {
    reloadBtn.addEventListener("click", () => {
      const sceneFile = String(menu.dataset.scene || "").trim();
      close();
      if (!sceneFile) return;
      reloadSceneFile(sceneFile).catch((err) => {
        setStatus(`error: ${err.message}`);
        setSceneLoadStatus("error", err.message || "Scene reload failed.", "");
        appendLog(`scene reload error: ${err.message}`);
      });
    });
  }
  if (deleteBtn) {
    deleteBtn.addEventListener("click", () => {
      const sceneFile = String(menu.dataset.scene || "").trim();
      close();
      if (!sceneFile) return;
      deleteSceneFile(sceneFile).catch((err) => {
        setStatus(`error: ${err.message}`);
        setSceneLoadStatus("error", err.message || "Scene delete failed.", "");
        appendLog(`scene delete error: ${err.message}`);
      });
    });
  }

  document.addEventListener("click", (evt) => {
    if (menu.hidden) return;
    if (evt && evt.target && menu.contains(evt.target)) return;
    close();
  });
  document.addEventListener("keydown", (evt) => {
    if (evt && evt.key === "Escape") close();
  });
  window.addEventListener("resize", close);
  document.addEventListener("scroll", close, true);
}

function openSceneContextMenu(sceneFile, x, y) {
  ensureSceneContextMenu();
  const menu = document.getElementById("sceneFileContextMenu");
  if (!menu) return;

  const sceneName = String(sceneFile || "").trim();
  menu.dataset.scene = sceneName;
  menu.hidden = false;

  const isActive = String(el.scene && el.scene.value ? el.scene.value : "").trim() === sceneName;
  const setActiveBtn = menu.querySelector("#sceneCtxSetActive");
  if (setActiveBtn) setActiveBtn.disabled = isActive;
  const refetchBtn = menu.querySelector("#sceneCtxRefetch");
  if (refetchBtn) refetchBtn.disabled = false;
  const reloadBtn = menu.querySelector("#sceneCtxReload");
  if (reloadBtn) {
    reloadBtn.hidden = !isActive;
    reloadBtn.disabled = !isActive;
  }

  const deleteBtn = menu.querySelector("#sceneCtxDelete");
  if (deleteBtn) deleteBtn.disabled = !hasBackendMethod(api, "deleteScene");

  const viewportW = Math.max(0, window.innerWidth || 0);
  const viewportH = Math.max(0, window.innerHeight || 0);
  const menuW = menu.offsetWidth || 180;
  const menuH = menu.offsetHeight || 120;
  const clampedX = Math.max(8, Math.min((x || 8), viewportW - menuW - 8));
  const clampedY = Math.max(8, Math.min((y || 8), viewportH - menuH - 8));
  menu.style.left = `${clampedX}px`;
  menu.style.top = `${clampedY}px`;
}

async function deleteSceneFile(sceneFile) {
  const sceneName = String(sceneFile || "").trim();
  if (!sceneName) return;
  if (!hasBackendMethod(api, "deleteScene")) throw new Error("scene delete endpoint unavailable");

  const widgets = window.XTracerWidgets;
  const ok = await (widgets && typeof widgets.showModal === "function"
    ? widgets.showModal({
      title: "Delete Scene",
      body: `Delete "${sceneName}"? This cannot be undone.`,
      confirmLabel: "Delete",
      danger: true,
    })
    : Promise.resolve(window.confirm(`Delete scene ${sceneName}? This cannot be undone.`)));
  if (!ok) return;

  const deletedWasActive = String(el.scene && el.scene.value ? el.scene.value : "").trim() === sceneName;
  setSceneLoadStatus("loading", `Deleting ${sceneName}...`, "");
  await api.deleteScene(sceneName);

  await loadScenes();
  const nextScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  await loadVariants(nextScene);
  const tasks = [loadCameras(nextScene)];
  if (hasBackendMethod(api, "getSceneRuntimeGraph")) tasks.push(loadSceneRuntimeGraph(nextScene));
  if (uiOptions.autoLoadEditor) tasks.push(loadSceneSource(nextScene));
  await Promise.all(tasks);
  if (visualEditor && deletedWasActive && nextScene) await loadVisualSceneFromSelected();

  if (nextScene) {
    setStatus(`deleted ${sceneName} (active ${nextScene})`);
    setSceneLoadStatus("idle", `Deleted ${sceneName}. Active scene: ${nextScene}.`, "");
  } else {
    setStatus(`deleted ${sceneName}`);
    setSceneLoadStatus("idle", `Deleted ${sceneName}. No scenes available.`, "");
  }
  appendLog(`scene deleted: ${sceneName}`);
}

async function refetchSceneFile(sceneFile) {
  const sceneName = String(sceneFile || "").trim();
  if (!sceneName) return;

  setSceneLoadStatus("loading", `Refetching ${sceneName}...`, "");
  const data = await api.getSceneSource(sceneName);
  const source = String(data && data.source ? data.source : "");
  const sourceOrigin = normalizeSceneSourceOrigin(data && data.source_origin ? data.source_origin : "");
  const title = extractSceneTitle(source);
  const description = extractSceneDescription(source);
  const dependsExternal = sceneDependsOnExternalFiles(source);
  const variantMeta = extractSceneVariantNames(source);
  const variantCount = Array.isArray(variantMeta && variantMeta.variants) ? variantMeta.variants.length : 0;
  const cameraCount = countTopLevelSceneEntries(source, "camera");
  const hasVariants = variantCount > 0;

  for (let i = 0; i < sceneCatalog.length; i += 1) {
    const item = sceneCatalog[i];
    if (String(item && item.sceneFile ? item.sceneFile : "").trim() !== sceneName) continue;
    sceneCatalog[i] = {
      ...item,
      label: title || sceneName,
      title,
      description,
      sourceOrigin,
      dependsExternal,
      variantCount,
      cameraCount,
      hasVariants,
    };
    break;
  }
  renderSceneFileBrowser();

  const isActive = String(el.scene && el.scene.value ? el.scene.value : "").trim() === sceneName;
  if (isActive) {
    currentSceneSourceOrigin = sourceOrigin;
    el.sceneName.value = data.scene || sceneName;
    el.sceneSource.value = source;
    updateEditorMetrics();
    syncEditorScroll();
    renderSceneGraphView();
    resetSceneHistoriesFromCurrentSource();
    await loadVariants(sceneName, selectedSceneVariantValue());
    updateActiveSceneSidebarCard();
  }

  setStatus(`refetched ${sceneName} (${sceneSourceOriginLabel(sourceOrigin)})`);
  setSceneLoadStatus("idle", `Refetched ${sceneName} from ${sceneSourceOriginLabel(sourceOrigin)}.`, "");
  appendLog(`scene refetched: ${sceneName} origin=${sourceOrigin}`);
}

function renderSceneFileBrowser() {
  if (!el.sceneFileList) return;
  const widgets = window.XTracerWidgets || {};
  const preservedScrollTop = el.sceneFileList.scrollTop;
  const activeScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!sceneBrowserSelectedFile || !sceneCatalogHasFile(sceneBrowserSelectedFile)) {
    sceneBrowserSelectedFile = activeScene;
  }

  el.sceneFileList.replaceChildren();
  updateSceneFileCount();

  const visibleSceneCatalog = getFilteredSceneCatalog();
  if (!visibleSceneCatalog.length) {
    el.sceneFileList.appendChild(window.XTracerWidgets.createEmptyState({
      title: sceneSearchQuery ? "No matches" : "No scenes",
      message: sceneSearchQuery ? "No scenes match the search." : "No scene files found.",
      className: "scene-file-empty",
    }));
    return;
  }

  visibleSceneCatalog.forEach((item) => {
    const sceneFile = String(item && item.sceneFile ? item.sceneFile : "").trim();
    if (!sceneFile) return;
    const title = String(item && item.title ? item.title : "").trim();
    const sourceOrigin = String(item && item.sourceOrigin ? item.sourceOrigin : "disk").trim();
    const dependsExternal = !!(item && item.dependsExternal);
    const hasVariants = !!(item && item.hasVariants);
    const variantCount = Number((item && item.variantCount) || 0);
    const cameraCount = Number((item && item.cameraCount) || 0);
    const isSelected = sceneBrowserSelectedFile === sceneFile;
    const isActive = activeScene === sceneFile;

    const button = widgets.createSceneCard({
      sceneFile,
      title,
      sourceOrigin: normalizeSceneSourceOrigin(sourceOrigin),
      dependsExternal,
      hasVariants,
      variantCount,
      cameraCount,
      selected: isSelected,
      active: isActive,
      icon: createSceneFileIcon(),
    });
    button.addEventListener("click", () => {
      activateSceneFile(sceneFile);
    });
    button.addEventListener("contextmenu", (evt) => {
      evt.preventDefault();
      sceneBrowserSelectedFile = sceneFile;
      syncSceneFileBrowserSelectionUi();
      openSceneContextMenu(sceneFile, evt.clientX, evt.clientY);
    });
    button.addEventListener("keydown", (evt) => {
      if (evt.key === "Enter" || evt.key === " ") {
        evt.preventDefault();
        activateSceneFile(sceneFile);
      }
    });

    el.sceneFileList.appendChild(button);
  });

  // Keep viewport stable on mobile/desktop when a click re-renders the list.
  const maxScrollTop = Math.max(0, el.sceneFileList.scrollHeight - el.sceneFileList.clientHeight);
  el.sceneFileList.scrollTop = Math.max(0, Math.min(preservedScrollTop, maxScrollTop));
}

function syncSceneFileBrowserSelectionUi() {
  if (!el.sceneFileList) return;
  const activeScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const items = el.sceneFileList.querySelectorAll(".scene-file-item[data-scene]");
  if (!items || items.length === 0) return;
  items.forEach((button) => {
    const sceneFile = String(button.dataset && button.dataset.scene ? button.dataset.scene : "").trim();
    const isSelected = sceneBrowserSelectedFile === sceneFile;
    const isActive = activeScene === sceneFile;
    button.setAttribute("aria-selected", isSelected ? "true" : "false");
    button.classList.toggle("is-selected", isSelected);
    button.classList.toggle("is-active", isActive);
  });
}

function setSceneBrowserSelectedFile(sceneFile) {
  const name = String(sceneFile || "").trim();
  if (name && sceneCatalogHasFile(name)) sceneBrowserSelectedFile = name;
  else sceneBrowserSelectedFile = "";
  const filteredCount = getFilteredSceneCatalog().length;
  const renderedItems = el.sceneFileList
    ? el.sceneFileList.querySelectorAll(".scene-file-item[data-scene]").length
    : 0;
  if (!el.sceneFileList || renderedItems !== filteredCount) {
    renderSceneFileBrowser();
    return;
  }
  syncSceneFileBrowserSelectionUi();
}

function activateSceneFile(sceneFile) {
  const name = String(sceneFile || "").trim();
  if (!name || !sceneCatalogHasFile(name) || !el.scene) return;
  sceneBrowserSelectedFile = name;
  if (String(el.scene.value || "").trim() === name) {
    syncSceneFileBrowserSelectionUi();
    return;
  }
  el.scene.value = name;
  el.scene.dispatchEvent(new Event("change", { bubbles: true }));
}

function cameraCatalogHasName(cameraName) {
  const name = String(cameraName || "").trim();
  return cameraCatalog.some((item) => String(item && item.value ? item.value : "").trim() === name);
}

function variantCatalogHasName(variantName) {
  const name = normalizeVariantName(variantName);
  return variantCatalog.some((item) => normalizeVariantName(item && item.value ? item.value : "") === name);
}

function variantCatalogEntryByName(variantName) {
  const name = normalizeVariantName(variantName);
  for (let i = 0; i < variantCatalog.length; i += 1) {
    const item = variantCatalog[i];
    if (normalizeVariantName(item && item.value ? item.value : "") === name) return item || null;
  }
  return null;
}

function sceneCatalogEntryByFile(sceneFile) {
  const name = String(sceneFile || "").trim();
  for (let i = 0; i < sceneCatalog.length; i += 1) {
    const item = sceneCatalog[i];
    if (String(item && item.sceneFile ? item.sceneFile : "").trim() === name) return item || null;
  }
  return null;
}

function cameraCatalogEntryByName(cameraName) {
  const name = String(cameraName || "").trim();
  for (let i = 0; i < cameraCatalog.length; i += 1) {
    const item = cameraCatalog[i];
    if (String(item && item.value ? item.value : "").trim() === name) return item || null;
  }
  return null;
}

function updateActiveSceneSidebarCard() {
  const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const cameraName = String(el.camera && el.camera.value ? el.camera.value : "").trim();
  const variantName = selectedSceneVariantValue();
  const variantMeta = variantCatalogEntryByName(variantName);
  const variantLabel = String(variantMeta && variantMeta.label ? variantMeta.label : "").trim() || (variantName || "(base)");
  const variantDescription = String(variantMeta && variantMeta.description ? variantMeta.description : "").trim();
  const sceneMeta = sceneCatalogEntryByFile(sceneName);
  const sceneLabel = String(sceneMeta && sceneMeta.label ? sceneMeta.label : "").trim()
    || String(sceneMeta && sceneMeta.sceneFile ? sceneMeta.sceneFile : "").trim()
    || (sceneName || "-");
  const sceneDescription = String(sceneMeta && sceneMeta.description ? sceneMeta.description : "").trim() || "-";
  const sourceOrigin = sceneSourceOriginLabel(currentSceneSourceOrigin);

  if (el.activeSceneCardScene) {
    el.activeSceneCardScene.replaceChildren();
    const value = document.createElement("span");
    value.className = "active-scene-row-value active-scene-scene-name";
    value.textContent = sceneLabel;
    el.activeSceneCardScene.appendChild(value);
  }

  if (el.activeSceneCardDescription) {
    el.activeSceneCardDescription.textContent = sceneDescription;
  }

  if (el.activeSceneCardSource) {
    el.activeSceneCardSource.replaceChildren();
    const key = document.createElement("span");
    key.className = "active-scene-row-label";
    key.textContent = "Source";
    el.activeSceneCardSource.appendChild(key);

    const value = document.createElement("code");
    value.className = "active-scene-row-value";
    value.textContent = sourceOrigin;
    el.activeSceneCardSource.appendChild(value);
  }

  if (el.activeSceneCardVariant) {
    el.activeSceneCardVariant.replaceChildren();
    const key = document.createElement("span");
    key.className = "active-scene-row-label";
    key.textContent = "Variant";
    el.activeSceneCardVariant.appendChild(key);

    const value = document.createElement("code");
    value.className = "active-scene-row-value";
    value.textContent = variantLabel;
    el.activeSceneCardVariant.appendChild(value);
  }

  if (el.activeSceneCardVariantDescription) {
    el.activeSceneCardVariantDescription.textContent = variantDescription;
    el.activeSceneCardVariantDescription.style.display = variantDescription ? "" : "none";
  }

  syncActiveCameraSelect(cameraName);
}

function cameraTypeLooksGeneric(cameraType) {
  const raw = String(cameraType || "").trim().toLowerCase();
  return !raw || raw === "camera";
}

function applyCameraTypesFromRuntimeGraph(sceneName, graphData, variantName) {
  const currentScene = String(sceneName || "").trim();
  const activeScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const activeVariant = selectedSceneVariantValue();
  const targetVariant = variantName !== undefined ? normalizeVariantName(variantName) : activeVariant;
  if (!currentScene || !activeScene || currentScene !== activeScene) return;
  if (normalizeVariantName(activeVariant) !== targetVariant) return;

  const graphCameras = Array.isArray(graphData && graphData.cameras) ? graphData.cameras : [];
  if (!graphCameras.length) return;
  const typeById = new Map();
  const graphOrder = [];
  graphCameras.forEach((cam) => {
    const id = String(cam && cam.id ? cam.id : "").trim();
    const type = String(cam && cam.type ? cam.type : "").trim();
    if (!id) return;
    typeById.set(id, type);
    graphOrder.push(id);
  });

  let changed = false;
  if (!Array.isArray(cameraCatalog)) cameraCatalog = [];
  const catalogById = new Map();
  cameraCatalog.forEach((item) => {
    const value = String(item && item.value ? item.value : "").trim();
    if (value) catalogById.set(value, item);
  });

  graphOrder.forEach((id) => {
    if (catalogById.has(id)) return;
    const next = {
      value: id,
      label: id,
      type: String(typeById.get(id) || "").trim(),
    };
    cameraCatalog.push(next);
    catalogById.set(id, next);
    changed = true;
  });

  cameraCatalog = cameraCatalog.map((item) => {
    const value = String(item && item.value ? item.value : "").trim();
    const prevType = String(item && item.type ? item.type : "").trim();
    if (!value || !cameraTypeLooksGeneric(prevType)) return item;
    const nextType = String(typeById.get(value) || "").trim();
    if (!nextType || cameraTypeLooksGeneric(nextType)) return item;
    changed = true;
    return { ...item, type: nextType };
  });

  cameraCatalog.sort((a, b) => String(a && a.value ? a.value : "").localeCompare(String(b && b.value ? b.value : "")));

  if (el.camera) {
    const previous = String(el.camera.value || "").trim();
    el.camera.innerHTML = "";
    cameraCatalog.forEach((item) => addOption(el.camera, item.value, item.label || item.value));
    if (cameraCatalogHasName(previous)) el.camera.value = previous;
    else if (el.camera.options.length > 0) el.camera.selectedIndex = 0;
  }

  if (changed) {
    renderCameraBrowser();
    updateCameraActivePanel();
  }
}

function reconcileCameraCatalogFromRuntimeGraph(sceneName, variantName) {
  const currentScene = String(sceneName || "").trim();
  if (!currentScene) return;
  const targetVariant = normalizeVariantName(variantName !== undefined ? variantName : selectedSceneVariantValue());
  const cachedGraph = getRuntimeGraphForScene(currentScene, targetVariant);
  if (!cachedGraph) return;
  applyCameraTypesFromRuntimeGraph(currentScene, cachedGraph, targetVariant);
}

function updateCameraFileCount() {
  if (!el.cameraFileCount) return;
  const count = cameraCatalog.length;
  el.cameraFileCount.textContent = String(count);
}

function updateVariantFileCount() {
  if (!el.variantFileCount) return;
  const count = variantCatalog.filter((item) => normalizeVariantName(item && item.value ? item.value : "")).length;
  el.variantFileCount.textContent = String(count);
}

function updateCameraActivePanel() {
  const active = String(el.camera && el.camera.value ? el.camera.value : "").trim();
  if (el.cameraActiveName) el.cameraActiveName.textContent = active || "-";
  updateActiveSceneSidebarCard();
}

function updateVariantActivePanel() {
  const active = selectedSceneVariantValue();
  const item = variantCatalogEntryByName(active);
  const label = String(item && item.label ? item.label : "").trim();
  const description = String(item && item.description ? item.description : "").trim();
  if (el.variantActiveName) el.variantActiveName.textContent = label || active || "(base)";
  if (el.variantActiveDescription) {
    el.variantActiveDescription.textContent = description || (active ? "Variant has no description." : "Uses the base scene definition.");
  }
  updateActiveSceneSidebarCard();
}

function syncActiveCameraSelect(activeCameraName) {
  const select = document.getElementById("activeSceneCameraSelect");
  if (!select) return;

  const activeCamera = activeCameraName !== undefined
    ? String(activeCameraName || "")
    : String(el.camera && el.camera.value ? el.camera.value : "");

  const catalogValues = cameraCatalog.map((c) => String(c.value || ""));
  const currentValues = Array.from(select.options).map((o) => o.value);
  const needsRebuild = catalogValues.length !== currentValues.length
    || catalogValues.some((v, i) => v !== currentValues[i]);

  if (needsRebuild) {
    select.innerHTML = "";
    if (!cameraCatalog.length) {
      const opt = document.createElement("option");
      opt.value = "";
      opt.textContent = "No cameras";
      select.appendChild(opt);
    } else {
      cameraCatalog.forEach((item) => {
        const opt = document.createElement("option");
        opt.value = String(item.value || "");
        opt.textContent = String(item.label || item.value || "");
        select.appendChild(opt);
      });
    }
  }

  if (select.value !== activeCamera) select.value = activeCamera;

  if (!select._cameraSelectBound) {
    select._cameraSelectBound = true;
    select.addEventListener("change", () => {
      activateCamera(select.value);
    });
  }
}

function renderCameraBrowser() {
  if (!el.cameraFileList) return;
  const widgets = window.XTracerWidgets || {};
  if (!cameraBrowserSelectedName || !cameraCatalogHasName(cameraBrowserSelectedName)) {
    cameraBrowserSelectedName = String(el.camera && el.camera.value ? el.camera.value : "");
  }

  el.cameraFileList.replaceChildren();
  updateCameraFileCount();
  updateCameraActivePanel();

  if (!cameraCatalog.length) {
    el.cameraFileList.appendChild(window.XTracerWidgets.createEmptyState({
      title: "No cameras",
      message: "No cameras found in this scene.",
      className: "scene-file-empty",
    }));
    return;
  }

  cameraCatalog.forEach((item) => {
    const value = String(item && item.value ? item.value : "");
    const label = String(item && item.label ? item.label : "");
    const camType = String(item && item.type ? item.type : "").trim();
    const isSelected = cameraBrowserSelectedName === value;
    const isActive = String(el.camera && el.camera.value ? el.camera.value : "") === value;

    const button = widgets.createCameraCard({
      value,
      label: label || value || "-",
      description: `${cameraTypeLabel(camType)} camera`,
      selected: isSelected,
      active: isActive,
      icon: createCameraIcon(camType),
    });
    button.addEventListener("click", () => {
      activateCamera(value);
    });
    button.addEventListener("keydown", (evt) => {
      if (evt.key === "Enter" || evt.key === " ") {
        evt.preventDefault();
        activateCamera(value);
      }
    });

    el.cameraFileList.appendChild(button);
  });
  syncActiveCameraSelect();
}

function renderVariantBrowser() {
  if (!el.variantFileList) return;
  const widgets = window.XTracerWidgets || {};
  if (!variantBrowserSelectedName || !variantCatalogHasName(variantBrowserSelectedName)) {
    variantBrowserSelectedName = selectedSceneVariantValue();
  }

  el.variantFileList.replaceChildren();
  updateVariantFileCount();
  updateVariantActivePanel();

  if (!variantCatalog.length) {
    el.variantFileList.appendChild(window.XTracerWidgets.createEmptyState({
      title: "No variants",
      message: "No variants found in this scene.",
      className: "scene-file-empty",
    }));
    return;
  }

  variantCatalog.forEach((item) => {
    const value = normalizeVariantName(item && item.value ? item.value : "");
    const label = String(item && item.label ? item.label : "").trim() || "(base)";
    const description = String(item && item.description ? item.description : "").trim();
    const isBase = !value;
    const isSelected = normalizeVariantName(variantBrowserSelectedName) === value;
    const isActive = selectedSceneVariantValue() === value;

    const button = widgets.createVariantCard({
      value,
      label,
      description,
      base: isBase,
      selected: isSelected,
      active: isActive,
      icon: createVariantIcon(isBase),
    });
    button.addEventListener("click", () => {
      activateVariant(value);
    });
    button.addEventListener("keydown", (evt) => {
      if (evt.key === "Enter" || evt.key === " ") {
        evt.preventDefault();
        activateVariant(value);
      }
    });

    el.variantFileList.appendChild(button);
  });
}

function setCameraBrowserSelectedCamera(cameraName) {
  const name = String(cameraName || "");
  if (cameraCatalogHasName(name)) cameraBrowserSelectedName = name;
  else cameraBrowserSelectedName = "";
  renderCameraBrowser();
}

function setVariantBrowserSelectedVariant(variantName) {
  const name = normalizeVariantName(variantName);
  if (variantCatalogHasName(name)) variantBrowserSelectedName = name;
  else variantBrowserSelectedName = "";
  renderVariantBrowser();
}

function activateCamera(cameraName) {
  if (!el.camera) return;
  const name = String(cameraName || "");
  if (!cameraCatalogHasName(name)) return;
  cameraBrowserSelectedName = name;
  if (String(el.camera.value || "") === name) {
    updateCameraActivePanel();
    return;
  }
  el.camera.value = name;
  el.camera.dispatchEvent(new Event("change", { bubbles: true }));
}

function activateVariant(variantName) {
  if (!el.variant) return;
  const name = normalizeVariantName(variantName);
  if (!variantCatalogHasName(name)) return;
  variantBrowserSelectedName = name;
  if (selectedSceneVariantValue() === name) {
    updateVariantActivePanel();
    return;
  }
  el.variant.value = name;
  el.variant.dispatchEvent(new Event("change", { bubbles: true }));
}

function updateSceneDependencyPill(sceneFile) {
  const pill = el.sceneDependencyPill;
  const active = String(sceneFile || (el.scene && el.scene.value ? el.scene.value : "") || "").trim();
  if (el.sceneActiveFile) el.sceneActiveFile.textContent = active || "-";
  if (!pill) {
    updateActiveSceneSidebarCard();
    renderSceneFileBrowser();
    return;
  }
  const dependsExternal = !!sceneDependencyByFile.get(active || "");
  pill.hidden = !dependsExternal;
  pill.textContent = "EXT";
  pill.classList.toggle("scene-kind-ext", dependsExternal);
  pill.classList.toggle("scene-kind-self", !dependsExternal);
  updateActiveSceneSidebarCard();
  renderSceneFileBrowser();
}

function presetId(index) {
  return String(index).padStart(2, "0");
}

function formatResolutionAspect(width, height) {
  const w = Number(width);
  const h = Number(height);
  if (!Number.isFinite(w) || !Number.isFinite(h) || w <= 0 || h <= 0) return "-";
  if (Math.abs(w - h) < 0.0001) return "1:1";
  const fmt = (v) => {
    const rounded = Math.round(v * 10) / 10;
    return Number.isInteger(rounded) ? String(rounded) : rounded.toFixed(1);
  };
  if (w > h) return `${fmt(w / h)}:1`;
  return `1:${fmt(h / w)}`;
}

function orientationMode(width, height) {
  if (width > height) return "landscape";
  if (height > width) return "portrait";
  return "square";
}

function normalizeResolutionModeFilter(mode) {
  const value = String(mode || "").toLowerCase();
  if (value === "landscape" || value === "portrait" || value === "square") return value;
  return "all";
}

function updateResolutionModeFilterButtons() {
  const activeMode = normalizeResolutionModeFilter(resolutionPresetModeFilter);
  const buttons = [
    { mode: "all", node: el.resolutionModeFilterAll },
    { mode: "square", node: el.resolutionModeFilterSquare },
    { mode: "portrait", node: el.resolutionModeFilterPortrait },
    { mode: "landscape", node: el.resolutionModeFilterLandscape },
  ];
  buttons.forEach((entry) => {
    if (!entry.node) return;
    const active = entry.mode === activeMode;
    entry.node.classList.toggle("active", active);
    entry.node.setAttribute("aria-pressed", active ? "true" : "false");
  });
}

function setResolutionPresetModeFilter(mode) {
  resolutionPresetModeFilter = normalizeResolutionModeFilter(mode);
  updateResolutionModeFilterButtons();
}

function renderResolutionPresetList() {
  if (!el.resolutionPresetList) return;
  const selected = String(el.resolutionPreset ? el.resolutionPreset.value : "custom");
  const modeFilter = normalizeResolutionModeFilter(resolutionPresetModeFilter);
  updateResolutionModeFilterButtons();
  el.resolutionPresetList.innerHTML = "";

  const addRow = (value, name, width, height) => {
    const btn = document.createElement("button");
    btn.type = "button";
    btn.className = "resolution-preset-row";
    btn.setAttribute("role", "option");
    btn.setAttribute("data-value", value);
    const mode = (Number(width) > 0 && Number(height) > 0) ? orientationMode(width, height) : "custom";
    const aspect = (Number(width) > 0 && Number(height) > 0) ? formatResolutionAspect(width, height) : "-";
    const modeLabel = mode === "landscape" ? "Landscape" : (mode === "portrait" ? "Portrait" : (mode === "square" ? "Square" : "Custom"));
    btn.innerHTML = ""
      + `<span class="resolution-cell resolution-name">${name}</span>`
      + `<span class="resolution-cell resolution-mode" title="${modeLabel}" aria-label="${modeLabel}"><span class="resolution-orient is-${mode}" aria-hidden="true"></span></span>`
      + `<span class="resolution-cell resolution-aspect">${aspect}</span>`
      + `<span class="resolution-cell resolution-width">${Number(width) > 0 ? width : "-"}</span>`
      + `<span class="resolution-cell resolution-height">${Number(height) > 0 ? height : "-"}</span>`;
    const isActive = String(value) === selected;
    btn.classList.toggle("is-active", isActive);
    btn.setAttribute("aria-selected", isActive ? "true" : "false");
    btn.addEventListener("click", () => {
      if (!el.resolutionPreset) return;
      if (el.resolutionPreset.value === String(value)) return;
      el.resolutionPreset.value = String(value);
      el.resolutionPreset.dispatchEvent(new Event("change", { bubbles: true }));
      renderResolutionPresetList();
    });
    el.resolutionPresetList.appendChild(btn);
  };

  addRow("custom", "Custom", 0, 0);
  resolutionPresets.forEach((preset, index) => {
    const mode = orientationMode(preset.width, preset.height);
    if (modeFilter !== "all" && mode !== modeFilter) return;
    const name = String((preset && preset.description) || "").trim() || `Preset ${presetId(index)}`;
    addRow(String(index), name, preset.width, preset.height);
  });

  const active = el.resolutionPresetList.querySelector(".resolution-preset-row.is-active");
  if (active && active.scrollIntoView) {
    active.scrollIntoView({ block: "nearest", inline: "nearest", behavior: "auto" });
  }
}

function syncResolutionPresetFromInputs() {
  const { width, height } = currentRenderSize();
  const index = resolutionPresets.findIndex((p) => p.width === width && p.height === height);
  el.resolutionPreset.value = index >= 0 ? String(index) : "custom";
  renderResolutionPresetList();
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
  resolutionPresets.forEach((preset, index) => {
    const desc = String((preset && preset.description) || "").trim() || `Preset ${presetId(index)}`;
    const sizeLabel = `${preset.width}x${preset.height}`;
    addOption(el.resolutionPreset, String(index), `${desc} ${sizeLabel}`);
  });
  syncResolutionPresetFromInputs();
  renderResolutionPresetList();
}


async function loadScenes(opts) {
  const skipStorageRestore = !!(opts && opts.skipStorageRestore);
  const scenes = await api.getScenes();
  const sceneItems = await buildSceneLabels(scenes);
  const prev = el.scene.value;
  const saved = skipStorageRestore ? "" : String(localStorage.getItem(LAST_SCENE_KEY) || "").trim();
  el.scene.innerHTML = "";
  sceneCatalog = sceneItems.slice();
  sceneDependencyByFile = new Map(sceneItems.map((item) => [item.sceneFile, !!item.dependsExternal]));
  sceneItems.forEach((item) => addOption(el.scene, item.sceneFile, item.sceneFile));
  const preferred = prev || saved;
  if (preferred) el.scene.value = preferred;
  if (!el.scene.value && el.scene.options.length > 0 && !skipStorageRestore) el.scene.selectedIndex = 0;
  if (!sceneCatalogHasFile(sceneBrowserSelectedFile)) {
    sceneBrowserSelectedFile = String(el.scene.value || "").trim();
  }
  updateSceneDependencyPill(el.scene.value);
  if (!skipStorageRestore) {
    if (el.scene.value) localStorage.setItem(LAST_SCENE_KEY, el.scene.value);
    else localStorage.removeItem(LAST_SCENE_KEY);
  }
}


async function loadCameras(scene, variant) {
  const variantName = normalizeVariantName(variant !== undefined ? variant : selectedSceneVariantValue());
  const requestToken = ++cameraLoadToken;
  if (!scene) {
    el.camera.innerHTML = "";
    cameraCatalog = [];
    renderCameraBrowser();
    syncVisualCameraFromRenderSelection();
    return;
  }

  const info = await api.getCameras(scene, variantName);
  if (requestToken !== cameraLoadToken) return info;
  el.camera.innerHTML = "";
  const sceneName = String(scene || "").trim();
  const infoObj = Array.isArray(info) ? { cameras: info } : (info || {});
  const cameras = Array.isArray(infoObj.cameras) ? infoObj.cameras : [];
  const cameraEntriesRaw = Array.isArray(infoObj.cameraEntries) ? infoObj.cameraEntries : [];
  const defaultCamera = String(infoObj.defaultCamera || "");
  cameras.forEach((name) => addOption(el.camera, name, name));
  if (cameraEntriesRaw.length > 0) {
    cameraCatalog = cameraEntriesRaw
      .map((entry) => ({
        value: String(entry && entry.name ? entry.name : ""),
        label: String(entry && entry.name ? entry.name : ""),
        type: String(entry && entry.type ? entry.type : ""),
      }))
      .filter((entry) => entry.value.length > 0);
  } else {
    cameraCatalog = cameras.map((name) => ({ value: name, label: name, type: "" }));
  }

  if (defaultCamera && cameras.includes(defaultCamera)) {
    el.camera.value = defaultCamera;
  } else if (cameras.length > 0) {
    el.camera.value = cameras[0];
  } else {
    el.camera.value = "";
  }
  if (!cameraCatalogHasName(cameraBrowserSelectedName)) {
    cameraBrowserSelectedName = String(el.camera.value || "");
  }
  reconcileCameraCatalogFromRuntimeGraph(sceneName, variantName);
  renderCameraBrowser();

  syncVisualCameraFromRenderSelection();
}

async function loadVariants(scene, preferredVariant, preloadedSource) {
  if (!el.variant) return;
  el.variant.innerHTML = "";
  const sceneName = String(scene || "").trim();
  if (preferredVariant === undefined && pendingVariantForNextSceneLoad !== null) {
    preferredVariant = pendingVariantForNextSceneLoad;
    pendingVariantForNextSceneLoad = null;
  }
  let parsed = null;

  if (sceneName) {
    try {
      if (preloadedSource !== undefined) {
        parsed = extractSceneVariantNames(String(preloadedSource || ""));
      } else {
        const data = await api.getSceneSource(sceneName);
        parsed = extractSceneVariantNames(data && data.source ? data.source : "");
      }
    } catch (_) {
      // keep base-only when source is unavailable
    }
  }

  const hideBase = !!(parsed && parsed.hideBase);
  variantCatalog = [];
  if (!hideBase) {
    const baseLabel = String(parsed && parsed.base && parsed.base.label ? parsed.base.label : "(base)");
    const baseDescription = String(parsed && parsed.base && parsed.base.description ? parsed.base.description : "");
    variantCatalog.push({ value: "", label: baseLabel, description: baseDescription });
    addOption(el.variant, "", baseLabel);
  }

  const variants = Array.isArray(parsed && parsed.variants) ? parsed.variants : [];
  variants.forEach((entry) => {
    const id = normalizeVariantName(entry && entry.id ? entry.id : "");
    if (!id) return;
    const label = String(entry && entry.label ? entry.label : id).trim() || id;
    const description = String(entry && entry.description ? entry.description : "");
    variantCatalog.push({ value: id, label, description });
    addOption(el.variant, id, label);
  });

  const requested = preferredVariant !== undefined
    ? normalizeVariantName(preferredVariant)
    : selectedSceneVariantValue();
  if (variantCatalogHasName(requested)) el.variant.value = requested;
  else if (variantCatalog.length > 0) el.variant.value = String(variantCatalog[0].value || "");
  else el.variant.value = "";
  if (!variantCatalogHasName(variantBrowserSelectedName)) {
    variantBrowserSelectedName = selectedSceneVariantValue();
  }
  renderVariantBrowser();
}

async function loadIntegrators() {
  const integrators = await api.getIntegrators();
  const appConfig = await loadAppConfig();
  const configuredDefault = normalizeConfiguredDefaultIntegratorId(appConfig);
  integratorCatalog = Array.isArray(integrators) ? integrators : [];
  integratorById = new Map(integratorCatalog.map((it) => [it.id, it]));
  const prev = el.integrator.value;
  el.integrator.innerHTML = "";
  integratorCatalog.forEach((it) => addOption(el.integrator, it.id, it.label));
  if (prev && integratorCatalog.some((it) => it.id === prev)) {
    el.integrator.value = prev;
  } else if (configuredDefault && integratorCatalog.some((it) => it.id === configuredDefault)) {
    el.integrator.value = configuredDefault;
  } else if (integratorCatalog.some((it) => it.id === "pathtracer_mis")) {
    el.integrator.value = "pathtracer_mis";
  } else if (!el.integrator.value && el.integrator.options.length > 0) {
    el.integrator.selectedIndex = 0;
  }
  renderIntegratorControls();
}

async function loadSceneSource(scene, preloadedData) {
  if (!scene) {
    currentSceneSourceOrigin = "";
    el.sceneSource.value = "";
    updateEditorMetrics();
    renderSceneGraphView();
    resetSceneHistoriesFromCurrentSource();
    updateActiveSceneSidebarCard();
    return;
  }
  const data = (preloadedData !== undefined && preloadedData !== null)
    ? preloadedData
    : await api.getSceneSource(scene);
  const newSource = String(data.source || "");
  const sourceChanged = newSource !== String(el.sceneSource.value || "");
  currentSceneSourceOrigin = normalizeSceneSourceOrigin(data && data.source_origin ? data.source_origin : "");
  el.sceneName.value = data.scene || scene;
  el.sceneSource.value = newSource;
  updateEditorMetrics();
  syncEditorScroll();
  renderSceneGraphView();
  if (sourceChanged) resetSceneHistoriesFromCurrentSource();
  updateActiveSceneSidebarCard();
  // Enrich the catalog entry now that we have the source, avoiding a separate fetch.
  const loadedSceneName = String(scene || "").trim();
  for (let ci = 0; ci < sceneCatalog.length; ci += 1) {
    if (String(sceneCatalog[ci] && sceneCatalog[ci].sceneFile ? sceneCatalog[ci].sceneFile : "") !== loadedSceneName) continue;
    const eTitle = extractSceneTitle(newSource);
    const eDesc = extractSceneDescription(newSource);
    const eDepExt = sceneDependsOnExternalFiles(newSource);
    const eVarMeta = extractSceneVariantNames(newSource);
    const eVarCount = Array.isArray(eVarMeta && eVarMeta.variants) ? eVarMeta.variants.length : 0;
    const eCamCount = countTopLevelSceneEntries(newSource, "camera");
    sceneCatalog[ci] = { ...sceneCatalog[ci], label: eTitle || loadedSceneName, title: eTitle, description: eDesc, sourceOrigin: currentSceneSourceOrigin, dependsExternal: eDepExt, variantCount: eVarCount, cameraCount: eCamCount, hasVariants: eVarCount > 0 };
    renderSceneFileBrowser();
    break;
  }
}

async function loadSceneRuntimeGraph(scene, variant) {
  const sceneName = String(scene || "").trim();
  if (!sceneName || !hasBackendMethod(api, "getSceneRuntimeGraph")) return null;
  const variantName = normalizeVariantName(variant !== undefined ? variant : selectedSceneVariantValue());
  const requestToken = ++runtimeGraphLoadToken;
  const data = await api.getSceneRuntimeGraph(sceneName, variantName);
  if (requestToken !== runtimeGraphLoadToken) return data || null;
  const key = runtimeGraphCacheKey(sceneName, variantName);
  runtimeGraphByScene.set(key, data || { cameras: [], objects: [], surfaces: [], materials: [], media: [] });
  applyCameraTypesFromRuntimeGraph(sceneName, runtimeGraphByScene.get(key), variantName);
  if (String(el.scene && el.scene.value ? el.scene.value : "").trim() === sceneName) {
    renderSceneGraphView();
  }
  return runtimeGraphByScene.get(key) || null;
}

function renderThirdPartyLicenses(rawItems) {
  const items = Array.isArray(rawItems) && rawItems.length
    ? rawItems
    : DEFAULT_THIRD_PARTY_LICENSES;
  if (!el.aboutThirdPartyList) return;
  if (window.XTracerWidgets && typeof window.XTracerWidgets.renderDependencyList === "function") {
    window.XTracerWidgets.renderDependencyList(el.aboutThirdPartyList, items);
    return;
  }
  el.aboutThirdPartyList.replaceChildren(...items.map((item) => {
    const entry = document.createElement("article");
    entry.className = "xui-dependency";
    entry.innerHTML = `
      <div class="xui-dependency__main">
        <h4 class="xui-dependency__name">${escapeHtml(item && item.name ? String(item.name) : "Unknown")}</h4>
        ${item && item.description ? `<p class="xui-dependency__description">${escapeHtml(String(item.description))}</p>` : ""}
        <div class="xui-dependency__facts">
          <div class="xui-dependency__fact">
            <span class="xui-dependency__fact-label">Used in</span>
            <span class="xui-dependency__fact-value xui-dependency__usage">${escapeHtml(item && item.used_in ? String(item.used_in) : "Unknown")}</span>
          </div>
          <div class="xui-dependency__fact">
            <span class="xui-dependency__fact-label">License</span>
            <span class="xui-dependency__fact-value xui-dependency__license">${escapeHtml(item && item.license ? String(item.license) : "Unknown")}</span>
          </div>
        </div>
      </div>
      ${item && item.url ? `<a class="xui-dependency__link" href="${escapeAttr(String(item.url))}" target="_blank" rel="noopener noreferrer"><span class="xui-dependency__link-label">Repository</span><span class="xui-dependency__link-value">${escapeHtml(String(item.url))}</span></a>` : ""}
    `;
    return entry;
  }));
}

function fitAboutLicenseText() {
  if (!el.aboutLicense) return;
  const node = el.aboutLicense;
  const raw = String(node.textContent || "");
  if (!raw.trim()) {
    node.style.fontSize = "";
    return;
  }

  // Start from stylesheet size, then shrink only if needed.
  node.style.fontSize = "";
  const style = window.getComputedStyle(node);
  const basePx = parseFloat(style.fontSize) || 10;
  const padL = parseFloat(style.paddingLeft) || 0;
  const padR = parseFloat(style.paddingRight) || 0;
  const available = Math.max(1, node.clientWidth - padL - padR);
  if (available <= 1) return;

  const lines = raw.split(/\r?\n/);
  let longest = "";
  for (let i = 0; i < lines.length; i += 1) {
    if (lines[i].length > longest.length) longest = lines[i];
  }
  if (!longest) return;

  const canvas = fitAboutLicenseText._measureCanvas
    || (fitAboutLicenseText._measureCanvas = document.createElement("canvas"));
  const ctx = canvas.getContext("2d");
  if (!ctx) return;
  const fontStyle = style.fontStyle || "normal";
  const fontWeight = style.fontWeight || "400";
  const fontFamily = style.fontFamily || "monospace";
  ctx.font = `${fontStyle} ${fontWeight} ${basePx}px ${fontFamily}`;
  const measured = ctx.measureText(longest).width;
  if (!Number.isFinite(measured) || measured <= 0) return;
  if (measured <= available) return;

  const minPx = 4.5;
  const fittedPx = Math.max(minPx, basePx * (available / measured));
  node.style.fontSize = `${fittedPx.toFixed(3)}px`;
}

async function loadAbout() {
  const data = await api.getAbout();
  const rawVersion = (data.version || "").trim();
  const hideStandaloneVersion = !rawVersion || rawVersion.toLowerCase() === "standalone";
  const version = hideStandaloneVersion ? "" : rawVersion;
  el.aboutVersion.textContent = version;
  el.aboutVersionRow.hidden = hideStandaloneVersion;
  if (el.aboutBuildPill) {
    const bt = (data.build_type || "").toLowerCase();
    if (bt) {
      el.aboutBuildPill.textContent = bt;
      el.aboutBuildPill.dataset.buildType = bt;
      el.aboutBuildPill.hidden = false;
    }
  }
  const homepage = data.homepage || "https://www.4rknova.com";
  el.aboutHomepage.href = homepage;
  el.aboutHomepage.textContent = homepage;
  const repository = data.website || "https://github.com/4rknova/xtracer";
  el.aboutWebsite.href = repository;
  el.aboutWebsite.textContent = repository;
  el.aboutCopyright.textContent = data.copyright || "unknown";
  el.aboutLicense.textContent = data.license || "Unavailable";
  const backendLabel = data.backend || "xtracer_web";
  el.aboutBackend.textContent = backendLabel;
  el.aboutDefaultUrl.textContent = data.default_url || window.location.origin;
  el.aboutSceneDir.textContent = data.scene_dir || "scene/";
  el.aboutStaticAssets.textContent = data.static_assets || "/";
  updateWorkspaceServerStatsHints(data);
  renderThirdPartyLicenses(data.third_party_licenses);
  requestAnimationFrame(() => {
    fitAboutLicenseText();
  });
}

async function loadEmptySceneTemplate() {
  if (hasBackendMethod(api, "getEmptySceneTemplate")) {
    return api.getEmptySceneTemplate();
  }
  const data = await getJSON("/api/scenes/template/empty");
  return data.source || "";
}

function showSceneSelectModal(options) {
  const opts = options || {};
  const dom = window.XTracerWidgets && window.XTracerWidgets.dom;
  if (!dom) return;

  let selectedScene = null;
  let selectedVariant = null;
  let step = 1;
  let stepVariants = [];

  const overlay = document.createElement("div");
  overlay.className = "xui-modal scene-select-modal";
  overlay.setAttribute("role", "presentation");

  const backdrop = document.createElement("div");
  backdrop.className = "xui-modal-backdrop";
  backdrop.setAttribute("aria-hidden", "true");

  const dialog = document.createElement("div");
  dialog.className = "xui-modal-dialog scene-select-dialog";
  dialog.setAttribute("role", "dialog");
  dialog.setAttribute("aria-modal", "true");
  dialog.setAttribute("tabindex", "-1");

  overlay.appendChild(backdrop);
  overlay.appendChild(dialog);
  document.body.appendChild(overlay);

  function close() {
    overlay.remove();
  }

  backdrop.addEventListener("click", close);
  overlay.addEventListener("keydown", (e) => {
    if (e.key === "Escape") close();
  });

  function applySelection() {
    if (!selectedScene) return;
    close();
    if (typeof opts.onConfirm === "function") {
      opts.onConfirm(selectedScene, selectedVariant);
    } else {
      if (selectedVariant !== null) pendingVariantForNextSceneLoad = selectedVariant;
      activateSceneFile(selectedScene);
    }
  }

  function buildStep2() {
    step = 2;
    dialog.innerHTML = "";

    const head = document.createElement("div");
    head.className = "xui-modal-head";
    const title = document.createElement("h2");
    title.className = "xui-modal-title";
    title.textContent = "Select Variant";
    const stepLabel = document.createElement("span");
    stepLabel.className = "scene-select-step-label";
    stepLabel.textContent = "Step 2 of 2";
    head.appendChild(title);
    head.appendChild(stepLabel);

    const body = document.createElement("div");
    body.className = "xui-modal-body scene-select-body";

    const list = document.createElement("div");
    list.className = "scene-select-variant-list";

    stepVariants.forEach((entry) => {
      const value = String(entry.value || "");
      const label = String(entry.label || value || "(base)");
      const description = String(entry.description || "");
      const isBase = !value;

      const btn = document.createElement("button");
      btn.type = "button";
      btn.className = "scene-select-variant-btn";
      btn.setAttribute("role", "option");
      if (isBase) btn.classList.add("is-base");
      if (value === (selectedVariant || "")) btn.classList.add("is-selected");

      const nameEl = document.createElement("span");
      nameEl.className = "scene-select-variant-name";
      nameEl.textContent = label;
      btn.appendChild(nameEl);

      if (description) {
        const descEl = document.createElement("span");
        descEl.className = "scene-select-variant-desc";
        descEl.textContent = description;
        btn.appendChild(descEl);
      }

      btn.addEventListener("click", () => {
        list.querySelectorAll(".scene-select-variant-btn").forEach((b) => b.classList.remove("is-selected"));
        btn.classList.add("is-selected");
        selectedVariant = value;
      });

      list.appendChild(btn);
    });

    body.appendChild(list);

    const actions = document.createElement("div");
    actions.className = "xui-modal-actions";

    const backBtn = document.createElement("button");
    backBtn.type = "button";
    backBtn.className = "xui-button xui-button--ghost";
    backBtn.textContent = "Back";
    backBtn.addEventListener("click", buildStep1);

    const selectBtn = document.createElement("button");
    selectBtn.type = "button";
    selectBtn.className = "xui-button xui-button--primary";
    selectBtn.textContent = "Select";
    selectBtn.addEventListener("click", applySelection);

    actions.appendChild(backBtn);
    actions.appendChild(selectBtn);

    dialog.appendChild(head);
    dialog.appendChild(body);
    dialog.appendChild(actions);
    dialog.setAttribute("aria-label", "Select Variant");
    dialog.focus();
  }

  async function advanceFromStep1() {
    if (!selectedScene) return;
    try {
      const data = await api.getSceneSource(selectedScene);
      const parsed = extractSceneVariantNames(data && data.source ? data.source : "");
      stepVariants = [];
      if (parsed && !parsed.hideBase) {
        const baseLabel = String(parsed.base && parsed.base.label ? parsed.base.label : "(base)");
        const baseDesc = String(parsed.base && parsed.base.description ? parsed.base.description : "");
        stepVariants.push({ value: "", label: baseLabel, description: baseDesc });
      }
      const variants = Array.isArray(parsed && parsed.variants) ? parsed.variants : [];
      variants.forEach((v) => {
        const id = normalizeVariantName(v && v.id ? v.id : "");
        if (!id) return;
        stepVariants.push({
          value: id,
          label: String(v.label || id).trim() || id,
          description: String(v.description || ""),
        });
      });

      if (stepVariants.length <= 1) {
        selectedVariant = stepVariants.length ? String(stepVariants[0].value || "") : null;
        applySelection();
        return;
      }

      selectedVariant = stepVariants[0] ? String(stepVariants[0].value || "") : null;
      buildStep2();
    } catch (_) {
      selectedVariant = null;
      applySelection();
    }
  }

  function buildStep1() {
    step = 1;
    dialog.innerHTML = "";

    const head = document.createElement("div");
    head.className = "xui-modal-head";
    const title = document.createElement("h2");
    title.className = "xui-modal-title";
    title.textContent = "Select Scene";
    const stepHint = document.createElement("span");
    stepHint.className = "scene-select-step-hint";
    stepHint.textContent = "Choose a scene for the active workspace";
    head.appendChild(title);
    head.appendChild(stepHint);

    const body = document.createElement("div");
    body.className = "xui-modal-body scene-select-body";

    const searchInput = document.createElement("input");
    searchInput.type = "search";
    searchInput.className = "scene-select-search xui-input";
    searchInput.placeholder = "Search scenes…";
    searchInput.setAttribute("aria-label", "Search scenes");

    const list = document.createElement("div");
    list.className = "scene-select-list";
    list.setAttribute("role", "listbox");
    list.setAttribute("aria-label", "Scene files");

    const currentScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
    if (!selectedScene && currentScene) selectedScene = currentScene;

    function renderSceneList(query) {
      list.innerHTML = "";
      const q = String(query || "").trim().toLowerCase();
      const items = sceneCatalog.filter((s) => {
        if (!q) return true;
        return String(s.sceneFile || "").toLowerCase().includes(q)
          || String(s.label || "").toLowerCase().includes(q)
          || String(s.title || "").toLowerCase().includes(q);
      });

      if (!items.length) {
        const empty = document.createElement("p");
        empty.className = "scene-select-empty";
        empty.textContent = q ? "No scenes match your search." : "No scenes available.";
        list.appendChild(empty);
        return;
      }

      items.forEach((s) => {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.className = "scene-select-scene-btn";
        btn.setAttribute("role", "option");
        if (s.sceneFile === selectedScene) btn.classList.add("is-selected");

        const nameEl = document.createElement("span");
        nameEl.className = "scene-select-scene-name";
        nameEl.textContent = s.label || s.sceneFile || "-";
        btn.appendChild(nameEl);

        const metaEl = document.createElement("span");
        metaEl.className = "scene-select-scene-meta";
        const pills = [];
        if (s.hasVariants) pills.push(`${s.variantCount} variant${s.variantCount !== 1 ? "s" : ""}`);
        if (s.cameraCount) pills.push(`${s.cameraCount} cam${s.cameraCount !== 1 ? "s" : ""}`);
        if (s.dependsExternal) pills.push("EXT");
        metaEl.textContent = pills.join(" · ");
        if (pills.length) btn.appendChild(metaEl);

        btn.addEventListener("click", () => {
          list.querySelectorAll(".scene-select-scene-btn").forEach((b) => b.classList.remove("is-selected"));
          btn.classList.add("is-selected");
          selectedScene = s.sceneFile;
        });

        btn.addEventListener("dblclick", () => {
          selectedScene = s.sceneFile;
          advanceFromStep1();
        });

        list.appendChild(btn);
      });
    }

    searchInput.addEventListener("input", () => renderSceneList(searchInput.value));
    renderSceneList("");

    body.appendChild(searchInput);
    body.appendChild(list);

    const actions = document.createElement("div");
    actions.className = "xui-modal-actions";

    const cancelBtn = document.createElement("button");
    cancelBtn.type = "button";
    cancelBtn.className = "xui-button xui-button--ghost";
    cancelBtn.textContent = "Cancel";
    cancelBtn.addEventListener("click", close);

    const nextBtn = document.createElement("button");
    nextBtn.type = "button";
    nextBtn.className = "xui-button xui-button--primary";
    nextBtn.textContent = "Next";
    nextBtn.addEventListener("click", () => advanceFromStep1());

    actions.appendChild(cancelBtn);
    actions.appendChild(nextBtn);

    dialog.appendChild(head);
    dialog.appendChild(body);
    dialog.appendChild(actions);
    dialog.setAttribute("aria-label", "Select Scene");
    dialog.focus();
    requestAnimationFrame(() => searchInput.focus());
  }

  buildStep1();
}
