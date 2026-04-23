// ─── Utilities ────────────────────────────────────────────────────────────────

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

// ─── Port system ──────────────────────────────────────────────────────────────

function getNodePorts(kind) {
  switch (kind) {
    case "object":
      return {
        inputs: [
          { id: "geometry", type: "surface",  label: "Surface"  },
          { id: "material", type: "material", label: "Material" },
          { id: "medium",   type: "medium",   label: "Medium"   },
        ],
        outputs: [],
      };
    case "geometry":
      return { inputs: [], outputs: [{ id: "out", type: "surface",  label: "" }] };
    case "material":
      return { inputs: [], outputs: [{ id: "out", type: "material", label: "" }] };
    case "medium":
      return { inputs: [], outputs: [{ id: "out", type: "medium",   label: "" }] };
    case "sampler":
      return { inputs: [], outputs: [{ id: "out", type: "color",   label: "" }] };
    case "scalar":
      return { inputs: [], outputs: [{ id: "out", type: "scalar",  label: "" }] };
    case "vec3":
      return { inputs: [], outputs: [{ id: "out", type: "vec3",    label: "" }] };
    default:
      return { inputs: [], outputs: [] };
  }
}

function portTypeColor(type) {
  switch (type) {
    case "surface":  return "#4f9a8f";
    case "material": return "#5a9a4f";
    case "medium":   return "#7d66b4";
    case "color":    return "#c87a4f";
    case "texture":  return "#9a70b4";
    case "vec3":     return "#5a8abf";
    case "scalar":   return "#a89a5a";
    default:         return "#7a8fa8";
  }
}

// Returns world-space {x, y} of a port given its index and total count on that side.
function getPortWorldPos(node, isOutput, portIdx, portCount) {
  const headerH = 48;
  const x = isOutput ? node.x + node.w : node.x;
  let y;
  if (!isOutput && !node.expanded && portCount > 1) {
    // Multiple inputs, collapsed: all converge to the single merged circle
    y = node.y + headerH / 2;
  } else if (!isOutput && node.expanded) {
    // Expanded: align with the property row this port belongs to
    const port = node.ports && node.ports.inputs && node.ports.inputs[portIdx];
    const rowIdx = (port && Number.isFinite(port.rowIndex)) ? port.rowIndex : portIdx;
    y = node.y + headerH + rowIdx * 18 + 9;
  } else {
    y = node.y + headerH / 2;
  }
  return { x, y };
}

function buildPortElements(node) {
  const ports = node.ports || getNodePorts(node.kind);
  const headerH = 48;
  const elements = [];

  // Output ports — always shown individually
  ports.outputs.forEach((port, i) => {
    const div = document.createElement("div");
    div.className = "ng-port ng-port--output";
    div.dataset.nodeKey  = node.key;
    div.dataset.portId   = port.id;
    div.dataset.portType = port.type;
    div.style.setProperty("--port-color", portTypeColor(port.type));
    div.style.top = `${headerH / 2 - 7}px`;
    elements.push(div);
  });

  const inputCount = ports.inputs.length;
  if (inputCount === 0) return elements;

  if (!node.expanded && inputCount > 1) {
    // Multiple inputs, collapsed: single merged circle at header center
    const anyConnected = ports.inputs.some((p) => !!(node.connections && node.connections[p.id]));
    const div = document.createElement("div");
    div.className = `ng-port ng-port--input ng-port--merged${anyConnected ? " ng-port--connected" : ""}`;
    div.dataset.nodeKey  = node.key;
    div.dataset.portId   = "__merged__";
    div.dataset.portType = ports.inputs[0].type || "";
    div.style.setProperty("--port-color", portTypeColor(ports.inputs[0].type || ""));
    div.style.top = `${headerH / 2 - 8}px`;
    elements.push(div);
  } else {
    // Expanded, or single input regardless of expansion: individual circles, row-aligned
    ports.inputs.forEach((port, i) => {
      const div = document.createElement("div");
      div.className = "ng-port ng-port--input";
      div.dataset.nodeKey  = node.key;
      div.dataset.portId   = port.id;
      div.dataset.portType = port.type;
      div.style.setProperty("--port-color", portTypeColor(port.type));

      let centerY;
      if (node.expanded) {
        const rowIdx = Number.isFinite(port.rowIndex) ? port.rowIndex : i;
        centerY = headerH + rowIdx * 18 + 9;
      } else {
        centerY = headerH / 2;
      }
      div.style.top = `${centerY - 7}px`;

      const connectedTo = (node.connections && node.connections[port.id]) || "";
      if (connectedTo) {
        div.dataset.connectedTo = connectedTo;
        div.classList.add("ng-port--connected");
      }
      if (port.label) {
        const lbl = document.createElement("span");
        lbl.className = "ng-port-label";
        lbl.textContent = port.label;
        div.appendChild(lbl);
      }
      elements.push(div);
    });
  }

  return elements;
}

// ─── Connect-drag state helpers ───────────────────────────────────────────────

function ensureConnectDragPath() {
  let path = document.getElementById("ngDragPath");
  if (!path && el.nodeGraphLinks) {
    path = document.createElementNS("http://www.w3.org/2000/svg", "path");
    path.id = "ngDragPath";
    path.setAttribute("fill", "none");
    path.setAttribute("pointer-events", "none");
    el.nodeGraphLinks.appendChild(path);
  }
  return path;
}

function updateConnectDragLine() {
  const drag = graphView.connectDrag;
  const path = ensureConnectDragPath();
  if (!drag || !path) return;

  const { fromX, fromY, curX, curY, fromIsOutput } = drag;
  const cp1x = fromIsOutput ? fromX + 80 : fromX - 80;
  const cp2x = fromIsOutput ? curX  - 80 : curX  + 80;
  path.setAttribute("d", `M ${fromX} ${fromY} C ${cp1x} ${fromY} ${cp2x} ${curY} ${curX} ${curY}`);
  path.setAttribute("stroke", "rgba(220,235,255,0.85)");
  path.setAttribute("stroke-width", "2");
  path.setAttribute("stroke-dasharray", "6 4");
  path.style.display = "";
}

function hideConnectDragLine() {
  const path = document.getElementById("ngDragPath");
  if (path) path.style.display = "none";
}

function removeGraphLink(objectNodeKey, fieldId) {
  const objectId = String(objectNodeKey || "").replace(/^object:/, "");
  const source = el.sceneSource ? String(el.sceneSource.value || "") : "";
  const newSource = setObjectRefInSource(source, objectId, fieldId, "");
  if (newSource !== source) {
    updateSceneSourceText(newSource, { history: "visual" });
    renderSceneGraphView();
  }
}

function finishConnectDrag(evt) {
  const drag = graphView.connectDrag;
  if (!drag) return;
  graphView.connectDrag = null;
  try { el.nodeGraph.releasePointerCapture(evt.pointerId); } catch (_) {}
  hideConnectDragLine();

  const target = document.elementFromPoint(evt.clientX, evt.clientY);
  const portEl = target && target.closest ? target.closest(".ng-port") : null;

  // Tap (no movement) on own port that has a connection → disconnect
  if (!drag.moved) {
    if (!portEl || (portEl.dataset.nodeKey === drag.fromNodeKey && portEl.dataset.portId === drag.fromPortId)) {
      if (drag.fromConnectedTo && !drag.fromIsOutput) {
        removeGraphLink(drag.fromNodeKey, drag.fromPortId);
      }
      return;
    }
  }

  if (!portEl) return;

  const toNodeKey  = portEl.dataset.nodeKey  || "";
  const toPortId   = portEl.dataset.portId   || "";
  const toPortType = portEl.dataset.portType || "";
  const toIsOutput = portEl.classList.contains("ng-port--output");

  // Validate: must be opposite direction, compatible type, different node
  if (toNodeKey === drag.fromNodeKey) return;
  if (toIsOutput === drag.fromIsOutput) return;
  if (toPortType !== drag.fromPortType) return;

  const outputNodeKey = drag.fromIsOutput ? drag.fromNodeKey : toNodeKey;
  const inputNodeKey  = drag.fromIsOutput ? toNodeKey        : drag.fromNodeKey;
  const inputPortId   = drag.fromIsOutput ? toPortId         : drag.fromPortId;

  const outputNode = findGraphNodeData(outputNodeKey);
  const inputNode  = findGraphNodeData(inputNodeKey);
  if (!outputNode || !inputNode || inputNode.kind !== "object") return;

  const source = el.sceneSource ? String(el.sceneSource.value || "") : "";
  const newSource = setObjectRefInSource(source, inputNode.id, inputPortId, outputNode.id);
  if (newSource !== source) {
    updateSceneSourceText(newSource, { history: "visual" });
    renderSceneGraphView();
  }
}

// ─── Transform & hit-test ─────────────────────────────────────────────────────

function applyGraphTransform() {
  if (!el.nodeGraph || !el.nodeGraphWorld) return;
  const { tx, ty, scale } = graphView;
  el.nodeGraphWorld.style.transform = `translate(${tx}px,${ty}px) scale(${scale})`;
  const step = 40 * scale;
  if (step >= 12) {
    el.nodeGraph.style.backgroundSize = `${step}px ${step}px`;
    const ox = ((tx % step) + step) % step;
    const oy = ((ty % step) + step) % step;
    el.nodeGraph.style.backgroundPosition = `${ox}px ${oy}px`;
  }
  el.nodeGraph.classList.toggle("is-panning", !!graphView.panning);
}

function graphWorldPointFromClient(clientX, clientY) {
  if (!el.nodeGraph) return null;
  const rect = el.nodeGraph.getBoundingClientRect();
  const x = (clientX - rect.left  - graphView.tx) / graphView.scale;
  const y = (clientY - rect.top   - graphView.ty) / graphView.scale;
  return { x, y };
}

function findGraphNodeAt(clientX, clientY) {
  const target = document.elementFromPoint(clientX, clientY);
  if (!target) return "";
  const node = target.closest ? target.closest(".ng-node") : null;
  return node ? String(node.dataset.key || "") : "";
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
  return raw.indexOf("camera:")   === 0
    || raw.indexOf("object:")     === 0
    || raw.indexOf("geometry:")   === 0
    || raw.indexOf("material:")   === 0
    || raw.indexOf("medium:")     === 0;
}

function fitGraphToViewport() {
  if (!el.nodeGraph || !graphView.worldW || !graphView.worldH) return;
  const rect = el.nodeGraph.getBoundingClientRect();
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

// ─── SVG links ────────────────────────────────────────────────────────────────

function updateNodeGraphLinks() {
  const svg = el.nodeGraphLinks;
  const data = graphView.data;
  if (!svg || !data) return;

  // Remove all children except the persistent drag-path
  Array.from(svg.children).forEach((c) => { if (c.id !== "ngDragPath") c.remove(); });

  const hoveredKey = graphView.hoverKey || "";
  const ns = "http://www.w3.org/2000/svg";

  (data.links || []).forEach((ln) => {
    const fromPorts = ln.from.ports || getNodePorts(ln.from.kind);
    const fromIdx   = fromPorts.outputs.findIndex((p) => p.id === "out");
    const fromPos   = getPortWorldPos(ln.from, true,  fromIdx < 0 ? 0 : fromIdx, fromPorts.outputs.length || 1);

    const toPorts   = ln.to.ports || getNodePorts(ln.to.kind);
    const toIdx     = toPorts.inputs.findIndex((p) => p.id === ln.field);
    const toPos     = getPortWorldPos(ln.to, false, toIdx < 0 ? 0 : toIdx, toPorts.inputs.length || 1);

    const x1 = fromPos.x, y1 = fromPos.y;
    const x2 = toPos.x,   y2 = toPos.y;
    const cp1x = x1 + 80, cp2x = x2 - 80;
    const d = `M ${x1} ${y1} C ${cp1x} ${y1} ${cp2x} ${y2} ${x2} ${y2}`;

    const active = !!hoveredKey && (ln.from.key === hoveredKey || ln.to.key === hoveredKey);
    const fromPort = fromPorts.outputs[fromIdx >= 0 ? fromIdx : 0] || {};
    const linkColor = portTypeColor(fromPort.type || "surface");

    // Wide invisible hit area (click to remove) — fixed links are not removable
    if (!ln.fixed) {
      const hit = document.createElementNS(ns, "path");
      hit.setAttribute("d", d);
      hit.setAttribute("stroke", "transparent");
      hit.setAttribute("stroke-width", "14");
      hit.setAttribute("fill", "none");
      hit.setAttribute("pointer-events", "stroke");
      hit.style.cursor = "pointer";
      hit.addEventListener("click", () => removeGraphLink(ln.to.key, ln.field));
      svg.appendChild(hit);
    }

    // Visible path
    const vis = document.createElementNS(ns, "path");
    vis.setAttribute("d", d);
    vis.setAttribute("stroke", linkColor);
    vis.setAttribute("stroke-opacity", ln.fixed ? (active ? "0.65" : "0.3") : (active ? "0.9" : "0.55"));
    vis.setAttribute("stroke-width", ln.fixed ? (active ? "1.8" : "1.4") : (active ? "2.8" : "2"));
    vis.setAttribute("stroke-dasharray", ln.fixed ? "4 3" : "none");
    vis.setAttribute("fill", "none");
    vis.setAttribute("pointer-events", "none");
    svg.appendChild(vis);
  });

  // Re-append drag path so it renders on top
  const dragPath = document.getElementById("ngDragPath");
  if (dragPath && dragPath.parentNode === svg) svg.appendChild(dragPath);
}

// ─── Hover highlighting ───────────────────────────────────────────────────────

function updateNodeGraphHover(key) {
  const world = el.nodeGraphWorld;
  if (!world) return;
  world.classList.toggle("has-hover", !!key);
  const linkedKeys = new Set();
  if (key && graphView.data) {
    linkedKeys.add(key);
    (graphView.data.links || []).forEach((ln) => {
      if (ln.from.key === key) linkedKeys.add(ln.to.key);
      if (ln.to.key   === key) linkedKeys.add(ln.from.key);
    });
  }
  world.querySelectorAll(".ng-node").forEach((nodeEl) => {
    const nodeKey = nodeEl.dataset.key || "";
    nodeEl.classList.toggle("is-hovered", nodeKey === key);
    nodeEl.classList.toggle("is-linked",  key ? linkedKeys.has(nodeKey) : false);
  });
  updateNodeGraphLinks();
}

// ─── DOM node builder ─────────────────────────────────────────────────────────

function buildNodeElement(n) {
  const div = document.createElement("div");
  div.className = `ng-node ng-node--${n.kind}${n.expanded ? " is-expanded" : ""}`;
  div.dataset.key = n.key;
  div.style.left  = `${n.x}px`;
  div.style.top   = `${n.y}px`;
  div.style.width = `${n.w}px`;
  div.style.setProperty("--node-color", n.color);

  const header = document.createElement("div");
  header.className = "ng-node-header";

  const title = document.createElement("div");
  title.className = "ng-node-title";
  title.textContent = n.label || n.id;
  header.appendChild(title);

  const subtitle = document.createElement("div");
  subtitle.className = "ng-node-subtitle";
  subtitle.textContent = n.subtitle || "";
  header.appendChild(subtitle);

  div.appendChild(header);

  // Vec3 nodes show three editable XYZ inputs
  if (n.kind === "vec3" && Array.isArray(n.valueVec3)) {
    const xyzRow = document.createElement("div");
    xyzRow.className = "ng-node-vec3-inputs";
    ["X", "Y", "Z"].forEach((axis, idx) => {
      const lbl = document.createElement("span");
      lbl.className = "ng-node-color-label";
      lbl.textContent = axis;
      xyzRow.appendChild(lbl);
      const inp = document.createElement("input");
      inp.type  = "number";
      inp.className = "ng-node-row-input ng-node-vec3-channel";
      inp.step  = "any";
      inp.value = formatSceneNumber(n.valueVec3[idx], 4);
      inp.dataset.channel = String(idx);
      if (n.updateInfo) {
        inp.dataset.updateKind     = n.updateInfo.kind;
        inp.dataset.updateParentId = n.updateInfo.parentId;
        inp.dataset.updateProp     = n.updateInfo.prop;
      }
      inp.addEventListener("change", (e) => {
        e.stopPropagation();
        const kind     = e.target.dataset.updateKind;
        const parentId = e.target.dataset.updateParentId;
        const prop     = e.target.dataset.updateProp;
        if (!kind || !parentId || !prop) return;
        const v = Number(e.target.value);
        if (!Number.isFinite(v)) return;
        const nodeEl = e.target.closest(".ng-node");
        const allCh  = nodeEl ? nodeEl.querySelectorAll(".ng-node-vec3-channel") : [];
        const vec = [0, 0, 0];
        allCh.forEach((ci) => {
          const c = Number(ci.dataset.channel);
          if (c >= 0 && c <= 2) vec[c] = (ci === e.target) ? v : Number(ci.value);
        });
        const src = el.sceneSource ? String(el.sceneSource.value || "") : "";
        let next = src;
        if (kind === "camera_vec3")   next = updateCameraVec3InSource(src, parentId, prop, vec);
        if (kind === "geometry_vec3") next = updateGeometryVec3InSource(src, parentId, prop, vec);
        if (next !== src) updateSceneSourceText(next, { history: "visual" });
      });
      inp.addEventListener("mousedown", (e) => e.stopPropagation());
      inp.addEventListener("click",     (e) => e.stopPropagation());
      xyzRow.appendChild(inp);
    });
    div.appendChild(xyzRow);
  }

  // Scalar nodes show a single editable number input
  if (n.kind === "scalar" && n.valueNum !== null) {
    const scalarRow = document.createElement("div");
    scalarRow.className = "ng-node-scalar-input";
    const inp = document.createElement("input");
    inp.type  = "number";
    inp.className = "ng-node-row-input ng-node-scalar-value";
    inp.step  = "any";
    inp.value = formatSceneNumber(n.valueNum, 4);
    if (n.updateInfo) {
      inp.dataset.updateKind     = n.updateInfo.kind;
      inp.dataset.updateParentId = n.updateInfo.parentId;
      inp.dataset.updateProp     = n.updateInfo.prop;
    }
    inp.addEventListener("change", (e) => {
      e.stopPropagation();
      const kind     = e.target.dataset.updateKind;
      const parentId = e.target.dataset.updateParentId;
      const prop     = e.target.dataset.updateProp;
      if (!kind || !parentId || !prop) return;
      const v = Number(e.target.value);
      if (!Number.isFinite(v)) return;
      const src = el.sceneSource ? String(el.sceneSource.value || "") : "";
      let next = src;
      if (kind === "camera_scalar")   next = updateCameraScalarInSource(src, parentId, prop, v);
      if (kind === "geometry_scalar") next = updateGeometryScalarInSource(src, parentId, prop, v);
      if (next !== src) updateSceneSourceText(next, { history: "visual" });
    });
    inp.addEventListener("mousedown", (e) => e.stopPropagation());
    inp.addEventListener("click",     (e) => e.stopPropagation());
    scalarRow.appendChild(inp);
    div.appendChild(scalarRow);
  }

  // Sampler nodes show a color swatch bar or texture thumbnail below the header
  if (n.kind === "sampler") {
    if (n.samplerColor) {
      const r = Math.round(clamp(Number(n.samplerColor[0]) || 0, 0, 1) * 255);
      const g = Math.round(clamp(Number(n.samplerColor[1]) || 0, 0, 1) * 255);
      const b = Math.round(clamp(Number(n.samplerColor[2]) || 0, 0, 1) * 255);
      const bar = document.createElement("div");
      bar.className = "ng-node-swatch-bar";
      bar.style.background = `rgb(${r},${g},${b})`;
      div.appendChild(bar);
      // Editable RGB channel inputs (only for pure color samplers, not textures)
      if (n.materialId && n.samplerName) {
        const rgbRow = document.createElement("div");
        rgbRow.className = "ng-node-color-inputs";
        ["R", "G", "B"].forEach((ch, idx) => {
          const lbl = document.createElement("span");
          lbl.className = "ng-node-color-label";
          lbl.textContent = ch;
          rgbRow.appendChild(lbl);
          const inp = document.createElement("input");
          inp.type  = "number";
          inp.className = "ng-node-row-input ng-node-color-channel";
          inp.step  = "0.001";
          inp.min   = "0";
          inp.value = formatSceneNumber(n.samplerColor[idx], 0);
          inp.dataset.materialId  = n.materialId;
          inp.dataset.samplerName = n.samplerName;
          inp.dataset.channel     = String(idx);
          inp.addEventListener("change", (e) => {
            e.stopPropagation();
            const mId   = e.target.dataset.materialId;
            const sName = e.target.dataset.samplerName;
            const ch    = Number(e.target.dataset.channel);
            const v     = Number(e.target.value);
            if (!Number.isFinite(v)) return;
            const nodeEl = e.target.closest(".ng-node");
            const allCh  = nodeEl ? nodeEl.querySelectorAll(".ng-node-color-channel") : [];
            const rgb = [0, 0, 0];
            allCh.forEach((ci) => {
              const c = Number(ci.dataset.channel);
              if (c >= 0 && c <= 2) rgb[c] = (ci === e.target) ? v : Number(ci.value);
            });
            const src  = el.sceneSource ? String(el.sceneSource.value || "") : "";
            const next = updateSamplerColorInSource(src, mId, sName, rgb);
            if (next !== src) updateSceneSourceText(next, { history: "visual" });
          });
          inp.addEventListener("mousedown", (e) => e.stopPropagation());
          inp.addEventListener("click",     (e) => e.stopPropagation());
          rgbRow.appendChild(inp);
        });
        div.appendChild(rgbRow);
      }
    } else if (n.samplerPreviewUrl) {
      const img = document.createElement("img");
      img.className = "ng-node-preview-img ng-node-preview-full";
      img.src     = n.samplerPreviewUrl;
      img.alt     = n.label || n.id;
      img.loading = "lazy";
      div.appendChild(img);
    }
  }

  if (n.expanded) {
    const rows = Array.isArray(n.propertyRows) ? n.propertyRows : [];
    if (rows.length > 0) {
      const body = document.createElement("div");
      body.className = "ng-node-body";
      rows.forEach((row) => {
        const rowDiv = document.createElement("div");
        rowDiv.className = "ng-node-row";

        const keySpan = document.createElement("span");
        keySpan.className = "ng-node-row-key";
        keySpan.textContent = String(row && row.key ? row.key : "").toUpperCase();
        rowDiv.appendChild(keySpan);

        const valSpan = document.createElement("span");
        valSpan.className = "ng-node-row-val";
        if (row && row.scalarKey !== undefined && row.materialId) {
          const inp = document.createElement("input");
          inp.type = "number";
          inp.className = "ng-node-row-input";
          inp.value = Number.isFinite(Number(row.scalar)) ? Number(row.scalar) : 0;
          inp.step = "any";
          inp.dataset.materialId = row.materialId;
          inp.dataset.scalarKey  = row.scalarKey;
          inp.addEventListener("change", (e) => {
            e.stopPropagation();
            const v = Number(e.target.value);
            if (!Number.isFinite(v)) return;
            const src = el.sceneSource ? String(el.sceneSource.value || "") : "";
            const next = updateMaterialScalarInSource(src, e.target.dataset.materialId, e.target.dataset.scalarKey, v);
            if (next !== src) updateSceneSourceText(next, { history: "visual" });
          });
          inp.addEventListener("mousedown", (e) => e.stopPropagation());
          inp.addEventListener("click",     (e) => e.stopPropagation());
          valSpan.appendChild(inp);
        } else if (row && Array.isArray(row.color) && row.color.length >= 3) {
          const r = Math.round(clamp(Number(row.color[0]) || 0, 0, 1) * 255);
          const g = Math.round(clamp(Number(row.color[1]) || 0, 0, 1) * 255);
          const b = Math.round(clamp(Number(row.color[2]) || 0, 0, 1) * 255);
          const swatch = document.createElement("span");
          swatch.className = "ng-node-color-swatch";
          swatch.style.background = `rgb(${r},${g},${b})`;
          valSpan.appendChild(swatch);
          valSpan.appendChild(document.createTextNode(graphColorLabel(row.color)));
        } else {
          valSpan.textContent = String(row && row.value !== undefined ? row.value : "-");
        }
        rowDiv.appendChild(valSpan);
        body.appendChild(rowDiv);
      });
      div.appendChild(body);
    }

    const previews = Array.isArray(n.texturePreviews) ? n.texturePreviews.slice(0, 3) : [];
    if (previews.length > 0) {
      const previewsDiv = document.createElement("div");
      previewsDiv.className = "ng-node-previews";
      previews.forEach((p) => {
        const img = document.createElement("img");
        img.className = "ng-node-preview-img";
        img.src     = p.url  || "";
        img.alt     = p.name || "texture";
        img.loading = "lazy";
        previewsDiv.appendChild(img);
      });
      div.appendChild(previewsDiv);
    }
  }

  // Port circles
  buildPortElements(n).forEach((portEl) => div.appendChild(portEl));

  return div;
}

// ─── Full DOM render ──────────────────────────────────────────────────────────

function renderNodeGraphDOM() {
  const world = el.nodeGraphWorld;
  if (!world || !graphView.data) return;

  Array.from(world.children).forEach((child) => {
    if (child !== el.nodeGraphLinks) child.remove();
  });

  const { columns, nodes, viewW, viewH } = graphView.data;

  if (el.nodeGraphLinks) {
    el.nodeGraphLinks.style.width  = `${viewW}px`;
    el.nodeGraphLinks.style.height = `${viewH}px`;
    el.nodeGraphLinks.setAttribute("viewBox", `0 0 ${viewW} ${viewH}`);
  }
  updateNodeGraphLinks();

  columns.forEach((c) => {
    const label = document.createElement("div");
    label.className  = "ng-col-label";
    label.textContent = c.title.toUpperCase();
    label.style.left  = `${c.x}px`;
    label.style.top   = "20px";
    label.style.color = c.color;
    world.appendChild(label);
  });

  nodes.forEach((n) => world.appendChild(buildNodeElement(n)));

  updateNodeGraphHover(graphView.hoverKey);
}

// ─── Render scheduling ────────────────────────────────────────────────────────

function scheduleGraphRender() {
  if (!el.nodeGraph) return;
  if (graphRenderRafPrimary)   { cancelAnimationFrame(graphRenderRafPrimary);   graphRenderRafPrimary   = 0; }
  if (graphRenderRafSecondary) { cancelAnimationFrame(graphRenderRafSecondary); graphRenderRafSecondary = 0; }
  graphRenderRafPrimary = requestAnimationFrame(() => {
    graphRenderRafPrimary = 0;
    graphRenderRafSecondary = requestAnimationFrame(() => {
      graphRenderRafSecondary = 0;
      if (activeTabMode !== "visual" || editorViewMode !== "graph") return;
      renderSceneGraphView();
    });
  });
}

// ─── Input binding ────────────────────────────────────────────────────────────

function bindGraphInteraction() {
  if (!el.nodeGraph || graphView.bound) return;
  graphView.bound = true;

  // Persistent drag-preview path in the SVG
  ensureConnectDragPath();

  // ── touch helpers ──
  const setTouchPoint    = (id, x, y) => { graphView.touchPoints[String(id)] = { id, x, y }; };
  const removeTouchPoint = (id)        => { delete graphView.touchPoints[String(id)]; };
  const touchPointCount  = ()          => Object.keys(graphView.touchPoints).length;
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
    graphView.pointerDown         = false;
    graphView.pointerDownNodeKey  = "";
    graphView.panning             = false;
    graphView.pointerId           = null;
    graphView.dragNodeKey         = "";
    graphView.dragNodeOffsetX     = 0;
    graphView.dragNodeOffsetY     = 0;
    graphView.movedSincePointerDown = false;
  };

  const zoomAtClient = (clientX, clientY, factor) => {
    if (!graphView.worldW || !graphView.worldH) return false;
    const rect      = el.nodeGraph.getBoundingClientRect();
    const cx        = clientX - rect.left;
    const cy        = clientY - rect.top;
    const nextScale = clamp(graphView.scale * factor, graphView.minScale, graphView.maxScale);
    if (!Number.isFinite(nextScale) || Math.abs(nextScale - graphView.scale) < 1e-6) return false;
    const ratio  = nextScale / graphView.scale;
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
    graphView.pinchActive        = true;
    graphView.pinchLastCenterX   = cx;
    graphView.pinchLastCenterY   = cy;
    graphView.pinchLastDistance  = dist;
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
      const zf = dist / graphView.pinchLastDistance;
      if (Number.isFinite(zf) && Math.abs(zf - 1) > 0.001) changed = zoomAtClient(cx, cy, zf) || changed;
    }
    graphView.pinchLastCenterX  = cx;
    graphView.pinchLastCenterY  = cy;
    graphView.pinchLastDistance = dist;
    return changed;
  };

  const moveDraggedNode = (clientX, clientY) => {
    const node   = findGraphNodeData(graphView.dragNodeKey);
    const p      = graphWorldPointFromClient(clientX, clientY);
    if (!node || !p) return;
    node.x = p.x - graphView.dragNodeOffsetX;
    node.y = p.y - graphView.dragNodeOffsetY;
    graphView.userAdjusted = true;
    if (node.manualKey) graphView.manualNodePos.set(node.manualKey, { x: node.x, y: node.y });
    const nodeEl = el.nodeGraphWorld
      ? el.nodeGraphWorld.querySelector(`[data-key="${CSS.escape(node.key)}"]`)
      : null;
    if (nodeEl) { nodeEl.style.left = `${node.x}px`; nodeEl.style.top = `${node.y}px`; }
    updateNodeGraphLinks();
  };

  const endPrimaryPointer = (evt) => {
    // Connect-drag takes priority
    if (graphView.connectDrag && graphView.connectDrag.pointerId === evt.pointerId) {
      finishConnectDrag(evt);
      return true;
    }
    const wasPanning      = !!graphView.panning;
    const wasDraggingNode = !!graphView.dragNodeKey;
    const moved           = !!graphView.movedSincePointerDown;
    const downNodeKey     = String(graphView.pointerDownNodeKey || "");
    clearPrimaryPointerState();
    try { el.nodeGraph.releasePointerCapture(evt.pointerId); } catch (_) {}

    if (evt.type === "pointerup" && evt.button === 0) {
      const upTarget   = document.elementFromPoint(evt.clientX, evt.clientY);
      const onHeader   = !!(upTarget && upTarget.closest && upTarget.closest(".ng-node-header"));
      const upNodeKey  = onHeader ? findGraphNodeAt(evt.clientX, evt.clientY) : "";
      if (onHeader && upNodeKey && upNodeKey === downNodeKey && isGraphExpandableNode(upNodeKey)
          && (!wasDraggingNode || !moved)) {
        if (graphView.expandedNodeKeys.has(upNodeKey)) graphView.expandedNodeKeys.delete(upNodeKey);
        else graphView.expandedNodeKeys.add(upNodeKey);
        renderSceneGraphView();
        return true;
      }
    }
    return false;
  };

  // ── Wheel zoom ──
  el.nodeGraph.addEventListener("wheel", (evt) => {
    if (!graphView.worldW || !graphView.worldH) return;
    evt.preventDefault();
    if (!zoomAtClient(evt.clientX, evt.clientY, Math.exp(-evt.deltaY * 0.0015))) return;
    applyGraphTransform();
  }, { passive: false });

  // ── Double-click to reset view ──
  el.nodeGraph.addEventListener("dblclick", (evt) => {
    if (evt.target.closest && evt.target.closest(".ng-node")) return;
    evt.preventDefault();
    resetGraphView();
  });

  // ── Pointer down ──
  el.nodeGraph.addEventListener("pointerdown", (evt) => {
    // Let native input elements handle their own pointer events
    if (evt.target && (evt.target.tagName === "INPUT" || evt.target.tagName === "SELECT" || evt.target.tagName === "TEXTAREA")) return;

    if (evt.pointerType === "touch") {
      evt.preventDefault();
      setTouchPoint(evt.pointerId, evt.clientX, evt.clientY);
      try { el.nodeGraph.setPointerCapture(evt.pointerId); } catch (_) {}
      if (touchPointCount() >= 2) { beginTouchPinch(); applyGraphTransform(); return; }
    }
    if (evt.button !== 0 && evt.button !== 1) return;
    evt.preventDefault();

    // ── Port drag: start connect ──
    const portEl = evt.target.closest ? evt.target.closest(".ng-port") : null;
    if (portEl && evt.button === 0) {
      const portNodeKey  = portEl.dataset.nodeKey  || "";
      const portId       = portEl.dataset.portId   || "";
      const portType     = portEl.dataset.portType || "";
      const isOutput     = portEl.classList.contains("ng-port--output");
      const connectedTo  = portEl.dataset.connectedTo || "";
      const node         = findGraphNodeData(portNodeKey);
      const ports        = node ? getNodePorts(node.kind) : { inputs: [], outputs: [] };
      const list         = isOutput ? ports.outputs : ports.inputs;
      const portIdx      = list.findIndex((p) => p.id === portId);
      const portPos      = node
        ? getPortWorldPos(node, isOutput, portIdx < 0 ? 0 : portIdx, list.length || 1)
        : { x: 0, y: 0 };
      graphView.connectDrag = {
        pointerId:     evt.pointerId,
        fromNodeKey:   portNodeKey,
        fromPortId:    portId,
        fromPortType:  portType,
        fromIsOutput:  isOutput,
        fromConnectedTo: connectedTo,
        fromX: portPos.x,
        fromY: portPos.y,
        curX:  portPos.x,
        curY:  portPos.y,
        moved: false,
      };
      el.nodeGraph.setPointerCapture(evt.pointerId);
      return;
    }

    // ── Normal node drag / canvas pan ──
    graphView.pointerDown         = true;
    graphView.pointerId           = evt.pointerId;
    graphView.pointerDownNodeKey  = findGraphNodeAt(evt.clientX, evt.clientY);
    graphView.dragStartX          = evt.clientX;
    graphView.dragStartY          = evt.clientY;
    graphView.lastX               = evt.clientX;
    graphView.lastY               = evt.clientY;
    graphView.dragNodeKey         = "";
    graphView.dragNodeOffsetX     = 0;
    graphView.dragNodeOffsetY     = 0;
    graphView.movedSincePointerDown = false;

    if (evt.button === 0 && graphView.pointerDownNodeKey) {
      const node = findGraphNodeData(graphView.pointerDownNodeKey);
      const p    = graphWorldPointFromClient(evt.clientX, evt.clientY);
      if (node && p) {
        graphView.dragNodeKey     = node.key;
        graphView.dragNodeOffsetX = p.x - node.x;
        graphView.dragNodeOffsetY = p.y - node.y;
      }
      graphView.panning = false;
    } else {
      graphView.panning = true;
    }
    el.nodeGraph.setPointerCapture(evt.pointerId);
    applyGraphTransform();
  });

  // ── Pointer move ──
  el.nodeGraph.addEventListener("pointermove", (evt) => {
    // Connect drag
    if (graphView.connectDrag && graphView.connectDrag.pointerId === evt.pointerId) {
      const p = graphWorldPointFromClient(evt.clientX, evt.clientY);
      if (p) {
        const dx = p.x - graphView.connectDrag.fromX;
        const dy = p.y - graphView.connectDrag.fromY;
        if (Math.abs(dx) + Math.abs(dy) > 4) graphView.connectDrag.moved = true;
        graphView.connectDrag.curX = p.x;
        graphView.connectDrag.curY = p.y;
        updateConnectDragLine();
      }
      return;
    }

    // Touch pinch
    if (evt.pointerType === "touch") {
      if (!Object.prototype.hasOwnProperty.call(graphView.touchPoints, String(evt.pointerId))) return;
      evt.preventDefault();
      setTouchPoint(evt.pointerId, evt.clientX, evt.clientY);
      if (touchPointCount() >= 2) {
        if (!graphView.pinchActive) beginTouchPinch();
        if (updateTouchPinch()) applyGraphTransform();
        return;
      }
      if (graphView.pinchActive) { graphView.pinchActive = false; graphView.pinchLastDistance = 0; }
      if (!(graphView.pointerDown && graphView.pointerId === evt.pointerId)) return;
      if (!graphView.panning) {
        if ((Math.abs(evt.clientX - graphView.dragStartX) + Math.abs(evt.clientY - graphView.dragStartY)) > 4)
          graphView.movedSincePointerDown = true;
      }
      if (graphView.dragNodeKey) { moveDraggedNode(evt.clientX, evt.clientY); return; }
      graphView.tx += evt.clientX - graphView.lastX;
      graphView.ty += evt.clientY - graphView.lastY;
      graphView.lastX = evt.clientX;
      graphView.lastY = evt.clientY;
      graphView.userAdjusted = true;
      applyGraphTransform();
      return;
    }

    if (graphView.pointerDown && graphView.pointerId === evt.pointerId) {
      if (!graphView.panning) {
        if ((Math.abs(evt.clientX - graphView.dragStartX) + Math.abs(evt.clientY - graphView.dragStartY)) > 4)
          graphView.movedSincePointerDown = true;
      }
      if (graphView.dragNodeKey) { moveDraggedNode(evt.clientX, evt.clientY); return; }
    }

    if (graphView.panning && graphView.pointerId === evt.pointerId) {
      graphView.tx += evt.clientX - graphView.lastX;
      graphView.ty += evt.clientY - graphView.lastY;
      graphView.lastX = evt.clientX;
      graphView.lastY = evt.clientY;
      graphView.userAdjusted = true;
      applyGraphTransform();
      return;
    }

    // Hover (only when idle)
    const nextHover = findGraphNodeAt(evt.clientX, evt.clientY);
    if (nextHover !== graphView.hoverKey) {
      graphView.hoverKey = nextHover;
      updateNodeGraphHover(nextHover);
    }
  });

  // ── Pointer up / cancel ──
  const endPan = (evt) => {
    if (graphView.connectDrag && graphView.connectDrag.pointerId === evt.pointerId) {
      finishConnectDrag(evt);
      return;
    }
    if (evt.pointerType === "touch") {
      evt.preventDefault();
      if (Object.prototype.hasOwnProperty.call(graphView.touchPoints, String(evt.pointerId)))
        removeTouchPoint(evt.pointerId);
      try { el.nodeGraph.releasePointerCapture(evt.pointerId); } catch (_) {}
      if (graphView.pinchActive) {
        if (touchPointCount() >= 2) beginTouchPinch();
        else { graphView.pinchActive = false; graphView.pinchLastDistance = 0; }
      }
      if (!graphView.pointerDown || graphView.pointerId !== evt.pointerId) { applyGraphTransform(); return; }
      if (endPrimaryPointer(evt)) return;
      applyGraphTransform();
      return;
    }
    if (!graphView.pointerDown || graphView.pointerId !== evt.pointerId) return;
    if (endPrimaryPointer(evt)) return;
    applyGraphTransform();
  };

  el.nodeGraph.addEventListener("pointerup",     endPan);
  el.nodeGraph.addEventListener("pointercancel", endPan);
  el.nodeGraph.addEventListener("pointerleave",  (evt) => {
    endPan(evt);
    if (!graphView.hoverKey) return;
    graphView.hoverKey = "";
    updateNodeGraphHover("");
  });
}

// ─── Scene graph layout & render ──────────────────────────────────────────────

function renderSceneGraphView() {
  if (!el.nodeGraph) return;

  const sceneName  = String(el.scene   && el.scene.value   ? el.scene.value   : "").trim();
  const variantName = String(el.variant && el.variant.value ? el.variant.value : "").trim();
  const runtime = sceneName
    ? ((typeof getRuntimeGraphForScene === "function")
      ? getRuntimeGraphForScene(sceneName)
      : runtimeGraphByScene.get(sceneName))
    : null;

  let cameras = [], objects = [], geometries = [], materials = [], media = [];

  if (runtime) {
    cameras = (runtime.cameras || [])
      .map((c) => ({
        id:          String((c && c.id)   || "").trim(),
        type:        String((c && c.type) || "camera"),
        position:    Array.isArray(c && c.position)    ? c.position.slice(0,3).map(Number)    : null,
        target:      Array.isArray(c && c.target)      ? c.target.slice(0,3).map(Number)      : null,
        up:          Array.isArray(c && c.up)           ? c.up.slice(0,3).map(Number)          : null,
        orientation: Array.isArray(c && c.orientation) ? c.orientation.slice(0,3).map(Number) : null,
        fov:      Number.isFinite(Number(c && c.fov))      ? Number(c.fov)      : null,
        aperture: Number.isFinite(Number(c && c.aperture)) ? Number(c.aperture) : null,
        flength:  Number.isFinite(Number(c && c.flength))  ? Number(c.flength)  : null,
        ipd:      Number.isFinite(Number(c && c.ipd))      ? Number(c.ipd)      : null,
      }))
      .filter((c) => c.id).sort((a, b) => a.id.localeCompare(b.id));

    objects = (runtime.objects || [])
      .map((o) => ({
        id:       String((o && o.id)       || "").trim(),
        geometry: String((o && o.surface)  || "").trim(),
        material: String((o && o.material) || "").trim(),
        medium:   String((o && o.medium)   || "").trim(),
      }))
      .filter((o) => o.id).sort((a, b) => a.id.localeCompare(b.id));

    geometries = (runtime.surfaces || [])
      .map((g) => ({
        id:         String((g && g.id)   || "").trim(),
        type:       String((g && g.type) || "surface"),
        op:         String((g && g.op)   || "").trim(),
        smoothness: Number.isFinite(Number(g && g.smoothness)) ? Number(g.smoothness) : null,
        left_type:  String((g && g.left_type)  || "").trim(),
        right_type: String((g && g.right_type) || "").trim(),
        position:   Array.isArray(g && g.position)   ? g.position.slice(0,3).map(Number)   : null,
        radius:     Number.isFinite(Number(g && g.radius))   ? Number(g.radius)   : null,
        normal:     Array.isArray(g && g.normal)     ? g.normal.slice(0,3).map(Number)     : null,
        distance:   Number.isFinite(Number(g && g.distance)) ? Number(g.distance) : null,
        triangles:  Number.isFinite(Number(g && g.triangles)) ? Number(g.triangles) : null,
        bounds_min: Array.isArray(g && g.bounds_min) ? g.bounds_min.slice(0,3).map(Number) : null,
        bounds_max: Array.isArray(g && g.bounds_max) ? g.bounds_max.slice(0,3).map(Number) : null,
        v0: Array.isArray(g && g.v0) ? g.v0.slice(0,3).map(Number) : null,
        v1: Array.isArray(g && g.v1) ? g.v1.slice(0,3).map(Number) : null,
        v2: Array.isArray(g && g.v2) ? g.v2.slice(0,3).map(Number) : null,
      }))
      .filter((g) => g.id).sort((a, b) => a.id.localeCompare(b.id));

    materials = (runtime.materials || [])
      .map((m) => ({
        id:   String((m && m.id)   || "").trim(),
        type: String((m && m.type) || "material"),
        scalars: Array.isArray(m && m.scalars)
          ? m.scalars.map((s) => ({
              name:  String((s && s.name) || "").trim(),
              value: Number.isFinite(Number(s && s.value)) ? Number(s.value) : (s && s.value),
            })).filter((s) => s.name)
          : [],
        samplers: Array.isArray(m && m.samplers)
          ? m.samplers.map((s) => {
              const mId  = String((m && m.id)   || "").trim();
              const sName = String((s && s.name) || "").trim();
              const sType = String((s && s.type) || "sampler");
              const asset = String((s && s.asset) || "").trim();
              let previewUrl = (sceneName && asset)
                ? `/api/scenes/${encodeURIComponent(sceneName)}/asset?path=${encodeURIComponent(asset)}`
                : "";
              if (!previewUrl && sceneName && mId && sName && sType === "texture") {
                previewUrl = `/api/scenes/${encodeURIComponent(sceneName)}/runtime_texture?material=${encodeURIComponent(mId)}&sampler=${encodeURIComponent(sName)}`;
                if (variantName) previewUrl += `&variant=${encodeURIComponent(variantName)}`;
              }
              return {
                name: sName, type: sType, asset,
                previewUrl,
                color: Array.isArray(s && s.color) ? s.color.slice(0,3).map(Number) : null,
              };
            }).filter((s) => s.name)
          : [],
      }))
      .filter((m) => m.id).sort((a, b) => a.id.localeCompare(b.id));

    media = (runtime.media || [])
      .map((m) => ({
        id:       String((m && m.id)   || "").trim(),
        type:     String((m && m.type) || "medium"),
        sigma_a:  Array.isArray(m && m.sigma_a) ? m.sigma_a.slice(0,3).map(Number) : null,
        sigma_s:  Array.isArray(m && m.sigma_s) ? m.sigma_s.slice(0,3).map(Number) : null,
        emission: Array.isArray(m && m.emission) ? m.emission.slice(0,3).map(Number) : null,
        g:        Number.isFinite(Number(m && m.g)) ? Number(m.g) : null,
      }))
      .filter((m) => m.id).sort((a, b) => a.id.localeCompare(b.id));

  } else {
    const source = el.sceneSource ? String(el.sceneSource.value || "") : "";
    const model  = parseSceneEditModel(source);
    cameras    = (model.cameras || []).slice().sort((a, b) => a.id.localeCompare(b.id));
    objects    = Array.from(model.objects.values()).sort((a, b) => a.id.localeCompare(b.id));
    geometries = Array.from(model.geometries.values()).map((g) => ({ ...g })).sort((a, b) => a.id.localeCompare(b.id));
    materials  = Array.from((model.materials || new Map()).values())
      .map((m) => ({ ...m, scalars: [], samplers: [] }))
      .sort((a, b) => a.id.localeCompare(b.id));
    media = [];
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
      const ar  = ranks.get(aId) || [];
      const br  = ranks.get(bId) || [];
      const as  = ar.length ? (ar.reduce((s, v) => s + v, 0) / ar.length) : 1e9;
      const bs  = br.length ? (br.reduce((s, v) => s + v, 0) / br.length) : 1e9;
      if (Math.abs(as - bs) > 1e-6) return as - bs;
      return aId.localeCompare(bId);
    });
  };

  geometries = sortByObjectRefs(geometries, (g) => g.id, (o) => o.geometry);
  materials  = sortByObjectRefs(materials,  (m) => m.id, (o) => o.material);
  media      = sortByObjectRefs(media,      (m) => m.id, (o) => o.medium);

  const nodeW        = 248;
  const nodeH        = 48;
  const topPad       = 70;
  const rowGap       = 14;
  const bottomPad    = 40;
  const laneGap      = 44;
  const sectionGap   = 84;
  const graphRect    = el.nodeGraph.getBoundingClientRect();
  const availableH   = Math.max(320, (Number(graphRect && graphRect.height) || 720) - topPad - bottomPad);
  const nominalRows  = Math.max(4, Math.floor(availableH / (nodeH + rowGap)));
  const maxRowsPerLane = clamp(nominalRows, 4, 16);

  const nodes      = [];
  const pos        = new Map();
  const columns    = [];
  const kindLayout = new Map();

  const detailsNodeHeight = (expanded, rowCount, previewCount) => {
    if (!expanded) return nodeH;
    return nodeH + 6 + Math.max(0, rowCount) * 18 + (previewCount > 0 ? 48 : 0);
  };

  const totalSamplerCount = materials.reduce((sum, m) => sum + (Array.isArray(m.samplers) ? m.samplers.length : 0), 0);
  const totalVec3Count = geometries.reduce((s, g) => s + [g.position, g.normal, g.v0, g.v1, g.v2].filter(Array.isArray).length, 0)
    + cameras.reduce((s, c) => s + [c.position, c.target, c.up].filter(Array.isArray).length, 0);
  const totalScalarCount = geometries.reduce((s, g) => s + [g.radius, g.distance].filter(Number.isFinite).length, 0)
    + cameras.reduce((s, c) => s + [c.fov, c.aperture, c.flength].filter(Number.isFinite).length, 0);
  const totalValueCount = totalSamplerCount + totalVec3Count + totalScalarCount;

  // Data-flow column order: value nodes on far left, then components, then aggregates
  const kinds = [
    { key: "sampler",  title: "Values",   color: "#b07840", count: Math.max(1, totalValueCount) },
    { key: "geometry", title: "Surface",  color: "#4f9a8f", count: geometries.length },
    { key: "material", title: "Material", color: "#5a9a4f", count: materials.length  },
    { key: "medium",   title: "Medium",   color: "#7d66b4", count: media.length      },
    { key: "object",   title: "Object",   color: "#9a6846", count: objects.length    },
    { key: "camera",   title: "Camera",   color: "#5a88cf", count: cameras.length    },
  ];
  let xCursor = 40;
  kinds.forEach((k) => {
    const laneCount = Math.max(1, Math.ceil(Math.max(1, k.count) / maxRowsPerLane));
    const laneXs    = [];
    for (let i = 0; i < laneCount; i += 1) laneXs.push(xCursor + i * (nodeW + laneGap));
    const laneNextY = new Array(laneCount).fill(topPad);
    kindLayout.set(k.key, { laneCount, laneXs, laneNextY, color: k.color, title: k.title, x: xCursor });
    columns.push({ key: k.key, title: k.title, x: xCursor, color: k.color });
    xCursor += laneCount * (nodeW + laneGap) - laneGap + sectionGap;
  });
  const viewW = Math.max(980, xCursor - sectionGap + 40);

  const pushNode = (kind, id, subtitle, extras) => {
    const ext    = (extras && typeof extras === "object") ? extras : {};
    const layout = kindLayout.get(ext.columnKey || kind);
    if (!layout) return;
    const h   = Number(ext.h) > 0 ? Number(ext.h) : nodeH;
    const w   = Number(ext.w) > 0 ? Number(ext.w) : nodeW;
    let lane  = 0;
    for (let i = 1; i < layout.laneNextY.length; i += 1) {
      if (layout.laneNextY[i] < layout.laneNextY[lane]) lane = i;
    }
    const y    = layout.laneNextY[lane] || topPad;
    const node = {
      key:              `${kind}:${id}`,
      id,
      label:            String(ext.label || ""),
      kind,
      subtitle:         subtitle || "",
      x:                layout.laneXs[lane],
      y,
      w,
      h,
      color:            layout.color,
      expanded:         !!ext.expanded,
      propertyRows:     Array.isArray(ext.propertyRows)     ? ext.propertyRows     : [],
      texturePreviews:  Array.isArray(ext.texturePreviews)  ? ext.texturePreviews  : [],
      csgOp:            ext.csgOp || "",
      connections:      ext.connections || {},
      ports:            ext.ports || null,
      samplerColor:     Array.isArray(ext.samplerColor) && ext.samplerColor.length >= 3 ? ext.samplerColor.slice(0, 3) : null,
      samplerPreviewUrl: String(ext.samplerPreviewUrl || ""),
      valueVec3:        Array.isArray(ext.valueVec3) && ext.valueVec3.length >= 3 ? ext.valueVec3.slice(0, 3) : null,
      valueNum:         Number.isFinite(Number(ext.valueNum)) ? Number(ext.valueNum) : null,
      updateInfo:       (ext.updateInfo && typeof ext.updateInfo === "object") ? ext.updateInfo : null,
      materialId:        String(ext.materialId  || ""),
      samplerName:       String(ext.samplerName || ""),
    };
    node.manualKey = `${sceneName || ""}|${node.key}`;
    const manual   = graphView.manualNodePos.get(node.manualKey);
    if (manual && Number.isFinite(manual.x) && Number.isFinite(manual.y)) {
      node.x = manual.x;
      node.y = manual.y;
    }
    nodes.push(node);
    pos.set(node.key, node);
    layout.laneNextY[lane] = y + node.h + rowGap;
  };

  cameras.forEach((c) => {
    const nodeKey  = `camera:${c.id}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows  = [];
    const camInputPorts = [];
    const camConnections = {};

    propertyRows.push({ key: "type", value: String(c.type || "camera") });

    const addCamVec3 = (prop, vec) => {
      const rowIndex = propertyRows.length;
      propertyRows.push({ key: prop, value: graphVec3Label(vec) });
      camInputPorts.push({ id: prop, type: "vec3", label: prop, rowIndex });
      camConnections[prop] = `vec3:${c.id}:${prop}`;
      pushNode("vec3", `${c.id}:${prop}`, graphVec3Label(vec), {
        columnKey: "sampler", w: 220, h: nodeH + 30, label: prop,
        ports: { inputs: [], outputs: [{ id: "out", type: "vec3", label: "" }] },
        valueVec3: vec.slice(),
        updateInfo: runtime ? null : { kind: "camera_vec3", parentId: c.id, prop },
      });
    };
    const addCamScalar = (prop, val) => {
      const rowIndex = propertyRows.length;
      propertyRows.push({ key: prop, value: formatGraphNumeric(val) });
      camInputPorts.push({ id: prop, type: "scalar", label: prop, rowIndex });
      camConnections[prop] = `scalar:${c.id}:${prop}`;
      pushNode("scalar", `${c.id}:${prop}`, formatGraphNumeric(val), {
        columnKey: "sampler", w: 220, h: nodeH + 26, label: prop,
        ports: { inputs: [], outputs: [{ id: "out", type: "scalar", label: "" }] },
        valueNum: val,
        updateInfo: runtime ? null : { kind: "camera_scalar", parentId: c.id, prop },
      });
    };

    if (Array.isArray(c.position))    addCamVec3("position", c.position);
    if (Array.isArray(c.target))      addCamVec3("target",   c.target);
    if (Array.isArray(c.up))          addCamVec3("up",       c.up);
    if (Array.isArray(c.orientation)) propertyRows.push({ key: "orientation", value: graphVec3Label(c.orientation) });
    if (Number.isFinite(c.fov))       addCamScalar("fov",      c.fov);
    if (Number.isFinite(c.aperture))  addCamScalar("aperture", c.aperture);
    if (Number.isFinite(c.flength))   addCamScalar("flength",  c.flength);
    if (Number.isFinite(c.ipd))       propertyRows.push({ key: "ipd", value: formatGraphNumeric(c.ipd) });

    pushNode("camera", c.id, c.type || "camera", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded, propertyRows, texturePreviews: [],
      ports: { inputs: camInputPorts, outputs: [] },
      connections: camConnections,
    });
  });

  objects.forEach((o) => {
    const nodeKey  = `object:${o.id}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows = [
      { key: "surface",  value: String(o.geometry || "-") },
      { key: "material", value: String(o.material || "-") },
      { key: "medium",   value: String(o.medium   || "-") },
    ];
    pushNode("object", o.id, runtime ? "runtime object" : "scene object", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded, propertyRows, texturePreviews: [],
      connections: { geometry: o.geometry || "", material: o.material || "", medium: o.medium || "" },
    });
  });

  geometries.forEach((g) => {
    const nodeKey  = `geometry:${g.id}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows  = [];
    const geoInputPorts = [];
    const geoConnections = {};

    propertyRows.push({ key: "type", value: String(g.type || "surface") });

    const addGeoVec3 = (prop, vec) => {
      const rowIndex = propertyRows.length;
      propertyRows.push({ key: prop, value: graphVec3Label(vec) });
      geoInputPorts.push({ id: prop, type: "vec3", label: prop, rowIndex });
      geoConnections[prop] = `vec3:${g.id}:${prop}`;
      pushNode("vec3", `${g.id}:${prop}`, graphVec3Label(vec), {
        columnKey: "sampler", w: 220, h: nodeH + 30, label: prop,
        ports: { inputs: [], outputs: [{ id: "out", type: "vec3", label: "" }] },
        valueVec3: vec.slice(),
        updateInfo: runtime ? null : { kind: "geometry_vec3", parentId: g.id, prop },
      });
    };
    const addGeoScalar = (prop, val) => {
      const rowIndex = propertyRows.length;
      propertyRows.push({ key: prop, value: formatGraphNumeric(val) });
      geoInputPorts.push({ id: prop, type: "scalar", label: prop, rowIndex });
      geoConnections[prop] = `scalar:${g.id}:${prop}`;
      pushNode("scalar", `${g.id}:${prop}`, formatGraphNumeric(val), {
        columnKey: "sampler", w: 220, h: nodeH + 26, label: prop,
        ports: { inputs: [], outputs: [{ id: "out", type: "scalar", label: "" }] },
        valueNum: val,
        updateInfo: runtime ? null : { kind: "geometry_scalar", parentId: g.id, prop },
      });
    };

    if (g.type === "csg") {
      if (g.op)                          propertyRows.push({ key: "op",         value: g.op });
      if (Number.isFinite(g.smoothness)) propertyRows.push({ key: "smoothness", value: formatGraphNumeric(g.smoothness) });
      if (g.left_type)                   propertyRows.push({ key: "left",       value: g.left_type  });
      if (g.right_type)                  propertyRows.push({ key: "right",      value: g.right_type });
    }
    if (Array.isArray(g.position))    addGeoVec3("position",   g.position);
    if (Number.isFinite(g.radius))    addGeoScalar("radius",   g.radius);
    if (Array.isArray(g.normal))      addGeoVec3("normal",     g.normal);
    if (Number.isFinite(g.distance))  addGeoScalar("distance", g.distance);
    if (Number.isFinite(g.triangles)) propertyRows.push({ key: "triangles", value: formatGraphNumeric(g.triangles) });
    if (Array.isArray(g.bounds_min))  propertyRows.push({ key: "bounds_min", value: graphVec3Label(g.bounds_min) });
    if (Array.isArray(g.bounds_max))  propertyRows.push({ key: "bounds_max", value: graphVec3Label(g.bounds_max) });
    if (Array.isArray(g.v0))          addGeoVec3("v0", g.v0);
    if (Array.isArray(g.v1))          addGeoVec3("v1", g.v1);
    if (Array.isArray(g.v2))          addGeoVec3("v2", g.v2);

    pushNode("geometry", g.id, g.type || "surface", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded, propertyRows, texturePreviews: [],
      csgOp: g.type === "csg" ? String(g.op || "union") : "",
      ports: { inputs: geoInputPorts, outputs: [{ id: "out", type: "geometry", label: "" }] },
      connections: geoConnections,
    });
  });

  materials.forEach((m) => {
    const nodeKey   = `material:${String(m.id || "")}`;
    const expanded  = graphView.expandedNodeKeys.has(nodeKey);
    const scalars   = Array.isArray(m && m.scalars)  ? m.scalars  : [];
    const mSamplers = Array.isArray(m && m.samplers) ? m.samplers : [];

    // Push one sampler node per slot — these feed into the material's input ports
    mSamplers.forEach((s) => {
      const sName   = String(s && s.name || "");
      const sType   = String(s && s.type || "sampler");
      const isTexture = sType === "texture";
      const outType = isTexture ? "texture" : "color";
      const sColor  = !isTexture && Array.isArray(s && s.color) && s.color.length >= 3 ? s.color.slice(0, 3) : null;
      let subtitle = sName;
      if (isTexture) {
        const asset = String(s && s.asset || "");
        subtitle = asset ? (asset.split("/").pop() || sName) : sName;
      } else if (sColor) {
        subtitle = graphColorLabel(sColor);
      }
      const samplerPreviewUrl = isTexture && s && s.previewUrl ? String(s.previewUrl) : "";
      const samplerH = samplerPreviewUrl ? nodeH + 56 : (sColor ? nodeH + 46 : nodeH);
      pushNode("sampler", `${m.id}:${sName}`, subtitle, {
        w: 220,
        h: samplerH,
        label: sName,
        ports: { inputs: [], outputs: [{ id: "out", type: outType, label: "" }] },
        samplerColor:      sColor,
        samplerPreviewUrl,
        materialId:  m.id,
        samplerName: sName,
      });
    });

    // Build per-node input ports for the material (one per sampler slot).
    // rowIndex skips the "type" row (1) and any scalar rows so the port dot
    // aligns with the correct property row in the expanded body.
    const matInputPorts = mSamplers.map((s, i) => {
      const sType = String(s && s.type || "sampler");
      return {
        id:       `sampler:${String(s && s.name || "")}`,
        type:     sType === "texture" ? "texture" : "color",
        label:    String(s && s.name || ""),
        rowIndex: 1 + scalars.length + i,
      };
    });
    const matConnections = {};
    mSamplers.forEach((s) => {
      const sName = String(s && s.name || "");
      if (sName) matConnections[`sampler:${sName}`] = `${m.id}:${sName}`;
    });

    const propertyRows = [{ key: "type", value: String(m && m.type ? m.type : "material") }];
    scalars.forEach((s) => propertyRows.push({
      key:       String(s && s.name  ? s.name  : "scalar"),
      scalar:    (s && s.value !== undefined) ? s.value : 0,
      scalarKey: String(s && s.name  ? s.name  : ""),
      materialId: m.id,
    }));
    mSamplers.forEach((s) => {
      const row = { key: String(s && s.name ? s.name : "sampler"), value: String(s && s.type ? s.type : "sampler") };
      if (Array.isArray(s && s.color) && s.color.length >= 3) row.color = s.color.slice(0, 3);
      propertyRows.push(row);
    });
    const texturePreviews = mSamplers
      .filter((s) => !!(s && s.previewUrl))
      .map((s) => ({ name: String(s.name || "texture"), url: String(s.previewUrl) }));

    pushNode("material", m.id, m.type || "material", {
      h: detailsNodeHeight(expanded, propertyRows.length, texturePreviews.length),
      expanded, propertyRows, texturePreviews,
      ports: { inputs: matInputPorts, outputs: [{ id: "out", type: "material", label: "" }] },
      connections: matConnections,
    });
  });

  media.forEach((m) => {
    const nodeKey  = `medium:${String(m.id || "")}`;
    const expanded = graphView.expandedNodeKeys.has(nodeKey);
    const propertyRows = [{ key: "type", value: String(m && m.type ? m.type : "medium") }];
    if (Array.isArray(m.sigma_a))  propertyRows.push({ key: "sigma_a",  value: graphColorLabel(m.sigma_a),  color: m.sigma_a.slice(0,3)  });
    if (Array.isArray(m.sigma_s))  propertyRows.push({ key: "sigma_s",  value: graphColorLabel(m.sigma_s),  color: m.sigma_s.slice(0,3)  });
    if (Array.isArray(m.emission)) propertyRows.push({ key: "emission", value: graphColorLabel(m.emission), color: m.emission.slice(0,3) });
    if (Number.isFinite(m.g))      propertyRows.push({ key: "g",        value: formatGraphNumeric(m.g)                                   });
    pushNode("medium", m.id, m.type || "medium", {
      h: detailsNodeHeight(expanded, propertyRows.length, 0),
      expanded, propertyRows, texturePreviews: [],
    });
  });

  // Links flow left→right: component → Object
  const links = [];
  objects.forEach((o) => {
    const obj = pos.get(`object:${o.id}`);
    const geo = pos.get(`geometry:${o.geometry || ""}`);
    const mat = pos.get(`material:${o.material || ""}`);
    const med = pos.get(`medium:${o.medium   || ""}`);
    if (obj && geo) links.push({ from: geo, to: obj, field: "geometry" });
    if (obj && mat) links.push({ from: mat, to: obj, field: "material" });
    if (obj && med) links.push({ from: med, to: obj, field: "medium"   });
  });

  // Sampler → Material links (fixed — always connected, not user-removable)
  materials.forEach((m) => {
    const matNode = pos.get(`material:${m.id}`);
    if (!matNode) return;
    (Array.isArray(m.samplers) ? m.samplers : []).forEach((s) => {
      const sName = String(s && s.name || "");
      if (!sName) return;
      const samplerNode = pos.get(`sampler:${m.id}:${sName}`);
      if (!samplerNode) return;
      links.push({ from: samplerNode, to: matNode, field: `sampler:${sName}`, fixed: true });
    });
  });

  // Vec3/Scalar → Geometry links (fixed)
  geometries.forEach((g) => {
    const geoNode = pos.get(`geometry:${g.id}`);
    if (!geoNode) return;
    const addLink = (prop) => {
      const valueNode = pos.get(`vec3:${g.id}:${prop}`) || pos.get(`scalar:${g.id}:${prop}`);
      if (valueNode) links.push({ from: valueNode, to: geoNode, field: prop, fixed: true });
    };
    if (Array.isArray(g.position))   addLink("position");
    if (Array.isArray(g.normal))     addLink("normal");
    if (Number.isFinite(g.radius))   addLink("radius");
    if (Number.isFinite(g.distance)) addLink("distance");
    if (Array.isArray(g.v0))         addLink("v0");
    if (Array.isArray(g.v1))         addLink("v1");
    if (Array.isArray(g.v2))         addLink("v2");
  });

  // Vec3/Scalar → Camera links (fixed)
  cameras.forEach((c) => {
    const camNode = pos.get(`camera:${c.id}`);
    if (!camNode) return;
    const addLink = (prop) => {
      const valueNode = pos.get(`vec3:${c.id}:${prop}`) || pos.get(`scalar:${c.id}:${prop}`);
      if (valueNode) links.push({ from: valueNode, to: camNode, field: prop, fixed: true });
    };
    if (Array.isArray(c.position))  addLink("position");
    if (Array.isArray(c.target))    addLink("target");
    if (Array.isArray(c.up))        addLink("up");
    if (Number.isFinite(c.fov))     addLink("fov");
    if (Number.isFinite(c.aperture)) addLink("aperture");
    if (Number.isFinite(c.flength)) addLink("flength");
  });

  const viewH = Math.max(
    topPad + rowGap + bottomPad,
    ...Array.from(kindLayout.values()).map((layout) =>
      Math.max(topPad, ...layout.laneNextY) + bottomPad),
    ...nodes.map((n) => n.y + n.h + bottomPad)
  );

  graphView.data   = { columns, nodes, links, viewW, viewH };
  graphView.worldW = viewW;
  graphView.worldH = viewH;
  if (!graphView.userAdjusted) fitGraphToViewport();
  renderNodeGraphDOM();
  applyGraphTransform();

  if (el.graphLegend) {
    const tbody = el.graphLegend.querySelector("tbody");
    if (tbody) {
      tbody.innerHTML = "";
      [["Camera", cameras.length], ["Object", objects.length],
       ["Surface", geometries.length], ["Material", materials.length],
       ["Sampler", totalSamplerCount], ["Vec3", totalVec3Count],
       ["Scalar", totalScalarCount], ["Medium", media.length]]
        .forEach(([label, count]) => {
          const tr = document.createElement("tr");
          const td1 = document.createElement("td"); td1.textContent = String(label);
          const td2 = document.createElement("td"); td2.textContent = String(count);
          tr.appendChild(td1); tr.appendChild(td2); tbody.appendChild(tr);
        });
    }
  }
}
