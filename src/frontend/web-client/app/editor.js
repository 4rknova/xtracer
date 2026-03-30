function postFilterCatalogById(id) {
  const key = String(id || "").trim().toLowerCase();
  return POST_FILTER_CATALOG.find((it) => it.id === key) || null;
}

function isPostFilterStackEnabled() {
  return !!postFilterStackEnabled;
}

function normalizePostFilterStage(stage) {
  return String(stage || "").toLowerCase() === "before" ? "before" : "after";
}

function normalizePostFilterId(id) {
  const item = postFilterCatalogById(id);
  return item ? item.id : "";
}

function defaultPostFilterParams(filterId) {
  if (filterId === "chromatic_aberration") {
    return {
      amount: "1.5",
      center_x: "0.5",
      center_y: "0.5",
      falloff: "1.0",
    };
  }
  if (filterId === "vignette") {
    return {
      strength: "0.35",
      radius: "0.5",
      softness: "0.35",
      center_x: "0.5",
      center_y: "0.5",
    };
  }
  if (filterId === "film_grain") {
    return {
      amount: "0.06",
      size: "1.0",
      seed: "1",
      luma_weighted: "1",
    };
  }
  if (filterId === "sharpen") {
    return {
      amount: "0.8",
      radius: "1.0",
      threshold: "0.02",
    };
  }
  if (filterId === "brightness") {
    return {
      amount: "0.0",
    };
  }
  if (filterId === "contrast") {
    return {
      amount: "1.0",
      pivot: "0.5",
    };
  }
  if (filterId === "raindrops_lens") {
    return {
      density: "0.35",
      size: "0.45",
      distortion: "12.0",
      seed: "1",
    };
  }
  return {};
}

function normalizeChromaticAberrationParams(raw) {
  const src = raw && typeof raw === "object" ? raw : {};
  const amountNum = Number(src.amount);
  const centerXNum = Number(src.center_x);
  const centerYNum = Number(src.center_y);
  const falloffNum = Number(src.falloff);
  const amount = Number.isFinite(amountNum) ? Math.max(0, Math.min(64, amountNum)) : 1.5;
  const centerX = Number.isFinite(centerXNum) ? Math.max(0, Math.min(1, centerXNum)) : 0.5;
  const centerY = Number.isFinite(centerYNum) ? Math.max(0, Math.min(1, centerYNum)) : 0.5;
  const falloff = Number.isFinite(falloffNum) ? Math.max(0, Math.min(8, falloffNum)) : 1.0;
  return {
    amount: amount.toFixed(3),
    center_x: centerX.toFixed(3),
    center_y: centerY.toFixed(3),
    falloff: falloff.toFixed(3),
  };
}

function normalizeVignetteParams(raw) {
  const src = raw && typeof raw === "object" ? raw : {};
  const strengthNum = Number(src.strength);
  const radiusNum = Number(src.radius);
  const softnessNum = Number(src.softness);
  const centerXNum = Number(src.center_x);
  const centerYNum = Number(src.center_y);
  const strength = Number.isFinite(strengthNum) ? Math.max(0, Math.min(1, strengthNum)) : 0.35;
  const radius = Number.isFinite(radiusNum) ? Math.max(0, Math.min(1, radiusNum)) : 0.5;
  const softness = Number.isFinite(softnessNum) ? Math.max(0.001, Math.min(1, softnessNum)) : 0.35;
  const centerX = Number.isFinite(centerXNum) ? Math.max(0, Math.min(1, centerXNum)) : 0.5;
  const centerY = Number.isFinite(centerYNum) ? Math.max(0, Math.min(1, centerYNum)) : 0.5;
  return {
    strength: strength.toFixed(3),
    radius: radius.toFixed(3),
    softness: softness.toFixed(3),
    center_x: centerX.toFixed(3),
    center_y: centerY.toFixed(3),
  };
}

function normalizeFilmGrainParams(raw) {
  const src = raw && typeof raw === "object" ? raw : {};
  const amountNum = Number(src.amount);
  const sizeNum = Number(src.size);
  const seedNum = Number(src.seed);
  const lumaRaw = String(src.luma_weighted === undefined ? "1" : src.luma_weighted).toLowerCase();
  const amount = Number.isFinite(amountNum) ? Math.max(0, Math.min(1, amountNum)) : 0.06;
  const size = Number.isFinite(sizeNum) ? Math.max(1, Math.min(16, sizeNum)) : 1.0;
  const seed = Number.isFinite(seedNum) ? Math.max(0, Math.min(1000000, Math.floor(seedNum))) : 1;
  const lumaWeighted = (lumaRaw === "0" || lumaRaw === "false" || lumaRaw === "off") ? "0" : "1";
  return {
    amount: amount.toFixed(3),
    size: size.toFixed(3),
    seed: String(seed),
    luma_weighted: lumaWeighted,
  };
}

function normalizeSharpenParams(raw) {
  const src = raw && typeof raw === "object" ? raw : {};
  const amountNum = Number(src.amount);
  const radiusNum = Number(src.radius);
  const thresholdNum = Number(src.threshold);
  const amount = Number.isFinite(amountNum) ? Math.max(0, Math.min(4, amountNum)) : 0.8;
  const radius = Number.isFinite(radiusNum) ? Math.max(1, Math.min(4, radiusNum)) : 1.0;
  const threshold = Number.isFinite(thresholdNum) ? Math.max(0, Math.min(1, thresholdNum)) : 0.02;
  return {
    amount: amount.toFixed(3),
    radius: radius.toFixed(3),
    threshold: threshold.toFixed(3),
  };
}

function normalizeBrightnessParams(raw) {
  const src = raw && typeof raw === "object" ? raw : {};
  const amountNum = Number(src.amount);
  const amount = Number.isFinite(amountNum) ? Math.max(-4, Math.min(4, amountNum)) : 0.0;
  return {
    amount: amount.toFixed(3),
  };
}

function normalizeContrastParams(raw) {
  const src = raw && typeof raw === "object" ? raw : {};
  const amountNum = Number(src.amount);
  const pivotNum = Number(src.pivot);
  const amount = Number.isFinite(amountNum) ? Math.max(0, Math.min(4, amountNum)) : 1.0;
  const pivot = Number.isFinite(pivotNum) ? Math.max(0, Math.min(4, pivotNum)) : 0.5;
  return {
    amount: amount.toFixed(3),
    pivot: pivot.toFixed(3),
  };
}

function normalizeRaindropsLensParams(raw) {
  const src = raw && typeof raw === "object" ? raw : {};
  const densityNum = Number(src.density);
  const sizeNum = Number(src.size);
  const distortionNum = Number(src.distortion);
  const seedNum = Number(src.seed);
  const density = Number.isFinite(densityNum) ? Math.max(0, Math.min(1, densityNum)) : 0.35;
  const size = Number.isFinite(sizeNum) ? Math.max(0, Math.min(1, sizeNum)) : 0.45;
  const distortion = Number.isFinite(distortionNum) ? Math.max(0, Math.min(64, distortionNum)) : 12.0;
  const seed = Number.isFinite(seedNum) ? Math.max(0, Math.min(1000000, Math.floor(seedNum))) : 1;
  return {
    density: density.toFixed(3),
    size: size.toFixed(3),
    distortion: distortion.toFixed(3),
    seed: String(seed),
  };
}

function normalizePostFilterParams(filterId, raw) {
  if (filterId === "chromatic_aberration") return normalizeChromaticAberrationParams(raw);
  if (filterId === "vignette") return normalizeVignetteParams(raw);
  if (filterId === "film_grain") return normalizeFilmGrainParams(raw);
  if (filterId === "sharpen") return normalizeSharpenParams(raw);
  if (filterId === "brightness") return normalizeBrightnessParams(raw);
  if (filterId === "contrast") return normalizeContrastParams(raw);
  if (filterId === "raindrops_lens") return normalizeRaindropsLensParams(raw);
  return {};
}

function normalizePostFilterEntry(entry) {
  const filterId = normalizePostFilterId(entry && entry.filter);
  if (!filterId) return null;
  return {
    filter: filterId,
    stage: normalizePostFilterStage(entry && entry.stage),
    params: normalizePostFilterParams(filterId, entry && entry.params),
  };
}

function postFilterParamSpecs(filterId) {
  if (filterId === "chromatic_aberration") {
    return [
      { key: "amount", label: "Amount", min: "0", max: "64", step: "0.1" },
      { key: "center_x", label: "Center X", min: "0", max: "1", step: "0.01" },
      { key: "center_y", label: "Center Y", min: "0", max: "1", step: "0.01" },
      { key: "falloff", label: "Falloff", min: "0", max: "8", step: "0.1" },
    ];
  }
  if (filterId === "vignette") {
    return [
      { key: "strength", label: "Strength", min: "0", max: "1", step: "0.01" },
      { key: "radius", label: "Radius", min: "0", max: "1", step: "0.01" },
      { key: "softness", label: "Softness", min: "0.001", max: "1", step: "0.01" },
      { key: "center_x", label: "Center X", min: "0", max: "1", step: "0.01" },
      { key: "center_y", label: "Center Y", min: "0", max: "1", step: "0.01" },
    ];
  }
  if (filterId === "film_grain") {
    return [
      { key: "amount", label: "Amount", min: "0", max: "1", step: "0.01" },
      { key: "size", label: "Size", min: "1", max: "16", step: "0.5" },
      { key: "seed", label: "Seed", min: "0", max: "1000000", step: "1" },
      { key: "luma_weighted", label: "Luma Weighted (1/0)", min: "0", max: "1", step: "1" },
    ];
  }
  if (filterId === "sharpen") {
    return [
      { key: "amount", label: "Amount", min: "0", max: "4", step: "0.1" },
      { key: "radius", label: "Radius", min: "1", max: "4", step: "1" },
      { key: "threshold", label: "Threshold", min: "0", max: "1", step: "0.01" },
    ];
  }
  if (filterId === "brightness") {
    return [
      { key: "amount", label: "Amount", min: "-4", max: "4", step: "0.05" },
    ];
  }
  if (filterId === "contrast") {
    return [
      { key: "amount", label: "Amount", min: "0", max: "4", step: "0.05" },
      { key: "pivot", label: "Pivot", min: "0", max: "4", step: "0.05" },
    ];
  }
  if (filterId === "raindrops_lens") {
    return [
      { key: "density", label: "Density", min: "0", max: "1", step: "0.01" },
      { key: "size", label: "Size", min: "0", max: "1", step: "0.01" },
      { key: "distortion", label: "Distortion", min: "0", max: "64", step: "0.1" },
      { key: "seed", label: "Seed", min: "0", max: "1000000", step: "1" },
    ];
  }
  return [];
}

function currentToneMappingLabel() {
  if (!el.toneMapping) return "ACES (Fitted)";
  const selected = el.toneMapping.options && el.toneMapping.selectedIndex >= 0
    ? el.toneMapping.options[el.toneMapping.selectedIndex]
    : null;
  return String((selected && selected.textContent) || el.toneMapping.value || "ACES (Fitted)");
}

function renderPostPipelineGraph() {
  if (!el.postPipelineGraph) return;
  const stackEnabled = isPostFilterStackEnabled();
  const chain = Array.isArray(postFilterChain) ? postFilterChain : [];
  const before = [];
  const after = [];
  chain.forEach((entry) => {
    const normalized = normalizePostFilterEntry(entry);
    if (!normalized) return;
    const info = postFilterCatalogById(normalized.filter);
    const label = info ? info.label : normalized.filter;
    if (normalized.stage === "before") before.push(label);
    else after.push(label);
  });

  const nodes = [];
  nodes.push({ kind: "io", label: "Input sRGB" });
  nodes.push({ kind: "conversion", label: "sRGB -> Linear" });
  if (stackEnabled) {
    before.forEach((label) => nodes.push({ kind: "filter", label: `FX: ${label}` }));
  } else {
    nodes.push({ kind: "disabled", label: "Post Filters Disabled" });
  }
  nodes.push({ kind: "tone", label: `Tone Mapping: ${currentToneMappingLabel()}` });
  if (stackEnabled) {
    after.forEach((label) => nodes.push({ kind: "filter", label: `FX: ${label}` }));
  }
  nodes.push({ kind: "conversion", label: "Linear -> sRGB" });
  nodes.push({ kind: "io", label: "Preview Output sRGB" });

  const lane = document.createElement("div");
  lane.className = "post-pipeline-lane";
  nodes.forEach((node, i) => {
    const n = document.createElement("div");
    n.className = `post-pipeline-node is-${node.kind}`;
    n.textContent = node.label;
    lane.appendChild(n);
    if (i < nodes.length - 1) {
      const arrow = document.createElement("span");
      arrow.className = "post-pipeline-arrow";
      arrow.textContent = "->";
      lane.appendChild(arrow);
    }
  });

  el.postPipelineGraph.innerHTML = "";
  el.postPipelineGraph.appendChild(lane);
  if (el.postPipelineSummary) {
    const beforeCount = before.length;
    const afterCount = after.length;
    el.postPipelineSummary.textContent = stackEnabled
      ? `${beforeCount} before TM, ${afterCount} after TM`
      : "Post filters disabled";
  }
}

function renderPostFilterChain() {
  if (!el.postFiltersChain) return;
  el.postFiltersChain.innerHTML = "";
  const stackEnabled = isPostFilterStackEnabled();
  if (!Array.isArray(postFilterChain) || postFilterChain.length === 0) {
    const empty = document.createElement("p");
    empty.className = "post-filters-empty";
    empty.textContent = "No filters in chain.";
    el.postFiltersChain.appendChild(empty);
    renderPostPipelineGraph();
    return;
  }

  postFilterChain.forEach((entry, index) => {
    const normalized = normalizePostFilterEntry(entry);
    if (!normalized) return;
    postFilterChain[index] = normalized;
    const filterId = normalized.filter;
    const filterInfo = postFilterCatalogById(filterId);
    if (!filterInfo) return;

    const row = document.createElement("details");
    row.className = "post-filter-entry";
    row.dataset.index = String(index);
    row.open = index === 0;

    const summary = document.createElement("summary");
    summary.className = "post-filter-summary";

    const name = document.createElement("span");
    name.className = "post-filter-name";
    name.textContent = filterInfo.label;

    const stageChip = document.createElement("span");
    stageChip.className = "post-filter-stage-chip";
    stageChip.textContent = normalized.stage === "before" ? "Before TM" : "After TM";

    summary.appendChild(name);
    summary.appendChild(stageChip);
    row.appendChild(summary);

    const body = document.createElement("div");
    body.className = "post-filter-body";

    const controls = document.createElement("div");
    controls.className = "post-filter-controls";

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
    stage.value = normalized.stage;
    stage.disabled = !stackEnabled;
    stage.addEventListener("change", () => {
      const idx = Number(row.dataset.index);
      if (!Number.isFinite(idx) || idx < 0 || idx >= postFilterChain.length) return;
      postFilterChain[idx].stage = normalizePostFilterStage(stage.value);
      stageChip.textContent = postFilterChain[idx].stage === "before" ? "Before TM" : "After TM";
      appendLog(`post_filter stage idx=${idx} stage=${postFilterChain[idx].stage}`);
      renderPostPipelineGraph();
      queueWorkspaceSettingsSave();
    });

    const removeBtn = document.createElement("button");
    removeBtn.type = "button";
    removeBtn.className = "post-filter-remove";
    removeBtn.textContent = "Remove";
    removeBtn.disabled = !stackEnabled;
    removeBtn.addEventListener("click", () => {
      const idx = Number(row.dataset.index);
      if (!Number.isFinite(idx) || idx < 0 || idx >= postFilterChain.length) return;
      postFilterChain.splice(idx, 1);
      renderPostFilterChain();
      appendLog(`post_filter removed idx=${idx}`);
      queueWorkspaceSettingsSave();
    });

    controls.appendChild(stage);
    controls.appendChild(removeBtn);
    body.appendChild(controls);

    const paramSpecs = postFilterParamSpecs(filterId);
    if (paramSpecs.length > 0) {
      const params = normalized.params || defaultPostFilterParams(filterId);
      const paramsWrap = document.createElement("div");
      paramsWrap.className = "post-filter-params";
      paramSpecs.forEach((field) => {
        const label = document.createElement("label");
        label.className = "post-filter-param";
        const text = document.createElement("span");
        text.textContent = field.label;
        const input = document.createElement("input");
        input.type = "number";
        input.min = field.min;
        input.max = field.max;
        input.step = field.step;
        input.value = String((params && params[field.key]) || "");
        input.disabled = !stackEnabled;
        input.setAttribute("aria-label", `${field.label} for ${filterInfo.label}`);
        input.addEventListener("change", () => {
          const idx = Number(row.dataset.index);
          if (!Number.isFinite(idx) || idx < 0 || idx >= postFilterChain.length) return;
          const current = normalizePostFilterEntry(postFilterChain[idx]);
          if (!current) return;
          const nextParams = { ...(current.params || {}), [field.key]: String(input.value || "") };
          current.params = normalizePostFilterParams(current.filter, nextParams);
          postFilterChain[idx] = current;
          input.value = current.params[field.key];
          appendLog(`post_filter param idx=${idx} ${field.key}=${input.value}`);
          queueWorkspaceSettingsSave();
        });
        label.appendChild(text);
        label.appendChild(input);
        paramsWrap.appendChild(label);
      });
      body.appendChild(paramsWrap);
    }
    row.appendChild(body);
    el.postFiltersChain.appendChild(row);
  });
  renderPostPipelineGraph();
}

function updatePostFilterUiState() {
  const stackEnabled = isPostFilterStackEnabled();
  if (el.postFilterType) el.postFilterType.disabled = !stackEnabled;
  if (el.postFilterAddBtn) el.postFilterAddBtn.disabled = !stackEnabled;
  if (el.postFiltersRecalcBtn) el.postFiltersRecalcBtn.disabled = !stackEnabled;
  renderPostFilterChain();
}

function addPostFilterToChain() {
  const filterId = normalizePostFilterId(el.postFilterType && el.postFilterType.value);
  if (!filterId) return;
  postFilterChain.push({
    filter: filterId,
    stage: "after",
    params: defaultPostFilterParams(filterId),
  });
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
  if (!isPostFilterStackEnabled()) return "";
  const parts = [];
  (postFilterChain || []).forEach((entry) => {
    const normalized = normalizePostFilterEntry(entry);
    if (!normalized) return;
    const stage = normalized.stage;
    const filterId = normalized.filter;
    const paramPairs = [];
    const params = normalized.params || {};
    const specs = postFilterParamSpecs(filterId);
    specs.forEach((spec) => {
      if (params[spec.key] === undefined) return;
      paramPairs.push(`${spec.key}=${params[spec.key]}`);
    });
    if (paramPairs.length > 0) {
      parts.push(`${stage}:${filterId}:${paramPairs.join(":")}`);
    } else {
      parts.push(`${stage}:${filterId}`);
    }
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
