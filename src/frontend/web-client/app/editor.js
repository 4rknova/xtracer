function postFilterCatalogById(id) {
  const key = String(id || "").trim().toLowerCase();
  return POST_FILTER_CATALOG.find((it) => it.id === key) || null;
}

function normalizePostFilterStage(stage) {
  return String(stage || "").toLowerCase() === "before" ? "before" : "after";
}

function normalizePostFilterId(id) {
  const item = postFilterCatalogById(id);
  return item ? item.id : "";
}

function renderPostFilterChain() {
  if (!el.postFiltersChain) return;
  el.postFiltersChain.innerHTML = "";
  if (!Array.isArray(postFilterChain) || postFilterChain.length === 0) {
    const empty = document.createElement("p");
    empty.className = "post-filters-empty";
    empty.textContent = "No filters in chain.";
    el.postFiltersChain.appendChild(empty);
    return;
  }

  postFilterChain.forEach((entry, index) => {
    const filterId = normalizePostFilterId(entry && entry.filter);
    const filterInfo = postFilterCatalogById(filterId);
    if (!filterInfo) return;

    const row = document.createElement("div");
    row.className = "post-filter-entry";
    row.dataset.index = String(index);

    const name = document.createElement("span");
    name.className = "post-filter-name";
    name.textContent = filterInfo.label;

    const stage = document.createElement("select");
    stage.className = "post-filter-stage";
    stage.setAttribute("aria-label", `Filter stage for ${filterInfo.label}`);
    const beforeOpt = document.createElement("option");
    beforeOpt.value = "before";
    beforeOpt.textContent = "Before TM";
    const afterOpt = document.createElement("option");
    afterOpt.value = "after";
    afterOpt.textContent = "After TM";
    stage.appendChild(beforeOpt);
    stage.appendChild(afterOpt);
    stage.value = normalizePostFilterStage(entry && entry.stage);
    stage.addEventListener("change", () => {
      const idx = Number(row.dataset.index);
      if (!Number.isFinite(idx) || idx < 0 || idx >= postFilterChain.length) return;
      postFilterChain[idx].stage = normalizePostFilterStage(stage.value);
      appendLog(`post_filter stage idx=${idx} stage=${postFilterChain[idx].stage}`);
      queueWorkspaceSettingsSave();
    });

    const removeBtn = document.createElement("button");
    removeBtn.type = "button";
    removeBtn.className = "post-filter-remove";
    removeBtn.textContent = "Remove";
    removeBtn.addEventListener("click", () => {
      const idx = Number(row.dataset.index);
      if (!Number.isFinite(idx) || idx < 0 || idx >= postFilterChain.length) return;
      postFilterChain.splice(idx, 1);
      renderPostFilterChain();
      appendLog(`post_filter removed idx=${idx}`);
      queueWorkspaceSettingsSave();
    });

    row.appendChild(name);
    row.appendChild(stage);
    row.appendChild(removeBtn);
    el.postFiltersChain.appendChild(row);
  });
}

function addPostFilterToChain() {
  const filterId = normalizePostFilterId(el.postFilterType && el.postFilterType.value);
  if (!filterId) return;
  postFilterChain.push({ filter: filterId, stage: "after" });
  renderPostFilterChain();
  appendLog(`post_filter add filter=${filterId} stage=after`);
  queueWorkspaceSettingsSave();
}

function populatePostFilterTypeOptions() {
  if (!el.postFilterType) return;
  const prev = String(el.postFilterType.value || "").trim();
  el.postFilterType.innerHTML = "";
  POST_FILTER_CATALOG.forEach((item) => {
    addOption(el.postFilterType, item.id, item.label);
  });
  if (prev && normalizePostFilterId(prev)) {
    el.postFilterType.value = normalizePostFilterId(prev);
  } else if (POST_FILTER_CATALOG.length > 0) {
    el.postFilterType.value = POST_FILTER_CATALOG[0].id;
  }
}

function gatherPostFilterParams() {
  const parts = [];
  (postFilterChain || []).forEach((entry) => {
    const filterId = normalizePostFilterId(entry && entry.filter);
    if (!filterId) return;
    const stage = normalizePostFilterStage(entry && entry.stage);
    parts.push(`${stage}:${filterId}`);
  });
  return parts.join(",");
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

function handleEditorTabKey(event) {
  if (event && (event.ctrlKey || event.metaKey) && String(event.key || "").toLowerCase() === "s") {
    event.preventDefault();
    triggerSceneSave();
    return;
  }

  if (!event || event.key !== "Tab" || !el.sceneSource) return;
  event.preventDefault();

  const ta = el.sceneSource;
  const value = ta.value || "";
  const start = ta.selectionStart || 0;
  const end = ta.selectionEnd || 0;

  if (start === end && !event.shiftKey) {
    ta.value = `${value.slice(0, start)}\t${value.slice(end)}`;
    ta.selectionStart = start + 1;
    ta.selectionEnd = start + 1;
    updateEditorMetrics();
    syncEditorScroll();
    return;
  }

  const lineStart = value.lastIndexOf("\n", Math.max(0, start - 1)) + 1;
  let lineEnd = value.indexOf("\n", end);
  if (lineEnd < 0) lineEnd = value.length;

  const lines = value.slice(lineStart, lineEnd).split("\n");
  if (event.shiftKey) {
    let removedTotal = 0;
    let removedOnFirst = 0;
    const outdented = lines.map((line, idx) => {
      if (line.startsWith("\t")) {
        if (idx === 0) removedOnFirst = 1;
        removedTotal += 1;
        return line.slice(1);
      }
      return line;
    });
    ta.value = `${value.slice(0, lineStart)}${outdented.join("\n")}${value.slice(lineEnd)}`;
    ta.selectionStart = Math.max(lineStart, start - removedOnFirst);
    ta.selectionEnd = Math.max(ta.selectionStart, end - removedTotal);
  } else {
    const indented = lines.map((line) => `\t${line}`);
    ta.value = `${value.slice(0, lineStart)}${indented.join("\n")}${value.slice(lineEnd)}`;
    ta.selectionStart = start + 1;
    ta.selectionEnd = end + lines.length;
  }

  updateEditorMetrics();
  syncEditorScroll();
}



function historyLimitFor(type) {
  return type === "text"
    ? Math.max(10, Number(uiOptions.textHistoryLimit) || 200)
    : Math.max(10, Number(uiOptions.visualHistoryLimit) || 200);
}

function trimHistoryStore(type) {
  const store = historyStores[type];
  if (!store) return;
  const limit = historyLimitFor(type);
  if (store.states.length <= limit) return;
  const drop = store.states.length - limit;
  store.states.splice(0, drop);
  store.index = Math.max(0, store.index - drop);
}

function applyHistoryLimits() {
  trimHistoryStore("text");
  trimHistoryStore("visual");
}

function commitHistoryState(type, source, selStart, selEnd) {
  const store = historyStores[type];
  if (!store) return;
  const snapshot = {
    source: String(source || ""),
    selStart: Number.isFinite(selStart) ? selStart : 0,
    selEnd: Number.isFinite(selEnd) ? selEnd : 0,
  };
  const current = (store.index >= 0 && store.index < store.states.length)
    ? store.states[store.index]
    : null;
  if (current && current.source === snapshot.source) {
    current.selStart = snapshot.selStart;
    current.selEnd = snapshot.selEnd;
    return;
  }
  if (store.index < store.states.length - 1) {
    store.states.splice(store.index + 1);
  }
  store.states.push(snapshot);
  store.index = store.states.length - 1;
  trimHistoryStore(type);
}

function resetSceneHistoriesFromCurrentSource() {
  if (textHistoryCommitTimer) {
    clearTimeout(textHistoryCommitTimer);
    textHistoryCommitTimer = null;
  }
  const source = String((el.sceneSource && el.sceneSource.value) || "");
  const selStart = el.sceneSource ? (el.sceneSource.selectionStart || 0) : 0;
  const selEnd = el.sceneSource ? (el.sceneSource.selectionEnd || selStart) : selStart;
  historyStores.text.states = [];
  historyStores.text.index = -1;
  historyStores.visual.states = [];
  historyStores.visual.index = -1;
  commitHistoryState("text", source, selStart, selEnd);
  commitHistoryState("visual", source, selStart, selEnd);
}

function scheduleTextHistoryCommit() {
  if (suppressHistoryTracking) return;
  if (textHistoryCommitTimer) clearTimeout(textHistoryCommitTimer);
  textHistoryCommitTimer = setTimeout(() => {
    textHistoryCommitTimer = null;
    if (!el.sceneSource) return;
    commitHistoryState("text", el.sceneSource.value || "", el.sceneSource.selectionStart, el.sceneSource.selectionEnd);
  }, 180);
}

function flushTextHistoryCommit() {
  if (!textHistoryCommitTimer) return;
  clearTimeout(textHistoryCommitTimer);
  textHistoryCommitTimer = null;
  if (!el.sceneSource) return;
  commitHistoryState("text", el.sceneSource.value || "", el.sceneSource.selectionStart, el.sceneSource.selectionEnd);
}

function applyHistoryStep(type, direction) {
  const store = historyStores[type];
  if (!store || store.states.length === 0) return false;
  if (type === "text") flushTextHistoryCommit();

  const nextIndex = store.index + direction;
  if (nextIndex < 0 || nextIndex >= store.states.length) return false;
  store.index = nextIndex;
  const snap = store.states[store.index];

  suppressHistoryTracking = true;
  try {
    el.sceneSource.value = String(snap.source || "");
    updateEditorMetrics();
    refreshSceneEditControls();
    syncEditorScroll();
    renderSceneGraphView();
    if (type === "text" && el.sceneSource && editorViewMode === "text") {
      const max = el.sceneSource.value.length;
      const a = Math.max(0, Math.min(max, Number(snap.selStart) || 0));
      const b = Math.max(0, Math.min(max, Number(snap.selEnd) || a));
      el.sceneSource.focus();
      el.sceneSource.setSelectionRange(a, b);
    }
  } finally {
    suppressHistoryTracking = false;
  }

  queueWorkspaceDraftSave();
  if (type === "visual" && visualEditor) {
    rebuildVisualFromEditorSource()
      .then(() => {
        const selected = String(el.editObjectSelect && el.editObjectSelect.value ? el.editObjectSelect.value : "").trim();
        if (selected && visualEditor.selectObjectById) visualEditor.selectObjectById(selected, false);
      })
      .catch((err) => appendLog(`visual refresh error: ${err.message}`));
  }
  return true;
}

function handleUndoRedoShortcut(event) {
  const key = String(event.key || "").toLowerCase();
  if (key !== "z") return;
  if (!(event.ctrlKey || event.metaKey) || event.altKey) return;
  if (activeTabMode !== "visual") return;

  const active = document.activeElement;
  if (active && active !== el.sceneSource) {
    const tag = String(active.tagName || "").toLowerCase();
    if (tag === "input" || tag === "textarea" || tag === "select" || active.isContentEditable) return;
  }

  const type = (active === el.sceneSource || editorViewMode === "text") ? "text" : "visual";
  const ok = event.shiftKey ? applyHistoryStep(type, +1) : applyHistoryStep(type, -1);
  if (ok) event.preventDefault();
}

function updateSceneSourceText(nextSource, options) {
  const opts = options && typeof options === "object" ? options : {};
  const historyType = opts.history || "visual";
  const prev = String(el.sceneSource.value || "");
  const next = String(nextSource || "");
  if (prev === next) return;

  el.sceneSource.value = next;
  updateEditorMetrics();
  syncEditorScroll();
  refreshSceneEditControls();
  renderSceneGraphView();
  if (!suppressHistoryTracking) {
    if (historyType === "visual") {
      commitHistoryState("visual", next, el.sceneSource.selectionStart, el.sceneSource.selectionEnd);
    } else if (historyType === "text") {
      commitHistoryState("text", next, el.sceneSource.selectionStart, el.sceneSource.selectionEnd);
    }
  }
  queueWorkspaceDraftSave();
}

function refreshSceneEditControls() {
  if (!el.editObjectSelect || !el.createMaterialSelect) return;
  const source = el.sceneSource ? (el.sceneSource.value || "") : "";
  const model = parseSceneEditModel(source);
  const prevObject = String(el.editObjectSelect.value || "");
  const prevMaterial = String(el.createMaterialSelect.value || "");

  el.editObjectSelect.innerHTML = "";
  addOption(el.editObjectSelect, "", "Select object...");
  Array.from(model.objects.values())
    .sort((a, b) => a.id.localeCompare(b.id))
    .forEach((obj) => {
      const g = model.geometries.get(obj.geometry);
      const type = g ? (g.type || "?") : "?";
      addOption(el.editObjectSelect, obj.id, `${obj.id} (${type})`);
    });
  if (prevObject && model.objects.has(prevObject)) el.editObjectSelect.value = prevObject;
  else el.editObjectSelect.value = "";

  el.createMaterialSelect.innerHTML = "";
  const materialIds = Array.from((model.materials || new Map()).keys());
  if (!materialIds.length) {
    addOption(el.createMaterialSelect, "", "No material");
  } else {
    materialIds.forEach((id) => addOption(el.createMaterialSelect, id, id));
    if (prevMaterial && materialIds.includes(prevMaterial)) el.createMaterialSelect.value = prevMaterial;
    else el.createMaterialSelect.value = materialIds[0];
  }
  syncTransformInputsFromObject(el.editObjectSelect.value || "");
}

function setTransformInputs(values) {
  const t = values && values.translation ? values.translation : [0, 0, 0];
  const r = values && values.rotation ? values.rotation : [0, 0, 0];
  const s = values && values.scale ? values.scale : [1, 1, 1];
  if (el.editTranslateX) el.editTranslateX.value = formatSceneNumber(t[0], 0);
  if (el.editTranslateY) el.editTranslateY.value = formatSceneNumber(t[1], 0);
  if (el.editTranslateZ) el.editTranslateZ.value = formatSceneNumber(t[2], 0);
  if (el.editRotateX) el.editRotateX.value = formatSceneNumber(r[0], 0);
  if (el.editRotateY) el.editRotateY.value = formatSceneNumber(r[1], 0);
  if (el.editRotateZ) el.editRotateZ.value = formatSceneNumber(r[2], 0);
  if (el.editScaleX) el.editScaleX.value = formatSceneNumber(s[0], 1);
  if (el.editScaleY) el.editScaleY.value = formatSceneNumber(s[1], 1);
  if (el.editScaleZ) el.editScaleZ.value = formatSceneNumber(s[2], 1);
}

function currentTransformInputs() {
  return {
    translation: [
      readSceneNumber(el.editTranslateX ? el.editTranslateX.value : 0, 0),
      readSceneNumber(el.editTranslateY ? el.editTranslateY.value : 0, 0),
      readSceneNumber(el.editTranslateZ ? el.editTranslateZ.value : 0, 0),
    ],
    rotation: [
      readSceneNumber(el.editRotateX ? el.editRotateX.value : 0, 0),
      readSceneNumber(el.editRotateY ? el.editRotateY.value : 0, 0),
      readSceneNumber(el.editRotateZ ? el.editRotateZ.value : 0, 0),
    ],
    scale: [
      Math.max(0.0001, readSceneNumber(el.editScaleX ? el.editScaleX.value : 1, 1)),
      Math.max(0.0001, readSceneNumber(el.editScaleY ? el.editScaleY.value : 1, 1)),
      Math.max(0.0001, readSceneNumber(el.editScaleZ ? el.editScaleZ.value : 1, 1)),
    ],
  };
}

function syncTransformInputsFromObject(objectId) {
  const info = objectId ? getObjectTransformFromSource(el.sceneSource.value || "", objectId) : null;
  if (el.editGeometryType) el.editGeometryType.value = info ? (info.geometryType || "") : "";
  setTransformInputs(info || null);
}

async function rebuildVisualFromEditorSource() {
  if (!visualEditor) return;
  const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const runtimeData = (sceneName && hasBackendMethod(api, "getSceneRuntimeGraph"))
    ? await loadSceneRuntimeGraph(sceneName).catch(() => null)
    : null;
  let geometryData = { meshes: {} };
  if (sceneName && hasBackendMethod(api, "getSceneGeometry")) {
    try {
      geometryData = await api.getSceneGeometry(sceneName);
    } catch (_) {
      geometryData = { meshes: {} };
    }
  }
  await visualEditor.buildScene(sceneName, "", geometryData, runtimeData);
  refreshVisualCameraOptions();
  syncVisualCameraFromRenderSelection();
}
