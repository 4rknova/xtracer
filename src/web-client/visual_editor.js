(function () {
  "use strict";
  var ELEVATION_LIMIT = Math.PI * 0.5 - 0.01;

  function clamp(v, lo, hi) {
    return Math.min(hi, Math.max(lo, v));
  }

  function degToRad(v) {
    return (v || 0) * Math.PI / 180.0;
  }

  function radToDeg(v) {
    return (v || 0) * 180.0 / Math.PI;
  }

  function vec3(x, y, z) {
    return new THREE.Vector3(Number(x) || 0, Number(y) || 0, Number(z) || 0);
  }

  function sanitizeSource(src) {
    return String(src || "")
      .replace(/\r\n/g, "\n")
      .replace(/#[^\n]*/g, "");
  }

  function parsePathAliases(text) {
    var aliases = {};
    var re = /\b(path_[A-Za-z0-9_]+)\s*=\s*([^\n\r]+)/g;
    var m;
    while ((m = re.exec(text)) !== null) {
      var key = String(m[1] || "").trim();
      var val = String(m[2] || "").trim();
      if (!key || !val) continue;
      aliases[key] = val;
      if (key.indexOf("path_") === 0 && key.length > 5) aliases[key.slice(5)] = val;
    }
    return aliases;
  }

  function trimQuotes(s) {
    var v = String(s || "").trim();
    if (v.length >= 2) {
      var a = v[0];
      var b = v[v.length - 1];
      if ((a === "\"" && b === "\"") || (a === "'" && b === "'")) return v.slice(1, -1);
    }
    return v;
  }

  function resolveSourcePath(raw, aliases) {
    var src = trimQuotes(raw || "");
    if (!src) return "";
    src = src.replace(/<([A-Za-z0-9_]+)>/g, function (_m, name) {
      var k = String(name || "");
      if (Object.prototype.hasOwnProperty.call(aliases, k)) return aliases[k];
      return "<" + k + ">";
    });
    return src;
  }

  function findGroupBlock(text, groupName) {
    var re = new RegExp("\\b" + groupName + "\\s*=\\s*\\{", "m");
    var m = re.exec(text);
    if (!m) return "";
    var start = text.indexOf("{", m.index);
    if (start < 0) return "";
    var depth = 0;
    for (var i = start; i < text.length; i += 1) {
      var c = text[i];
      if (c === "{") depth += 1;
      else if (c === "}") {
        depth -= 1;
        if (depth === 0) return text.slice(start + 1, i);
      }
    }
    return "";
  }

  function parseTopLevelBlocks(groupText) {
    var out = {};
    var i = 0;
    while (i < groupText.length) {
      while (i < groupText.length && /\s/.test(groupText[i])) i += 1;
      var nameMatch = /^([A-Za-z0-9_\-]+)\s*=\s*\{/.exec(groupText.slice(i));
      if (!nameMatch) {
        i += 1;
        continue;
      }
      var name = nameMatch[1];
      i += nameMatch[0].length;
      var bodyStart = i;
      var depth = 1;
      while (i < groupText.length && depth > 0) {
        if (groupText[i] === "{") depth += 1;
        else if (groupText[i] === "}") depth -= 1;
        i += 1;
      }
      var body = groupText.slice(bodyStart, i - 1);
      out[name] = body;
    }
    return out;
  }

  function readStringProp(block, key) {
    var re = new RegExp("\\b" + key + "\\s*=\\s*([^\\n\\r]+)", "i");
    var m = re.exec(block);
    if (!m) return "";
    return String(m[1] || "").trim();
  }

  function readRefProp(block, key) {
    var re = new RegExp("\\b" + key + "\\s*=\\s*([A-Za-z0-9_.\\-]+)", "i");
    var m = re.exec(block);
    if (!m) return "";
    return String(m[1] || "").trim();
  }

  function readNumberProp(block, key, fallback) {
    var raw = readStringProp(block, key);
    if (!raw) return fallback;
    var n = Number(raw.replace(/[^0-9+\-.eE]/g, ""));
    return Number.isFinite(n) ? n : fallback;
  }

  function readCol3Prop(block, key) {
    var re = new RegExp("\\b" + key + "\\s*=\\s*col3\\(([^\\)]*)\\)", "i");
    var m = re.exec(block);
    if (!m) return null;
    var parts = String(m[1] || "").split(",").map(function (x) { return Number(x.trim()); });
    if (parts.length < 3 || !Number.isFinite(parts[0]) || !Number.isFinite(parts[1]) || !Number.isFinite(parts[2])) return null;
    return new THREE.Color(clamp(parts[0], 0, 1), clamp(parts[1], 0, 1), clamp(parts[2], 0, 1));
  }

  function readSamplerColor(block, samplerName) {
    if (!block || !samplerName) return null;
    var re = new RegExp("\\b" + samplerName + "\\s*=\\s*\\{([\\s\\S]*?)\\}", "i");
    var m = re.exec(block);
    if (!m) return null;
    return readCol3Prop(m[1] || "", "value");
  }

  function readGroupBody(block, key) {
    if (!block || !key) return "";
    var m = new RegExp("\\b" + key + "\\s*=\\s*\\{", "i").exec(block);
    if (!m) return "";
    var start = block.indexOf("{", m.index);
    if (start < 0) return "";
    var depth = 0;
    for (var i = start; i < block.length; i += 1) {
      if (block[i] === "{") depth += 1;
      else if (block[i] === "}") {
        depth -= 1;
        if (depth === 0) return block.slice(start + 1, i);
      }
    }
    return "";
  }

  function readVec3Prop(block, key, fallback) {
    var inline = new RegExp("\\b" + key + "\\s*=\\s*vec3\\(([^\\)]*)\\)", "i").exec(block);
    if (inline) {
      var p = String(inline[1] || "").split(",").map(function (x) { return Number(x.trim()); });
      if (p.length >= 3 && Number.isFinite(p[0]) && Number.isFinite(p[1]) && Number.isFinite(p[2])) {
        return vec3(p[0], p[1], p[2]);
      }
    }

    var nestedMatch = new RegExp("\\b" + key + "\\s*=\\s*\\{", "i").exec(block);
    if (nestedMatch) {
      var start = block.indexOf("{", nestedMatch.index);
      var depth = 0;
      for (var i = start; i < block.length; i += 1) {
        if (block[i] === "{") depth += 1;
        else if (block[i] === "}") {
          depth -= 1;
          if (depth === 0) {
            var body = block.slice(start + 1, i);
            var hasX = new RegExp("\\bx\\s*=", "i").test(body);
            var hasY = new RegExp("\\by\\s*=", "i").test(body);
            var hasZ = new RegExp("\\bz\\s*=", "i").test(body);
            var any = hasX || hasY || hasZ;
            var x = readNumberProp(body, "x", any ? 0 : fallback.x);
            var y = readNumberProp(body, "y", any ? 0 : fallback.y);
            var z = readNumberProp(body, "z", any ? 0 : fallback.z);
            return vec3(x, y, z);
          }
        }
      }
    }

    return fallback.clone();
  }

  function parseModifiers(block) {
    var m = new RegExp("\\bmodifiers\\s*=\\s*\\{", "i").exec(block);
    if (!m) {
      return {
        rotation: vec3(0, 0, 0),
        scale: vec3(1, 1, 1),
        translation: vec3(0, 0, 0),
      };
    }

    var start = block.indexOf("{", m.index);
    var depth = 0;
    var end = -1;
    for (var i = start; i < block.length; i += 1) {
      if (block[i] === "{") depth += 1;
      else if (block[i] === "}") {
        depth -= 1;
        if (depth === 0) {
          end = i;
          break;
        }
      }
    }
    var body = end > start ? block.slice(start + 1, end) : "";

    return {
      rotation: readVec3Prop(body, "rotation", vec3(0, 0, 0)),
      scale: readVec3Prop(body, "scale", vec3(1, 1, 1)),
      translation: readVec3Prop(body, "translation", vec3(0, 0, 0)),
    };
  }

  function parseSceneSource(source) {
    var txt = sanitizeSource(source);
    var aliases = parsePathAliases(txt);
    var cameraGroup = parseTopLevelBlocks(findGroupBlock(txt, "camera"));
    var geometryGroup = parseTopLevelBlocks(findGroupBlock(txt, "geometry"));
    var objectGroup = parseTopLevelBlocks(findGroupBlock(txt, "object"));
    var materialGroup = parseTopLevelBlocks(findGroupBlock(txt, "material"));

    var geometries = {};
    Object.keys(geometryGroup).forEach(function (id) {
      var body = geometryGroup[id];
      var type = readStringProp(body, "type").toLowerCase();
      var sourceProp = readStringProp(body, "source");
      var geo = {
        id: id,
        type: type,
        source: sourceProp,
        sourceResolved: resolveSourcePath(sourceProp, aliases),
        resolution: readNumberProp(body, "resolution", 24),
        radius: readNumberProp(body, "radius", 0.5),
        distance: readNumberProp(body, "distance", 0),
        position: readVec3Prop(body, "position", vec3(0, 0, 0)),
        normal: readVec3Prop(body, "normal", vec3(0, 1, 0)),
        v0: readVec3Prop(body, "v0", vec3(0, 0, 0)),
        v1: readVec3Prop(body, "v1", vec3(1, 0, 0)),
        v2: readVec3Prop(body, "v2", vec3(0, 1, 0)),
        modifiers: parseModifiers(body),
      };

      var vecDataMatch = /\bvecdata\s*=\s*\{([\s\S]*?)\}/i.exec(body);
      if (vecDataMatch) {
        var vecData = vecDataMatch[1] || "";
        geo.v0 = readVec3Prop(vecData, "v0", geo.v0);
        geo.v1 = readVec3Prop(vecData, "v1", geo.v1);
        geo.v2 = readVec3Prop(vecData, "v2", geo.v2);
      }

      geometries[id] = geo;
    });

    var materials = {};
    Object.keys(materialGroup).forEach(function (id) {
      var body = materialGroup[id];
      var type = readStringProp(body, "type").toLowerCase();
      var diffuse = /\bdiffuse\s*=\s*\{([\s\S]*?)\}/i.exec(body);
      var specular = /\bspecular\s*=\s*\{([\s\S]*?)\}/i.exec(body);
      var emissive = /\bemissive\s*=\s*\{([\s\S]*?)\}/i.exec(body);
      var col = diffuse ? readCol3Prop(diffuse[1], "value") : null;
      var sCol = specular ? readCol3Prop(specular[1], "value") : null;
      var eCol = emissive ? readCol3Prop(emissive[1], "value") : null;
      var samplersBody = readGroupBody(readGroupBody(body, "properties"), "samplers");
      if (!col) {
        col = readSamplerColor(samplersBody, "diffuse");
      }
      if (!sCol) {
        sCol = readSamplerColor(samplersBody, "specular");
      }
      if (!eCol) {
        eCol = readSamplerColor(samplersBody, "emissive");
      }
      materials[id] = {
        type: type,
        diffuse: col,
        specular: sCol,
        emissive: eCol,
      };
    });

    var objects = [];
    Object.keys(objectGroup).forEach(function (id) {
      var body = objectGroup[id];
      var geoId = readRefProp(body, "geometry");
      var matId = readRefProp(body, "material");
      if (!geoId) return;
      objects.push({ id: id, geometry: geoId, material: matId });
    });

    var cameras = [];
    Object.keys(cameraGroup).forEach(function (id) {
      var body = cameraGroup[id];
      cameras.push({
        id: id,
        type: readStringProp(body, "type").toLowerCase(),
        position: readVec3Prop(body, "position", vec3(0, 2, -6)),
        target: readVec3Prop(body, "target", vec3(0, 0, 0)),
        up: readVec3Prop(body, "up", vec3(0, 1, 0)),
        orientation: readVec3Prop(body, "orientation", vec3(0, 0, 0)),
        fov: readNumberProp(body, "fov", 45),
        flength: readNumberProp(body, "flength", 0),
        aperture: readNumberProp(body, "aperture", 0),
      });
    });

    return {
      aliases: aliases,
      cameras: cameras,
      geometries: geometries,
      materials: materials,
      objects: objects,
    };
  }

  function generatedMeshGeometry(token, resolution) {
    var t = String(token || "").trim().toLowerCase();
    var seg = Math.max(8, Math.floor(Number(resolution) || 24));

    if (t === "icosahedron") return new THREE.IcosahedronGeometry(1, 0);
    if (t === "tetrahedron") return new THREE.TetrahedronGeometry(1, 0);
    if (t === "cube" || t === "hexahedron") return new THREE.BoxGeometry(1, 1, 1);
    if (t === "octahedron") return new THREE.OctahedronGeometry(1, 0);
    if (t === "dodecahedron") return new THREE.DodecahedronGeometry(1, 0);
    if (t === "icosphere") return new THREE.IcosahedronGeometry(1, clamp(Math.floor(seg / 16), 0, 3));
    if (t === "geodesic_dome") return new THREE.SphereGeometry(1, Math.max(12, seg), Math.max(8, Math.floor(seg / 2)), 0, Math.PI * 2, 0, Math.PI * 0.5);
    if (t === "capsule") return new THREE.CapsuleGeometry(0.35, 0.8, 6, Math.max(8, Math.floor(seg / 2)));
    if (t === "cylinder" || t === "capped_cylinder") return new THREE.CylinderGeometry(0.5, 0.5, 1.0, seg);
    if (t === "cone") return new THREE.ConeGeometry(0.5, 1.0, seg);
    if (t === "truncated_cone") return new THREE.CylinderGeometry(0.35, 0.6, 1.0, seg);
    if (t === "ring") return new THREE.TorusGeometry(0.6, 0.18, Math.max(8, Math.floor(seg / 2)), seg);
    if (t === "torus_knot") return new THREE.TorusKnotGeometry(0.52, 0.16, Math.max(48, seg * 2), 12);
    if (t === "plane") return new THREE.PlaneGeometry(1, 1, Math.max(1, Math.floor(seg / 8)), Math.max(1, Math.floor(seg / 8)));

    // Fallback for currently unsupported generators in the visual pass.
    return new THREE.IcosahedronGeometry(0.7, 0);
  }

  function geometryForDef(def) {
    var type = String(def.type || "").toLowerCase();

    if (type === "sphere") {
      return new THREE.SphereGeometry(Math.max(0.001, def.radius || 0.5), 24, 16);
    }
    if (type === "point") {
      return new THREE.SphereGeometry(0.03, 16, 12);
    }
    if (type === "plane") {
      var pg = new THREE.PlaneGeometry(24, 24, 1, 1);
      // xtcore plane normals are authored in Y-up space; align primitive normal to +Y.
      pg.rotateX(-Math.PI * 0.5);
      return pg;
    }
    if (type === "triangle") {
      var g = new THREE.BufferGeometry();
      var arr = new Float32Array([
        def.v0.x, def.v0.y, def.v0.z,
        def.v1.x, def.v1.y, def.v1.z,
        def.v2.x, def.v2.y, def.v2.z,
      ]);
      g.setAttribute("position", new THREE.BufferAttribute(arr, 3));
      g.computeVertexNormals();
      return g;
    }
    if (type === "mesh") {
      var src = String(def.source || "").trim();
      var gen = /gen\s*\(\s*([a-zA-Z0-9_\-]+)\s*\)/i.exec(src);
      if (gen) return generatedMeshGeometry(gen[1], def.resolution);
      return new THREE.BoxGeometry(1, 1, 1);
    }

    return new THREE.BoxGeometry(0.5, 0.5, 0.5);
  }

function materialForDef(matDef) {
  if (!matDef) {
    return new THREE.MeshPhongMaterial({
      color: 0x95a4b5,
      specular: 0x111111,
      shininess: 50,
      side: THREE.DoubleSide,
    });
  }

  if (matDef.type === "emissive") {
    var e = matDef.emissive || new THREE.Color(1, 0.95, 0.8);
    return new THREE.MeshPhongMaterial({
      color: matDef.diffuse || new THREE.Color(0.08, 0.08, 0.08),
      emissive: e,
      emissiveIntensity: 2.0,
      specular: matDef.specular || new THREE.Color(0.0, 0.0, 0.0),
      shininess: 20,
      side: THREE.DoubleSide,
    });
  }

  return new THREE.MeshPhongMaterial({
    color: matDef.diffuse || new THREE.Color(0.75, 0.78, 0.82),
    specular: matDef.specular || new THREE.Color(0.08, 0.08, 0.08),
    shininess: 90,
    side: THREE.DoubleSide,
  });
}

  function parseObjToGeometry(objText) {
    var lines = String(objText || "").replace(/\r\n/g, "\n").split("\n");
    var v = [];
    var n = [];
    var posOut = [];
    var nOut = [];

    function parseIndex(token, len) {
      var i = parseInt(token, 10);
      if (!Number.isFinite(i) || i === 0) return -1;
      if (i < 0) return len + i;
      return i - 1;
    }

    for (var li = 0; li < lines.length; li += 1) {
      var line = lines[li].trim();
      if (!line || line[0] === "#") continue;
      var parts = line.split(/\s+/);
      var head = parts[0];
      if (head === "v" && parts.length >= 4) {
        v.push(vec3(parts[1], parts[2], parts[3]));
      } else if (head === "vn" && parts.length >= 4) {
        n.push(vec3(parts[1], parts[2], parts[3]).normalize());
      } else if (head === "f" && parts.length >= 4) {
        var verts = [];
        for (var pi = 1; pi < parts.length; pi += 1) {
          var p = parts[pi].split("/");
          var vi = parseIndex(p[0], v.length);
          var ni = (p.length >= 3 && p[2] !== "") ? parseIndex(p[2], n.length) : -1;
          if (vi < 0 || vi >= v.length) continue;
          verts.push({ vi: vi, ni: ni });
        }
        if (verts.length < 3) continue;

        for (var ti = 1; ti + 1 < verts.length; ti += 1) {
          var tri = [verts[0], verts[ti], verts[ti + 1]];
          var faceNormal = null;
          if (tri[0].ni < 0 || tri[1].ni < 0 || tri[2].ni < 0) {
            var a = v[tri[0].vi];
            var b = v[tri[1].vi];
            var c = v[tri[2].vi];
            faceNormal = b.clone().sub(a).cross(c.clone().sub(a));
            if (faceNormal.lengthSq() < 1e-12) faceNormal.set(0, 1, 0);
            else faceNormal.normalize();
          }

          for (var k = 0; k < 3; k += 1) {
            var pv = v[tri[k].vi];
            posOut.push(pv.x, pv.y, pv.z);
            if (tri[k].ni >= 0 && tri[k].ni < n.length) {
              var pn = n[tri[k].ni];
              nOut.push(pn.x, pn.y, pn.z);
            } else {
              nOut.push(faceNormal.x, faceNormal.y, faceNormal.z);
            }
          }
        }
      }
    }

    if (posOut.length === 0) return null;
    var g = new THREE.BufferGeometry();
    g.setAttribute("position", new THREE.Float32BufferAttribute(posOut, 3));
    g.setAttribute("normal", new THREE.Float32BufferAttribute(nOut, 3));
    g.computeBoundingSphere();
    return g;
  }

  function SceneVisualEditor(viewportEl, statusEl, fetchSceneGeometry, fetchSceneAssetText) {
    this.viewportEl = viewportEl;
    this.statusEl = statusEl;
    this.fetchSceneGeometry = (typeof fetchSceneGeometry === "function") ? fetchSceneGeometry : null;
    this.fetchSceneAssetText = (typeof fetchSceneAssetText === "function") ? fetchSceneAssetText : null;

    this.renderer = null;
    this.scene = null;
    this.camera = null;
    this.perspectiveCamera = null;
    this.orthoCamera = null;
    this.projectionMode = "perspective";
    this.orthoViewScale = 1.0;
    this.modelRoot = null;
    this.grid = null;
    this.keyLight = null;
    this.rimLight = null;
    this.sceneGeometry = { meshes: {} };
    this.sceneCameraMap = {};
    this.sceneCameraOrder = [];
    this.activeSceneCamera = "";
    this.selectedCameraPose = null;
    this.selectedObjectName = "";
    this.selectedCameraHFov = 45;
    this.selectedCameraAperture = 0;
    this.selectedCameraFLength = 0;
    this.selectedCameraType = "";
    this.onSelectionChanged = null;
    this.selectedMesh = null;
    this.parsedScene = null;
    this.currentSceneName = "";
    this.objectMeshById = {};
    this.objectMetaById = {};
    this.assetTextCache = {};
    this.cameraWidgetRoot = null;
    this.cameraWidget = null;
    this.axisWidgetScene = null;
    this.axisWidgetCamera = null;
    this.axisWidgetRoot = null;
    this.axisWidgetPickables = [];
    this.axisWidgetRaycaster = null;
    this.axisWidgetSize = 122;
    this.axisWidgetMargin = 12;
    this.photonRoot = null;
    this.photonDiffuse = null;
    this.photonCaustic = null;
    this.sceneRadius = 2.0;
    this.infinitePlanes = [];
    this.viewUp = vec3(0, 1, 0);
    this.frameAspect = 1;
    this.renderViewport = { x: 0, y: 0, w: 1, h: 1 };

    this.target = new THREE.Vector3(0, 0.8, 0);
    this.azimuth = -0.6;
    this.elevation = 0.4;
    this.distance = 8.0;

    this.dragMode = "";
    this.dragPointerId = null;
    this.dragButton = 0;
    this.dragMoved = false;
    this.lastX = 0;
    this.lastY = 0;
    this.movePlane = null;
    this.moveOffset = new THREE.Vector3(0, 0, 0);
    this.moveStartPosition = null;
    this.onObjectTransformChanged = null;

    this._animateBound = this.animate.bind(this);
  }

  SceneVisualEditor.prototype.setStatus = function (msg) {
    if (this.statusEl) this.statusEl.textContent = String(msg || "");
  };

  SceneVisualEditor.prototype.setSelectionChangeHandler = function (handler) {
    this.onSelectionChanged = (typeof handler === "function") ? handler : null;
  };

  SceneVisualEditor.prototype.setObjectTransformChangeHandler = function (handler) {
    this.onObjectTransformChanged = (typeof handler === "function") ? handler : null;
  };

  SceneVisualEditor.prototype.notifySelectionChanged = function () {
    if (!this.onSelectionChanged) return;
    var selectedId = this.getSelectedObjectId();
    var payload = selectedId ? this.getObjectMeta(selectedId) : null;
    this.onSelectionChanged(payload);
  };

  SceneVisualEditor.prototype.extractObjectTransform = function (mesh) {
    if (!mesh) return null;
    return {
      translation: [mesh.position.x, mesh.position.y, mesh.position.z],
      rotation: [radToDeg(mesh.rotation.x), radToDeg(mesh.rotation.y), radToDeg(mesh.rotation.z)],
      scale: [mesh.scale.x, mesh.scale.y, mesh.scale.z],
    };
  };

  SceneVisualEditor.prototype.notifyObjectTransformChanged = function (objectId, transform, deltaTranslation) {
    if (!this.onObjectTransformChanged) return;
    this.onObjectTransformChanged({
      objectId: String(objectId || ""),
      transform: transform || null,
      deltaTranslation: Array.isArray(deltaTranslation) ? deltaTranslation.slice(0, 3) : [0, 0, 0],
    });
  };

  SceneVisualEditor.prototype.init = function () {
    if (!this.viewportEl || typeof THREE === "undefined") return false;

    this.renderer = new THREE.WebGLRenderer({ antialias: true });
    this.renderer.setPixelRatio(window.devicePixelRatio || 1);
    this.renderer.setClearColor(0x121820, 1);
    this.renderer.autoClear = false;
    this.renderer.shadowMap.enabled = true;
    this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    this.viewportEl.innerHTML = "";
    this.viewportEl.appendChild(this.renderer.domElement);

    this.scene = new THREE.Scene();

    this.perspectiveCamera = new THREE.PerspectiveCamera(45, 1, 0.01, 500);
    this.orthoCamera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0.01, 500);
    this.camera = this.perspectiveCamera;

    this.modelRoot = new THREE.Group();
    this.scene.add(this.modelRoot);
    this.cameraWidgetRoot = new THREE.Group();
    this.scene.add(this.cameraWidgetRoot);
    this.initAxisWidget();
    this.photonRoot = new THREE.Group();
    this.scene.add(this.photonRoot);

    this.grid = new THREE.GridHelper(24, 24, 0x5f7a8e, 0x2b3642);
    this.grid.position.y = -0.003;
    if (Array.isArray(this.grid.material)) {
      for (var gi = 0; gi < this.grid.material.length; gi += 1) {
        this.grid.material[gi].transparent = true;
        this.grid.material[gi].opacity = 0.72;
        this.grid.material[gi].depthWrite = false;
      }
    } else if (this.grid.material) {
      this.grid.material.transparent = true;
      this.grid.material.opacity = 0.72;
      this.grid.material.depthWrite = false;
    }
    this.scene.add(this.grid);

    var amb = new THREE.AmbientLight(0xffffff, 0.55);
    this.scene.add(amb);

    this.keyLight = new THREE.DirectionalLight(0xffffff, 0.9);
    this.keyLight.position.set(8, 10, 6);
    this.keyLight.castShadow = true;
    this.keyLight.shadow.mapSize.width = 2048;
    this.keyLight.shadow.mapSize.height = 2048;
    this.keyLight.shadow.bias = 0.00045;
    this.keyLight.shadow.normalBias = 0.04;
    this.scene.add(this.keyLight);
    this.scene.add(this.keyLight.target);

    this.rimLight = new THREE.DirectionalLight(0x89a7ff, 0.35);
    this.rimLight.position.set(-6, 4, -7);
    this.scene.add(this.rimLight);

    this.bindInput();
    this.resize();
    this.setProjectionMode(this.projectionMode);
    requestAnimationFrame(this._animateBound);
    return true;
  };

  SceneVisualEditor.prototype.updateProjectionForDistance = function () {
    var aspect = (this.renderViewport && this.renderViewport.h > 0)
      ? (this.renderViewport.w / this.renderViewport.h)
      : 1;
    var dist = clamp(this.distance, 0.3, 120);

    if (this.projectionMode === "isometric" && this.orthoCamera) {
      // Keep scene scale comparable to perspective zoom by mapping distance to ortho span.
      var baseFov = 45.0;
      var halfH = Math.max(0.01, Math.tan(degToRad(baseFov) * 0.5) * dist * this.orthoViewScale);
      var halfW = Math.max(0.01, halfH * Math.max(1e-6, aspect));
      this.orthoCamera.left = -halfW;
      this.orthoCamera.right = halfW;
      this.orthoCamera.top = halfH;
      this.orthoCamera.bottom = -halfH;
      this.orthoCamera.near = 0.01;
      this.orthoCamera.far = 500;
      this.orthoCamera.updateProjectionMatrix();
      return;
    }

    if (this.perspectiveCamera) {
      this.perspectiveCamera.aspect = Math.max(1e-6, aspect);
      this.perspectiveCamera.near = 0.01;
      this.perspectiveCamera.far = 500;
      this.perspectiveCamera.updateProjectionMatrix();
    }
  };

  SceneVisualEditor.prototype.getProjectionMode = function () {
    return this.projectionMode;
  };

  SceneVisualEditor.prototype.setProjectionMode = function (mode) {
    var next = String(mode || "perspective").toLowerCase();
    if (next !== "isometric" && next !== "perspective") next = "perspective";
    this.projectionMode = next;

    if (next === "isometric") {
      if (!this.orthoCamera) return;
      this.camera = this.orthoCamera;
      if (Math.abs(this.elevation) < 1e-5) {
        this.elevation = degToRad(35.26438968);
      }
      this.updateProjectionForDistance();
      return;
    }

    if (!this.perspectiveCamera) return;
    this.camera = this.perspectiveCamera;
    this.updateProjectionForDistance();
  };

  SceneVisualEditor.prototype.bindInput = function () {
    var self = this;
    var canvas = this.renderer.domElement;

    this.viewportEl.addEventListener("contextmenu", function (e) { e.preventDefault(); });

    this.viewportEl.addEventListener("wheel", function (e) {
      e.preventDefault();
      var factor = Math.exp(e.deltaY * 0.0015);
      self.distance = clamp(self.distance * factor, 0.3, 120);
    }, { passive: false });

    this.viewportEl.addEventListener("pointerdown", function (e) {
      if (e.button === 0 && self.handleAxisWidgetPointerDown(e.clientX, e.clientY)) return;
      if (e.button === 0 && (e.ctrlKey || e.metaKey)) {
        var picked = self.pickObjectAt(e.clientX, e.clientY);
        if (picked && picked.mesh) {
          self.setSelectedMesh(picked.mesh, false);
          var meta = self.getObjectMeta(self.getSelectedObjectId());
          var gt = meta ? String(meta.geometryType || "").toLowerCase() : "";
          var canMove = (gt === "mesh" || gt === "sphere" || gt === "point" || gt === "triangle");
          if (canMove && self.beginMoveDrag(e.clientX, e.clientY)) self.dragMode = "move";
          else self.dragMode = "orbit";
        } else self.dragMode = "orbit";
      } else if (e.button === 2 || e.button === 1) self.dragMode = "pan";
      else self.dragMode = "orbit";
      self.dragPointerId = e.pointerId;
      self.dragButton = e.button;
      self.dragMoved = false;
      self.lastX = e.clientX;
      self.lastY = e.clientY;
      canvas.setPointerCapture(e.pointerId);
    });

    this.viewportEl.addEventListener("pointermove", function (e) {
      if (self.dragPointerId !== e.pointerId) return;
      var dx = e.clientX - self.lastX;
      var dy = e.clientY - self.lastY;
      if (Math.abs(dx) + Math.abs(dy) > 1.0) self.dragMoved = true;
      self.lastX = e.clientX;
      self.lastY = e.clientY;

      if (self.dragMode === "move") {
        if (self.updateMoveDrag(e.clientX, e.clientY)) self.dragMoved = true;
      } else if (self.dragMode === "pan") {
        var panScale = self.distance * 0.0016;
        var right = new THREE.Vector3();
        var up = new THREE.Vector3();
        self.camera.matrixWorld.extractBasis(right, up, new THREE.Vector3());
        right.normalize();
        up.normalize();
        self.target.addScaledVector(right, -dx * panScale);
        self.target.addScaledVector(up, dy * panScale);
      } else {
        self.azimuth -= dx * 0.006;
        self.elevation = clamp(self.elevation - dy * 0.006, -ELEVATION_LIMIT, ELEVATION_LIMIT);
      }
    });
    function endDrag(e) {
      if (self.dragPointerId !== e.pointerId) return;
      if (self.dragMode === "move") {
        self.endMoveDrag(self.dragMoved);
      }
      var leftClick = self.dragButton === 0 && !self.dragMoved && self.dragMode !== "move";
      if (leftClick) {
        self.pickSelectAt(e.clientX, e.clientY);
      }
      self.dragMode = "";
      self.dragPointerId = null;
      self.dragButton = 0;
      self.dragMoved = false;
      try { canvas.releasePointerCapture(e.pointerId); } catch (_) {}
    }

    this.viewportEl.addEventListener("pointerup", endDrag);
    this.viewportEl.addEventListener("pointercancel", endDrag);

    window.addEventListener("keydown", function (e) {
      self.handleKeyDown(e);
    });
  };

  SceneVisualEditor.prototype.rayFromClient = function (clientX, clientY) {
    if (!this.renderer || !this.camera) return null;
    var rect = this.renderer.domElement.getBoundingClientRect();
    if (!rect || rect.width <= 0 || rect.height <= 0) return null;
    if (clientX < rect.left || clientX > rect.right || clientY < rect.top || clientY > rect.bottom) return null;
    var nx = ((clientX - rect.left) / rect.width) * 2 - 1;
    var ny = -(((clientY - rect.top) / rect.height) * 2 - 1);
    var ray = new THREE.Raycaster();
    ray.setFromCamera(new THREE.Vector2(nx, ny), this.camera);
    return ray.ray;
  };

  SceneVisualEditor.prototype.beginMoveDrag = function (clientX, clientY) {
    if (!this.selectedMesh) return false;
    var ray = this.rayFromClient(clientX, clientY);
    if (!ray) return false;
    var normal = new THREE.Vector3();
    this.camera.getWorldDirection(normal);
    if (normal.lengthSq() < 1e-10) normal.set(0, 0, -1);
    normal.normalize();
    this.movePlane = new THREE.Plane().setFromNormalAndCoplanarPoint(normal, this.selectedMesh.position.clone());
    var hit = new THREE.Vector3();
    if (!ray.intersectPlane(this.movePlane, hit)) return false;
    this.moveOffset.copy(this.selectedMesh.position).sub(hit);
    this.moveStartPosition = this.selectedMesh.position.clone();
    this.setStatus("Dragging: " + this.getSelectedObjectId());
    return true;
  };

  SceneVisualEditor.prototype.updateMoveDrag = function (clientX, clientY) {
    if (!this.selectedMesh || !this.movePlane) return false;
    var ray = this.rayFromClient(clientX, clientY);
    if (!ray) return false;
    var hit = new THREE.Vector3();
    if (!ray.intersectPlane(this.movePlane, hit)) return false;
    var next = hit.add(this.moveOffset);
    if (!Number.isFinite(next.x) || !Number.isFinite(next.y) || !Number.isFinite(next.z)) return false;
    this.selectedMesh.position.copy(next);
    return true;
  };

  SceneVisualEditor.prototype.endMoveDrag = function (moved) {
    if (this.selectedMesh && moved) {
      var oid = this.getSelectedObjectId();
      var t = this.extractObjectTransform(this.selectedMesh);
      var delta = [0, 0, 0];
      if (this.moveStartPosition) {
        delta = [
          this.selectedMesh.position.x - this.moveStartPosition.x,
          this.selectedMesh.position.y - this.moveStartPosition.y,
          this.selectedMesh.position.z - this.moveStartPosition.z,
        ];
      }
      this.notifyObjectTransformChanged(oid, t, delta);
      this.setStatus("Selected: " + oid);
    }
    this.movePlane = null;
    this.moveOffset.set(0, 0, 0);
    this.moveStartPosition = null;
  };

  SceneVisualEditor.prototype.handleKeyDown = function (e) {
    if (!e) return;
    var target = e.target;
    if (target && (target.isContentEditable
      || target.tagName === "INPUT"
      || target.tagName === "TEXTAREA"
      || target.tagName === "SELECT")) {
      return;
    }
    var k = String(e.key || "").toLowerCase();
    if (k === "m") {
      if (this.jumpToSelectedCameraView()) e.preventDefault();
      return;
    }
    if (k === "f") {
      if (this.focusSelectedObject()) e.preventDefault();
    }
  };

  SceneVisualEditor.prototype.jumpToSelectedCameraView = function () {
    if (!this.selectedCameraPose) {
      this.setStatus("View: no selected camera");
      return false;
    }

    var pos = this.selectedCameraPose.position.clone();
    var tgt = this.selectedCameraPose.target.clone();
    var toCam = pos.clone().sub(tgt);
    var dist = clamp(toCam.length(), 0.3, 120);
    if (toCam.lengthSq() < 1e-8) toCam.set(0, 0, 1);

    var azimuth = Math.atan2(toCam.x, toCam.z);
    var horiz = Math.sqrt(toCam.x * toCam.x + toCam.z * toCam.z);
    var elevation = Math.atan2(toCam.y, Math.max(1e-6, horiz));

    this.target.copy(tgt);
    this.distance = dist;
    this.azimuth = azimuth;
    this.elevation = clamp(elevation, -ELEVATION_LIMIT, ELEVATION_LIMIT);
    var label = this.activeSceneCamera ? ("camera " + this.activeSceneCamera) : "selected camera";
    this.setStatus("View: " + label);
    return true;
  };

  SceneVisualEditor.prototype.focusSelectedObject = function () {
    if (!this.selectedMesh) {
      this.setStatus("Focus: no selected object");
      return false;
    }
    var center = new THREE.Vector3();
    if (this.selectedMesh.geometry) {
      if (!this.selectedMesh.geometry.boundingBox) this.selectedMesh.geometry.computeBoundingBox();
      if (this.selectedMesh.geometry.boundingBox) {
        this.selectedMesh.geometry.boundingBox.getCenter(center);
        center.applyMatrix4(this.selectedMesh.matrixWorld);
      }
    }
    if (!Number.isFinite(center.x) || !Number.isFinite(center.y) || !Number.isFinite(center.z)) {
      new THREE.Box3().setFromObject(this.selectedMesh).getCenter(center);
    }
    if (!Number.isFinite(center.x) || !Number.isFinite(center.y) || !Number.isFinite(center.z)) {
      return false;
    }

    var toCam = this.camera.position.clone().sub(center);
    var dist = clamp(toCam.length(), 0.3, 120);
    var azimuth = Math.atan2(toCam.x, toCam.z);
    var horiz = Math.sqrt(toCam.x * toCam.x + toCam.z * toCam.z);
    var elevation = Math.atan2(toCam.y, Math.max(1e-6, horiz));

    this.target.copy(center);
    this.distance = dist;
    this.azimuth = azimuth;
    this.elevation = clamp(elevation, -ELEVATION_LIMIT, ELEVATION_LIMIT);
    this.setStatus("Focus: " + this.getSelectedObjectId());
    return true;
  };

  SceneVisualEditor.prototype.pickObjectAt = function (clientX, clientY) {
    if (!this.renderer || !this.camera || !this.modelRoot) return false;

    this.modelRoot.updateMatrixWorld(true);
    var ray = this.rayFromClient(clientX, clientY);
    if (!ray) return null;
    var caster = new THREE.Raycaster();
    caster.ray.copy(ray);
    var hits = caster.intersectObjects(this.modelRoot.children, true);
    if (!hits || hits.length === 0) return null;

    var chosenHit = null;
    var firstHit = null;
    var firstInfinitePlaneHit = null;
    for (var hi = 0; hi < hits.length; hi += 1) {
      var h = hits[hi];
      var obj = h.object;
      while (obj && obj.parent && obj.parent !== this.modelRoot) obj = obj.parent;
      if (!obj || !obj.isMesh || obj.parent !== this.modelRoot) continue;
      if (!firstHit) firstHit = { hit: h, mesh: obj };
      if (obj.userData && obj.userData.infinitePlane) {
        if (!firstInfinitePlaneHit) firstInfinitePlaneHit = { hit: h, mesh: obj };
        continue;
      }
      chosenHit = { hit: h, mesh: obj };
      break;
    }
    if (!chosenHit) chosenHit = firstHit || firstInfinitePlaneHit;
    if (!chosenHit || !chosenHit.mesh) return null;
    return chosenHit;
  };

  SceneVisualEditor.prototype.resolveMeshCenter = function (hitInfo) {
    if (!hitInfo || !hitInfo.mesh) return null;
    var picked = hitInfo.mesh;
    var center = new THREE.Vector3();
    if (picked.userData && picked.userData.infinitePlane) {
      center.copy(hitInfo.hit.point);
    } else if (picked.geometry) {
      if (!picked.geometry.boundingBox) picked.geometry.computeBoundingBox();
      if (picked.geometry.boundingBox) {
        picked.geometry.boundingBox.getCenter(center);
        center.applyMatrix4(picked.matrixWorld);
      }
    }
    if (!Number.isFinite(center.x) || !Number.isFinite(center.y) || !Number.isFinite(center.z)) {
      new THREE.Box3().setFromObject(picked).getCenter(center);
    }
    if (!Number.isFinite(center.x) || !Number.isFinite(center.y) || !Number.isFinite(center.z)) {
      center.copy(hitInfo.hit.point);
    }
    return center;
  };

  SceneVisualEditor.prototype.applySelectionHighlight = function (mesh, selected) {
    if (!mesh || !mesh.material) return;
    var mats = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
    for (var i = 0; i < mats.length; i += 1) {
      var m = mats[i];
      if (!m || !m.isMeshPhongMaterial) continue;
      if (selected) {
        if (!m.userData._selOrigEmissive) {
          m.userData._selOrigEmissive = m.emissive ? m.emissive.clone() : new THREE.Color(0, 0, 0);
          m.userData._selOrigEmissiveIntensity = Number(m.emissiveIntensity) || 1.0;
        }
        m.emissive = new THREE.Color(0x16384d);
        m.emissiveIntensity = 0.75;
      } else if (m.userData._selOrigEmissive) {
        m.emissive.copy(m.userData._selOrigEmissive);
        m.emissiveIntensity = m.userData._selOrigEmissiveIntensity;
        delete m.userData._selOrigEmissive;
        delete m.userData._selOrigEmissiveIntensity;
      }
    }
  };

  SceneVisualEditor.prototype.setSelectedMesh = function (mesh, focusView) {
    if (this.selectedMesh && this.selectedMesh !== mesh) this.applySelectionHighlight(this.selectedMesh, false);
    this.selectedMesh = mesh || null;
    this.selectedObjectName = (mesh && mesh.userData && mesh.userData.objectId) || "";
    if (this.selectedMesh) {
      this.applySelectionHighlight(this.selectedMesh, true);
      if (focusView) {
        var c = this.resolveMeshCenter({ mesh: this.selectedMesh, hit: { point: this.target.clone() } });
        if (c) {
          var toCam = this.camera.position.clone().sub(c);
          var dist = clamp(toCam.length(), 0.3, 120);
          var azimuth = Math.atan2(toCam.x, toCam.z);
          var horiz = Math.sqrt(toCam.x * toCam.x + toCam.z * toCam.z);
          var elevation = Math.atan2(toCam.y, Math.max(1e-6, horiz));
          this.target.copy(c);
          this.distance = dist;
          this.azimuth = azimuth;
          this.elevation = clamp(elevation, -ELEVATION_LIMIT, ELEVATION_LIMIT);
        }
      }
      this.setStatus("Selected: " + this.selectedObjectName);
    } else {
      this.setStatus("Selection: none (click to select, Ctrl+drag to move)");
    }
    this.notifySelectionChanged();
  };

  SceneVisualEditor.prototype.pickSelectAt = function (clientX, clientY) {
    var chosenHit = this.pickObjectAt(clientX, clientY);
    if (!chosenHit || !chosenHit.mesh) return false;
    this.setSelectedMesh(chosenHit.mesh, false);
    return true;
  };

  SceneVisualEditor.prototype.pickPivotAt = function (clientX, clientY) {
    var chosenHit = this.pickObjectAt(clientX, clientY);
    if (!chosenHit || !chosenHit.mesh) return false;
    var picked = chosenHit.mesh;
    this.setSelectedMesh(picked, false);
    var center = this.resolveMeshCenter(chosenHit);
    if (!center) return true;
    var toCam = this.camera.position.clone().sub(center);
    var dist = clamp(toCam.length(), 0.3, 120);
    var azimuth = Math.atan2(toCam.x, toCam.z);
    var horiz = Math.sqrt(toCam.x * toCam.x + toCam.z * toCam.z);
    var elevation = Math.atan2(toCam.y, Math.max(1e-6, horiz));

    this.target.copy(center);
    this.distance = dist;
    this.azimuth = azimuth;
    this.elevation = clamp(elevation, -ELEVATION_LIMIT, ELEVATION_LIMIT);
    this.setStatus("Selected: " + ((picked.userData && picked.userData.objectId) || picked.name || "object"));
    return true;
  };

  SceneVisualEditor.prototype.clearCameraWidget = function () {
    if (this.cameraWidget) {
      this.cameraWidget.traverse(function (node) {
        if (node && node.geometry) node.geometry.dispose();
        if (node && node.material) {
          if (Array.isArray(node.material)) node.material.forEach(function (m) { if (m) m.dispose(); });
          else node.material.dispose();
        }
      });
      this.cameraWidgetRoot.remove(this.cameraWidget);
      this.cameraWidget = null;
    }
  };

  SceneVisualEditor.prototype.clearPhotonPoints = function () {
    var clearPoints = function (obj, root) {
      if (!obj) return null;
      if (obj.geometry) obj.geometry.dispose();
      if (obj.material) obj.material.dispose();
      if (root) root.remove(obj);
      return null;
    };
    this.photonDiffuse = clearPoints(this.photonDiffuse, this.photonRoot);
    this.photonCaustic = clearPoints(this.photonCaustic, this.photonRoot);
  };

  SceneVisualEditor.prototype.initAxisWidget = function () {
    this.axisWidgetScene = new THREE.Scene();
    this.axisWidgetCamera = new THREE.OrthographicCamera(-1.35, 1.35, 1.35, -1.35, 0.01, 10);
    this.axisWidgetCamera.position.set(0, 0, 5);
    this.axisWidgetCamera.lookAt(0, 0, 0);
    this.axisWidgetRoot = new THREE.Group();
    this.axisWidgetScene.add(this.axisWidgetRoot);
    this.axisWidgetPickables = [];
    this.axisWidgetRaycaster = new THREE.Raycaster();

    var axes = [
      { dir: vec3(1, 0, 0), color: 0xf56b6b, label: "+X" },
      { dir: vec3(0, 1, 0), color: 0x65d188, label: "+Y" },
      { dir: vec3(0, 0, 1), color: 0x6ca8ff, label: "+Z" },
    ];

    for (var i = 0; i < axes.length; i += 1) {
      var a = axes[i];
      var lineGeom = new THREE.BufferGeometry().setFromPoints([vec3(0, 0, 0), a.dir.clone().multiplyScalar(0.9)]);
      var line = new THREE.Line(lineGeom, new THREE.LineBasicMaterial({ color: a.color }));
      this.axisWidgetRoot.add(line);

      var pSphere = new THREE.Mesh(
        new THREE.SphereGeometry(0.22, 20, 16),
        new THREE.MeshBasicMaterial({ color: a.color })
      );
      pSphere.position.copy(a.dir).multiplyScalar(1.0);
      pSphere.userData = {
        axisDir: a.dir.clone(),
        axisLabel: a.label,
      };
      this.axisWidgetRoot.add(pSphere);
      this.axisWidgetPickables.push(pSphere);

      var nSphere = new THREE.Mesh(
        new THREE.SphereGeometry(0.18, 16, 12),
        new THREE.MeshBasicMaterial({ color: 0x90a0b2 })
      );
      nSphere.position.copy(a.dir).multiplyScalar(-1.0);
      nSphere.userData = {
        axisDir: a.dir.clone().multiplyScalar(-1),
        axisLabel: "-" + a.label.slice(1),
      };
      this.axisWidgetRoot.add(nSphere);
      this.axisWidgetPickables.push(nSphere);

    }
  };

  SceneVisualEditor.prototype.getAxisWidgetRect = function () {
    var vp = this.renderViewport || { x: 0, y: 0, w: 1, h: 1 };
    var base = Number(this.axisWidgetSize) || 94;
    var size = clamp(Math.round(Math.min(Math.min(vp.w, vp.h) * 0.18, base * 1.25)), 72, Math.max(72, base));
    var m = this.axisWidgetMargin;
    return {
      x: Math.round(vp.x + vp.w - size - m),
      y: Math.round(vp.y + vp.h - size - m),
      w: size,
      h: size,
    };
  };

  SceneVisualEditor.prototype.snapViewToAxis = function (axisDir, axisLabel) {
    if (!axisDir || axisDir.lengthSq() < 1e-8) return false;
    var d = axisDir.clone().normalize();
    var horiz = Math.sqrt(d.x * d.x + d.z * d.z);
    var az = this.azimuth;
    if (horiz > 1e-6) az = Math.atan2(d.x, d.z);
    this.azimuth = az;
    this.elevation = clamp(Math.atan2(d.y, Math.max(1e-6, horiz)), -ELEVATION_LIMIT, ELEVATION_LIMIT);
    this.setStatus("View: " + String(axisLabel || "axis"));
    return true;
  };

  SceneVisualEditor.prototype.handleAxisWidgetPointerDown = function (clientX, clientY) {
    if (!this.renderer || !this.axisWidgetCamera || !this.axisWidgetPickables || this.axisWidgetPickables.length === 0) return false;
    var rect = this.renderer.domElement.getBoundingClientRect();
    if (!rect || rect.width <= 0 || rect.height <= 0) return false;

    var rx = clientX - rect.left;
    var ry = rect.bottom - clientY;
    var wr = this.getAxisWidgetRect();
    if (rx < wr.x || rx > (wr.x + wr.w) || ry < wr.y || ry > (wr.y + wr.h)) return false;

    var nx = ((rx - wr.x) / wr.w) * 2 - 1;
    var ny = ((ry - wr.y) / wr.h) * 2 - 1;
    this.axisWidgetRaycaster.setFromCamera(new THREE.Vector2(nx, ny), this.axisWidgetCamera);
    var hits = this.axisWidgetRaycaster.intersectObjects(this.axisWidgetPickables, false);
    if (!hits || hits.length === 0) return false;

    var hit = hits[0].object;
    var dir = hit && hit.userData ? hit.userData.axisDir : null;
    var label = hit && hit.userData ? hit.userData.axisLabel : "";
    if (!dir) return false;
    this.snapViewToAxis(dir, label);
    return true;
  };

  SceneVisualEditor.prototype.setPhotonPoints = function (diffuse, caustic) {
    if (!this.photonRoot) return;
    this.clearPhotonPoints();

    var build = function (points, colorHex) {
      if (!points || !points.length) return null;
      var arr = new Float32Array(points.length * 3);
      for (var i = 0; i < points.length; i += 1) {
        var p = points[i] || [0, 0, 0];
        var x = Array.isArray(p) ? Number(p[0]) : Number(p.x);
        var y = Array.isArray(p) ? Number(p[1]) : Number(p.y);
        var z = Array.isArray(p) ? Number(p[2]) : Number(p.z);
        arr[i * 3 + 0] = Number.isFinite(x) ? x : 0;
        arr[i * 3 + 1] = Number.isFinite(y) ? y : 0;
        arr[i * 3 + 2] = Number.isFinite(z) ? z : 0;
      }
      var g = new THREE.BufferGeometry();
      g.setAttribute("position", new THREE.BufferAttribute(arr, 3));
      var m = new THREE.PointsMaterial({
        color: colorHex,
        size: 0.12,
        sizeAttenuation: true,
        transparent: true,
        opacity: 0.95,
        depthTest: true,
        depthWrite: false,
        polygonOffset: true,
        polygonOffsetFactor: -1.0,
        polygonOffsetUnits: -2.0,
      });
      return new THREE.Points(g, m);
    };

    this.photonDiffuse = build(diffuse || [], 0x5fd3ff);
    this.photonCaustic = build(caustic || [], 0xffca4f);
    if (this.photonDiffuse) this.photonRoot.add(this.photonDiffuse);
    if (this.photonCaustic) this.photonRoot.add(this.photonCaustic);
  };

  SceneVisualEditor.prototype.buildCameraWidgetGeometry = function (hfov, aspect, span) {
    var a = Math.max(1e-6, Number(aspect) || 1);
    var depth = Math.max(0.2, Number(span) || 1.2);
    var near = depth * 0.24;
    var far = depth;
    var halfWn = Math.tan(degToRad(clamp(hfov, 1, 179)) * 0.5) * near;
    var halfHn = halfWn / a;
    var halfWf = Math.tan(degToRad(clamp(hfov, 1, 179)) * 0.5) * far;
    var halfHf = halfWf / a;

    var n0 = vec3(-halfWn, -halfHn, near);
    var n1 = vec3(halfWn, -halfHn, near);
    var n2 = vec3(halfWn, halfHn, near);
    var n3 = vec3(-halfWn, halfHn, near);
    var f0 = vec3(-halfWf, -halfHf, far);
    var f1 = vec3(halfWf, -halfHf, far);
    var f2 = vec3(halfWf, halfHf, far);
    var f3 = vec3(-halfWf, halfHf, far);
    var o = vec3(0, 0, 0);
    var up = vec3(0, depth * 0.2, 0);
    var fw = vec3(0, 0, depth * 0.34);

    var seg = [
      o, f0, o, f1, o, f2, o, f3,
      n0, n1, n1, n2, n2, n3, n3, n0,
      f0, f1, f1, f2, f2, f3, f3, f0,
      n0, f0, n1, f1, n2, f2, n3, f3,
      o, up, o, fw,
    ];
    var p = new Float32Array(seg.length * 3);
    for (var i = 0; i < seg.length; i += 1) {
      p[i * 3 + 0] = seg[i].x;
      p[i * 3 + 1] = seg[i].y;
      p[i * 3 + 2] = seg[i].z;
    }
    var g = new THREE.BufferGeometry();
    g.setAttribute("position", new THREE.BufferAttribute(p, 3));
    return g;
  };

  SceneVisualEditor.prototype.updateCameraWidget = function () {
    this.clearCameraWidget();
    if (!this.selectedCameraPose || !this.cameraWidgetRoot) return;

    var aspect = (Number.isFinite(this.frameAspect) && this.frameAspect > 0)
      ? this.frameAspect
      : (this.camera && this.camera.aspect ? this.camera.aspect : 1);
    var span = clamp((this.sceneRadius || 2.0) * 0.6, 0.8, 16.0);
    var g = this.buildCameraWidgetGeometry(this.selectedCameraHFov, aspect, span);
    var m = new THREE.LineBasicMaterial({ color: 0x55d3ff });
    this.cameraWidget = new THREE.Group();
    this.cameraWidget.add(new THREE.LineSegments(g, m));

    var position = this.selectedCameraPose.position.clone();
    var target = this.selectedCameraPose.target.clone();
    var upHint = (this.selectedCameraPose.up && this.selectedCameraPose.up.lengthSq() > 1e-10)
      ? this.selectedCameraPose.up.clone().normalize()
      : vec3(0, 1, 0);
    var forward = target.sub(position);
    if (forward.lengthSq() < 1e-10) forward.set(0, 0, 1);
    forward.normalize();
    var right = new THREE.Vector3().crossVectors(upHint, forward);
    if (right.lengthSq() < 1e-10) {
      upHint = Math.abs(forward.y) > 0.98 ? vec3(0, 0, 1) : vec3(0, 1, 0);
      right.crossVectors(upHint, forward);
    }
    right.normalize();
    var up = new THREE.Vector3().crossVectors(forward, right).normalize();
    var basis = new THREE.Matrix4();
    // Camera widget geometry is authored looking down +Z.
    basis.makeBasis(right, up, forward);
    var q = new THREE.Quaternion().setFromRotationMatrix(basis);

    this.cameraWidget.position.copy(this.selectedCameraPose.position);
    this.cameraWidget.quaternion.copy(q);

    if (this.selectedCameraType === "thin-lens") {
      var fl = Math.max(0, Number(this.selectedCameraFLength) || 0);
      var ap = Math.max(0, Number(this.selectedCameraAperture) || 0);

      if (ap > 0) {
        var ar = Math.max(ap * 0.5, span * 0.04);
        var ringGeo = new THREE.RingGeometry(ar * 0.92, ar, 48, 1);
        var ringMat = new THREE.MeshBasicMaterial({
          color: 0xffb347,
          side: THREE.DoubleSide,
          transparent: true,
          opacity: 0.72,
          depthWrite: false,
        });
        var ring = new THREE.Mesh(ringGeo, ringMat);
        this.cameraWidget.add(ring);
      }

      if (fl > 0) {
        var halfWf = Math.tan(degToRad(clamp(this.selectedCameraHFov, 1, 179)) * 0.5) * fl;
        var halfHf = halfWf / Math.max(1e-6, aspect);
        var p0 = vec3(-halfWf, -halfHf, fl);
        var p1 = vec3(halfWf, -halfHf, fl);
        var p2 = vec3(halfWf, halfHf, fl);
        var p3 = vec3(-halfWf, halfHf, fl);
        var seg = [p0, p1, p1, p2, p2, p3, p3, p0];
        var arr = new Float32Array(seg.length * 3);
        for (var si = 0; si < seg.length; si += 1) {
          arr[si * 3 + 0] = seg[si].x;
          arr[si * 3 + 1] = seg[si].y;
          arr[si * 3 + 2] = seg[si].z;
        }
        var fg = new THREE.BufferGeometry();
        fg.setAttribute("position", new THREE.BufferAttribute(arr, 3));
        var fm = new THREE.LineBasicMaterial({
          color: 0xff8f5e,
          transparent: true,
          opacity: 0.9,
        });
        this.cameraWidget.add(new THREE.LineSegments(fg, fm));

        var ag = new THREE.BufferGeometry();
        ag.setAttribute("position", new THREE.BufferAttribute(new Float32Array([0, 0, 0, 0, 0, fl]), 3));
        var am = new THREE.LineBasicMaterial({ color: 0xffc58a, transparent: true, opacity: 0.85 });
        this.cameraWidget.add(new THREE.LineSegments(ag, am));
      }
    }

    this.cameraWidgetRoot.add(this.cameraWidget);
  };

  SceneVisualEditor.prototype.fitDistanceForRadius = function (radius) {
    var r = Math.max(1e-4, Number(radius) || 1.0);
    var aspect = (Number.isFinite(this.frameAspect) && this.frameAspect > 0)
      ? this.frameAspect
      : (this.camera && this.camera.aspect ? this.camera.aspect : 1);
    var vfov = (this.perspectiveCamera && Number.isFinite(this.perspectiveCamera.fov) && this.perspectiveCamera.fov > 1 && this.perspectiveCamera.fov < 179)
      ? this.perspectiveCamera.fov
      : 45;
    var hfov = radToDeg(2.0 * Math.atan(Math.tan(degToRad(vfov) * 0.5) * Math.max(1e-6, aspect)));
    var lim = Math.min(vfov, hfov);
    var half = degToRad(clamp(lim, 1, 179)) * 0.5;
    var d = r / Math.max(1e-6, Math.sin(half));
    return d * 1.12;
  };

  SceneVisualEditor.prototype.applyDefTransform = function (mesh, def) {
    var hasBakedMeshTransform = String(def.type || "").toLowerCase() === "mesh"
      && !!(this.sceneGeometry && this.sceneGeometry.meshes && this.sceneGeometry.meshes[def.id]);

    var mods = def.modifiers || {
      rotation: vec3(0, 0, 0),
      scale: vec3(1, 1, 1),
      translation: vec3(0, 0, 0),
    };

    // For mesh surfaces loaded from xtcore triangles, modifiers are already baked by parser/build.
    if (!hasBakedMeshTransform) {
      mesh.scale.copy(mods.scale || vec3(1, 1, 1));
      mesh.rotation.set(degToRad(mods.rotation.x), degToRad(mods.rotation.y), degToRad(mods.rotation.z));
      mesh.position.add(mods.translation || vec3(0, 0, 0));
    }

    if (def.type === "sphere" || def.type === "point") {
      mesh.position.add(def.position || vec3(0, 0, 0));
    } else if (def.type === "plane") {
      var n = (def.normal || vec3(0, 1, 0)).clone();
      if (n.lengthSq() < 1e-8) n.set(0, 1, 0);
      n.normalize();
      var q = new THREE.Quaternion().setFromUnitVectors(new THREE.Vector3(0, 1, 0), n);
      mesh.quaternion.multiply(q);
      // Mirror xtcore plane convention used in intersection:
      // reference point v = abs(normal) * distance.
      var d0 = Number(def.distance) || 0;
      var v0 = new THREE.Vector3(Math.abs(n.x), Math.abs(n.y), Math.abs(n.z)).multiplyScalar(d0);
      mesh.position.add(v0);
    }
  };

  SceneVisualEditor.prototype.geometryFromSceneData = function (def) {
    if (!def || !def.id || !this.sceneGeometry || !this.sceneGeometry.meshes) return null;
    var meshDef = this.sceneGeometry.meshes[def.id];
    if (!meshDef || !Array.isArray(meshDef.positions) || meshDef.positions.length < 9) return null;

    var g = new THREE.BufferGeometry();
    g.setAttribute("position", new THREE.Float32BufferAttribute(meshDef.positions, 3));

    if (Array.isArray(meshDef.normals) && meshDef.normals.length === meshDef.positions.length) {
      g.setAttribute("normal", new THREE.Float32BufferAttribute(meshDef.normals, 3));
    } else {
      g.computeVertexNormals();
    }
    g.computeBoundingSphere();
    return g;
  };

  SceneVisualEditor.prototype.geometryForDefAsync = async function (sceneName, def) {
    if (String(def.type || "").toLowerCase() !== "mesh") return geometryForDef(def);
    var meshFromScene = this.geometryFromSceneData(def);
    if (meshFromScene) return meshFromScene;
    var srcResolved = String(def.sourceResolved || def.source || "").trim();
    if (this.fetchSceneAssetText && sceneName && srcResolved && /\.obj$/i.test(srcResolved)) {
      try {
        var key = sceneName + "::" + srcResolved;
        var objText = this.assetTextCache[key];
        if (!objText) {
          objText = await this.fetchSceneAssetText(sceneName, srcResolved);
          this.assetTextCache[key] = objText;
        }
        var parsedObj = parseObjToGeometry(objText);
        if (parsedObj) return parsedObj;
      } catch (_) {}
    }
    return geometryForDef(def);
  };

  SceneVisualEditor.prototype.fitDirectionalShadowToBox = function (light, box) {
    if (!light || !light.shadow || !light.shadow.camera || !box || box.isEmpty()) return;
    var cam = light.shadow.camera;
    var target = (light.target && light.target.position) ? light.target.position.clone() : new THREE.Vector3();
    var lp = light.position.clone();
    if (lp.distanceToSquared(target) < 1e-8) {
      lp.add(new THREE.Vector3(1.0, 2.0, 1.0));
    }

    cam.position.copy(lp);
    cam.up.set(0, 1, 0);
    cam.lookAt(target);
    cam.updateMatrixWorld(true);

    var min = new THREE.Vector3(Infinity, Infinity, Infinity);
    var max = new THREE.Vector3(-Infinity, -Infinity, -Infinity);
    var corners = [
      new THREE.Vector3(box.min.x, box.min.y, box.min.z),
      new THREE.Vector3(box.min.x, box.min.y, box.max.z),
      new THREE.Vector3(box.min.x, box.max.y, box.min.z),
      new THREE.Vector3(box.min.x, box.max.y, box.max.z),
      new THREE.Vector3(box.max.x, box.min.y, box.min.z),
      new THREE.Vector3(box.max.x, box.min.y, box.max.z),
      new THREE.Vector3(box.max.x, box.max.y, box.min.z),
      new THREE.Vector3(box.max.x, box.max.y, box.max.z),
    ];
    for (var i = 0; i < corners.length; i += 1) {
      var v = corners[i].clone().applyMatrix4(cam.matrixWorldInverse);
      min.min(v);
      max.max(v);
    }

    var size = box.getSize(new THREE.Vector3());
    var padXY = Math.max(0.02, Math.max(size.x, Math.max(size.y, size.z)) * 0.06);
    var padZ = Math.max(0.05, size.length() * 0.06);
    cam.left = min.x - padXY;
    cam.right = max.x + padXY;
    cam.bottom = min.y - padXY;
    cam.top = max.y + padXY;
    cam.near = Math.max(0.01, -max.z - padZ);
    cam.far = Math.max(cam.near + 0.2, -min.z + padZ);
    cam.updateProjectionMatrix();
  };

  SceneVisualEditor.prototype.buildScene = async function (sceneName, source, geometryData) {
    this.currentSceneName = String(sceneName || "");
    this.assetTextCache = {};
    this.parsedScene = null;
    this.objectMeshById = {};
    this.objectMetaById = {};
    if (this.selectedMesh) this.applySelectionHighlight(this.selectedMesh, false);
    this.selectedMesh = null;
    this.selectedObjectName = "";
    this.clearPhotonPoints();
    while (this.modelRoot.children.length > 0) {
      var c = this.modelRoot.children.pop();
      if (c.geometry) c.geometry.dispose();
      if (c.material) {
        if (Array.isArray(c.material)) c.material.forEach(function (m) { if (m) m.dispose(); });
        else c.material.dispose();
      }
    }
    this.infinitePlanes = [];

    this.sceneGeometry = geometryData && geometryData.meshes ? geometryData : { meshes: {} };
    if (this.fetchSceneGeometry && (!geometryData || !geometryData.meshes)) {
      try {
        this.sceneGeometry = await this.fetchSceneGeometry(sceneName);
      } catch (_) {
        this.sceneGeometry = { meshes: {} };
      }
    }

    var parsed = parseSceneSource(source || "");
    this.parsedScene = parsed;
    this.sceneCameraMap = {};
    this.sceneCameraOrder = [];
    for (var ci = 0; ci < parsed.cameras.length; ci += 1) {
      var cam = parsed.cameras[ci];
      this.sceneCameraMap[cam.id] = cam;
      this.sceneCameraOrder.push(cam.id);
    }
    var count = 0;
    var areaLightCenters = [];

    for (var i = 0; i < parsed.objects.length; i += 1) {
      var obj = parsed.objects[i];
      var geoDef = parsed.geometries[obj.geometry];
      if (!geoDef) continue;

      var geo = await this.geometryForDefAsync(sceneName, geoDef);
      var matDef = parsed.materials[obj.material];
      var mat = materialForDef(matDef);
      var mesh = new THREE.Mesh(geo, mat);
      mesh.name = obj.id;
      mesh.userData = mesh.userData || {};
      mesh.userData.objectId = obj.id;
      mesh.userData.geometryId = obj.geometry;
      mesh.userData.materialId = obj.material || "";
      mesh.userData.geometryType = String(geoDef.type || "").toLowerCase();
      mesh.userData.infinitePlane = mesh.userData.geometryType === "plane";
      this.objectMeshById[obj.id] = mesh;
      this.objectMetaById[obj.id] = {
        objectId: obj.id,
        geometryId: obj.geometry,
        materialId: obj.material || "",
        geometryType: mesh.userData.geometryType,
      };
      var isEmissive = !!(matDef && String(matDef.type || "").toLowerCase() === "emissive");
      mesh.castShadow = !isEmissive;
      mesh.receiveShadow = !isEmissive;
      this.applyDefTransform(mesh, geoDef);
      this.modelRoot.add(mesh);

      if (isEmissive) {
        var lc = new THREE.Vector3();
        new THREE.Box3().setFromObject(mesh).getCenter(lc);
        if (Number.isFinite(lc.x) && Number.isFinite(lc.y) && Number.isFinite(lc.z)) {
          areaLightCenters.push(lc);
        }
      }

      if (String(geoDef.type || "").toLowerCase() === "plane") {
        var n = (geoDef.normal || vec3(0, 1, 0)).clone();
        if (n.lengthSq() < 1e-8) n.set(0, 1, 0);
        n.normalize();
        this.infinitePlanes.push({
          mesh: mesh,
          normal: n,
          distance: Number(geoDef.distance) || 0,
        });
      } else {
        var addEdges = true;
        if (geo && geo.boundingBox === null && typeof geo.computeBoundingBox === "function") {
          geo.computeBoundingBox();
        }
        if (geo && geo.boundingBox) {
          var edgeSize = new THREE.Vector3();
          geo.boundingBox.getSize(edgeSize);
          var edgeMax = Math.max(edgeSize.x, Math.max(edgeSize.y, edgeSize.z));
          var edgeMin = Math.min(edgeSize.x, Math.min(edgeSize.y, edgeSize.z));
          if (edgeMax > 1e-6 && (edgeMin / edgeMax) < 0.02) {
            // Skip wire edges on near-planar meshes (e.g. mesh ground quads) to avoid moire striping.
            addEdges = false;
          }
        }
        if (!addEdges) {
          count += 1;
          continue;
        }
        var edges = new THREE.LineSegments(
          new THREE.EdgesGeometry(geo, 35),
          new THREE.LineBasicMaterial({ color: 0x10161d, transparent: true, opacity: 0.35 })
        );
        mesh.add(edges);
      }

      count += 1;
    }

    if (count > 0) {
      var box = new THREE.Box3().setFromObject(this.modelRoot);
      var size = new THREE.Vector3();
      var center = new THREE.Vector3();
      box.getSize(size);
      box.getCenter(center);
      var radius = Math.max(size.x, Math.max(size.y, size.z)) * 0.5;
      this.sceneRadius = Math.max(0.25, radius);
      this.target.copy(center);
      this.distance = clamp(this.fitDistanceForRadius(this.sceneRadius), 1.5, 120);

      if (this.keyLight) {
        var lr = Math.max(1.5, this.sceneRadius * 2.0);
        if (areaLightCenters.length > 0) {
          var la = new THREE.Vector3();
          for (var ai = 0; ai < areaLightCenters.length; ai += 1) la.add(areaLightCenters[ai]);
          la.multiplyScalar(1.0 / areaLightCenters.length);
          this.keyLight.position.copy(la);
        } else {
          this.keyLight.position.set(center.x + lr, center.y + lr * 1.4, center.z + lr * 0.8);
        }
        this.keyLight.target.position.copy(center);
        this.keyLight.target.updateMatrixWorld();
        this.fitDirectionalShadowToBox(this.keyLight, box);
      }

      if (this.rimLight) {
        if (areaLightCenters.length > 1) this.rimLight.position.copy(areaLightCenters[1]);
        else this.rimLight.position.set(center.x - this.sceneRadius * 1.5, center.y + this.sceneRadius, center.z - this.sceneRadius * 1.2);
      }
    } else {
      this.sceneRadius = 2.0;
    }

    if (this.sceneCameraMap.default) this.setActiveCamera("default");
    else if (this.sceneCameraOrder.length > 0) this.setActiveCamera(this.sceneCameraOrder[0]);
    else this.setActiveCamera("");

    this.notifySelectionChanged();
    this.setStatus("Selection: none (click to select, Ctrl+drag to move)");
  };

  SceneVisualEditor.prototype.getSelectedObjectId = function () {
    return this.selectedObjectName || "";
  };

  SceneVisualEditor.prototype.getObjectMeta = function (objectId) {
    if (!objectId || !this.objectMetaById) return null;
    return this.objectMetaById[objectId] || null;
  };

  SceneVisualEditor.prototype.getObjectList = function () {
    var out = [];
    var self = this;
    Object.keys(this.objectMetaById || {}).forEach(function (id) {
      if (self.objectMetaById[id]) out.push(self.objectMetaById[id]);
    });
    out.sort(function (a, b) {
      return String(a.objectId || "").localeCompare(String(b.objectId || ""));
    });
    return out;
  };

  SceneVisualEditor.prototype.selectObjectById = function (objectId, focusView) {
    var id = String(objectId || "").trim();
    if (!id || !this.objectMeshById || !this.objectMeshById[id]) {
      this.setSelectedMesh(null, false);
      return false;
    }
    this.setSelectedMesh(this.objectMeshById[id], !!focusView);
    return true;
  };

  SceneVisualEditor.prototype.getCameraNames = function () {
    return this.sceneCameraOrder.slice();
  };

  SceneVisualEditor.prototype.getActiveCamera = function () {
    return this.activeSceneCamera || "";
  };

  SceneVisualEditor.prototype.setActiveCamera = function (name) {
    var key = String(name || "").trim();
    if (!key || !this.sceneCameraMap[key]) {
      this.activeSceneCamera = "";
      this.selectedCameraPose = null;
      this.selectedCameraHFov = 45;
      this.selectedCameraAperture = 0;
      this.selectedCameraFLength = 0;
      this.selectedCameraType = "";
      this.updateCameraWidget();
      return;
    }

    var c = this.sceneCameraMap[key];
    var pos = c.position ? c.position.clone() : vec3(0, 2, -6);
    var target = c.target ? c.target.clone() : vec3(0, 0, 0);
    var up = c.up ? c.up.clone() : vec3(0, 1, 0);

    // If target is missing/non-sensical for non-thin-lens cameras, use orientation as forward.
    if (c.type !== "thin-lens") {
      var hasExplicitTarget = target.lengthSq() > 1e-8;
      if (!hasExplicitTarget && c.orientation && c.orientation.lengthSq() > 1e-8) {
        // xtcore camera orientation is Euler XYZ in radians.
        var e = new THREE.Euler(c.orientation.x, c.orientation.y, c.orientation.z, "XYZ");
        var fwd = new THREE.Vector3(0, 0, 1).applyEuler(e);
        if (fwd.lengthSq() > 1e-8) target = pos.clone().add(fwd.normalize());
      }
    }

    var hfov = (Number.isFinite(c.fov) && c.fov > 1 && c.fov < 179) ? c.fov : 45;
    this.selectedCameraHFov = hfov;
    this.selectedCameraAperture = Math.max(0, Number(c.aperture) || 0);
    this.selectedCameraFLength = Math.max(0, Number(c.flength) || 0);
    this.selectedCameraType = String(c.type || "").toLowerCase();
    this.selectedCameraPose = {
      position: pos.clone(),
      target: target.clone(),
      up: up.lengthSq() > 1e-8 ? up.normalize() : vec3(0, 1, 0),
    };
    this.activeSceneCamera = key;
    this.updateCameraWidget();
  };

  SceneVisualEditor.prototype.resize = function () {
    if (!this.renderer || !this.viewportEl || !this.camera) return;
    var w = Math.max(1, this.viewportEl.clientWidth);
    var h = Math.max(1, this.viewportEl.clientHeight);
    this.renderer.setSize(w, h, false);
    this.renderViewport = {
      x: 0,
      y: 0,
      w: w,
      h: h,
    };
    if (this.perspectiveCamera) {
      this.perspectiveCamera.aspect = w / h;
      this.perspectiveCamera.updateProjectionMatrix();
    }
    this.updateProjectionForDistance();
  };

  SceneVisualEditor.prototype.setFrameAspect = function (width, height) {
    var w = Number(width);
    var h = Number(height);
    if (!Number.isFinite(w) || !Number.isFinite(h) || w <= 0 || h <= 0) return;
    this.frameAspect = w / h;
    this.resize();
    this.updateCameraWidget();
  };

  SceneVisualEditor.prototype.onShow = function () {
    this.resize();
  };

  SceneVisualEditor.prototype.setGridVisible = function (visible) {
    if (!this.grid) return;
    this.grid.visible = !!visible;
  };

  SceneVisualEditor.prototype.computeXtcoreBasis = function (position, target, upHint) {
    var forward = target.clone().sub(position);
    if (forward.lengthSq() < 1e-10) forward.set(0, 0, 1);
    forward.normalize();

    var up = (upHint && upHint.lengthSq() > 1e-10) ? upHint.clone().normalize() : vec3(0, 1, 0);
    var right = new THREE.Vector3().crossVectors(up, forward);
    if (right.lengthSq() < 1e-10) {
      up = Math.abs(forward.y) > 0.98 ? vec3(0, 0, 1) : vec3(0, 1, 0);
      right.crossVectors(up, forward);
    }
    right.normalize();

    // xtcore raster y grows downward; map Three.js +Y raster direction to -ry.
    var camUp = new THREE.Vector3().crossVectors(forward, right).normalize();
    var back = forward.clone().multiplyScalar(-1);

    var basis = new THREE.Matrix4();
    basis.makeBasis(right, camUp, back);

    var q = new THREE.Quaternion().setFromRotationMatrix(basis);
    return {
      quaternion: q,
      up: camUp,
    };
  };

  SceneVisualEditor.prototype.applyXtcoreCameraBasis = function (position, target, upHint) {
    var up = (upHint && upHint.lengthSq() > 1e-10) ? upHint.clone().normalize() : vec3(0, 1, 0);
    this.camera.position.copy(position);
    this.camera.up.copy(up);
    this.camera.lookAt(target);
  };

  SceneVisualEditor.prototype.updateInfinitePlanes = function () {
    if (!this.infinitePlanes || this.infinitePlanes.length === 0) return;

    var camPos = this.camera.position;
    var anchor = this.target.clone();

    for (var i = 0; i < this.infinitePlanes.length; i += 1) {
      var p = this.infinitePlanes[i];
      var n = p.normal;
      var d = p.distance;

      // Mirror xtcore plane convention: v = abs(n) * d, plane through v with normal n.
      var v = new THREE.Vector3(Math.abs(n.x), Math.abs(n.y), Math.abs(n.z)).multiplyScalar(d);
      var signed = anchor.clone().sub(v).dot(n);
      var center = anchor.clone().addScaledVector(n, -signed);

      var dist = Math.abs(camPos.clone().sub(center).dot(n));
      var span = clamp(40 + dist * 6.0 + this.distance * 2.0, 80, 4000);

      p.mesh.position.copy(center);
      // Plane primitive is aligned to XZ in local space; keep Y as thickness axis.
      p.mesh.scale.set(span, 1, span);
    }
  };

  SceneVisualEditor.prototype.animate = function () {
    if (!this.renderer || !this.scene || !this.camera) return;

    var cp = new THREE.Vector3();
    cp.x = this.target.x + this.distance * Math.cos(this.elevation) * Math.sin(this.azimuth);
    cp.y = this.target.y + this.distance * Math.sin(this.elevation);
    cp.z = this.target.z + this.distance * Math.cos(this.elevation) * Math.cos(this.azimuth);
    this.updateProjectionForDistance();
    this.applyXtcoreCameraBasis(cp, this.target, this.viewUp);
    this.updateInfinitePlanes();

    var vp = this.renderViewport || { x: 0, y: 0, w: 1, h: 1 };
    var canvas = this.renderer.domElement;
    var fullW = canvas ? canvas.width : 1;
    var fullH = canvas ? canvas.height : 1;
    this.renderer.setScissorTest(true);
    this.renderer.setViewport(0, 0, fullW, fullH);
    this.renderer.setScissor(0, 0, fullW, fullH);
    this.renderer.clear(true, true, true);
    this.renderer.setViewport(vp.x, vp.y, vp.w, vp.h);
    this.renderer.setScissor(vp.x, vp.y, vp.w, vp.h);
    this.renderer.render(this.scene, this.camera);

    if (this.axisWidgetScene && this.axisWidgetCamera && this.axisWidgetRoot) {
      var wr = this.getAxisWidgetRect();
      this.axisWidgetRoot.quaternion.copy(this.camera.quaternion).invert();
      this.renderer.clearDepth();
      this.renderer.setViewport(wr.x, wr.y, wr.w, wr.h);
      this.renderer.setScissor(wr.x, wr.y, wr.w, wr.h);
      this.renderer.render(this.axisWidgetScene, this.axisWidgetCamera);
    }

    this.renderer.setScissorTest(false);
    requestAnimationFrame(this._animateBound);
  };

  window.SceneVisualEditor = SceneVisualEditor;
})();
