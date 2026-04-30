let ftueInitialized = false;
let ftueOpen = false;
let ftueStepIndex = 0;
let ftueSteps = [];
let ftueLastFocused = null;
let ftueCurrentTarget = null;
let ftueLayoutTimer = null;
let ftueConfigSteps = null;
let ftueConfigLoadPromise = null;
let ftueConfigError = "";

const FTUE_STEPS_CONFIG_URL = "/app/data/ftue_steps.json";

function hasCompletedFtueVersion() {
  return String(localStorage.getItem(FTUE_STATE_VERSION_KEY) || "") === String(FTUE_VERSION);
}

function markFtueCompleted() {
  localStorage.setItem(FTUE_STATE_VERSION_KEY, String(FTUE_VERSION));
  localStorage.removeItem(FTUE_FORCE_NEXT_KEY);
  syncFtueSettingsUi();
}

function queueFtueForNextLaunch() {
  localStorage.removeItem(FTUE_STATE_VERSION_KEY);
  localStorage.setItem(FTUE_FORCE_NEXT_KEY, "1");
  syncFtueSettingsUi();
}

function clearFtueNextLaunchFlag() {
  localStorage.removeItem(FTUE_FORCE_NEXT_KEY);
  syncFtueSettingsUi();
}

function shouldStartFtueNow() {
  const forced = localStorage.getItem(FTUE_FORCE_NEXT_KEY) === "1";
  if (forced) localStorage.removeItem(FTUE_FORCE_NEXT_KEY);
  return forced || !hasCompletedFtueVersion();
}

function syncFtueSettingsUi() {
  if (!el.ftueShowOnNextLaunch) return;
  el.ftueShowOnNextLaunch.checked = localStorage.getItem(FTUE_FORCE_NEXT_KEY) === "1";
}

function safeFocus(node) {
  if (!node || typeof node.focus !== "function") return;
  try {
    node.focus({ preventScroll: true });
  } catch (_) {
    node.focus();
  }
}

function openSettingsCard() {
  const card = document.getElementById("settingsControlsCard");
  if (!card) return;
  card.open = true;
}

function normalizeFtuePlacement(value) {
  const raw = String(value || "").toLowerCase();
  if (raw === "right" || raw === "left" || raw === "top" || raw === "bottom") return raw;
  return "";
}

function normalizeFtueConfigSteps(rawSteps) {
  if (!Array.isArray(rawSteps)) return [];
  const out = [];
  rawSteps.forEach((raw, idx) => {
    if (!raw || typeof raw !== "object" || Array.isArray(raw)) return;
    const title = String(raw.title || "").trim() || `Step ${idx + 1}`;
    const body = String(raw.body || "").trim() || "Continue to the next step.";
    const targetSelector = String(raw.target_selector || "").trim();
    const focusSelector = String(raw.focus_selector || "").trim();
    const tab = String(raw.tab || "").trim();
    const editorView = String(raw.editor_view || "").trim();
    const placement = normalizeFtuePlacement(raw.placement);
    const openCards = Array.isArray(raw.open_cards)
      ? raw.open_cards.filter((id) => typeof id === "string" && id.trim()).map((id) => id.trim())
      : [];
    out.push({
      title,
      body,
      placement,
      target: targetSelector || "",
      onEnter: () => {
        if (tab) setActiveTab(tab);
        if (editorView) setEditorViewMode(editorView);
        if (openCards.length > 0) {
          openCards.forEach((id) => {
            const card = document.getElementById(id);
            if (card && card.tagName && card.tagName.toLowerCase() === "details") card.open = true;
          });
        }
        if (tab === "settings") openSettingsCard();
        if (focusSelector) {
          const focusTarget = document.querySelector(focusSelector);
          safeFocus(focusTarget);
        }
      },
    });
  });
  return out;
}

async function loadFtueConfigSteps(forceReload) {
  if (forceReload) {
    ftueConfigLoadPromise = null;
    ftueConfigSteps = null;
    ftueConfigError = "";
  }
  if (ftueConfigLoadPromise) return ftueConfigLoadPromise;
  ftueConfigLoadPromise = (async () => {
    const sep = FTUE_STEPS_CONFIG_URL.includes("?") ? "&" : "?";
    const url = `${FTUE_STEPS_CONFIG_URL}${sep}t=${Date.now()}`;
    const response = await fetch(url, { cache: "no-store" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const payload = await response.json();
    const loaded = normalizeFtueConfigSteps(payload && Array.isArray(payload.steps) ? payload.steps : payload);
    if (loaded.length === 0) throw new Error("empty step list");
    ftueConfigError = "";
    ftueConfigSteps = loaded;
    appendLog(`loaded ftue config from ${FTUE_STEPS_CONFIG_URL}`);
  })();
  try {
    await ftueConfigLoadPromise;
  } catch (err) {
    ftueConfigSteps = null;
    ftueConfigError = String(err && err.message ? err.message : err || "unknown error");
    appendLog(`ftue config error: ${ftueConfigError}`);
    ftueConfigLoadPromise = null;
    throw err;
  }
  return ftueConfigLoadPromise;
}

function isVisibleTarget(target) {
  if (!target || !target.getBoundingClientRect) return false;
  const rect = target.getBoundingClientRect();
  return rect.width > 0 && rect.height > 0;
}

function expandTargetContainers(target) {
  if (!target || !target.closest) return;
  const detailsNodes = [];
  let node = target.closest("details");
  while (node) {
    detailsNodes.push(node);
    node = node.parentElement ? node.parentElement.closest("details") : null;
  }
  detailsNodes.forEach((details) => {
    details.open = true;
  });
}

function clearFtueTargetHighlight() {
  if (ftueCurrentTarget) ftueCurrentTarget.classList.remove("ftue-target");
  ftueCurrentTarget = null;
  if (el.ftueSpotlight) el.ftueSpotlight.hidden = true;
  if (el.ftueOverlay) {
    el.ftueOverlay.style.setProperty("--ftue-hole-x", "0px");
    el.ftueOverlay.style.setProperty("--ftue-hole-y", "0px");
    el.ftueOverlay.style.setProperty("--ftue-hole-w", "0px");
    el.ftueOverlay.style.setProperty("--ftue-hole-h", "0px");
  }
}

function applyFtueSpotlight(target) {
  if (!el.ftueSpotlight || !isVisibleTarget(target)) {
    if (el.ftueSpotlight) el.ftueSpotlight.hidden = true;
    return;
  }
  const rect = target.getBoundingClientRect();
  const pad = 5;
  const x = Math.max(4, rect.left - pad);
  const y = Math.max(4, rect.top - pad);
  const w = Math.max(16, rect.width + (pad * 2));
  const h = Math.max(16, rect.height + (pad * 2));
  el.ftueSpotlight.hidden = false;
  el.ftueSpotlight.style.left = `${x}px`;
  el.ftueSpotlight.style.top = `${y}px`;
  el.ftueSpotlight.style.width = `${w}px`;
  el.ftueSpotlight.style.height = `${h}px`;
  el.ftueSpotlight.style.transform = "none";
  if (el.ftueOverlay) {
    el.ftueOverlay.style.setProperty("--ftue-hole-x", `${x}px`);
    el.ftueOverlay.style.setProperty("--ftue-hole-y", `${y}px`);
    el.ftueOverlay.style.setProperty("--ftue-hole-w", `${w}px`);
    el.ftueOverlay.style.setProperty("--ftue-hole-h", `${h}px`);
  }
}

function positionFtueDialogForTarget(target, preferredPlacement) {
  if (!el.ftueDialog || !isVisibleTarget(target)) return false;
  const rect = target.getBoundingClientRect();
  const gap = 14;
  const vw = window.innerWidth;
  const vh = window.innerHeight;
  const cardRect = el.ftueDialog.getBoundingClientRect();
  const mobile = vw <= 700;
  const minCardW = mobile ? 220 : 320;
  const minCardH = mobile ? 150 : 220;
  const cardW = Math.max(minCardW, Math.round(cardRect.width) || Math.min(684, Math.floor(vw * 0.92)));
  const cardH = Math.max(minCardH, Math.round(cardRect.height) || el.ftueDialog.offsetHeight || minCardH);
  const placements = [preferredPlacement, "right", "left", "bottom", "top"]
    .filter((v, i, a) => !!v && a.indexOf(v) === i);

  const fits = (x, y) => (x >= 8 && y >= 8 && (x + cardW + 8) <= vw && (y + cardH + 8) <= vh);
  const coordsByPlacement = {
    right: { x: rect.right + gap, y: rect.top + ((rect.height - cardH) / 2) },
    left: { x: rect.left - gap - cardW, y: rect.top + ((rect.height - cardH) / 2) },
    bottom: { x: rect.left + ((rect.width - cardW) / 2), y: rect.bottom + gap },
    top: { x: rect.left + ((rect.width - cardW) / 2), y: rect.top - gap - cardH },
  };

  let pos = null;
  for (let i = 0; i < placements.length; i += 1) {
    const p = placements[i];
    const c = coordsByPlacement[p];
    if (!c) continue;
    if (fits(c.x, c.y)) {
      pos = c;
      break;
    }
  }
  if (!pos) {
    const fallbackX = Math.max(8, Math.min(vw - cardW - 8, rect.left + ((rect.width - cardW) / 2)));
    const fallbackY = Math.max(8, Math.min(vh - cardH - 8, rect.bottom + gap));
    pos = { x: fallbackX, y: fallbackY };
  }

  el.ftueDialog.style.left = `${Math.round(pos.x)}px`;
  el.ftueDialog.style.top = `${Math.round(pos.y)}px`;
  el.ftueDialog.style.transform = "none";
  return true;
}

function centerFtueDialog() {
  if (!el.ftueDialog) return;
  el.ftueDialog.style.left = "50%";
  el.ftueDialog.style.top = "50%";
  el.ftueDialog.style.transform = "translate(-50%, -50%)";
}

function getStepTarget(step) {
  if (!step) return null;
  if (typeof step.target === "function") return step.target() || null;
  if (typeof step.target === "string") return document.querySelector(step.target);
  return null;
}

function updateFtueStepLayout() {
  if (!ftueOpen) return;
  const step = ftueSteps[ftueStepIndex];
  if (!step) return;
  const target = getStepTarget(step);
  if (target) expandTargetContainers(target);
  clearFtueTargetHighlight();
  if (!target || !isVisibleTarget(target)) {
    centerFtueDialog();
    return;
  }
  ftueCurrentTarget = target;
  ftueCurrentTarget.classList.add("ftue-target");
  try {
    target.scrollIntoView({ block: "center", inline: "nearest", behavior: "auto" });
  } catch (_) {
    // ignore scroll errors
  }
  applyFtueSpotlight(target);
  if (!positionFtueDialogForTarget(target, step.placement || "")) {
    centerFtueDialog();
  }
}

function queueFtueStepLayout() {
  if (ftueLayoutTimer) clearTimeout(ftueLayoutTimer);
  ftueLayoutTimer = setTimeout(() => {
    ftueLayoutTimer = null;
    updateFtueStepLayout();
  }, 10);
}

function buildFtueSteps() {
  if (Array.isArray(ftueConfigSteps) && ftueConfigSteps.length > 0) return ftueConfigSteps;
  return [];
}

function renderFtueStep() {
  if (!ftueOpen || !el.ftueTitle || !el.ftueBody || !el.ftueStepLabel) return;
  const step = ftueSteps[ftueStepIndex];
  if (!step) return;
  el.ftueTitle.textContent = step.title;
  el.ftueBody.textContent = step.body;
  el.ftueStepLabel.textContent = `Step ${ftueStepIndex + 1} of ${ftueSteps.length}`;
  if (el.ftueBackBtn) el.ftueBackBtn.disabled = ftueStepIndex <= 0;
  if (el.ftueNextBtn) el.ftueNextBtn.textContent = (ftueStepIndex >= ftueSteps.length - 1) ? "Finish" : "Next";
  if (typeof step.onEnter === "function") step.onEnter();
  queueFtueStepLayout();
}

function getFtueFocusable() {
  if (!el.ftueOverlay) return [];
  return Array.from(el.ftueOverlay.querySelectorAll("button,[href],input,select,textarea,[tabindex]:not([tabindex='-1'])"))
    .filter((node) => !node.hasAttribute("disabled") && node.getAttribute("aria-hidden") !== "true");
}

function isFtueDialogTarget(node) {
  if (!el.ftueDialog || !node || !(node instanceof Node)) return false;
  return el.ftueDialog.contains(node);
}

function onFtueFocusIn(ev) {
  if (!ftueOpen) return;
  if (isFtueDialogTarget(ev.target)) return;
  const focusable = getFtueFocusable();
  safeFocus(focusable[0] || el.ftueNextBtn || el.ftueSkipBtn);
}

function onFtueKeydown(ev) {
  if (!ftueOpen) return;

  if (!isFtueDialogTarget(ev.target)) {
    ev.preventDefault();
    ev.stopPropagation();
  }

  if (ev.key === "Escape") {
    ev.preventDefault();
    ev.stopPropagation();
    closeFtueTutorial(true);
    return;
  }
  if (ev.key === "ArrowRight") {
    ev.preventDefault();
    ev.stopPropagation();
    nextFtueStep();
    return;
  }
  if (ev.key === "ArrowLeft") {
    ev.preventDefault();
    ev.stopPropagation();
    prevFtueStep();
    return;
  }
  if (ev.key !== "Tab") {
    ev.stopPropagation();
    return;
  }
  const focusable = getFtueFocusable();
  if (focusable.length === 0) {
    ev.preventDefault();
    ev.stopPropagation();
    return;
  }
  const first = focusable[0];
  const last = focusable[focusable.length - 1];
  const current = document.activeElement;
  if (ev.shiftKey && (current === first || !isFtueDialogTarget(current))) {
    ev.preventDefault();
    ev.stopPropagation();
    safeFocus(last);
    return;
  }
  if (!ev.shiftKey && current === last) {
    ev.preventDefault();
    ev.stopPropagation();
    safeFocus(first);
    return;
  }
  if (!isFtueDialogTarget(current)) {
    ev.preventDefault();
    ev.stopPropagation();
    safeFocus(first);
    return;
  }
  ev.stopPropagation();
}

function openFtueTutorial(source) {
  if (!el.ftueOverlay) return;
  ftueSteps = buildFtueSteps();
  if (!Array.isArray(ftueSteps) || ftueSteps.length === 0) {
    appendLog(`ftue config error: tutorial not started (${ftueConfigError || "no steps available"})`);
    if (source === "manual") {
      setStatus(`error: ftue config unavailable (${ftueConfigError || "no steps"})`);
    }
    return;
  }
  ftueOpen = true;
  ftueStepIndex = 0;
  ftueLastFocused = document.activeElement;
  el.ftueOverlay.hidden = false;
  el.ftueOverlay.setAttribute("aria-hidden", "false");
  document.addEventListener("keydown", onFtueKeydown, true);
  document.addEventListener("focusin", onFtueFocusIn, true);
  window.addEventListener("resize", queueFtueStepLayout);
  window.addEventListener("scroll", queueFtueStepLayout, true);
  renderFtueStep();
  safeFocus(el.ftueNextBtn || el.ftueSkipBtn);
  if (source === "manual") appendLog("tutorial started");
}

function closeFtueTutorial(markCompleted) {
  if (!ftueOpen || !el.ftueOverlay) return;
  ftueOpen = false;
  el.ftueOverlay.hidden = true;
  el.ftueOverlay.setAttribute("aria-hidden", "true");
  document.removeEventListener("keydown", onFtueKeydown, true);
  document.removeEventListener("focusin", onFtueFocusIn, true);
  window.removeEventListener("resize", queueFtueStepLayout);
  window.removeEventListener("scroll", queueFtueStepLayout, true);
  if (ftueLayoutTimer) {
    clearTimeout(ftueLayoutTimer);
    ftueLayoutTimer = null;
  }
  clearFtueTargetHighlight();
  if (markCompleted) markFtueCompleted();
  if (ftueLastFocused && typeof ftueLastFocused.focus === "function") safeFocus(ftueLastFocused);
  ftueLastFocused = null;
}

function nextFtueStep() {
  if (!ftueOpen) return;
  if (ftueStepIndex >= ftueSteps.length - 1) {
    closeFtueTutorial(true);
    appendLog("tutorial completed");
    return;
  }
  ftueStepIndex += 1;
  renderFtueStep();
}

function prevFtueStep() {
  if (!ftueOpen) return;
  if (ftueStepIndex <= 0) return;
  ftueStepIndex -= 1;
  renderFtueStep();
}

function initializeFtueTutorial() {
  if (ftueInitialized) return;
  ftueInitialized = true;
  void loadFtueConfigSteps().catch(() => {});
  syncFtueSettingsUi();
  if (el.ftueNextBtn) el.ftueNextBtn.addEventListener("click", nextFtueStep);
  if (el.ftueBackBtn) el.ftueBackBtn.addEventListener("click", prevFtueStep);
  if (el.ftueSkipBtn) {
    el.ftueSkipBtn.addEventListener("click", () => {
      closeFtueTutorial(true);
      appendLog("tutorial skipped");
    });
  }
  if (el.ftueShowOnNextLaunch) {
    el.ftueShowOnNextLaunch.addEventListener("change", () => {
      if (el.ftueShowOnNextLaunch.checked) {
        queueFtueForNextLaunch();
        appendLog("tutorial reset for next launch");
      } else {
        clearFtueNextLaunchFlag();
      }
    });
  }
  if (el.ftueStartNowBtn) {
    el.ftueStartNowBtn.addEventListener("click", () => {
      localStorage.removeItem(FTUE_STATE_VERSION_KEY);
      clearFtueNextLaunchFlag();
      loadFtueConfigSteps(true)
        .then(() => {
          openFtueTutorial("manual");
        })
        .catch(() => {
          setStatus(`error: ftue config unavailable (${ftueConfigError || "load failed"})`);
        });
    });
  }
}

async function maybeStartFtueTutorial() {
  try {
    await loadFtueConfigSteps();
  } catch (_) {
    return;
  }
  if (!shouldStartFtueNow()) {
    syncFtueSettingsUi();
    return;
  }
  openFtueTutorial("auto");
}
