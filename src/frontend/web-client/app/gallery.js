// Gallery — fetches and displays cached renders from /api/gallery

let galleryEntries = [];
let galleryDetailId = null;
let galleryCurrentPassIndex = -1; // -1 = main render, >=0 = pass index

// ── Gallery TM state ───────────────────────────────────────────────────────

const galleryTm = {
  op:               "aces",
  exposure:         1.0,
  whitePoint:       1.0,
  mantiukContrast:  0.1,
  mantiukSaturation: 0.8,
  mantiukDetail:    1.0,
};

function buildGalleryDetailImageUrl(entryId, passIndex) {
  const op = galleryTm.op;
  const parts = [`t=${Date.now()}`, `tm=${encodeURIComponent(op)}`];
  const spec = toneMappingControlSpec(op);
  if (spec.usesExposure) {
    parts.push(`tm_exposure=${encodeURIComponent(galleryTm.exposure)}`);
  }
  if (spec.usesWhitePoint) {
    parts.push(`tm_white_point=${encodeURIComponent(galleryTm.whitePoint)}`);
  }
  if (spec.usesMantiuk) {
    parts.push(`tm_mantiuk_contrast=${encodeURIComponent(galleryTm.mantiukContrast)}`);
    parts.push(`tm_mantiuk_saturation=${encodeURIComponent(galleryTm.mantiukSaturation)}`);
    parts.push(`tm_mantiuk_detail=${encodeURIComponent(galleryTm.mantiukDetail)}`);
  }
  const qs = `?${parts.join("&")}`;
  if (passIndex >= 0) {
    return `/api/gallery/${encodeURIComponent(entryId)}/pass/${passIndex}/image${qs}`;
  }
  return `/api/gallery/${encodeURIComponent(entryId)}/image${qs}`;
}

function syncGalleryTmParamsVisibility() {
  const op = galleryTm.op;
  const spec = toneMappingControlSpec(op);
  const paramsRow    = document.getElementById("galleryTmParams");
  const exposureRow  = document.getElementById("galleryTmExposureRow");
  const wpRow        = document.getElementById("galleryTmWhitePointRow");
  const mContRow     = document.getElementById("galleryTmMantiukContrastRow");
  const mSatRow      = document.getElementById("galleryTmMantiukSaturationRow");
  const mDetRow      = document.getElementById("galleryTmMantiukDetailRow");
  const anyParams    = spec.usesExposure || spec.usesWhitePoint || spec.usesMantiuk;
  if (paramsRow)   paramsRow.hidden   = !anyParams;
  if (exposureRow) exposureRow.hidden = !spec.usesExposure;
  if (wpRow)       wpRow.hidden       = !spec.usesWhitePoint;
  if (mContRow)    mContRow.hidden    = !spec.usesMantiuk;
  if (mSatRow)     mSatRow.hidden     = !spec.usesMantiuk;
  if (mDetRow)     mDetRow.hidden     = !spec.usesMantiuk;
}

function reloadGalleryDetailImage() {
  if (!galleryDetailId) return;
  const img = document.getElementById("galleryDetailImage");
  if (!img) return;
  img.onload = () => { resetGalleryView(); syncGalleryCanvasSize(); };
  img.src = buildGalleryDetailImageUrl(galleryDetailId, galleryCurrentPassIndex);
}

function initGalleryTmFromEntry(entry) {
  galleryTm.op               = (entry && entry.tm_op)               || "aces";
  galleryTm.exposure         = (entry && entry.tm_exposure   != null) ? entry.tm_exposure   : 1.0;
  galleryTm.whitePoint       = (entry && entry.tm_white_point != null) ? entry.tm_white_point : 1.0;
  galleryTm.mantiukContrast  = (entry && entry.tm_mantiuk_contrast  != null) ? entry.tm_mantiuk_contrast  : 0.1;
  galleryTm.mantiukSaturation = (entry && entry.tm_mantiuk_saturation != null) ? entry.tm_mantiuk_saturation : 0.8;
  galleryTm.mantiukDetail    = (entry && entry.tm_mantiuk_detail    != null) ? entry.tm_mantiuk_detail    : 1.0;

  const opSel  = document.getElementById("galleryTmOp");
  const expInp = document.getElementById("galleryTmExposure");
  const wpInp  = document.getElementById("galleryTmWhitePoint");
  const mCont  = document.getElementById("galleryTmMantiukContrast");
  const mSat   = document.getElementById("galleryTmMantiukSaturation");
  const mDet   = document.getElementById("galleryTmMantiukDetail");
  if (opSel)  opSel.value  = galleryTm.op;
  if (expInp) expInp.value = galleryTm.exposure;
  if (wpInp)  wpInp.value  = galleryTm.whitePoint;
  if (mCont)  mCont.value  = galleryTm.mantiukContrast;
  if (mSat)   mSat.value   = galleryTm.mantiukSaturation;
  if (mDet)   mDet.value   = galleryTm.mantiukDetail;
  syncGalleryTmParamsVisibility();
}

// ── Gallery detail zoom / pan / minimap / sampling ────────────────────────

const galleryView = {
  scale: 1,
  tx: 0,
  ty: 0,
  minScale: 1,
  maxScale: 12,
  panning: false,
  panMode: "",
  pointerId: null,
  lastX: 0,
  lastY: 0,
};
let galleryViewSampling = "smooth"; // "smooth" | "nearest"
let galleryMinimapLayout = null;
let galleryViewResizeObserver = null;

function getGalleryViewEls() {
  return {
    canvas: document.getElementById("galleryDetailCanvas"),
    img:    document.getElementById("galleryDetailImage"),
    wrap:   document.querySelector(".gallery-detail-image-wrap"),
    btn:    document.getElementById("galleryResetViewBtn"),
  };
}

function isGalleryNearestSampling() {
  return galleryViewSampling === "nearest";
}

function getGalleryFittedSize(canvas, img) {
  const frameW = canvas.width;
  const frameH = canvas.height;
  if (!frameW || !frameH || !img.naturalWidth || !img.naturalHeight) return null;
  const fit = Math.min(frameW / img.naturalWidth, frameH / img.naturalHeight);
  return {
    frameW,
    frameH,
    width:  img.naturalWidth  * fit,
    height: img.naturalHeight * fit,
  };
}

function clampGalleryPan() {
  const { canvas, img } = getGalleryViewEls();
  if (!canvas || !img || !img.naturalWidth) return;
  const fitted = getGalleryFittedSize(canvas, img);
  if (!fitted) return;
  const scaledW = fitted.width  * galleryView.scale;
  const scaledH = fitted.height * galleryView.scale;
  const maxX = Math.max(0, (scaledW - fitted.frameW) * 0.5);
  const maxY = Math.max(0, (scaledH - fitted.frameH) * 0.5);
  galleryView.tx = Math.min(maxX, Math.max(-maxX, galleryView.tx));
  galleryView.ty = Math.min(maxY, Math.max(-maxY, galleryView.ty));
}

// ── Minimap ────────────────────────────────────────────────────────────────

function computeGalleryMinimapLayout(fitted, imageX, imageY, imageW, imageH) {
  const isZoomed = galleryView.scale > 1.001 || Math.abs(galleryView.tx) > 0.5 || Math.abs(galleryView.ty) > 0.5;
  if (!fitted || !isZoomed) return null;
  const miniMargin = 14;
  const miniSize = Math.min(180, Math.max(96, Math.round(Math.min(fitted.frameW, fitted.frameH) * 0.22)));
  const miniX = fitted.frameW - miniSize - miniMargin;
  const miniY = fitted.frameH - miniSize - miniMargin;
  const aspect = Math.max(1e-6, imageW / Math.max(1, imageH));
  let mapW = miniSize;
  let mapH = Math.round(mapW / aspect);
  if (mapH > miniSize) { mapH = miniSize; mapW = Math.round(mapH * aspect); }
  const mapX = miniX + Math.round((miniSize - mapW) * 0.5);
  const mapY = miniY + Math.round((miniSize - mapH) * 0.5);
  return { miniX, miniY, miniSize, mapX, mapY, mapW, mapH, imageW, imageH };
}

function getGalleryMinimapHit(canvas, clientX, clientY, clampToBounds) {
  if (!galleryMinimapLayout) return null;
  const rect = canvas.getBoundingClientRect();
  const scaleX = canvas.width  / rect.width;
  const scaleY = canvas.height / rect.height;
  const localX = (clientX - rect.left) * scaleX;
  const localY = (clientY - rect.top)  * scaleY;
  const { mapX, mapY, mapW, mapH } = galleryMinimapLayout;
  if (!clampToBounds && (localX < mapX || localX > mapX + mapW || localY < mapY || localY > mapY + mapH)) return null;
  const hitX = clampToBounds ? Math.min(mapX + mapW, Math.max(mapX, localX)) : localX;
  const hitY = clampToBounds ? Math.min(mapY + mapH, Math.max(mapY, localY)) : localY;
  return {
    u: Math.min(1, Math.max(0, (hitX - mapX) / Math.max(1e-6, mapW))),
    v: Math.min(1, Math.max(0, (hitY - mapY) / Math.max(1e-6, mapH))),
  };
}

function recenterGalleryFromMinimap(canvas, clientX, clientY, clampToBounds) {
  const hit = getGalleryMinimapHit(canvas, clientX, clientY, !!clampToBounds);
  if (!hit || !galleryMinimapLayout) return false;
  galleryView.tx = (0.5 - hit.u) * galleryMinimapLayout.imageW;
  galleryView.ty = (0.5 - hit.v) * galleryMinimapLayout.imageH;
  applyGalleryTransform();
  return true;
}

function drawGalleryMinimap(ctx, fitted, img, imageX, imageY, imageW, imageH) {
  galleryMinimapLayout = null;
  const layout = computeGalleryMinimapLayout(fitted, imageX, imageY, imageW, imageH);
  if (!layout) return;
  galleryMinimapLayout = layout;
  const { miniX, miniY, miniSize, mapX, mapY, mapW, mapH } = layout;

  ctx.save();
  ctx.fillStyle = "rgba(9, 16, 24, 0.62)";
  ctx.strokeStyle = "rgba(173, 214, 255, 0.72)";
  ctx.lineWidth = 1.25;
  ctx.beginPath();
  ctx.roundRect(miniX - 6, miniY - 6, miniSize + 12, miniSize + 12, 10);
  ctx.fill();
  ctx.stroke();

  ctx.save();
  ctx.beginPath();
  ctx.rect(mapX, mapY, mapW, mapH);
  ctx.clip();
  ctx.imageSmoothingEnabled = !isGalleryNearestSampling();
  ctx.drawImage(img, mapX, mapY, mapW, mapH);
  ctx.restore();

  const imgToMapX = mapW / Math.max(1e-6, imageW);
  const imgToMapY = mapH / Math.max(1e-6, imageH);
  const viewLeft   = Math.max(0, -imageX);
  const viewTop    = Math.max(0, -imageY);
  const viewRight  = Math.min(imageW, fitted.frameW - imageX);
  const viewBottom = Math.min(imageH, fitted.frameH - imageY);
  if (viewRight > viewLeft && viewBottom > viewTop) {
    const rectX = mapX + viewLeft   * imgToMapX;
    const rectY = mapY + viewTop    * imgToMapY;
    const rectW = Math.max(6, (viewRight  - viewLeft)   * imgToMapX);
    const rectH = Math.max(6, (viewBottom - viewTop)    * imgToMapY);
    ctx.fillStyle = "rgba(255, 214, 92, 0.14)";
    ctx.strokeStyle = "rgba(255, 196, 64, 0.95)";
    ctx.lineWidth = 1.5;
    ctx.fillRect(rectX, rectY, rectW, rectH);
    ctx.strokeRect(rectX, rectY, rectW, rectH);
  }

  ctx.strokeStyle = "rgba(232, 240, 248, 0.78)";
  ctx.lineWidth = 1;
  ctx.strokeRect(mapX, mapY, mapW, mapH);
  ctx.restore();
}

// ── Canvas draw ────────────────────────────────────────────────────────────

function drawGalleryCanvas() {
  const { canvas, img } = getGalleryViewEls();
  if (!canvas || !img || !img.naturalWidth) return;
  const ctx = canvas.getContext("2d");
  if (!ctx) return;
  const nearest = isGalleryNearestSampling();
  const fitted  = getGalleryFittedSize(canvas, img);
  if (!fitted) return;
  const scale = nearest ? Math.max(1, Math.round(galleryView.scale)) : galleryView.scale;
  const tx    = nearest ? Math.round(galleryView.tx) : galleryView.tx;
  const ty    = nearest ? Math.round(galleryView.ty) : galleryView.ty;
  const drawW = fitted.width  * scale;
  const drawH = fitted.height * scale;
  const x = (fitted.frameW - drawW) * 0.5 + tx;
  const y = (fitted.frameH - drawH) * 0.5 + ty;
  ctx.clearRect(0, 0, fitted.frameW, fitted.frameH);
  ctx.imageSmoothingEnabled = !nearest;
  ctx.imageSmoothingQuality = "high";
  ctx.drawImage(img, x, y, drawW, drawH);
  drawGalleryMinimap(ctx, fitted, img, x, y, drawW, drawH);
}

function syncGalleryCanvasSize() {
  const { canvas, wrap } = getGalleryViewEls();
  if (!canvas || !wrap) return;
  const w = Math.floor(wrap.clientWidth);
  const h = Math.floor(wrap.clientHeight);
  if (w > 0 && h > 0 && (canvas.width !== w || canvas.height !== h)) {
    canvas.width  = w;
    canvas.height = h;
  }
  drawGalleryCanvas();
}

// ── View state ─────────────────────────────────────────────────────────────

function applyGalleryTransform() {
  const { wrap } = getGalleryViewEls();
  const isZoomed = galleryView.scale > 1.001 || Math.abs(galleryView.tx) > 0.5 || Math.abs(galleryView.ty) > 0.5;
  if (wrap) {
    wrap.classList.toggle("is-zoomed",  isZoomed);
    wrap.classList.toggle("is-panning", !!galleryView.panning);
  }
  const btn = document.getElementById("galleryResetViewBtn");
  if (btn) {
    btn.hidden = !isZoomed;
    btn.classList.toggle("is-disabled", !isZoomed);
    btn.disabled = !isZoomed;
    btn.setAttribute("aria-disabled", isZoomed ? "false" : "true");
  }
  clampGalleryPan();
  drawGalleryCanvas();
}

function resetGalleryView() {
  galleryView.scale = 1;
  galleryView.tx    = 0;
  galleryView.ty    = 0;
  galleryView.panning  = false;
  galleryView.panMode  = "";
  galleryView.pointerId = null;
  galleryMinimapLayout  = null;
  applyGalleryTransform();
}

function zoomGalleryAt(clientX, clientY, wheelDeltaY) {
  const { canvas, img } = getGalleryViewEls();
  if (!canvas || !img || !img.naturalWidth) return;
  const rect = canvas.getBoundingClientRect();
  const cx = clientX - rect.left - rect.width  * 0.5;
  const cy = clientY - rect.top  - rect.height * 0.5;
  let nextScale;
  if (isGalleryNearestSampling()) {
    const dir = wheelDeltaY < 0 ? 1 : -1;
    nextScale = Math.min(galleryView.maxScale, Math.max(galleryView.minScale, galleryView.scale + dir));
  } else {
    const zoomFactor = Math.exp((-wheelDeltaY) * 0.0015);
    nextScale = Math.min(galleryView.maxScale, Math.max(galleryView.minScale, galleryView.scale * zoomFactor));
  }
  if (!Number.isFinite(nextScale) || Math.abs(nextScale - galleryView.scale) < 1e-6) return;
  const k = nextScale / galleryView.scale;
  galleryView.tx    = cx - (cx - galleryView.tx) * k;
  galleryView.ty    = cy - (cy - galleryView.ty) * k;
  galleryView.scale = nextScale;
  applyGalleryTransform();
}

// ── Sampling switch ────────────────────────────────────────────────────────

function setGalleryViewSampling(mode) {
  galleryViewSampling = mode === "nearest" ? "nearest" : "smooth";
  window.XTracerWidgets.syncSamplingSwitch(
    document.getElementById("gallerySamplingSwitch"),
    galleryViewSampling,
  );
  if (galleryViewSampling === "nearest") {
    // Snap to integer scale when switching to nearest
    galleryView.scale = Math.max(1, Math.round(galleryView.scale));
  }
  drawGalleryCanvas();
}


// ── Event binding ──────────────────────────────────────────────────────────

function bindGalleryViewEvents() {
  const { canvas, wrap } = getGalleryViewEls();
  if (!canvas) return;
  const touchPoints = {};
  let pinchDistance = 0;
  let pinchCenterX = 0;
  let pinchCenterY = 0;

  // Populate the static sampling switch container in the toolbar.
  const sw = document.getElementById("gallerySamplingSwitch");
  if (sw && !sw._galleryBound) {
    sw._galleryBound = true;
    const switchWidget = window.XTracerWidgets.createSamplingSwitch({
      value: galleryViewSampling,
      onChange: setGalleryViewSampling,
    });
    while (switchWidget.firstChild) sw.appendChild(switchWidget.firstChild);
  }

  canvas.addEventListener("wheel", (evt) => {
    evt.preventDefault();
    zoomGalleryAt(evt.clientX, evt.clientY, evt.deltaY);
  }, { passive: false });

  canvas.addEventListener("dblclick", () => resetGalleryView());

  canvas.addEventListener("pointerdown", (evt) => {
    if (evt.button !== 0 && evt.button !== 1) return;
    evt.preventDefault();
    if (evt.pointerType === "touch") {
      touchPoints[String(evt.pointerId)] = { x: evt.clientX, y: evt.clientY };
      if (Object.keys(touchPoints).length >= 2) {
        const ids = Object.keys(touchPoints).slice(0, 2);
        const a = touchPoints[ids[0]];
        const b = touchPoints[ids[1]];
        pinchDistance = Math.hypot(a.x - b.x, a.y - b.y);
        pinchCenterX = (a.x + b.x) * 0.5;
        pinchCenterY = (a.y + b.y) * 0.5;
        canvas.setPointerCapture(evt.pointerId);
        return;
      }
    }
    // Minimap click-to-recenter
    if (evt.button === 0 && recenterGalleryFromMinimap(canvas, evt.clientX, evt.clientY, false)) {
      galleryView.panning  = true;
      galleryView.panMode  = "minimap";
      galleryView.pointerId = evt.pointerId;
      galleryView.lastX    = evt.clientX;
      galleryView.lastY    = evt.clientY;
      canvas.setPointerCapture(evt.pointerId);
      return;
    }
    galleryView.panning   = true;
    galleryView.panMode   = "image";
    galleryView.pointerId = evt.pointerId;
    galleryView.lastX     = evt.clientX;
    galleryView.lastY     = evt.clientY;
    canvas.setPointerCapture(evt.pointerId);
    applyGalleryTransform();
  });

  canvas.addEventListener("pointermove", (evt) => {
    if (evt.pointerType === "touch" && Object.prototype.hasOwnProperty.call(touchPoints, String(evt.pointerId))) {
      touchPoints[String(evt.pointerId)] = { x: evt.clientX, y: evt.clientY };
      const ids = Object.keys(touchPoints);
      if (ids.length >= 2) {
        const a = touchPoints[ids[0]];
        const b = touchPoints[ids[1]];
        const dist = Math.max(1e-6, Math.hypot(a.x - b.x, a.y - b.y));
        const centerX = (a.x + b.x) * 0.5;
        const centerY = (a.y + b.y) * 0.5;
        if (pinchDistance > 1e-6) {
          // Zoom around the old pinch center, then pan by center movement.
          const { canvas: cv } = getGalleryViewEls();
          const rect = cv ? cv.getBoundingClientRect() : null;
          if (rect) {
            const c1x = pinchCenterX - rect.left - rect.width * 0.5;
            const c1y = pinchCenterY - rect.top - rect.height * 0.5;
            const nextScale = Math.min(galleryView.maxScale, Math.max(galleryView.minScale, galleryView.scale * (dist / pinchDistance)));
            const k = nextScale / galleryView.scale;
            galleryView.tx = c1x - (c1x - galleryView.tx) * k;
            galleryView.ty = c1y - (c1y - galleryView.ty) * k;
            galleryView.scale = nextScale;
          }
        }
        galleryView.tx += centerX - pinchCenterX;
        galleryView.ty += centerY - pinchCenterY;
        applyGalleryTransform();
        pinchDistance = dist;
        pinchCenterX = centerX;
        pinchCenterY = centerY;
        return;
      }
    }
    if (!galleryView.panning || galleryView.pointerId !== evt.pointerId) return;
    if (galleryView.panMode === "minimap") {
      galleryView.lastX = evt.clientX;
      galleryView.lastY = evt.clientY;
      recenterGalleryFromMinimap(canvas, evt.clientX, evt.clientY, true);
      return;
    }
    const dx = evt.clientX - galleryView.lastX;
    const dy = evt.clientY - galleryView.lastY;
    galleryView.lastX = evt.clientX;
    galleryView.lastY = evt.clientY;
    galleryView.tx += dx;
    galleryView.ty += dy;
    applyGalleryTransform();
  });

  const endPan = (evt) => {
    if (evt.pointerType === "touch") {
      delete touchPoints[String(evt.pointerId)];
      if (Object.keys(touchPoints).length < 2) {
        pinchDistance = 0;
        pinchCenterX = 0;
        pinchCenterY = 0;
      }
    }
    if (!galleryView.panning || galleryView.pointerId !== evt.pointerId) return;
    galleryView.panning  = false;
    galleryView.panMode  = "";
    galleryView.pointerId = null;
    try { canvas.releasePointerCapture(evt.pointerId); } catch (_) {}
    applyGalleryTransform();
  };

  canvas.addEventListener("pointerup",     endPan);
  canvas.addEventListener("pointercancel", endPan);

  const btn = document.getElementById("galleryResetViewBtn");
  if (btn) btn.addEventListener("click", () => resetGalleryView());

  if (typeof ResizeObserver === "function" && wrap && !galleryViewResizeObserver) {
    galleryViewResizeObserver = new ResizeObserver(() => syncGalleryCanvasSize());
    galleryViewResizeObserver.observe(wrap);
  }
}


function formatResolution(w, h) {
  return (w && h) ? `${w} × ${h}` : "-";
}

function galleryThumbUrl(id) {
  return `/api/gallery/${encodeURIComponent(id)}/image?t=${Date.now()}`;
}

function galleryPassThumbUrl(id, passIndex) {
  return `/api/gallery/${encodeURIComponent(id)}/pass/${passIndex}/image?t=${Date.now()}`;
}

function renderGalleryGrid(entries) {
  const grid = document.getElementById("galleryGrid");
  const empty = document.getElementById("galleryEmpty");
  if (!grid) return;

  if (!entries || entries.length === 0) {
    grid.innerHTML = "";
    if (empty) empty.hidden = false;
    return;
  }
  if (empty) empty.hidden = true;

  grid.innerHTML = "";
  entries.forEach((entry) => {
    const card = document.createElement("button");
    card.className = "gallery-card";
    card.type = "button";
    card.setAttribute("aria-label", `View render: ${entry.scene || entry.id}`);

    const thumb = document.createElement("img");
    thumb.className = "gallery-card-thumb";
    thumb.alt = entry.scene || entry.id;
    thumb.loading = "lazy";
    thumb.src = galleryThumbUrl(entry.id);

    const info = document.createElement("div");
    info.className = "gallery-card-info";

    const scene = document.createElement("div");
    scene.className = "gallery-card-scene";
    scene.textContent = entry.scene || entry.id;

    const meta = document.createElement("div");
    meta.className = "gallery-card-meta";
    const parts = [];
    if (entry.integrator) parts.push(entry.integrator);
    if (entry.width && entry.height) parts.push(formatResolution(entry.width, entry.height));
    if (entry.samples) parts.push(`${entry.samples}spp`);
    if (entry.elapsed_ms) parts.push(formatElapsedVerbose(entry.elapsed_ms));
    meta.textContent = parts.join(" · ");

    const mode = document.createElement("div");
    mode.className = "gallery-card-mode";
    if (entry.render_mode) {
      mode.textContent = entry.render_mode;
      if (entry.pass_count > 0) {
        mode.textContent += ` · ${entry.pass_count} pass${entry.pass_count !== 1 ? "es" : ""}`;
      }
    }

    info.appendChild(scene);
    info.appendChild(meta);
    if (entry.render_mode) info.appendChild(mode);
    card.appendChild(thumb);
    card.appendChild(info);

    card.addEventListener("click", () => openGalleryDetail(entry));
    grid.appendChild(card);
  });
}

function openGalleryDetail(entry) {
  galleryDetailId = entry.id;
  galleryCurrentPassIndex = -1;

  const panel = document.getElementById("galleryDetail");
  const grid = document.querySelector(".gallery-panel");
  const img = document.getElementById("galleryDetailImage");
  const title = document.getElementById("galleryDetailTitle");
  const metaDiv = document.getElementById("galleryDetailMeta");
  const passStrip = document.getElementById("galleryPassStrip");
  const passThumbs = document.getElementById("galleryPassThumbs");

  if (grid) grid.hidden = true;
  if (panel) panel.hidden = false;

  const exportFormatSel = document.getElementById("galleryExportFormat");
  if (exportFormatSel && window.XTracerWidgets && typeof window.XTracerWidgets.enhanceSelect === "function") {
    window.XTracerWidgets.enhanceSelect(exportFormatSel);
  }

  initGalleryTmFromEntry(entry);
  updateGalleryExportUi();

  if (title) title.textContent = entry.scene || entry.id;
  if (img) {
    resetGalleryView();
    img.onload = () => {
      resetGalleryView();
      syncGalleryCanvasSize();
    };
    img.src = buildGalleryDetailImageUrl(entry.id, -1);
    img.alt = entry.scene || entry.id;
    if (img.complete && img.naturalWidth) syncGalleryCanvasSize();
  }

  if (metaDiv) {
    const rows = [
      ["Scene",       entry.scene || "-"],
      ["Integrator",  entry.integrator || "-"],
      ["Mode",        entry.render_mode || "-"],
      ["Resolution",  formatResolution(entry.width, entry.height)],
      ["Samples",     entry.samples ? `${entry.samples} spp` : "-"],
      ["AA",          entry.aa != null ? String(entry.aa) : "-"],
      ["Ray depth",   entry.rdepth != null ? String(entry.rdepth) : "-"],
      ["Threads",     entry.threads ? String(entry.threads) : "-"],
      ["Render time", formatElapsedVerbose(entry.elapsed_ms)],
      ["Workspace",   entry.workspace_id || "-"],
    ];
    metaDiv.innerHTML = rows.map(([k, v]) =>
      `<div class="gallery-meta-row"><span class="gallery-meta-key">${k}</span><span class="gallery-meta-val">${v}</span></div>`
    ).join("");
  }

  if (passStrip && passThumbs) {
    if (entry.pass_count > 0) {
      passThumbs.innerHTML = "";
      for (let i = 0; i < entry.pass_count; i++) {
        const btn = document.createElement("button");
        btn.type = "button";
        btn.className = "gallery-pass-thumb-btn";
        btn.setAttribute("aria-label", `View pass ${i + 1}`);
        const t = document.createElement("img");
        t.className = "gallery-pass-thumb";
        t.src = galleryPassThumbUrl(entry.id, i);
        t.loading = "lazy";
        t.alt = `Pass ${i + 1}`;
        btn.appendChild(t);
        const dot = document.createElement("span");
        dot.className = "gallery-pass-thumb-dot";
        dot.setAttribute("aria-hidden", "true");
        btn.appendChild(dot);
        const num = document.createElement("span");
        num.className = "gallery-pass-thumb-num";
        num.textContent = String(i + 1);
        btn.appendChild(num);
        btn.addEventListener("click", () => {
          if (passThumbs._passStripDragged && passThumbs._passStripDragged()) return;
          galleryCurrentPassIndex = i;
          if (img) {
            img.onload = () => {
              resetGalleryView();
              syncGalleryCanvasSize();
            };
            img.src = buildGalleryDetailImageUrl(entry.id, i);
          }
          passThumbs.querySelectorAll(".gallery-pass-thumb-btn").forEach((b) => b.classList.remove("is-active"));
          btn.classList.add("is-active");
        });
        passThumbs.appendChild(btn);
      }
      passStrip.hidden = false;
    } else {
      passStrip.hidden = true;
    }
  }
}

// ── Gallery export ──────────────────────────────────────────────────────────

let galleryExportInFlight = false;

function selectedGalleryExportFormat() {
  const sel = document.getElementById("galleryExportFormat");
  const raw = String(sel && sel.value ? sel.value : "png").toLowerCase();
  if (raw === "png" || raw === "jpg" || raw === "bmp" || raw === "tga" || raw === "exr" || raw === "hdr") return raw;
  return "png";
}

function buildGalleryExportUrl(entryId, passIndex, format) {
  const parts = [`format=${encodeURIComponent(format)}`];
  const isHdrFormat = format === "exr" || format === "hdr";
  if (!isHdrFormat) {
    const op = galleryTm.op;
    parts.push(`tm=${encodeURIComponent(op)}`);
    const spec = toneMappingControlSpec(op);
    if (spec.usesExposure) parts.push(`tm_exposure=${encodeURIComponent(galleryTm.exposure)}`);
    if (spec.usesWhitePoint) parts.push(`tm_white_point=${encodeURIComponent(galleryTm.whitePoint)}`);
    if (spec.usesMantiuk) {
      parts.push(`tm_mantiuk_contrast=${encodeURIComponent(galleryTm.mantiukContrast)}`);
      parts.push(`tm_mantiuk_saturation=${encodeURIComponent(galleryTm.mantiukSaturation)}`);
      parts.push(`tm_mantiuk_detail=${encodeURIComponent(galleryTm.mantiukDetail)}`);
    }
  }
  const qs = `?${parts.join("&")}`;
  if (passIndex >= 0) {
    return `/api/gallery/${encodeURIComponent(entryId)}/pass/${passIndex}/export${qs}`;
  }
  return `/api/gallery/${encodeURIComponent(entryId)}/export${qs}`;
}

function updateGalleryExportUi() {
  const btn = document.getElementById("galleryExportBtn");
  const shell = document.getElementById("galleryExportShell");
  const fmt = selectedGalleryExportFormat().toUpperCase();
  const enabled = !!galleryDetailId && !galleryExportInFlight;
  const busy = galleryExportInFlight;
  if (btn) {
    btn.disabled = !enabled;
    btn.setAttribute("aria-disabled", enabled ? "false" : "true");
    btn.classList.toggle("is-disabled", !enabled);
    btn.classList.toggle("is-busy", busy);
    btn.classList.toggle("is-ready", enabled);
  }
  if (typeof syncRenderExportButtonDecor === "function") syncRenderExportButtonDecor(btn, fmt);
  if (shell) {
    shell.classList.toggle("is-disabled", !enabled);
    shell.classList.toggle("is-busy", busy);
    shell.classList.toggle("is-ready", enabled);
  }
}

async function handleGalleryExportClick() {
  if (!galleryDetailId || galleryExportInFlight) return;
  const format = selectedGalleryExportFormat();
  const url = buildGalleryExportUrl(galleryDetailId, galleryCurrentPassIndex, format);
  galleryExportInFlight = true;
  updateGalleryExportUi();
  try {
    const res = await fetch(url);
    if (!res.ok) throw new Error(`export failed: ${res.status}`);
    const blob = await res.blob();
    const disposition = res.headers.get("Content-Disposition") || "";
    const match = disposition.match(/filename="([^"]+)"/);
    const filename = match ? match[1] : `gallery_export.${format}`;
    const tmp = document.createElement("a");
    tmp.href = URL.createObjectURL(blob);
    tmp.download = filename;
    document.body.appendChild(tmp);
    tmp.click();
    tmp.remove();
    URL.revokeObjectURL(tmp.href);
  } catch (err) {
    if (typeof appendLog === "function") appendLog(`gallery export failed: ${err.message}`);
  } finally {
    galleryExportInFlight = false;
    updateGalleryExportUi();
  }
}

function closeGalleryDetail() {
  galleryDetailId = null;
  updateGalleryExportUi();
  const panel = document.getElementById("galleryDetail");
  const grid = document.querySelector(".gallery-panel");
  if (panel) panel.hidden = true;
  if (grid) grid.hidden = false;
}

async function deleteGalleryEntry(id) {
  try {
    await fetch(`/api/gallery/${encodeURIComponent(id)}`, { method: "DELETE" });
    closeGalleryDetail();
    await refreshGallery();
  } catch (err) {
    if (typeof appendLog === "function") appendLog(`gallery delete error: ${err.message}`);
  }
}

function getGallerySearchQuery() {
  const input = document.getElementById("gallerySearch");
  return input ? input.value.trim().toLowerCase() : "";
}

function filteredGalleryEntries() {
  const q = getGallerySearchQuery();
  if (!q) return galleryEntries;
  return galleryEntries.filter((e) => {
    const scene = (e.scene || e.id || "").toLowerCase();
    const integrator = (e.integrator || "").toLowerCase();
    const mode = (e.render_mode || "").toLowerCase();
    return scene.includes(q) || integrator.includes(q) || mode.includes(q);
  });
}

async function refreshGallery() {
  try {
    const res = await fetch("/api/gallery?t=" + Date.now());
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    galleryEntries = Array.isArray(data.entries) ? data.entries : [];
    renderGalleryGrid(filteredGalleryEntries());
  } catch (err) {
    if (typeof appendLog === "function") appendLog(`gallery fetch error: ${err.message}`);
  }
}

// Wire up static buttons once the DOM is ready
document.addEventListener("DOMContentLoaded", () => {
  const refreshBtn = document.getElementById("galleryRefreshBtn");
  if (refreshBtn) refreshBtn.addEventListener("click", () => refreshGallery());

  const searchInput = document.getElementById("gallerySearch");
  if (searchInput) {
    searchInput.addEventListener("input", () => renderGalleryGrid(filteredGalleryEntries()));
  }

  const backBtn = document.getElementById("galleryDetailBackBtn");
  if (backBtn) backBtn.addEventListener("click", closeGalleryDetail);

  const deleteBtn = document.getElementById("galleryDetailDeleteBtn");
  if (deleteBtn) {
    deleteBtn.addEventListener("click", () => {
      if (galleryDetailId) deleteGalleryEntry(galleryDetailId);
    });
  }

  // ── Tone mapping controls ────────────────────────────────────────────────
  const tmOp = document.getElementById("galleryTmOp");
  if (tmOp) {
    tmOp.addEventListener("change", () => {
      galleryTm.op = tmOp.value;
      syncGalleryTmParamsVisibility();
      reloadGalleryDetailImage();
    });
  }

  const tmExposure = document.getElementById("galleryTmExposure");
  if (tmExposure) {
    tmExposure.addEventListener("change", () => {
      const v = parseFloat(tmExposure.value);
      galleryTm.exposure = Number.isFinite(v) && v > 0 ? v : 1.0;
      reloadGalleryDetailImage();
    });
  }

  const tmWhitePoint = document.getElementById("galleryTmWhitePoint");
  if (tmWhitePoint) {
    tmWhitePoint.addEventListener("change", () => {
      const v = parseFloat(tmWhitePoint.value);
      galleryTm.whitePoint = Number.isFinite(v) && v > 0 ? v : 1.0;
      reloadGalleryDetailImage();
    });
  }

  const tmMantiukContrast = document.getElementById("galleryTmMantiukContrast");
  if (tmMantiukContrast) {
    tmMantiukContrast.addEventListener("change", () => {
      const v = parseFloat(tmMantiukContrast.value);
      galleryTm.mantiukContrast = Number.isFinite(v) ? Math.min(1.0, Math.max(0.0, v)) : 0.1;
      reloadGalleryDetailImage();
    });
  }

  const tmMantiukSaturation = document.getElementById("galleryTmMantiukSaturation");
  if (tmMantiukSaturation) {
    tmMantiukSaturation.addEventListener("change", () => {
      const v = parseFloat(tmMantiukSaturation.value);
      galleryTm.mantiukSaturation = Number.isFinite(v) ? Math.min(2.0, Math.max(0.0, v)) : 0.8;
      reloadGalleryDetailImage();
    });
  }

  const tmMantiukDetail = document.getElementById("galleryTmMantiukDetail");
  if (tmMantiukDetail) {
    tmMantiukDetail.addEventListener("change", () => {
      const v = parseFloat(tmMantiukDetail.value);
      galleryTm.mantiukDetail = Number.isFinite(v) ? Math.min(99.0, Math.max(1.0, v)) : 1.0;
      reloadGalleryDetailImage();
    });
  }

  // ── Export controls ──────────────────────────────────────────────────────
  const exportBtn = document.getElementById("galleryExportBtn");
  if (exportBtn) {
    if (typeof decorateRenderExportButton === "function") decorateRenderExportButton(exportBtn);
    exportBtn.addEventListener("click", handleGalleryExportClick);
  }

  const exportFormat = document.getElementById("galleryExportFormat");
  if (exportFormat) exportFormat.addEventListener("change", updateGalleryExportUi);

  // ── Pass strip drag-to-scroll ────────────────────────────────────────────
  const passThumbsEl = document.getElementById("galleryPassThumbs");
  if (passThumbsEl) {
    let dragging = false;
    let startX = 0;
    let scrollStart = 0;
    let moved = false;
    passThumbsEl._passStripDragged = () => moved;

    passThumbsEl.addEventListener("pointerdown", (e) => {
      if (e.button !== 0) return;
      dragging = true;
      moved = false;
      startX = e.clientX;
      scrollStart = passThumbsEl.scrollLeft;
      passThumbsEl.setPointerCapture(e.pointerId);
      passThumbsEl.classList.add("is-dragging");
    });

    passThumbsEl.addEventListener("pointermove", (e) => {
      if (!dragging) return;
      const dx = e.clientX - startX;
      if (Math.abs(dx) > 4) moved = true;
      passThumbsEl.scrollLeft = scrollStart - dx;
    });

    const endDrag = () => {
      dragging = false;
      passThumbsEl.classList.remove("is-dragging");
    };
    passThumbsEl.addEventListener("pointerup", endDrag);
    passThumbsEl.addEventListener("pointercancel", endDrag);
  }

  bindGalleryViewEvents();
});
