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
      const description = extractSceneDescription(source);
      const dependsExternal = sceneDependsOnExternalFiles(source);
      const variantMeta = extractSceneVariantNames(source);
      const hasVariants = Array.isArray(variantMeta && variantMeta.variants) && variantMeta.variants.length > 0;
      return {
        sceneFile,
        label: title || sceneFile,
        title,
        description,
        dependsExternal,
        hasVariants,
      };
    } catch (_) {
      return {
        sceneFile,
        label: sceneFile,
        title: "",
        description: "",
        dependsExternal: false,
        hasVariants: false,
      };
    }
  }));

  return items;
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
  if (raw.indexOf("perspective") >= 0 || raw.indexOf("thin") >= 0 || raw.indexOf("lens") >= 0) return "perspective";
  return "camera";
}

function cameraTypeLabel(cameraType) {
  const kind = normalizeCameraType(cameraType);
  if (kind === "perspective") return "Perspective";
  if (kind === "erp") return "Equirectangular";
  if (kind === "ods") return "Omni Stereo";
  if (kind === "cubemap") return "Cubemap";
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
  if (!text) return { base: { label: "(base)", description: "" }, variants: [] };

  const groupRange = findSceneGroupRange(text, "variants");
  if (!groupRange) return { base: { label: "(base)", description: "" }, variants: [] };

  const unquote = (value) => {
    const s = String(value || "").trim();
    if (s.length >= 2 && ((s[0] === "\"" && s[s.length - 1] === "\"") || (s[0] === "'" && s[s.length - 1] === "'"))) {
      return s.slice(1, -1).trim();
    }
    return s;
  };

  const entries = splitTopLevelSceneEntries(text, groupRange);
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
  };
}

function updateSceneFileCount() {
  if (!el.sceneFileCount) return;
  const count = sceneCatalog.length;
  el.sceneFileCount.textContent = String(count);
}

function ensureSceneContextMenu() {
  if (document.getElementById("sceneFileContextMenu")) return;
  const menu = document.createElement("div");
  menu.id = "sceneFileContextMenu";
  menu.className = "scene-file-context-menu";
  menu.hidden = true;
  menu.innerHTML = ""
    + "<button id=\"sceneCtxSetActive\" type=\"button\">Set Active</button>"
    + "<button id=\"sceneCtxDelete\" type=\"button\" class=\"danger\">Delete</button>";
  document.body.appendChild(menu);

  const close = () => {
    menu.hidden = true;
    delete menu.dataset.scene;
  };

  const setActiveBtn = menu.querySelector("#sceneCtxSetActive");
  const deleteBtn = menu.querySelector("#sceneCtxDelete");
  if (setActiveBtn) {
    setActiveBtn.addEventListener("click", () => {
      const sceneFile = String(menu.dataset.scene || "").trim();
      close();
      if (!sceneFile) return;
      activateSceneFile(sceneFile);
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

  const ok = window.confirm(`Delete scene ${sceneName}? This cannot be undone.`);
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

function renderSceneFileBrowser() {
  if (!el.sceneFileList) return;
  const activeScene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!sceneBrowserSelectedFile || !sceneCatalogHasFile(sceneBrowserSelectedFile)) {
    sceneBrowserSelectedFile = activeScene;
  }

  el.sceneFileList.replaceChildren();
  updateSceneFileCount();

  if (!sceneCatalog.length) {
    const empty = document.createElement("p");
    empty.className = "scene-file-empty";
    empty.textContent = "No scene files found.";
    el.sceneFileList.appendChild(empty);
    return;
  }

  sceneCatalog.forEach((item) => {
    const sceneFile = String(item && item.sceneFile ? item.sceneFile : "").trim();
    if (!sceneFile) return;
    const title = String(item && item.title ? item.title : "").trim();
    const dependsExternal = !!(item && item.dependsExternal);
    const hasVariants = !!(item && item.hasVariants);
    const isSelected = sceneBrowserSelectedFile === sceneFile;
    const isActive = activeScene === sceneFile;

    const button = document.createElement("button");
    button.type = "button";
    button.className = "scene-file-item";
    button.setAttribute("role", "option");
    button.setAttribute("aria-selected", isSelected ? "true" : "false");
    button.dataset.scene = sceneFile;
    if (isSelected) button.classList.add("is-selected");
    if (isActive) button.classList.add("is-active");

    button.appendChild(createSceneFileIcon());

    const body = document.createElement("span");
    body.className = "scene-file-meta";

    const nameNode = document.createElement("span");
    nameNode.className = "scene-file-name";
    nameNode.textContent = sceneFile;
    body.appendChild(nameNode);

    if (title && title !== sceneFile) {
      const titleNode = document.createElement("span");
      titleNode.className = "scene-file-title";
      titleNode.textContent = title;
      body.appendChild(titleNode);
    }

    if (dependsExternal || hasVariants) {
      const badgesNode = document.createElement("span");
      badgesNode.className = "scene-file-badges";

      if (dependsExternal) {
        const extNode = document.createElement("span");
        extNode.className = "scene-file-ext";
        extNode.textContent = "EXT";
        badgesNode.appendChild(extNode);
      }

      if (hasVariants) {
        const variantNode = document.createElement("span");
        variantNode.className = "scene-file-ext scene-file-var";
        variantNode.textContent = "VAR";
        badgesNode.appendChild(variantNode);
      }

      body.appendChild(badgesNode);
    }

    button.appendChild(body);
    button.addEventListener("click", () => {
      sceneBrowserSelectedFile = sceneFile;
      renderSceneFileBrowser();
    });
    button.addEventListener("dblclick", () => {
      activateSceneFile(sceneFile);
    });
    button.addEventListener("contextmenu", (evt) => {
      evt.preventDefault();
      sceneBrowserSelectedFile = sceneFile;
      renderSceneFileBrowser();
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
}

function setSceneBrowserSelectedFile(sceneFile) {
  const name = String(sceneFile || "").trim();
  if (name && sceneCatalogHasFile(name)) sceneBrowserSelectedFile = name;
  else sceneBrowserSelectedFile = "";
  renderSceneFileBrowser();
}

function activateSceneFile(sceneFile) {
  const name = String(sceneFile || "").trim();
  if (!name || !sceneCatalogHasFile(name) || !el.scene) return;
  sceneBrowserSelectedFile = name;
  if (String(el.scene.value || "").trim() === name) {
    updateSceneDependencyPill(name);
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
  const sceneMeta = sceneCatalogEntryByFile(sceneName);
  const sceneLabel = String(sceneMeta && sceneMeta.label ? sceneMeta.label : "").trim()
    || String(sceneMeta && sceneMeta.sceneFile ? sceneMeta.sceneFile : "").trim()
    || (sceneName || "-");
  const sceneDescription = String(sceneMeta && sceneMeta.description ? sceneMeta.description : "").trim() || "-";

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

  if (el.activeSceneCardCamera) {
    const cameraMeta = cameraCatalogEntryByName(cameraName);
    const cameraType = String(cameraMeta && cameraMeta.type ? cameraMeta.type : "").trim();
    const cameraLabel = cameraName || "-";
    el.activeSceneCardCamera.replaceChildren();

    const key = document.createElement("span");
    key.className = "active-scene-row-label";
    key.textContent = "Camera";
    el.activeSceneCardCamera.appendChild(key);

    const valueWrap = document.createElement("span");
    valueWrap.className = "active-scene-camera-field";
    const valueNode = document.createElement("code");
    valueNode.className = "active-scene-row-value active-scene-camera-name";
    valueNode.textContent = cameraLabel;
    valueWrap.appendChild(valueNode);

    if (cameraType && cameraLabel !== "-") {
      const icon = createCameraIcon(cameraType);
      icon.classList.add("active-scene-camera-icon");
      valueWrap.appendChild(icon);
    }

    el.activeSceneCardCamera.appendChild(valueWrap);
  }
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
  if (!Array.isArray(cameraCatalog) || cameraCatalog.length === 0) return;

  const graphCameras = Array.isArray(graphData && graphData.cameras) ? graphData.cameras : [];
  if (!graphCameras.length) return;
  const typeById = new Map();
  graphCameras.forEach((cam) => {
    const id = String(cam && cam.id ? cam.id : "").trim();
    const type = String(cam && cam.type ? cam.type : "").trim();
    if (id) typeById.set(id, type);
  });

  let changed = false;
  cameraCatalog = cameraCatalog.map((item) => {
    const value = String(item && item.value ? item.value : "").trim();
    const prevType = String(item && item.type ? item.type : "").trim();
    if (!value || !cameraTypeLooksGeneric(prevType)) return item;
    const nextType = String(typeById.get(value) || "").trim();
    if (!nextType || cameraTypeLooksGeneric(nextType)) return item;
    changed = true;
    return { ...item, type: nextType };
  });

  if (changed) renderCameraBrowser();
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

function renderCameraBrowser() {
  if (!el.cameraFileList) return;
  if (!cameraBrowserSelectedName || !cameraCatalogHasName(cameraBrowserSelectedName)) {
    cameraBrowserSelectedName = String(el.camera && el.camera.value ? el.camera.value : "");
  }

  el.cameraFileList.replaceChildren();
  updateCameraFileCount();
  updateCameraActivePanel();

  if (!cameraCatalog.length) {
    const empty = document.createElement("p");
    empty.className = "scene-file-empty";
    empty.textContent = "No cameras found in this scene.";
    el.cameraFileList.appendChild(empty);
    return;
  }

  cameraCatalog.forEach((item) => {
    const value = String(item && item.value ? item.value : "");
    const label = String(item && item.label ? item.label : "");
    const camType = String(item && item.type ? item.type : "").trim();
    const isSelected = cameraBrowserSelectedName === value;
    const isActive = String(el.camera && el.camera.value ? el.camera.value : "") === value;

    const button = document.createElement("button");
    button.type = "button";
    button.className = "scene-file-item camera-file-item";
    button.setAttribute("role", "option");
    button.setAttribute("aria-selected", isSelected ? "true" : "false");
    button.dataset.camera = value;
    if (isSelected) button.classList.add("is-selected");
    if (isActive) button.classList.add("is-active");

    button.appendChild(createCameraIcon(camType));

    const body = document.createElement("span");
    body.className = "scene-file-meta";

    const nameNode = document.createElement("span");
    nameNode.className = "scene-file-name";
    nameNode.textContent = label || value || "-";
    body.appendChild(nameNode);

    const descNode = document.createElement("span");
    descNode.className = "scene-file-title camera-file-description";
    descNode.textContent = `${cameraTypeLabel(camType)} camera`;
    body.appendChild(descNode);

    button.appendChild(body);
    button.addEventListener("click", () => {
      cameraBrowserSelectedName = value;
      renderCameraBrowser();
    });
    button.addEventListener("dblclick", () => {
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
}

function renderVariantBrowser() {
  if (!el.variantFileList) return;
  if (!variantBrowserSelectedName || !variantCatalogHasName(variantBrowserSelectedName)) {
    variantBrowserSelectedName = selectedSceneVariantValue();
  }

  el.variantFileList.replaceChildren();
  updateVariantFileCount();
  updateVariantActivePanel();

  if (!variantCatalog.length) {
    const empty = document.createElement("p");
    empty.className = "scene-file-empty";
    empty.textContent = "No variants found in this scene.";
    el.variantFileList.appendChild(empty);
    return;
  }

  variantCatalog.forEach((item) => {
    const value = normalizeVariantName(item && item.value ? item.value : "");
    const label = String(item && item.label ? item.label : "").trim() || "(base)";
    const description = String(item && item.description ? item.description : "").trim();
    const isBase = !value;
    const isSelected = normalizeVariantName(variantBrowserSelectedName) === value;
    const isActive = selectedSceneVariantValue() === value;

    const button = document.createElement("button");
    button.type = "button";
    button.className = "scene-file-item camera-file-item variant-file-item";
    button.setAttribute("role", "option");
    button.setAttribute("aria-selected", isSelected ? "true" : "false");
    button.dataset.variant = value;
    if (isSelected) button.classList.add("is-selected");
    if (isActive) button.classList.add("is-active");
    if (isBase) button.classList.add("is-base");
    if (description) button.classList.add("has-description");
    else button.classList.add("no-description");

    button.appendChild(createVariantIcon(isBase));

    const body = document.createElement("span");
    body.className = "scene-file-meta";

    const nameNode = document.createElement("span");
    nameNode.className = "scene-file-name";
    nameNode.textContent = label;
    body.appendChild(nameNode);

    if (description) {
      const descNode = document.createElement("span");
      descNode.className = "scene-file-title variant-file-description";
      descNode.textContent = description;
      body.appendChild(descNode);
    }

    button.appendChild(body);
    button.addEventListener("click", () => {
      variantBrowserSelectedName = value;
      renderVariantBrowser();
    });
    button.addEventListener("dblclick", () => {
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
  sceneCatalog = sceneItems.slice();
  sceneDependencyByFile = new Map(sceneItems.map((item) => [item.sceneFile, !!item.dependsExternal]));
  sceneItems.forEach((item) => addOption(el.scene, item.sceneFile, item.sceneFile));
  const preferred = prev || saved;
  if (preferred) el.scene.value = preferred;
  if (!el.scene.value && el.scene.options.length > 0) el.scene.selectedIndex = 0;
  if (!sceneCatalogHasFile(sceneBrowserSelectedFile)) {
    sceneBrowserSelectedFile = String(el.scene.value || "").trim();
  }
  updateSceneDependencyPill(el.scene.value);
  if (el.scene.value) localStorage.setItem(LAST_SCENE_KEY, el.scene.value);
  else localStorage.removeItem(LAST_SCENE_KEY);
}


async function loadCameras(scene, variant) {
  el.camera.innerHTML = "";
  const variantName = normalizeVariantName(variant !== undefined ? variant : selectedSceneVariantValue());
  const info = await api.getCameras(scene, variantName);
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

  const cachedGraph = getRuntimeGraphForScene(sceneName, variantName);
  if (cachedGraph) applyCameraTypesFromRuntimeGraph(sceneName, cachedGraph, variantName);

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
  renderCameraBrowser();

  syncVisualCameraFromRenderSelection();
}

async function loadVariants(scene, preferredVariant) {
  if (!el.variant) return;
  el.variant.innerHTML = "";
  const sceneName = String(scene || "").trim();
  variantCatalog = [{ value: "", label: "(base)", description: "" }];
  addOption(el.variant, "", "(base)");

  if (sceneName) {
    try {
      const data = await api.getSceneSource(sceneName);
      const parsed = extractSceneVariantNames(data && data.source ? data.source : "");
      if (parsed && parsed.base) {
        variantCatalog[0].label = String(parsed.base.label || "(base)");
        variantCatalog[0].description = String(parsed.base.description || "");
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
      const baseLabel = variantCatalog[0].label || "(base)";
      if (el.variant.options.length > 0) el.variant.options[0].text = baseLabel;
    } catch (_) {
      // keep base-only when source is unavailable
    }
  }

  const requested = preferredVariant !== undefined
    ? normalizeVariantName(preferredVariant)
    : selectedSceneVariantValue();
  el.variant.value = variantCatalogHasName(requested) ? requested : "";
  if (!variantCatalogHasName(variantBrowserSelectedName)) {
    variantBrowserSelectedName = selectedSceneVariantValue();
  }
  renderVariantBrowser();
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

async function loadSceneRuntimeGraph(scene, variant) {
  const sceneName = String(scene || "").trim();
  if (!sceneName || !hasBackendMethod(api, "getSceneRuntimeGraph")) return null;
  const variantName = normalizeVariantName(variant !== undefined ? variant : selectedSceneVariantValue());
  const data = await api.getSceneRuntimeGraph(sceneName, variantName);
  const key = runtimeGraphCacheKey(sceneName, variantName);
  runtimeGraphByScene.set(key, data || { cameras: [], objects: [], surfaces: [], materials: [] });
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
