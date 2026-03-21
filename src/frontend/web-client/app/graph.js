function ensureGraphCanvasSize() {
  if (!el.graphCanvas) return null;
  const cssW = Math.max(1, Math.floor(el.graphCanvas.clientWidth));
  const cssH = Math.max(1, Math.floor(el.graphCanvas.clientHeight));
  const dpr = window.devicePixelRatio || 1;
  const pxW = Math.max(1, Math.floor(cssW * dpr));
  const pxH = Math.max(1, Math.floor(cssH * dpr));
  if (el.graphCanvas.width !== pxW || el.graphCanvas.height !== pxH) {
    el.graphCanvas.width = pxW;
    el.graphCanvas.height = pxH;
  }
  return { cssW, cssH, dpr };
}

function getGraphTexturePreview(url) {
  const key = String(url || "").trim();
  if (!key) return null;
  let entry = graphTexturePreviewCache.get(key);
  if (entry) return entry;
  const img = new Image();
  entry = { img, loaded: false, failed: false };
  graphTexturePreviewCache.set(key, entry);
  img.onload = () => {
    entry.loaded = true;
    drawGraphCanvas();
  };
  img.onerror = () => {
    entry.failed = true;
    drawGraphCanvas();
  };
  img.src = key;
  return entry;
}

function formatGraphNumeric(v) {
  const n = Number(v);
  if (!Number.isFinite(n)) return String(v ?? "-");
  return Number.isInteger(n) ? String(n) : n.toFixed(3).replace(/\.?0+$/, "");
}

function graphColorLabel(rgb) {
  if (!Array.isArray(rgb) || rgb.length < 3) return "-";
  const fmt = (v) => {
    const n = Number(v);
    return Number.isFinite(n) ? n.toFixed(3) : "0.000";
  };
  return `${fmt(rgb[0])}, ${fmt(rgb[1])}, ${fmt(rgb[2])}`;
}

function graphVec3Label(v) {
  if (!Array.isArray(v) || v.length < 3) return "-";
  return `${formatGraphNumeric(v[0])}, ${formatGraphNumeric(v[1])}, ${formatGraphNumeric(v[2])}`;
}

function drawGraphFittedText(ctx, text, x, y, maxWidth) {
  const raw = String(text ?? "");
  const limit = Number(maxWidth) || 0;
  if (!ctx || limit <= 0) return;
  if (ctx.measureText(raw).width <= limit) {
    ctx.fillText(raw, x, y);
    return;
  }
  const ell = "...";
  let lo = 0;
  let hi = raw.length;
  while (lo < hi) {
    const mid = Math.ceil((lo + hi) / 2);
    const probe = `${raw.slice(0, mid)}${ell}`;
    if (ctx.measureText(probe).width <= limit) lo = mid;
    else hi = mid - 1;
  }
  const out = `${raw.slice(0, Math.max(0, lo))}${ell}`;
  ctx.fillText(out, x, y);
}

function drawCsgOperantIcon(ctx, cx, cy, op, dimmed) {
  if (!ctx) return;
  const leftX = cx - 6;
  const rightX = cx + 6;
  const radius = 8;
  const iconPath = new Path2D();
  iconPath.arc(leftX, cy, radius, 0, Math.PI * 2);
  iconPath.arc(rightX, cy, radius, 0, Math.PI * 2);

  const leftPath = new Path2D();
  leftPath.arc(leftX, cy, radius, 0, Math.PI * 2);
  const rightPath = new Path2D();
  rightPath.arc(rightX, cy, radius, 0, Math.PI * 2);

  const normOp = String(op || "union").toLowerCase();
  const alpha = dimmed ? 0.45 : 0.9;

  ctx.save();
  ctx.globalAlpha = alpha;
  if (normOp === "intersection") {
    ctx.save();
    ctx.clip(leftPath);
    ctx.clip(rightPath);
    ctx.fillStyle = "rgba(150,204,255,0.66)";
    ctx.fillRect(cx - 24, cy - 14, 48, 28);
    ctx.restore();
  } else if (normOp === "difference") {
    ctx.save();
    ctx.clip(leftPath);
    ctx.globalCompositeOperation = "destination-out";
    ctx.fill(rightPath);
    ctx.restore();
    ctx.fillStyle = "rgba(150,204,255,0.5)";
    ctx.fill(leftPath);
  } else {
    // union + soft_union share the same glyph.
    ctx.fillStyle = "rgba(150,204,255,0.32)";
    ctx.fill(leftPath);
    ctx.fill(rightPath);
  }

  ctx.strokeStyle = dimmed ? "rgba(170,188,205,0.5)" : "rgba(212,230,246,0.92)";
  ctx.lineWidth = 1.1;
  ctx.stroke(leftPath);
  ctx.stroke(rightPath);
  ctx.restore();
}

function drawGraphCanvas() {
  if (!el.graphCanvas) return;
  const dims = ensureGraphCanvasSize();
  const ctx = el.graphCanvas.getContext("2d");
  if (!dims || !ctx) return;

  ctx.setTransform(1, 0, 0, 1, 0, 0);
  ctx.clearRect(0, 0, el.graphCanvas.width, el.graphCanvas.height);

  const data = graphView.data;
  if (!data) return;

  const hoveredKey = graphView.hoverKey || "";
  const linkedKeys = new Set();
  if (hoveredKey) {
    linkedKeys.add(hoveredKey);
    (data.links || []).forEach((ln) => {
      if (ln.from.key === hoveredKey) linkedKeys.add(ln.to.key);
      if (ln.to.key === hoveredKey) linkedKeys.add(ln.from.key);
    });
  }

  const { cssW, cssH, dpr } = dims;
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.fillStyle = "rgba(9,15,21,0.22)";
  ctx.fillRect(0, 0, cssW, cssH);

  // Miro-like infinite grid in screen space derived from world transform.
  const baseStep = 40;
  const step = baseStep * graphView.scale;
  if (step >= 12) {
    const ox = ((graphView.tx % step) + step) % step;
    const oy = ((graphView.ty % step) + step) % step;
    ctx.beginPath();
    ctx.strokeStyle = "rgba(120,145,170,0.14)";
    ctx.lineWidth = 1;
    for (let x = ox; x <= cssW; x += step) {
      ctx.moveTo(x, 0);
      ctx.lineTo(x, cssH);
    }
    for (let y = oy; y <= cssH; y += step) {
      ctx.moveTo(0, y);
      ctx.lineTo(cssW, y);
    }
    ctx.stroke();
  }

  ctx.save();
  ctx.translate(graphView.tx, graphView.ty);
  ctx.scale(graphView.scale, graphView.scale);

  ctx.fillStyle = "rgba(9,15,21,0.22)";
  ctx.fillRect(0, 0, data.viewW, data.viewH);

  ctx.font = "700 14px IBM Plex Sans, sans-serif";
  data.columns.forEach((c) => {
    ctx.fillStyle = c.color;
    ctx.fillText(String(c.title || "").toUpperCase(), c.x, 36);
  });

  data.links.forEach((ln) => {
    const x1 = ln.from.x + ln.from.w;
    const y1 = ln.from.y + ln.from.h / 2;
    const x2 = ln.to.x;
    const y2 = ln.to.y + ln.to.h / 2;
    const c1 = x1 + 60;
    const c2 = x2 - 60;
    const active = !!hoveredKey && (ln.from.key === hoveredKey || ln.to.key === hoveredKey);
    ctx.strokeStyle = active ? "rgba(238,246,255,0.92)" : "rgba(179,195,214,0.55)";
    ctx.lineWidth = active ? 2.8 : 2;
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.bezierCurveTo(c1, y1, c2, y2, x2, y2);
    ctx.stroke();
  });

  data.nodes.forEach((n) => {
    const isHovered = hoveredKey && n.key === hoveredKey;
    const isLinked = hoveredKey && linkedKeys.has(n.key);
    const dimmed = hoveredKey && !isLinked;
    const r = 9;
    ctx.beginPath();
    ctx.moveTo(n.x + r, n.y);
    ctx.lineTo(n.x + n.w - r, n.y);
    ctx.quadraticCurveTo(n.x + n.w, n.y, n.x + n.w, n.y + r);
    ctx.lineTo(n.x + n.w, n.y + n.h - r);
    ctx.quadraticCurveTo(n.x + n.w, n.y + n.h, n.x + n.w - r, n.y + n.h);
    ctx.lineTo(n.x + r, n.y + n.h);
    ctx.quadraticCurveTo(n.x, n.y + n.h, n.x, n.y + n.h - r);
    ctx.lineTo(n.x, n.y + r);
    ctx.quadraticCurveTo(n.x, n.y, n.x + r, n.y);
    ctx.closePath();
    ctx.fillStyle = isHovered
      ? "rgba(46,71,99,0.96)"
      : (dimmed ? "rgba(15,22,29,0.55)" : "rgba(17,27,36,0.86)");
    ctx.fill();
    ctx.strokeStyle = isHovered ? "#ecf6ff" : n.color;
    ctx.globalAlpha = isHovered ? 1 : (dimmed ? 0.35 : 0.68);
    ctx.lineWidth = isHovered ? 2.2 : 1.2;
    ctx.stroke();
    ctx.globalAlpha = 1;

    ctx.fillStyle = dimmed ? "rgba(180,194,208,0.45)" : "#e6eff7";
    ctx.font = "600 13px IBM Plex Sans, sans-serif";
    const hasCsgIcon = n.kind === "geometry" && !!n.csgOp;
    if (hasCsgIcon) drawCsgOperantIcon(ctx, n.x + n.w - 20, n.y + 16, n.csgOp, dimmed);
    const titleMaxWidth = hasCsgIcon ? (n.w - 46) : (n.w - 20);
    drawGraphFittedText(ctx, n.id, n.x + 10, n.y + 20, titleMaxWidth);

    ctx.fillStyle = dimmed ? "rgba(145,160,176,0.42)" : "rgba(170,186,202,0.9)";
    ctx.font = "11px IBM Plex Sans, sans-serif";
    drawGraphFittedText(ctx, n.subtitle, n.x + 10, n.y + 37, titleMaxWidth);

    if (n.expanded) {
      const rows = Array.isArray(n.propertyRows) ? n.propertyRows : [];
      const rowH = 18;
      const tableTop = n.y + 46;
      const tableH = rows.length * rowH;
      const keyColW = 74;

      if (tableH > 0) {
        ctx.fillStyle = "rgba(8,13,19,0.45)";
        ctx.fillRect(n.x + 6, tableTop, n.w - 12, tableH);
        ctx.strokeStyle = "rgba(140,160,182,0.26)";
        ctx.lineWidth = 1;
        ctx.strokeRect(n.x + 6.5, tableTop + 0.5, n.w - 13, tableH - 1);

        rows.forEach((row, idx) => {
          const y0 = tableTop + idx * rowH;
          if (idx > 0) {
            ctx.strokeStyle = "rgba(130,152,176,0.18)";
            ctx.beginPath();
            ctx.moveTo(n.x + 7, y0 + 0.5);
            ctx.lineTo(n.x + n.w - 7, y0 + 0.5);
            ctx.stroke();
          }
          ctx.strokeStyle = "rgba(130,152,176,0.14)";
          ctx.beginPath();
          ctx.moveTo(n.x + 6 + keyColW, y0 + 1);
          ctx.lineTo(n.x + 6 + keyColW, y0 + rowH - 1);
          ctx.stroke();

          ctx.fillStyle = dimmed ? "rgba(150,166,180,0.42)" : "rgba(162,182,202,0.92)";
          ctx.font = "600 9px IBM Plex Sans, sans-serif";
          drawGraphFittedText(ctx, String(row && row.key ? row.key : "-").toUpperCase(), n.x + 11, y0 + 12, keyColW - 10);

          const valueX = n.x + 12 + keyColW;
          const valueMaxW = Math.max(8, n.w - (valueX - n.x) - 10);
          if (row && Array.isArray(row.color) && row.color.length >= 3) {
            const r = clamp(Number(row.color[0]) || 0, 0, 1);
            const g = clamp(Number(row.color[1]) || 0, 0, 1);
            const b = clamp(Number(row.color[2]) || 0, 0, 1);
            const sw = 12;
            const sh = 12;
            const sy = y0 + 3;
            ctx.fillStyle = `rgb(${Math.round(r * 255)}, ${Math.round(g * 255)}, ${Math.round(b * 255)})`;
            ctx.fillRect(valueX, sy, sw, sh);
            ctx.strokeStyle = "rgba(214,228,240,0.64)";
            ctx.lineWidth = 1;
            ctx.strokeRect(valueX + 0.5, sy + 0.5, sw - 1, sh - 1);
            ctx.fillStyle = dimmed ? "rgba(172,186,198,0.46)" : "rgba(206,220,234,0.95)";
            ctx.font = "10px IBM Plex Sans, sans-serif";
            drawGraphFittedText(ctx, graphColorLabel(row.color), valueX + sw + 6, y0 + 13, Math.max(8, valueMaxW - sw - 6));
          } else {
            ctx.fillStyle = dimmed ? "rgba(172,186,198,0.46)" : "rgba(206,220,234,0.95)";
            ctx.font = "10px IBM Plex Sans, sans-serif";
            drawGraphFittedText(ctx, String(row && row.value !== undefined ? row.value : "-"), valueX, y0 + 13, valueMaxW);
          }
        });
      }

      const previews = Array.isArray(n.texturePreviews) ? n.texturePreviews.slice(0, 3) : [];
      if (previews.length > 0) {
        const sw = 40;
        const sh = 40;
        const gap = 8;
        const py = tableTop + tableH + 6;
        previews.forEach((p, idx) => {
          const px = n.x + 10 + idx * (sw + gap);
          ctx.fillStyle = "rgba(12,18,24,0.7)";
          ctx.fillRect(px, py, sw, sh);
          ctx.strokeStyle = "rgba(165,182,198,0.65)";
          ctx.lineWidth = 1;
          ctx.strokeRect(px + 0.5, py + 0.5, sw - 1, sh - 1);

          const preview = getGraphTexturePreview(p && p.url ? p.url : "");
          if (preview && preview.loaded && preview.img) {
            ctx.drawImage(preview.img, px, py, sw, sh);
          } else {
            ctx.fillStyle = "rgba(168,184,200,0.72)";
            ctx.font = "9px IBM Plex Sans, sans-serif";
            ctx.fillText("tex", px + 11, py + 22);
          }
        });
      }
    }
  });

  ctx.restore();
  el.graphCanvas.classList.toggle("is-panning", !!graphView.panning);
}

function scheduleGraphRender() {
  if (!el.graphCanvas) return;
  if (graphRenderRafPrimary) {
    cancelAnimationFrame(graphRenderRafPrimary);
    graphRenderRafPrimary = 0;
  }
  if (graphRenderRafSecondary) {
    cancelAnimationFrame(graphRenderRafSecondary);
    graphRenderRafSecondary = 0;
  }
  graphRenderRafPrimary = requestAnimationFrame(() => {
    graphRenderRafPrimary = 0;
    graphRenderRafSecondary = requestAnimationFrame(() => {
      graphRenderRafSecondary = 0;
      if (activeTabMode !== "visual" || editorViewMode !== "graph") return;
      renderSceneGraphView();
    });
  });
}

function applyGraphTransform() {
  drawGraphCanvas();
}

function graphWorldPointFromClient(clientX, clientY) {
  if (!el.graphCanvas) return null;
  const rect = el.graphCanvas.getBoundingClientRect();
  const x = (clientX - rect.left - graphView.tx) / graphView.scale;
  const y = (clientY - rect.top - graphView.ty) / graphView.scale;
  return { x, y };
}

function findGraphNodeAt(clientX, clientY) {
  const data = graphView.data;
  if (!data || !data.nodes || !data.nodes.length) return "";
  const p = graphWorldPointFromClient(clientX, clientY);
  if (!p) return "";
  for (let i = data.nodes.length - 1; i >= 0; i -= 1) {
    const n = data.nodes[i];
    if (p.x >= n.x && p.x <= n.x + n.w && p.y >= n.y && p.y <= n.y + n.h) return n.key;
  }
  return "";
}

function findGraphNodeData(key) {
  const data = graphView.data;
  if (!data || !Array.isArray(data.nodes) || !key) return null;
  for (let i = 0; i < data.nodes.length; i += 1) {
    if (data.nodes[i] && data.nodes[i].key === key) return data.nodes[i];
  }
  return null;
}

function isGraphExpandableNode(key) {
  const raw = String(key || "");
  return raw.indexOf("camera:") === 0
    || raw.indexOf("object:") === 0
    || raw.indexOf("geometry:") === 0
    || raw.indexOf("material:") === 0;
}

function fitGraphToViewport() {
  if (!el.graphCanvas || !graphView.worldW || !graphView.worldH) return;
  const rect = el.graphCanvas.getBoundingClientRect();
  const cw = Math.max(1, rect.width);
  const ch = Math.max(1, rect.height);
  const fit = Math.min(cw / graphView.worldW, ch / graphView.worldH);
  graphView.scale = clamp(fit, graphView.minScale, graphView.maxScale);
  graphView.tx = (cw - graphView.worldW * graphView.scale) * 0.5;
  graphView.ty = (ch - graphView.worldH * graphView.scale) * 0.5;
}

function resetGraphView() {
  graphView.userAdjusted = false;
  fitGraphToViewport();
  applyGraphTransform();
}

function bindGraphInteraction() {
  if (!el.graphCanvas || graphView.bound) return;
  graphView.bound = true;

  const setTouchPoint = (pointerId, x, y) => {
    graphView.touchPoints[String(pointerId)] = { id: pointerId, x, y };
  };
  const removeTouchPoint = (pointerId) => {
    delete graphView.touchPoints[String(pointerId)];
  };
  const touchPointCount = () => Object.keys(graphView.touchPoints).length;
  const collectTouchPoints = (limit) => {
    const out = [];
    const max = Number.isFinite(limit) ? limit : 2;
    for (const key in graphView.touchPoints) {
      if (!Object.prototype.hasOwnProperty.call(graphView.touchPoints, key)) continue;
      out.push(graphView.touchPoints[key]);
      if (out.length >= max) break;
    }
    return out;
  };
  const clearPrimaryPointerState = () => {
    graphView.pointerDown = false;
    graphView.pointerDownNodeKey = "";
    graphView.panning = false;
    graphView.pointerId = null;
    graphView.dragNodeKey = "";
    graphView.dragNodeOffsetX = 0;
    graphView.dragNodeOffsetY = 0;
    graphView.movedSincePointerDown = false;
  };
  const zoomGraphAtClient = (clientX, clientY, factor) => {
    if (!graphView.worldW || !graphView.worldH) return false;
    const rect = el.graphCanvas.getBoundingClientRect();
    const cx = clientX - rect.left;
    const cy = clientY - rect.top;
    const nextScale = clamp(graphView.scale * factor, graphView.minScale, graphView.maxScale);
    if (!Number.isFinite(nextScale) || Math.abs(nextScale - graphView.scale) < 1e-6) return false;
    const ratio = nextScale / graphView.scale;
    graphView.tx = cx - (cx - graphView.tx) * ratio;
    graphView.ty = cy - (cy - graphView.ty) * ratio;
    graphView.scale = nextScale;
    graphView.userAdjusted = true;
    return true;
  };
  const beginTouchPinch = () => {
    const pts = collectTouchPoints(2);
    if (pts.length < 2) return false;
    const cx = (pts[0].x + pts[1].x) * 0.5;
    const cy = (pts[0].y + pts[1].y) * 0.5;
    const dist = Math.hypot(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
    graphView.pinchActive = true;
    graphView.pinchLastCenterX = cx;
    graphView.pinchLastCenterY = cy;
    graphView.pinchLastDistance = dist;
    clearPrimaryPointerState();
    return true;
  };
  const updateTouchPinch = () => {
    const pts = collectTouchPoints(2);
    if (pts.length < 2) return false;
    const cx = (pts[0].x + pts[1].x) * 0.5;
    const cy = (pts[0].y + pts[1].y) * 0.5;
    const dist = Math.hypot(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
    if (!Number.isFinite(dist) || dist <= 0.01) return false;

    let changed = false;
    const panDx = cx - graphView.pinchLastCenterX;
    const panDy = cy - graphView.pinchLastCenterY;
    if (Math.abs(panDx) + Math.abs(panDy) > 0.01) {
      graphView.tx += panDx;
      graphView.ty += panDy;
      graphView.userAdjusted = true;
      changed = true;
    }

    if (graphView.pinchLastDistance > 0.01) {
      const zoomFactor = dist / graphView.pinchLastDistance;
      if (Number.isFinite(zoomFactor) && Math.abs(zoomFactor - 1) > 0.001) {
        changed = zoomGraphAtClient(cx, cy, zoomFactor) || changed;
      }
    }

    graphView.pinchLastCenterX = cx;
    graphView.pinchLastCenterY = cy;
    graphView.pinchLastDistance = dist;
    return changed;
  };
  const endPrimaryPointer = (evt) => {
    const wasPanning = !!graphView.panning;
    const wasDraggingNode = !!graphView.dragNodeKey;
    const moved = !!graphView.movedSincePointerDown;
    const downNodeKey = String(graphView.pointerDownNodeKey || "");
    clearPrimaryPointerState();
    try {
      el.graphCanvas.releasePointerCapture(evt.pointerId);
    } catch (_) {
      // ignore release errors
    }
    if (!wasPanning && !wasDraggingNode && evt.type === "pointerup" && evt.button === 0) {
      const upNodeKey = findGraphNodeAt(evt.clientX, evt.clientY);
      if (upNodeKey && upNodeKey === downNodeKey && isGraphExpandableNode(upNodeKey)) {
        if (graphView.expandedNodeKeys.has(upNodeKey)) graphView.expandedNodeKeys.delete(upNodeKey);
        else graphView.expandedNodeKeys.add(upNodeKey);
        renderSceneGraphView();
        return true;
      }
    } else if (wasDraggingNode && !moved && evt.type === "pointerup" && evt.button === 0) {
      const upNodeKey = findGraphNodeAt(evt.clientX, evt.clientY);
      if (upNodeKey && upNodeKey === downNodeKey && isGraphExpandableNode(upNodeKey)) {
        if (graphView.expandedNodeKeys.has(upNodeKey)) graphView.expandedNodeKeys.delete(upNodeKey);
        else graphView.expandedNodeKeys.add(upNodeKey);
        renderSceneGraphView();
        return true;
      }
    }
    return false;
  };

  el.graphCanvas.addEventListener("wheel", (evt) => {
    if (!graphView.worldW || !graphView.worldH) return;
    evt.preventDefault();
    const k = Math.exp((-evt.deltaY) * 0.0015);
    if (!zoomGraphAtClient(evt.clientX, evt.clientY, k)) return;
    applyGraphTransform();
  }, { passive: false });

  el.graphCanvas.addEventListener("dblclick", (evt) => {
    evt.preventDefault();
    resetGraphView();
  });

  el.graphCanvas.addEventListener("pointerdown", (evt) => {
    if (evt.pointerType === "touch") {
      evt.preventDefault();
      setTouchPoint(evt.pointerId, evt.clientX, evt.clientY);
      try {
        el.graphCanvas.setPointerCapture(evt.pointerId);
      } catch (_) {
        // ignore capture errors
      }
      if (touchPointCount() >= 2) {
        beginTouchPinch();
        applyGraphTransform();
        return;
      }
    }
    if (evt.button !== 0 && evt.button !== 1) return;
    evt.preventDefault();
    graphView.pointerDown = true;
    graphView.pointerId = evt.pointerId;
    graphView.pointerDownNodeKey = findGraphNodeAt(evt.clientX, evt.clientY);
    graphView.dragStartX = evt.clientX;
    graphView.dragStartY = evt.clientY;
    graphView.lastX = evt.clientX;
    graphView.lastY = evt.clientY;
    graphView.dragNodeKey = "";
    graphView.dragNodeOffsetX = 0;
    graphView.dragNodeOffsetY = 0;
    graphView.movedSincePointerDown = false;
    if (evt.button === 0 && graphView.pointerDownNodeKey) {
      const node = findGraphNodeData(graphView.pointerDownNodeKey);
      const p = graphWorldPointFromClient(evt.clientX, evt.clientY);
      if (node && p) {
        graphView.dragNodeKey = node.key;
        graphView.dragNodeOffsetX = p.x - node.x;
        graphView.dragNodeOffsetY = p.y - node.y;
      }
      graphView.panning = false;
    } else {
      graphView.panning = true;
    }
    el.graphCanvas.setPointerCapture(evt.pointerId);
    applyGraphTransform();
  });

  el.graphCanvas.addEventListener("pointermove", (evt) => {
    if (evt.pointerType === "touch") {
      if (!Object.prototype.hasOwnProperty.call(graphView.touchPoints, String(evt.pointerId))) return;
      evt.preventDefault();
      setTouchPoint(evt.pointerId, evt.clientX, evt.clientY);
      if (touchPointCount() >= 2) {
        if (!graphView.pinchActive) beginTouchPinch();
        if (updateTouchPinch()) applyGraphTransform();
        return;
      }
      if (graphView.pinchActive) {
        graphView.pinchActive = false;
        graphView.pinchLastDistance = 0;
      }
      if (!(graphView.pointerDown && graphView.pointerId === evt.pointerId)) return;
      if (!graphView.panning) {
        const moveX = Math.abs(evt.clientX - graphView.dragStartX);
        const moveY = Math.abs(evt.clientY - graphView.dragStartY);
        if ((moveX + moveY) > 4) graphView.movedSincePointerDown = true;
      }
      if (graphView.dragNodeKey) {
        const node = findGraphNodeData(graphView.dragNodeKey);
        const p = graphWorldPointFromClient(evt.clientX, evt.clientY);
        if (node && p) {
          node.x = p.x - graphView.dragNodeOffsetX;
          node.y = p.y - graphView.dragNodeOffsetY;
          graphView.userAdjusted = true;
          if (node.manualKey) {
            graphView.manualNodePos.set(node.manualKey, { x: node.x, y: node.y });
          }
          applyGraphTransform();
          return;
        }
      }
      const dx = evt.clientX - graphView.lastX;
      const dy = evt.clientY - graphView.lastY;
      graphView.lastX = evt.clientX;
      graphView.lastY = evt.clientY;
      graphView.tx += dx;
      graphView.ty += dy;
      graphView.userAdjusted = true;
      applyGraphTransform();
      return;
    }
    if (graphView.pointerDown && graphView.pointerId === evt.pointerId) {
      if (!graphView.panning) {
        const moveX = Math.abs(evt.clientX - graphView.dragStartX);
        const moveY = Math.abs(evt.clientY - graphView.dragStartY);
        if ((moveX + moveY) > 4) graphView.movedSincePointerDown = true;
      }
      if (graphView.dragNodeKey) {
        const node = findGraphNodeData(graphView.dragNodeKey);
        const p = graphWorldPointFromClient(evt.clientX, evt.clientY);
        if (node && p) {
          node.x = p.x - graphView.dragNodeOffsetX;
          node.y = p.y - graphView.dragNodeOffsetY;
          graphView.userAdjusted = true;
          if (node.manualKey) {
            graphView.manualNodePos.set(node.manualKey, { x: node.x, y: node.y });
          }
          applyGraphTransform();
          return;
        }
      }
    }
    if (graphView.panning && graphView.pointerId === evt.pointerId) {
      const dx = evt.clientX - graphView.lastX;
      const dy = evt.clientY - graphView.lastY;
      graphView.lastX = evt.clientX;
      graphView.lastY = evt.clientY;
      graphView.tx += dx;
      graphView.ty += dy;
      graphView.userAdjusted = true;
      applyGraphTransform();
      return;
    }

    const nextHover = findGraphNodeAt(evt.clientX, evt.clientY);
    if (nextHover === graphView.hoverKey) return;
    graphView.hoverKey = nextHover;
    drawGraphCanvas();
  });

  const endPan = (evt) => {
    if (evt.pointerType === "touch") {
      evt.preventDefault();
      const tracked = Object.prototype.hasOwnProperty.call(graphView.touchPoints, String(evt.pointerId));
      if (tracked) removeTouchPoint(evt.pointerId);
      try {
        el.graphCanvas.releasePointerCapture(evt.pointerId);
      } catch (_) {
        // ignore release errors
      }
      if (graphView.pinchActive) {
        if (touchPointCount() >= 2) {
          beginTouchPinch();
        } else {
          graphView.pinchActive = false;
          graphView.pinchLastDistance = 0;
        }
      }
      if (!graphView.pointerDown || graphView.pointerId !== evt.pointerId) {
        applyGraphTransform();
        return;
      }
      if (endPrimaryPointer(evt)) return;
      applyGraphTransform();
      return;
    }

    if (!graphView.pointerDown || graphView.pointerId !== evt.pointerId) return;
    if (endPrimaryPointer(evt)) return;
    applyGraphTransform();
  };

  el.graphCanvas.addEventListener("pointerup", endPan);
  el.graphCanvas.addEventListener("pointercancel", endPan);
  el.graphCanvas.addEventListener("pointerleave", (evt) => {
    endPan(evt);
    if (!graphView.hoverKey) return;
    graphView.hoverKey = "";
    drawGraphCanvas();
  });
}

function renderSceneGraphView() {
  if (!el.graphCanvas) return;

  const sceneName = String(el.scene && el.scene.value ? el.scene.value : "").trim();
  const runtime = sceneName
    ? ((typeof getRuntimeGraphForScene === "function")
      ? getRuntimeGraphForScene(sceneName)
      : runtimeGraphByScene.get(sceneName))
    : null;
  let cameras = [];
  let objects = [];
  let geometries = [];
  let materials = [];

  if (runtime) {
    cameras = (runtime.cameras || [])
      .map((c) => ({
        id: String((c && c.id) || "").trim(),
        type: String((c && c.type) || "camera"),
        position: Array.isArray(c && c.position) ? c.position.slice(0, 3).map((v) => Number(v) || 0) : null,
        target: Array.isArray(c && c.target) ? c.target.slice(0, 3).map((v) => Number(v) || 0) : null,
        up: Array.isArray(c && c.up) ? c.up.slice(0, 3).map((v) => Number(v) || 0) : null,
        orientation: Array.isArray(c && c.orientation) ? c.orientation.slice(0, 3).map((v) => Number(v) || 0) : null,
        fov: Number.isFinite(Number(c && c.fov)) ? Number(c.fov) : null,
        aperture: Number.isFinite(Number(c && c.aperture)) ? Number(c.aperture) : null,
        flength: Number.isFinite(Number(c && c.flength)) ? Number(c.flength) : null,
        ipd: Number.isFinite(Number(c && c.ipd)) ? Number(c.ipd) : null,
      }))
      .filter((c) => c.id)
      .sort((a, b) => a.id.localeCompare(b.id));
    objects = (runtime.objects || [])
      .map((o) => ({
        id: String((o && o.id) || "").trim(),
        geometry: String((o && o.surface) || "").trim(),
        material: String((o && o.material) || "").trim(),
      }))
      .filter((o) => o.id)
      .sort((a, b) => a.id.localeCompare(b.id));
    geometries = (runtime.surfaces || [])
      .map((g) => ({
        id: String((g && g.id) || "").trim(),
        type: String((g && g.type) || "surface"),
        op: String((g && g.op) || "").trim(),
        smoothness: Number.isFinite(Number(g && g.smoothness)) ? Number(g.smoothness) : null,
        left_type: String((g && g.left_type) || "").trim(),
        right_type: String((g && g.right_type) || "").trim(),
        position: Array.isArray(g && g.position) ? g.position.slice(0, 3).map((v) => Number(v) || 0) : null,
        radius: Number.isFinite(Number(g && g.radius)) ? Number(g.radius) : null,
        normal: Array.isArray(g && g.normal) ? g.normal.slice(0, 3).map((v) => Number(v) || 0) : null,
        distance: Number.isFinite(Number(g && g.distance)) ? Number(g.distance) : null,
        triangles: Number.isFinite(Number(g && g.triangles)) ? Number(g.triangles) : null,
        bounds_min: Array.isArray(g && g.bounds_min) ? g.bounds_min.slice(0, 3).map((v) => Number(v) || 0) : null,
        bounds_max: Array.isArray(g && g.bounds_max) ? g.bounds_max.slice(0, 3).map((v) => Number(v) || 0) : null,
        v0: Array.isArray(g && g.v0) ? g.v0.slice(0, 3).map((v) => Number(v) || 0) : null,
        v1: Array.isArray(g && g.v1) ? g.v1.slice(0, 3).map((v) => Number(v) || 0) : null,
        v2: Array.isArray(g && g.v2) ? g.v2.slice(0, 3).map((v) => Number(v) || 0) : null,
      }))
      .filter((g) => g.id)
      .sort((a, b) => a.id.localeCompare(b.id));
    materials = (runtime.materials || [])
      .map((m) => ({
        id: String((m && m.id) || "").trim(),
        type: String((m && m.type) || "material"),
        scalars: Array.isArray(m && m.scalars) ? m.scalars.map((s) => ({
          name: String((s && s.name) || "").trim(),
          value: Number.isFinite(Number(s && s.value)) ? Number(s.value) : (s && s.value),
        })).filter((s) => s.name) : [],
        samplers: Array.isArray(m && m.samplers) ? m.samplers.map((s) => {
          const asset = String((s && s.asset) || "").trim();
          const previewUrl = (sceneName && asset)
            ? `/api/scenes/${encodeURIComponent(sceneName)}/asset?path=${encodeURIComponent(asset)}`
            : "";
          const color = Array.isArray(s && s.color) ? s.color.slice(0, 3).map((v) => Number(v) || 0) : null;
          return {
            name: String((s && s.name) || "").trim(),
            type: String((s && s.type) || "sampler"),
            asset,
            previewUrl,
            color,
          };
        }).filter((s) => s.name) : [],
      }))
      .filter((m) => m.id)
      .sort((a, b) => a.id.localeCompare(b.id));
  } else {
    const source = el.sceneSource ? String(el.sceneSource.value || "") : "";
    const model = parseSceneEditModel(source);
    cameras = (model.cameras || []).slice().sort((a, b) => a.id.localeCompare(b.id));
    objects = Array.from(model.objects.values()).sort((a, b) => a.id.localeCompare(b.id));
    geometries = Array.from(model.geometries.values()).map((g) => ({ ...g })).sort((a, b) => a.id.localeCompare(b.id));
    materials = Array.from((model.materials || new Map()).values())
      .map((m) => ({ ...m, scalars: [], samplers: [] }))
      .sort((a, b) => a.id.localeCompare(b.id));
  }

  const sortByObjectRefs = (items, idFromItem, refFromObject) => {
    const ranks = new Map();
    objects.forEach((o, idx) => {
      const ref = String(refFromObject(o) || "");
      if (!ref) return;
      if (!ranks.has(ref)) ranks.set(ref, []);
      ranks.get(ref).push(idx);
    });
    return (items || []).slice().sort((a, b) => {
      const aId = String(idFromItem(a) || "");
      const bId = String(idFromItem(b) || "");
      const ar = ranks.get(aId) || [];
      const br = ranks.get(bId) || [];
      const as = ar.length ? (ar.reduce((s, v) => s + v, 0) / ar.length) : 1e9;
      const bs = br.length ? (br.reduce((s, v) => s + v, 0) / br.length) : 1e9;
      if (Math.abs(as - bs) > 1e-6) return as - bs;
      return aId.localeCompare(bId);
    });
  };

  geometries = sortByObjectRefs(geometries, (g) => g.id, (o) => o.geometry);
  materials = sortByObjectRefs(materials, (m) => m.id, (o) => o.material);

  const nodeW = 248;
  const nodeH = 48;
  const topPad = 70;
  const rowGap = 14;
  const bottomPad = 40;
  const laneGap = 44;
  const sectionGap = 84;
  const graphRect = el.graphCanvas.getBoundingClientRect();
  const availableH = Math.max(320, (Number(graphRect && graphRect.height) || 720) - topPad - bottomPad);
  const nominalRows = Math.max(4, Math.floor(availableH / (nodeH + rowGap)));
  const maxRowsPerLane = clamp(nominalRows, 4, 16);

  const nodes = [];
  const pos = new Map();
  const columns = [];
  const kindLayout = new Map();

  const detailsNodeHeight = (expanded, rowCount, previewCount) => {
    if (!expanded) return nodeH;
    const detailsH = Math.max(0, rowCount) * 18;
    const previewH = previewCount > 0 ? 48 : 0;
    return nodeH + 6 + detailsH + previewH;
  };

  const kinds = [
    { key: "camera", title: "Camera", color: "#5a88cf", count: cameras.length },
    { key: "object", title: "Object", color: "#9a6846", count: objects.length },
    { key: "geometry", title: "Surface", color: "#4f9a8f", count: geometries.length },
    { key: "material", title: "Material", color: "#5a9a4f", count: materials.length },
  ];
  let xCursor = 40;
  kinds.forEach((k) => {
    const laneCount = Math.max(1, Math.ceil(Math.max(1, k.count) / maxRowsPerLane));
    const laneXs = [];
    for (let i = 0; i < laneCount; i += 1) laneXs.push(xCursor + i * (nodeW + laneGap));
    const laneNextY = new Array(laneCount).fill(topPad);
    kindLayout.set(k.key, {
      laneCount,
      laneXs,
      laneNextY,
      color: k.color,
      title: k.title,
      x: xCursor,
    });
    columns.push({ key: k.key, title: k.title, x: xCursor, color: k.color });
    xCursor += laneCount * (nodeW + laneGap) - laneGap + sectionGap;
  });
  const viewW = Math.max(980, xCursor - sectionGap + 40);

  const pushNode = (kind, id, subtitle, extras) => {
    const layout = kindLayout.get(kind);
    if (!layout) return;
    const ext = (extras && typeof extras === "object") ? extras : {};
    const h = Number(ext.h) > 0 ? Number(ext.h) : nodeH;
    let lane = 0;
    for (let i = 1; i < layout.laneNextY.length; i += 1) {
      if (layout.laneNextY[i] < layout.laneNextY[lane]) lane = i;
    }
    const y = layout.laneNextY[lane] || topPad;
    const node = {
      key: `${kind}:${id}`,
      id,
      kind,
      subtitle: subtitle || "",
      x: layout.laneXs[lane],
      y,
      w: nodeW,
      h,
      color: layout.color,
      expanded: !!ext.expanded,
      propertyRows: Array.isArray(ext.propertyRows) ? ext.propertyRows : [],
      texturePreviews: Array.isArray(ext.texturePreviews) ? ext.texturePreviews : [],
      csgOp: ext.csgOp || "",
    };
    node.manualKey = `${sceneName || ""}|${node.key}`;
    const manual = graphView.manualNodePos.get(node.manualKey);
    if (manual && Number.isFinite(manual.x) && Number.isFinite(manual.y)) {
      node.x = manual.x;
      node.y = manual.y;
    }
    nodes.push(node);
    pos.set(node.key, node);
    layout.laneNextY[lane] = y + node.h + rowGap;
  };

  cameras.forEach((c) => {
    const nodeKey = `camera:${c.id}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows = [{ key: "type", value: String(c.type || "camera") }];
    if (Array.isArray(c.position)) propertyRows.push({ key: "position", value: graphVec3Label(c.position) });
    if (Array.isArray(c.target)) propertyRows.push({ key: "target", value: graphVec3Label(c.target) });
    if (Array.isArray(c.up)) propertyRows.push({ key: "up", value: graphVec3Label(c.up) });
    if (Array.isArray(c.orientation)) propertyRows.push({ key: "orientation", value: graphVec3Label(c.orientation) });
    if (Number.isFinite(c.fov)) propertyRows.push({ key: "fov", value: formatGraphNumeric(c.fov) });
    if (Number.isFinite(c.aperture)) propertyRows.push({ key: "aperture", value: formatGraphNumeric(c.aperture) });
    if (Number.isFinite(c.flength)) propertyRows.push({ key: "flength", value: formatGraphNumeric(c.flength) });
    if (Number.isFinite(c.ipd)) propertyRows.push({ key: "ipd", value: formatGraphNumeric(c.ipd) });
    pushNode("camera", c.id, c.type || "camera", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded,
      propertyRows,
      texturePreviews: [],
    });
  });
  objects.forEach((o) => {
    const nodeKey = `object:${o.id}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows = [
      { key: "surface", value: String(o.geometry || "-") },
      { key: "material", value: String(o.material || "-") },
    ];
    pushNode("object", o.id, runtime ? "runtime object" : "scene object", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded,
      propertyRows,
      texturePreviews: [],
    });
  });
  geometries.forEach((g) => {
    const nodeKey = `geometry:${g.id}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows = [{ key: "type", value: String(g.type || "surface") }];
    if (g.type === "csg") {
      if (g.op) propertyRows.push({ key: "op", value: g.op });
      if (Number.isFinite(g.smoothness)) propertyRows.push({ key: "smoothness", value: formatGraphNumeric(g.smoothness) });
      if (g.left_type) propertyRows.push({ key: "left", value: g.left_type });
      if (g.right_type) propertyRows.push({ key: "right", value: g.right_type });
    }
    if (Array.isArray(g.position)) propertyRows.push({ key: "position", value: graphVec3Label(g.position) });
    if (Number.isFinite(g.radius)) propertyRows.push({ key: "radius", value: formatGraphNumeric(g.radius) });
    if (Array.isArray(g.normal)) propertyRows.push({ key: "normal", value: graphVec3Label(g.normal) });
    if (Number.isFinite(g.distance)) propertyRows.push({ key: "distance", value: formatGraphNumeric(g.distance) });
    if (Number.isFinite(g.triangles)) propertyRows.push({ key: "triangles", value: formatGraphNumeric(g.triangles) });
    if (Array.isArray(g.bounds_min)) propertyRows.push({ key: "bounds_min", value: graphVec3Label(g.bounds_min) });
    if (Array.isArray(g.bounds_max)) propertyRows.push({ key: "bounds_max", value: graphVec3Label(g.bounds_max) });
    if (Array.isArray(g.v0)) propertyRows.push({ key: "v0", value: graphVec3Label(g.v0) });
    if (Array.isArray(g.v1)) propertyRows.push({ key: "v1", value: graphVec3Label(g.v1) });
    if (Array.isArray(g.v2)) propertyRows.push({ key: "v2", value: graphVec3Label(g.v2) });
    pushNode("geometry", g.id, g.type || "surface", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded,
      propertyRows,
      texturePreviews: [],
      csgOp: g.type === "csg" ? String(g.op || "union") : "",
    });
  });
  materials.forEach((m) => {
    const nodeKey = `material:${String(m.id || "")}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const scalars = Array.isArray(m && m.scalars) ? m.scalars : [];
    const samplers = Array.isArray(m && m.samplers) ? m.samplers : [];
    const propertyRows = [];
    propertyRows.push({ key: "type", value: String(m && m.type ? m.type : "material") });
    scalars.forEach((s) => {
      const name = String(s && s.name ? s.name : "scalar");
      const value = (s && s.value !== undefined) ? formatGraphNumeric(s.value) : "-";
      propertyRows.push({ key: name, value });
    });
    samplers.forEach((s) => {
      const name = String(s && s.name ? s.name : "sampler");
      const type = String(s && s.type ? s.type : "sampler");
      const row = { key: name, value: type };
      if (Array.isArray(s && s.color) && s.color.length >= 3) row.color = s.color.slice(0, 3);
      propertyRows.push(row);
    });
    const texturePreviews = samplers
      .filter((s) => !!(s && s.previewUrl))
      .map((s) => ({ name: String(s.name || "texture"), url: String(s.previewUrl) }));
    pushNode("material", m.id, m.type || "material", {
      h: detailsNodeHeight(expanded, propertyRows.length, texturePreviews.length),
      expanded,
      propertyRows,
      texturePreviews,
    });
  });

  const links = [];
  objects.forEach((o) => {
    const src = pos.get(`object:${o.id}`);
    const geo = pos.get(`geometry:${o.geometry || ""}`);
    const mat = pos.get(`material:${o.material || ""}`);
    if (src && geo) links.push({ from: src, to: geo });
    if (src && mat) links.push({ from: src, to: mat });
  });

  const viewH = Math.max(
    topPad + rowGap + bottomPad,
    ...Array.from(kindLayout.values()).map((layout) => {
      const laneMax = Math.max(topPad, ...layout.laneNextY);
      return laneMax + bottomPad;
    }),
    ...nodes.map((n) => n.y + n.h + bottomPad)
  );

  graphView.data = {
    columns,
    nodes,
    links,
    viewW,
    viewH,
  };
  graphView.worldW = viewW;
  graphView.worldH = viewH;
  if (!graphView.userAdjusted) {
    fitGraphToViewport();
  }
  applyGraphTransform();
  if (el.graphLegend) {
    const tbody = el.graphLegend.querySelector("tbody");
    if (tbody) {
      tbody.innerHTML = "";
      const rows = [
        ["Camera", cameras.length],
        ["Object", objects.length],
        ["Surface", geometries.length],
        ["Material", materials.length],
      ];
      rows.forEach(([label, count]) => {
        const tr = document.createElement("tr");
        const tdLabel = document.createElement("td");
        const tdCount = document.createElement("td");
        tdLabel.textContent = String(label);
        tdCount.textContent = String(count);
        tr.appendChild(tdLabel);
        tr.appendChild(tdCount);
        tbody.appendChild(tr);
      });
    }
  }
}
