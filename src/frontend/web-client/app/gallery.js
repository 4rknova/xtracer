// Gallery — fetches and displays cached renders from /api/gallery

let galleryEntries = [];
let galleryDetailId = null;

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

  const panel = document.getElementById("galleryDetail");
  const grid = document.querySelector(".gallery-panel");
  const img = document.getElementById("galleryDetailImage");
  const title = document.getElementById("galleryDetailTitle");
  const metaDiv = document.getElementById("galleryDetailMeta");
  const passStrip = document.getElementById("galleryPassStrip");
  const passThumbs = document.getElementById("galleryPassThumbs");

  if (grid) grid.hidden = true;
  if (panel) panel.hidden = false;

  if (title) title.textContent = entry.scene || entry.id;
  if (img) {
    resetGalleryView();
    img.onload = () => {
      resetGalleryView();
      syncGalleryCanvasSize();
    };
    img.src = galleryThumbUrl(entry.id);
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
        btn.addEventListener("click", () => {
          if (img) {
            img.onload = () => {
              resetGalleryView();
              syncGalleryCanvasSize();
            };
            img.src = galleryPassThumbUrl(entry.id, i);
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

function closeGalleryDetail() {
  galleryDetailId = null;
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

async function refreshGallery() {
  try {
    const res = await fetch("/api/gallery?t=" + Date.now());
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    galleryEntries = Array.isArray(data.entries) ? data.entries : [];
    renderGalleryGrid(galleryEntries);
  } catch (err) {
    if (typeof appendLog === "function") appendLog(`gallery fetch error: ${err.message}`);
  }
}

// Wire up static buttons once the DOM is ready
document.addEventListener("DOMContentLoaded", () => {
  const refreshBtn = document.getElementById("galleryRefreshBtn");
  if (refreshBtn) refreshBtn.addEventListener("click", () => refreshGallery());

  const backBtn = document.getElementById("galleryDetailBackBtn");
  if (backBtn) backBtn.addEventListener("click", closeGalleryDetail);

  const deleteBtn = document.getElementById("galleryDetailDeleteBtn");
  if (deleteBtn) {
    deleteBtn.addEventListener("click", () => {
      if (galleryDetailId) deleteGalleryEntry(galleryDetailId);
    });
  }

  bindGalleryViewEvents();
});
