function formatSceneNumber(v, fallback) {
  const n = Number(v);
  const x = Number.isFinite(n) ? n : (fallback || 0);
  const s = x.toFixed(6);
  return s.replace(/\.?0+$/, "") || "0";
}

function readSceneNumber(v, fallback) {
  const n = Number(v);
  return Number.isFinite(n) ? n : fallback;
}

function findSceneGroupRange(source, groupName) {
  const re = new RegExp(`\\b${groupName}\\s*=\\s*\\{`, "m");
  const m = re.exec(source || "");
  if (!m) return null;
  const open = source.indexOf("{", m.index);
  if (open < 0) return null;
  let depth = 0;
  for (let i = open; i < source.length; i += 1) {
    const ch = source[i];
    if (ch === "{") depth += 1;
    else if (ch === "}") {
      depth -= 1;
      if (depth === 0) {
        return {
          groupStart: m.index,
          groupEnd: i + 1,
          bodyStart: open + 1,
          bodyEnd: i,
        };
      }
    }
  }
  return null;
}

function splitTopLevelSceneEntries(source, range) {
  const out = [];
  if (!range) return out;
  const body = source.slice(range.bodyStart, range.bodyEnd);
  let i = 0;
  while (i < body.length) {
    while (i < body.length && /\s/.test(body[i])) i += 1;
    const m = /^([A-Za-z0-9_\-]+)\s*=\s*\{/.exec(body.slice(i));
    if (!m) {
      i += 1;
      continue;
    }
    const name = m[1];
    const localStart = i;
    const openLocal = i + m[0].lastIndexOf("{");
    let depth = 0;
    let endLocal = -1;
    for (let j = openLocal; j < body.length; j += 1) {
      const ch = body[j];
      if (ch === "{") depth += 1;
      else if (ch === "}") {
        depth -= 1;
        if (depth === 0) {
          endLocal = j + 1;
          break;
        }
      }
    }
    if (endLocal < 0) break;
    const entryStart = range.bodyStart + localStart;
    const entryEnd = range.bodyStart + endLocal;
    const bodyStart = range.bodyStart + openLocal + 1;
    const bodyEnd = entryEnd - 1;
    out.push({
      id: name,
      entryStart,
      entryEnd,
      bodyStart,
      bodyEnd,
      body: source.slice(bodyStart, bodyEnd),
    });
    i = endLocal;
  }
  return out;
}

function readSceneRefProp(block, key) {
  const m = new RegExp(`\\b${key}\\s*=\\s*([A-Za-z0-9_.\\-]+)`, "i").exec(block || "");
  return m ? String(m[1] || "").trim() : "";
}

function readSceneStringProp(block, key) {
  const m = new RegExp(`\\b${key}\\s*=\\s*([^\\n\\r]+)`, "i").exec(block || "");
  return m ? String(m[1] || "").trim() : "";
}

function findNamedBlockRange(block, key) {
  const m = new RegExp(`\\b${key}\\s*=\\s*\\{`, "i").exec(block || "");
  if (!m) return null;
  const open = block.indexOf("{", m.index);
  if (open < 0) return null;
  let depth = 0;
  for (let i = open; i < block.length; i += 1) {
    const ch = block[i];
    if (ch === "{") depth += 1;
    else if (ch === "}") {
      depth -= 1;
      if (depth === 0) {
        return {
          start: m.index,
          end: i + 1,
          bodyStart: open + 1,
          bodyEnd: i,
          body: block.slice(open + 1, i),
        };
      }
    }
  }
  return null;
}

function readSceneVec3Prop(block, key, fallback) {
  const fb = fallback || [0, 0, 0];
  const inline = new RegExp(`\\b${key}\\s*=\\s*vec3\\(([^\\)]*)\\)`, "i").exec(block || "");
  if (inline) {
    const p = String(inline[1] || "").split(",").map((x) => Number(x.trim()));
    if (p.length >= 3 && Number.isFinite(p[0]) && Number.isFinite(p[1]) && Number.isFinite(p[2])) {
      return [p[0], p[1], p[2]];
    }
  }
  const group = findNamedBlockRange(block || "", key);
  if (group) {
    const x = readSceneNumber(readSceneStringProp(group.body, "x"), fb[0]);
    const y = readSceneNumber(readSceneStringProp(group.body, "y"), fb[1]);
    const z = readSceneNumber(readSceneStringProp(group.body, "z"), fb[2]);
    return [x, y, z];
  }
  return fb.slice();
}

function addVec3(a, b) {
  return [
    (Number(a && a[0]) || 0) + (Number(b && b[0]) || 0),
    (Number(a && a[1]) || 0) + (Number(b && b[1]) || 0),
    (Number(a && a[2]) || 0) + (Number(b && b[2]) || 0),
  ];
}

function subVec3(a, b) {
  return [
    (Number(a && a[0]) || 0) - (Number(b && b[0]) || 0),
    (Number(a && a[1]) || 0) - (Number(b && b[1]) || 0),
    (Number(a && a[2]) || 0) - (Number(b && b[2]) || 0),
  ];
}

function scaleVec3(a, s) {
  const k = Number(s) || 0;
  return [
    (Number(a && a[0]) || 0) * k,
    (Number(a && a[1]) || 0) * k,
    (Number(a && a[2]) || 0) * k,
  ];
}

function vec3Literal(v, fallback) {
  const fb = fallback || [0, 0, 0];
  return `vec3(${formatSceneNumber(v && v[0], fb[0])}, ${formatSceneNumber(v && v[1], fb[1])}, ${formatSceneNumber(v && v[2], fb[2])})`;
}

function upsertSceneVec3Prop(block, key, vec, indent) {
  const baseIndent = String(indent || "");
  const lit = vec3Literal(vec, [0, 0, 0]);
  const inlineRe = new RegExp(`(\\b${key}\\s*=\\s*)vec3\\([^\\)]*\\)`, "i");
  if (inlineRe.test(block || "")) return String(block || "").replace(inlineRe, `$1${lit}`);
  const nested = findNamedBlockRange(block || "", key);
  if (nested) {
    return `${block.slice(0, nested.start)}${key} = ${lit}${block.slice(nested.end)}`;
  }
  const trimmed = String(block || "").replace(/\s*$/, "");
  const suffix = String(block || "").slice(trimmed.length);
  const join = trimmed.length > 0 ? (trimmed.endsWith("\n") ? "" : "\n") : "";
  return `${trimmed}${join}${baseIndent}${key} = ${lit}${suffix}`;
}

function removeNamedSceneBlock(block, key) {
  const found = findNamedBlockRange(block || "", key);
  if (!found) return String(block || "");
  let out = `${block.slice(0, found.start)}${block.slice(found.end)}`;
  out = out.replace(/\n{3,}/g, "\n\n");
  return out;
}

function parseSceneEditModel(source) {
  const text = String(source || "");
  const cameraGroup = findSceneGroupRange(text, "camera");
  const geometryGroup = findSceneGroupRange(text, "geometry");
  const objectGroup = findSceneGroupRange(text, "object");
  const materialGroup = findSceneGroupRange(text, "material");
  const cameras = [];
  const geometries = new Map();
  const objects = new Map();
  const materials = new Map();
  splitTopLevelSceneEntries(text, cameraGroup).forEach((entry) => {
    cameras.push({
      ...entry,
      type: readSceneStringProp(entry.body, "type").replace(/["']/g, "").toLowerCase(),
    });
  });
  splitTopLevelSceneEntries(text, geometryGroup).forEach((entry) => {
    geometries.set(entry.id, {
      ...entry,
      type: readSceneStringProp(entry.body, "type").replace(/["']/g, "").toLowerCase(),
    });
  });
  splitTopLevelSceneEntries(text, objectGroup).forEach((entry) => {
    const geometry = readSceneRefProp(entry.body, "geometry");
    if (!geometry) return;
    objects.set(entry.id, {
      ...entry,
      geometry,
      material: readSceneRefProp(entry.body, "material"),
    });
  });
  splitTopLevelSceneEntries(text, materialGroup).forEach((entry) => {
    materials.set(entry.id, {
      ...entry,
      type: readSceneStringProp(entry.body, "type").replace(/["']/g, "").toLowerCase(),
    });
  });
  return {
    source: text,
    cameraGroup,
    geometryGroup,
    objectGroup,
    materialGroup,
    cameras,
    geometries,
    objects,
    materials,
  };
}

function getObjectTransformFromSource(source, objectId) {
  const model = parseSceneEditModel(source);
  const obj = model.objects.get(objectId);
  if (!obj) return null;
  const geo = model.geometries.get(obj.geometry);
  if (!geo) return null;
  const geoType = String(geo.type || "").toLowerCase();
  const modifiers = findNamedBlockRange(geo.body, "modifiers");
  const modsBody = modifiers ? modifiers.body : "";
  let translation = readSceneVec3Prop(modsBody, "translation", [0, 0, 0]);
  if (geoType === "sphere" || geoType === "point") {
    translation = readSceneVec3Prop(geo.body, "position", [0, 0, 0]);
  } else if (geoType === "triangle") {
    const vecData = findNamedBlockRange(geo.body, "vecdata");
    const vBody = vecData ? vecData.body : geo.body;
    const v0 = readSceneVec3Prop(vBody, "v0", [0, 0, 0]);
    const v1 = readSceneVec3Prop(vBody, "v1", [0, 0, 0]);
    const v2 = readSceneVec3Prop(vBody, "v2", [0, 0, 0]);
    translation = scaleVec3(addVec3(addVec3(v0, v1), v2), 1 / 3);
  }
  return {
    objectId,
    geometryId: obj.geometry,
    materialId: obj.material || "",
    geometryType: geoType,
    translation,
    rotation: readSceneVec3Prop(modsBody, "rotation", [0, 0, 0]),
    scale: readSceneVec3Prop(modsBody, "scale", [1, 1, 1]),
  };
}

function geometryEntryIndent(source, entryStart) {
  const lineStart = source.lastIndexOf("\n", Math.max(0, entryStart - 1)) + 1;
  const linePrefix = source.slice(lineStart, entryStart);
  const m = /^(\s*)/.exec(linePrefix);
  return m ? m[1] : "";
}

function replaceGeometryBody(source, geo, newGeoBody) {
  return `${source.slice(0, geo.bodyStart)}${newGeoBody}${source.slice(geo.bodyEnd)}`;
}

function buildModifiersBlock(indent, transform) {
  const inner = `${indent}\t`;
  const t = transform.translation || [0, 0, 0];
  const r = transform.rotation || [0, 0, 0];
  const s = transform.scale || [1, 1, 1];
  return `${indent}modifiers = {\n`
    + `${inner}rotation = vec3(${formatSceneNumber(r[0], 0)}, ${formatSceneNumber(r[1], 0)}, ${formatSceneNumber(r[2], 0)})\n`
    + `${inner}scale = vec3(${formatSceneNumber(s[0], 1)}, ${formatSceneNumber(s[1], 1)}, ${formatSceneNumber(s[2], 1)})\n`
    + `${inner}translation = vec3(${formatSceneNumber(t[0], 0)}, ${formatSceneNumber(t[1], 0)}, ${formatSceneNumber(t[2], 0)})\n`
    + `${indent}}`;
}

function updateObjectTransformInSource(source, objectId, transform, options) {
  const model = parseSceneEditModel(source);
  const obj = model.objects.get(String(objectId || ""));
  if (!obj) throw new Error("object not found");
  const geo = model.geometries.get(obj.geometry);
  if (!geo) throw new Error("geometry not found");
  const geoType = String(geo.type || "").toLowerCase();
  const deltaTranslation = Array.isArray(options && options.deltaTranslation)
    ? options.deltaTranslation.slice(0, 3).map((v) => Number(v) || 0)
    : null;
  const useDelta = !!(options && options.useDelta && deltaTranslation);

  const geoBody = source.slice(geo.bodyStart, geo.bodyEnd);
  const entryIndent = `${geometryEntryIndent(source, geo.entryStart)}\t`;

  if (geoType === "mesh") {
    const existingModifiers = findNamedBlockRange(geoBody, "modifiers");
    const block = buildModifiersBlock(entryIndent, transform);
    let newGeoBody = geoBody;
    if (existingModifiers) {
      newGeoBody = `${geoBody.slice(0, existingModifiers.start)}${block}${geoBody.slice(existingModifiers.end)}`;
    } else {
      const trimmed = geoBody.replace(/\s*$/, "");
      const suffix = geoBody.slice(trimmed.length);
      const leadNewline = trimmed.length > 0 && !trimmed.endsWith("\n") ? "\n" : "";
      newGeoBody = `${trimmed}${leadNewline}${block}\n${suffix.replace(/^\s*/, "")}`;
    }
    return replaceGeometryBody(source, geo, newGeoBody);
  }

  if (geoType === "sphere" || geoType === "point") {
    const currPos = readSceneVec3Prop(geoBody, "position", [0, 0, 0]);
    const nextPos = useDelta ? addVec3(currPos, deltaTranslation) : (transform.translation || currPos);
    let newGeoBody = upsertSceneVec3Prop(geoBody, "position", nextPos, entryIndent);
    newGeoBody = removeNamedSceneBlock(newGeoBody, "modifiers");
    return replaceGeometryBody(source, geo, newGeoBody);
  }

  if (geoType === "triangle") {
    const vecData = findNamedBlockRange(geoBody, "vecdata");
    if (!vecData) throw new Error("triangle vecdata not found");
    const vBody = vecData.body;
    const v0 = readSceneVec3Prop(vBody, "v0", [0, 0, 0]);
    const v1 = readSceneVec3Prop(vBody, "v1", [0, 0, 0]);
    const v2 = readSceneVec3Prop(vBody, "v2", [0, 0, 0]);
    const centroid = scaleVec3(addVec3(addVec3(v0, v1), v2), 1 / 3);
    const delta = useDelta
      ? deltaTranslation
      : subVec3(transform.translation || centroid, centroid);
    let nextVBody = vBody;
    nextVBody = upsertSceneVec3Prop(nextVBody, "v0", addVec3(v0, delta), `${entryIndent}\t`);
    nextVBody = upsertSceneVec3Prop(nextVBody, "v1", addVec3(v1, delta), `${entryIndent}\t`);
    nextVBody = upsertSceneVec3Prop(nextVBody, "v2", addVec3(v2, delta), `${entryIndent}\t`);
    let newGeoBody = `${geoBody.slice(0, vecData.bodyStart)}${nextVBody}${geoBody.slice(vecData.bodyEnd)}`;
    newGeoBody = removeNamedSceneBlock(newGeoBody, "modifiers");
    return replaceGeometryBody(source, geo, newGeoBody);
  }

  throw new Error(`move/edit not supported for geometry type: ${geoType}`);
}

function ensureSceneGroup(source, groupName) {
  const existing = findSceneGroupRange(source, groupName);
  if (existing) return source;
  const suffix = source.endsWith("\n") ? "" : "\n";
  return `${source}${suffix}\n${groupName} = {\n}\n`;
}

function appendEntryToSceneGroup(source, groupName, entryText) {
  const text = ensureSceneGroup(source, groupName);
  const range = findSceneGroupRange(text, groupName);
  if (!range) return text;
  const body = text.slice(range.bodyStart, range.bodyEnd);
  const pre = body.replace(/\s*$/, "");
  const post = body.slice(pre.length);
  const join = pre.length > 0 ? (pre.endsWith("\n") ? "" : "\n") : "";
  const nextBody = `${pre}${join}${entryText}\n${post.replace(/^\s*/, "")}`;
  return `${text.slice(0, range.bodyStart)}${nextBody}${text.slice(range.bodyEnd)}`;
}

function sanitizeSceneId(raw, fallback) {
  const cleaned = String(raw || "").trim().replace(/[^A-Za-z0-9_\-]/g, "_");
  if (cleaned) return cleaned;
  return fallback;
}

function uniqueSceneId(existing, base) {
  let id = base;
  let i = 1;
  while (existing.has(id)) {
    id = `${base}_${i}`;
    i += 1;
  }
  return id;
}

function addMeshObjectToSceneSource(source, options) {
  const model = parseSceneEditModel(source);
  const geometryIds = new Set(Array.from(model.geometries.keys()));
  const objectIds = new Set(Array.from(model.objects.keys()));
  const geometryBase = sanitizeSceneId(options.geometryId, "geo_new");
  const objectBase = sanitizeSceneId(options.objectId, "obj_new");
  const geometryId = uniqueSceneId(geometryIds, geometryBase);
  const objectId = uniqueSceneId(objectIds, objectBase);
  const materialIds = Array.from((model.materials || new Map()).keys());
  const material = String(options.material || "").trim() || (materialIds[0] || "");
  if (!material) throw new Error("no material available; create a material first");

  const transform = {
    translation: options.translation || [0, 0, 0],
    rotation: options.rotation || [0, 0, 0],
    scale: options.scale || [1, 1, 1],
  };
  const modifiers = buildModifiersBlock("\t\t", transform);
  const geometryEntry = `\t${geometryId} = {\n`
    + `\t\ttype = mesh\n`
    + `\t\tsource = gen(${options.generator || "cube"})\n`
    + `\t\tresolution = 24\n`
    + `${modifiers}\n`
    + `\t}`;
  const objectEntry = `\t${objectId} = {\n`
    + `\t\tgeometry = ${geometryId}\n`
    + `\t\tmaterial = ${material}\n`
    + `\t}`;

  let next = appendEntryToSceneGroup(source, "geometry", geometryEntry);
  next = appendEntryToSceneGroup(next, "object", objectEntry);
  return { source: next, objectId, geometryId };
}

function upsertSceneScalarProp(block, key, value, indent) {
  const baseIndent = String(indent || "");
  const formatted = formatSceneNumber(value, 0);
  const re = new RegExp(`(\\b${key}\\s*=\\s*)[^\\n\\r]+`, "i");
  if (re.test(block)) return String(block || "").replace(re, `$1${formatted}`);
  const trimmed = String(block || "").replace(/\s*$/, "");
  const suffix = String(block || "").slice(trimmed.length);
  const join = trimmed.length > 0 ? (trimmed.endsWith("\n") ? "" : "\n") : "";
  return `${trimmed}${join}${baseIndent}${key} = ${formatted}${suffix}`;
}

function updateCameraInSource(source, cameraId, params) {
  const model = parseSceneEditModel(String(source || ""));
  const cam = (model.cameras || []).find((c) => c.id === String(cameraId || ""));
  if (!cam) throw new Error(`camera not found: ${cameraId}`);

  const entryIndent = `${geometryEntryIndent(source, cam.entryStart)}\t`;
  let body = source.slice(cam.bodyStart, cam.bodyEnd);

  if (Array.isArray(params.position)) {
    body = upsertSceneVec3Prop(body, "position", params.position, entryIndent);
  }
  if (Array.isArray(params.target)) {
    body = upsertSceneVec3Prop(body, "target", params.target, entryIndent);
  }
  if (params.flength != null && Number.isFinite(Number(params.flength))) {
    body = upsertSceneScalarProp(body, "flength", Number(params.flength), entryIndent);
  }
  if (params.fov != null && Number.isFinite(Number(params.fov))) {
    body = upsertSceneScalarProp(body, "fov", Number(params.fov), entryIndent);
  }

  return `${source.slice(0, cam.bodyStart)}${body}${source.slice(cam.bodyEnd)}`;
}

function addInteractiveCameraToSceneSource(source, options) {
  const model = parseSceneEditModel(source);
  const cameraIds = new Set((model.cameras || []).map((cam) => String(cam && cam.id ? cam.id : "").trim()).filter(Boolean));
  const baseName = sanitizeSceneId(options && options.baseName, "interactive_camera");
  const cameraId = uniqueSceneId(cameraIds, baseName);

  const pos = Array.isArray(options && options.position) ? options.position : [0, 0, 0];
  const target = Array.isArray(options && options.target) ? options.target : [0, 0, -1];
  const up = Array.isArray(options && options.up) ? options.up : [0, 1, 0];
  const hfov = Number(options && options.hfov);
  const fov = Number.isFinite(hfov) ? Math.max(1, Math.min(179, hfov)) : 60;

  const cameraEntry = `\t${cameraId} = {\n`
    + `\t\ttype = thin-lens\n`
    + `\t\tposition = vec3(${formatSceneNumber(pos[0], 0)}, ${formatSceneNumber(pos[1], 0)}, ${formatSceneNumber(pos[2], 0)})\n`
    + `\t\ttarget = vec3(${formatSceneNumber(target[0], 0)}, ${formatSceneNumber(target[1], 0)}, ${formatSceneNumber(target[2], 0)})\n`
    + `\t\tup = vec3(${formatSceneNumber(up[0], 0)}, ${formatSceneNumber(up[1], 1)}, ${formatSceneNumber(up[2], 0)})\n`
    + `\t\tfov = ${formatSceneNumber(fov, 60)}\n`
    + `\t\tflength = 1.0\n`
    + `\t\taperture = 0.0\n`
    + `\t}`;

  return {
    source: appendEntryToSceneGroup(source, "camera", cameraEntry),
    cameraId,
  };
}
