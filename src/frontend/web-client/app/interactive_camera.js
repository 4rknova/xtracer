// Interactive preview camera: state queries, orbit/fly/pan/zoom, HUD, keyboard.
// Depends on globals: interactivePreviewCamera, interactivePreviewEnabled,
// interactivePreviewKeyState, interactivePreviewHudMode, interactivePreviewHudQuality,
// interactivePreviewFlySpeedScale, interactivePreviewFlyLastTickMs, interactivePreviewFlyTimer,
// interactivePreviewLastInputMs, interactivePreviewCameraSeq, interactivePreviewDirty,
// activeTabMode, renderMode, el, api,
// INTERACTIVE_PREVIEW_FLY_SPEED, INTERACTIVE_PREVIEW_FLY_SHIFT_MULTIPLIER.
// Calls into: syncRenderPreviewAuxPanel, requestInteractivePreviewRender,
// appendLog, hasBackendMethod, selectedSceneVariantValue, normalizeRenderMode,
// isInteractiveRenderMode (preview.js).
//
// Keyboard shortcuts (when not typing in an input):
//   W/A/S/D  — fly forward/left/back/right
//   Q/E      — fly down/up
//   Shift    — 3x speed multiplier
//   R        — reset camera to scene default
//   F        — re-anchor orbit pivot to current look-at target

function interactivePreviewAvailable() {
  return !!interactivePreviewEnabled
    && activeTabMode === "render"
    && interactivePreviewCamera.ready;
}

function markInteractiveInputActivity() {
  interactivePreviewLastInputMs = Date.now();
}

function renderInteractivePreviewHud() {
  if (!el.interactivePreviewHud) return;
  if (el.interactivePreviewControls) {
    el.interactivePreviewControls.hidden = !isInteractiveRenderMode();
  }
  if (typeof syncRenderPreviewAuxPanel === "function") syncRenderPreviewAuxPanel();
  if (el.renderMode) {
    el.renderMode.value = normalizeRenderMode(renderMode);
  }
  const show = !!interactivePreviewEnabled && activeTabMode === "render";
  el.interactivePreviewHud.hidden = !show;
  if (el.interactivePreviewSaveCameraBtn) {
    const canSave = !!interactivePreviewEnabled && !!interactivePreviewCamera.ready;
    el.interactivePreviewSaveCameraBtn.disabled = !canSave;
    el.interactivePreviewSaveCameraBtn.classList.toggle("is-disabled", !canSave);
    el.interactivePreviewSaveCameraBtn.setAttribute("aria-disabled", canSave ? "false" : "true");
  }
  if (!show) return;
  if (el.interactivePreviewHudMode) {
    el.interactivePreviewHudMode.textContent = `Mode: ${String(interactivePreviewHudMode || "LOOK").toUpperCase()}`;
  }
  if (el.interactivePreviewHudSpeed) {
    const sp = Number(interactivePreviewFlySpeedScale) || 1;
    el.interactivePreviewHudSpeed.textContent = `Speed: ${sp.toFixed(1)}x`;
  }
  if (el.interactivePreviewHudQuality) {
    el.interactivePreviewHudQuality.textContent = `Quality: ${interactivePreviewHudQuality || "idle"}`;
  }
  if (el.interactivePreviewHudFov) {
    const fov = Number(interactivePreviewCamera && interactivePreviewCamera.hfov) || 60;
    el.interactivePreviewHudFov.textContent = `FOV: ${fov.toFixed(1)}\u00b0`;
  }
}

function interactivePreviewCameraRequestParams() {
  if (!interactivePreviewCamera.ready) return null;
  const p = interactivePreviewCamera.position;
  const t = interactivePreviewCamera.target;
  const u = interactivePreviewCamera.up;
  const nums = [p[0], p[1], p[2], t[0], t[1], t[2], u[0], u[1], u[2], interactivePreviewCamera.hfov];
  for (let i = 0; i < nums.length; i += 1) {
    if (!Number.isFinite(nums[i])) return null;
  }
  return {
    camera: interactivePreviewCamera.sourceCamera || (el.camera ? (el.camera.value || "") : ""),
    cam_px: String(p[0]),
    cam_py: String(p[1]),
    cam_pz: String(p[2]),
    cam_tx: String(t[0]),
    cam_ty: String(t[1]),
    cam_tz: String(t[2]),
    cam_upx: String(u[0]),
    cam_upy: String(u[1]),
    cam_upz: String(u[2]),
    cam_hfov: String(interactivePreviewCamera.hfov || 60),
  };
}

async function refreshInteractivePreviewCameraFromSelection() {
  interactivePreviewCamera.ready = false;
  if (!interactivePreviewEnabled) return false;
  if (!hasBackendMethod(api, "getSceneResolvedCamera")) return false;
  const scene = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  if (!scene) return false;
  const variant = selectedSceneVariantValue();
  const cameraName = String(el.camera && el.camera.value ? el.camera.value : "").trim();
  try {
    const resolved = await api.getSceneResolvedCamera(scene, variant, cameraName);
    const pos = Array.isArray(resolved && resolved.position) ? resolved.position : null;
    const target = Array.isArray(resolved && resolved.target) ? resolved.target : null;
    const up = Array.isArray(resolved && resolved.up) ? resolved.up : null;
    const hfov = Number(resolved && resolved.hfov);
    if (!pos || !target || !up) {
      appendLog("interactive preview unavailable for current camera");
      return false;
    }
    const vals = [pos[0], pos[1], pos[2], target[0], target[1], target[2], up[0], up[1], up[2]];
    if (vals.some((v) => !Number.isFinite(Number(v)))) {
      appendLog("interactive preview unavailable: camera metadata is non-finite");
      return false;
    }
    interactivePreviewCamera.ready = true;
    interactivePreviewCamera.type = String((resolved && resolved.type) || "");
    interactivePreviewCamera.sourceScene = scene;
    interactivePreviewCamera.sourceVariant = variant;
    interactivePreviewCamera.sourceCamera = String((resolved && resolved.resolved) || cameraName);
    interactivePreviewCamera.position = v3(pos[0], pos[1], pos[2]);
    interactivePreviewCamera.target = v3(target[0], target[1], target[2]);
    interactivePreviewCamera.up = v3norm(v3(up[0], up[1], up[2]), [0, 1, 0]);
    interactivePreviewCamera.hfov = Number.isFinite(hfov) ? clamp(hfov, 1, 179) : 60;
    resetInteractiveOrbitFromCamera(true);
    interactivePreviewCameraSeq += 1;
    return true;
  } catch (err) {
    appendLog(`interactive camera resolve failed: ${err.message}`);
    return false;
  }
}

// Compute the camera's local forward/right/up basis from orbit yaw and pitch.
function interactiveCameraBasis() {
  const yaw = Number(interactivePreviewCamera.orbitYaw) || 0;
  const pitch = Number(interactivePreviewCamera.orbitPitch) || 0;
  const cp = Math.cos(pitch);
  const sp = Math.sin(pitch);
  const sy = Math.sin(yaw);
  const cy = Math.cos(yaw);
  let forward = v3norm([sy * cp, sp, -cy * cp], [0, 0, -1]);
  let right = v3cross(forward, [0, 1, 0]);
  if (v3len(right) < 1e-6) right = v3cross(forward, [1, 0, 0]);
  right = v3norm(right, [1, 0, 0]);
  let up = v3cross(right, forward);
  up = v3norm(up, [0, 1, 0]);
  forward = v3norm(forward, [0, 0, -1]);
  return { forward, right, up };
}

function applyInteractiveOrbitCameraState() {
  const pivot = Array.isArray(interactivePreviewCamera.pivot)
    ? v3(interactivePreviewCamera.pivot[0], interactivePreviewCamera.pivot[1], interactivePreviewCamera.pivot[2])
    : [0, 0, 0];
  const basis = interactiveCameraBasis();
  const dist = clamp(Number(interactivePreviewCamera.orbitDistance) || 1, 0.02, 1e6);
  interactivePreviewCamera.pivot = pivot;
  interactivePreviewCamera.orbitDistance = dist;
  interactivePreviewCamera.target = pivot;
  interactivePreviewCamera.position = v3sub(pivot, v3scale(basis.forward, dist));
  interactivePreviewCamera.up = basis.up;
}

function resetInteractiveOrbitFromCamera(anchorToOrigin) {
  const pos = v3(
    interactivePreviewCamera.position[0],
    interactivePreviewCamera.position[1],
    interactivePreviewCamera.position[2],
  );
  const target = v3(
    interactivePreviewCamera.target[0],
    interactivePreviewCamera.target[1],
    interactivePreviewCamera.target[2],
  );
  const pivot = anchorToOrigin ? [0, 0, 0] : target;
  let toPivot = v3sub(pivot, pos);
  let distance = v3len(toPivot);
  if (!Number.isFinite(distance) || distance < 1e-6) {
    toPivot = v3sub(target, pos);
    distance = v3len(toPivot);
  }
  if (!Number.isFinite(distance) || distance < 0.02) distance = 1.0;
  const forward = v3norm(toPivot, [0, 0, -1]);
  interactivePreviewCamera.pivot = pivot;
  interactivePreviewCamera.orbitDistance = distance;
  interactivePreviewCamera.orbitPitch = Math.asin(clamp(forward[1], -0.995, 0.995));
  interactivePreviewCamera.orbitYaw = Math.atan2(forward[0], -forward[2]);
  applyInteractiveOrbitCameraState();
}

function markInteractiveCameraDirty() {
  interactivePreviewCameraSeq += 1;
  interactivePreviewDirty = true;
  markInteractiveInputActivity();
  interactivePreviewHudQuality = "active";
  renderInteractivePreviewHud();
  if (typeof requestInteractivePreviewRender === "function") {
    requestInteractivePreviewRender();
  }
}

function interactiveLookCamera(dx, dy) {
  if (!interactivePreviewCamera.ready) return;
  const fov = Number(interactivePreviewCamera.hfov) || 60;
  const sensitivity = 0.005 * clamp(fov / 60, 0.25, 2.0);
  interactivePreviewCamera.orbitYaw = (Number(interactivePreviewCamera.orbitYaw) || 0) - (dx * sensitivity);
  interactivePreviewCamera.orbitPitch = clamp(
    (Number(interactivePreviewCamera.orbitPitch) || 0) - (dy * sensitivity),
    -1.45,
    1.45,
  );
  applyInteractiveOrbitCameraState();
  markInteractiveCameraDirty();
}

function hasInteractiveFlyInput() {
  return !!(interactivePreviewKeyState.w
    || interactivePreviewKeyState.a
    || interactivePreviewKeyState.s
    || interactivePreviewKeyState.d
    || interactivePreviewKeyState.q
    || interactivePreviewKeyState.e);
}

function tickInteractiveFly() {
  if (!interactivePreviewAvailable()) return;
  const now = Date.now();
  if (!interactivePreviewFlyLastTickMs) interactivePreviewFlyLastTickMs = now;
  const dt = Math.max(0.001, Math.min(0.05, (now - interactivePreviewFlyLastTickMs) / 1000));
  interactivePreviewFlyLastTickMs = now;
  if (!hasInteractiveFlyInput()) return;
  const basis = interactiveCameraBasis();
  const speed = INTERACTIVE_PREVIEW_FLY_SPEED
    * Math.max(0.2, Math.min(5.0, Number(interactivePreviewFlySpeedScale) || 1.0))
    * (interactivePreviewKeyState.shift ? INTERACTIVE_PREVIEW_FLY_SHIFT_MULTIPLIER : 1.0);
  let move = [0, 0, 0];
  if (interactivePreviewKeyState.w) move = v3add(move, basis.forward);
  if (interactivePreviewKeyState.s) move = v3sub(move, basis.forward);
  if (interactivePreviewKeyState.d) move = v3add(move, basis.right);
  if (interactivePreviewKeyState.a) move = v3sub(move, basis.right);
  if (interactivePreviewKeyState.e) move = v3add(move, basis.up);
  if (interactivePreviewKeyState.q) move = v3sub(move, basis.up);
  const moveNorm = v3norm(move, [0, 0, 0]);
  if (v3len(moveNorm) < 1e-6) return;
  const delta = v3scale(moveNorm, speed * dt);
  interactivePreviewCamera.pivot = v3add(
    Array.isArray(interactivePreviewCamera.pivot) ? interactivePreviewCamera.pivot : [0, 0, 0],
    delta,
  );
  applyInteractiveOrbitCameraState();
  markInteractiveCameraDirty();
}

function stopInteractiveFlyTicker() {
  if (interactivePreviewFlyTimer) {
    clearInterval(interactivePreviewFlyTimer);
    interactivePreviewFlyTimer = 0;
  }
  interactivePreviewFlyLastTickMs = 0;
}

function ensureInteractiveFlyTicker() {
  if (interactivePreviewFlyTimer) return;
  interactivePreviewFlyLastTickMs = Date.now();
  interactivePreviewFlyTimer = setInterval(() => {
    tickInteractiveFly();
  }, 16);
}

function bindInteractivePreviewKeyboard() {
  const isTypingTarget = (node) => {
    if (!node || !(node instanceof HTMLElement)) return false;
    const tag = String(node.tagName || "").toLowerCase();
    return tag === "input" || tag === "textarea" || tag === "select" || node.isContentEditable;
  };
  const applyKey = (evt, down) => {
    if (!interactivePreviewEnabled) return;
    if (activeTabMode !== "render") return;
    if (isTypingTarget(evt.target)) return;
    const k = String(evt.key || "").toLowerCase();
    let handled = true;
    if (k === "w") interactivePreviewKeyState.w = down;
    else if (k === "a") interactivePreviewKeyState.a = down;
    else if (k === "s") interactivePreviewKeyState.s = down;
    else if (k === "d") interactivePreviewKeyState.d = down;
    else if (k === "q") interactivePreviewKeyState.q = down;
    else if (k === "e") interactivePreviewKeyState.e = down;
    else if (k === "shift") interactivePreviewKeyState.shift = down;
    else if (k === "r" && down) {
      refreshInteractivePreviewCameraFromSelection()
        .then((ok) => {
          if (ok && typeof requestInteractivePreviewRender === "function") requestInteractivePreviewRender();
        }).catch(() => {});
    } else if (k === "f" && down) {
      resetInteractiveOrbitFromCamera(false);
      markInteractiveCameraDirty();
    } else handled = false;
    if (!handled) return;
    evt.preventDefault();
    if (k !== "r" && k !== "f") {
      interactivePreviewHudMode = "FLY";
      renderInteractivePreviewHud();
    }
    markInteractiveInputActivity();
  };
  window.addEventListener("keydown", (evt) => applyKey(evt, true));
  window.addEventListener("keyup", (evt) => applyKey(evt, false));
  window.addEventListener("blur", () => {
    interactivePreviewKeyState.w = false;
    interactivePreviewKeyState.a = false;
    interactivePreviewKeyState.s = false;
    interactivePreviewKeyState.d = false;
    interactivePreviewKeyState.q = false;
    interactivePreviewKeyState.e = false;
    interactivePreviewKeyState.shift = false;
    interactivePreviewHudMode = "LOOK";
    renderInteractivePreviewHud();
  });
}

function interactiveOrbitCamera(dx, dy) {
  interactiveLookCamera(dx, dy);
}

function interactivePanCamera(dx, dy) {
  if (!interactivePreviewCamera.ready) return;
  const basis = interactiveCameraBasis();
  const dist = Math.max(0.001, Number(interactivePreviewCamera.orbitDistance) || 1);
  const k = dist * 0.0018;
  const move = v3add(v3scale(basis.right, -dx * k), v3scale(basis.up, dy * k));
  interactivePreviewCamera.pivot = v3add(
    Array.isArray(interactivePreviewCamera.pivot) ? interactivePreviewCamera.pivot : [0, 0, 0],
    move,
  );
  applyInteractiveOrbitCameraState();
  markInteractiveCameraDirty();
}

function interactiveZoomCamera(deltaY) {
  if (!interactivePreviewCamera.ready) return;
  const dist = Math.max(0.001, Number(interactivePreviewCamera.orbitDistance) || 1);
  const amount = clamp(Math.exp(deltaY * 0.0015), 0.8, 1.25);
  const nextDist = clamp(dist * amount, 0.02, 1e6);
  interactivePreviewCamera.orbitDistance = nextDist;
  applyInteractiveOrbitCameraState();
  markInteractiveCameraDirty();
}

function interactiveAdjustFov(deltaY) {
  if (!interactivePreviewCamera.ready) return;
  const fov = Number(interactivePreviewCamera.hfov) || 60;
  const amount = Math.exp(deltaY * 0.001);
  interactivePreviewCamera.hfov = clamp(fov * amount, 1, 179);
  markInteractiveCameraDirty();
}
