(function () {
  "use strict";

  const LOCAL_SCENES_KEY = "xtracer-wasm-local-scenes-v1";
  const DEFAULT_INTEGRATORS = [
    { id: "pathtracer", label: "Pathtracer (Brute Force)" },
    { id: "pathtracer_mis", label: "Pathtracer (MIS Diffuse)" },
    { id: "pathtracer_mis_full", label: "Pathtracer (MIS Full)" },
    { id: "photon_mapping", label: "Photon Mapping" },
    { id: "debug_views", label: "Debug Views" },
    { id: "ao", label: "Ambient Occlusion" },
  ];
  const DEFAULT_POST_FILTERS = [
    { id: "desaturate", label: "Desaturate" },
    { id: "chromatic_aberration", label: "Chromatic Aberration" },
    { id: "vignette", label: "Vignette" },
    { id: "film_grain", label: "Film Grain" },
    { id: "denoise", label: "Bilateral Denoise" },
    { id: "fxaa", label: "FXAA" },
    { id: "sharpen", label: "Sharpen" },
    { id: "brightness", label: "Brightness" },
    { id: "contrast", label: "Contrast" },
    { id: "raindrops_lens", label: "Raindrops on Lens" },
  ];
  const DEFAULT_RESOLUTIONS = [
    { id: 0, description: "Square", width: 500, height: 500 },
    { id: 1, description: "VGA/SD", width: 640, height: 480 },
    { id: 2, description: "HD", width: 1280, height: 720 },
    { id: 3, description: "FHD", width: 1920, height: 1080 },
    { id: 4, description: "QHD/WQHD", width: 2560, height: 1440 },
    { id: 5, description: "4K UHD", width: 3840, height: 2160 },
  ];
  const THIRD_PARTY_LICENSES = [
    { name: "cgltf", description: "Single-file glTF 2.0 loader used for importing compact scene assets into the renderer.", used_in: "xtcore", license: "MIT", url: "https://github.com/jkuhlmann/cgltf" },
    { name: "TinyObjLoader", description: "Wavefront OBJ and MTL loader used by the mesh pipeline and scene import path.", used_in: "lib/nmesh, xtcore", license: "MIT", url: "https://github.com/tinyobjloader/tinyobjloader" },
    { name: "STB", description: "Collection of single-header image and utility libraries used for texture IO and image helpers.", used_in: "lib/nimg, xtcore, xtracer-web", license: "Public Domain / MIT", url: "https://github.com/nothings/stb" },
    { name: "TinyEXR", description: "OpenEXR reader and writer used for high-dynamic-range image support.", used_in: "lib/nimg", license: "BSD-3-Clause", url: "https://github.com/syoyo/tinyexr" },
    { name: "strpool", description: "String interning helper used to keep repeated identifiers compact in runtime data structures.", used_in: "xtcore, frontend/common, xtracer-web, xtracer-wasm", license: "MIT / Public Domain", url: "https://github.com/mattiasgustavsson/libs" },
    { name: "cpp-httplib", description: "HTTP server and client header library used by the web backend API layer.", used_in: "xtracer-web", license: "MIT", url: "https://github.com/yhirose/cpp-httplib" },
    { name: "Three.js", description: "3D scene graph and rendering toolkit used by the web visualizer and interactive previews.", used_in: "xtracer-web", license: "MIT", url: "https://github.com/mrdoob/three.js" },
    { name: "ufbx", description: "FBX parser and evaluator used to read production-style geometry, transforms, and animation data.", used_in: "xtcore", license: "MIT", url: "https://github.com/ufbx/ufbx" },
  ];

  function makeError(message, status) {
    const err = new Error(message || "request failed");
    if (status !== undefined) err.status = status;
    return err;
  }

  function nowIso() {
    return new Date().toISOString();
  }

  async function fetchJson(url) {
    const res = await fetch(url, { cache: "no-store" });
    if (!res.ok) throw makeError(`HTTP ${res.status}`, res.status);
    return res.json();
  }

  async function fetchText(url) {
    const res = await fetch(url, { cache: "no-store" });
    if (!res.ok) throw makeError(`HTTP ${res.status}`, res.status);
    return res.text();
  }

  function normalizeSceneName(name) {
    const n = (name || "").trim();
    if (!n) return "";
    return n.endsWith(".scn") ? n : `${n}.scn`;
  }

  function safeSceneFile(name) {
    return /^[A-Za-z0-9_.-]+\.scn$/.test(name || "");
  }

  function mergeUniqueSorted(items) {
    const s = new Set((items || []).filter((x) => typeof x === "string" && x));
    return Array.from(s).sort();
  }

  function loadLocalSceneMap() {
    try {
      const raw = localStorage.getItem(LOCAL_SCENES_KEY);
      if (!raw) return {};
      const parsed = JSON.parse(raw);
      if (!parsed || typeof parsed !== "object") return {};
      return parsed;
    } catch (_) {
      return {};
    }
  }

  function saveLocalSceneMap(map) {
    localStorage.setItem(LOCAL_SCENES_KEY, JSON.stringify(map || {}));
  }

  function extractBlockAfterKey(text, key) {
    const re = new RegExp(`\\b${key}\\s*=\\s*\\{`, "m");
    const m = re.exec(text || "");
    if (!m) return "";

    let i = m.index + m[0].length;
    let depth = 1;
    while (i < text.length && depth > 0) {
      const ch = text[i];
      if (ch === "{") depth += 1;
      else if (ch === "}") depth -= 1;
      i += 1;
    }

    if (depth !== 0) return "";
    return text.slice(m.index + m[0].length, i - 1);
  }

  function extractCameraNames(source) {
    const block = extractBlockAfterKey(source || "", "camera");
    if (!block) return [];
    const out = [];
    const re = /^\s*([A-Za-z0-9_.-]+)\s*=\s*\{/gm;
    let m = null;
    while ((m = re.exec(block)) !== null) {
      const name = m[1];
      if (name && name !== "camera") out.push(name);
    }
    return mergeUniqueSorted(out);
  }

  function sceneDependsOnExternalFiles(source) {
    const text = source || "";
    if (!text) return false;

    if (/\bpath_[A-Za-z0-9_]*\s*=/i.test(text)) return true;
    if (/=\s*<[^>\n]+>\s*\/[^\s#]+/i.test(text)) return true;
    if (/^\s*source\s*=\s*(?!gen\s*\()[^\n#]*\//im.test(text)) return true;
    if (/\bext\s*\(/i.test(text)) return true;

    return false;
  }

  window.XTracerWasmAdapter = function XTracerWasmAdapter(opts) {
    const options = opts || {};
    const serverApi = options.serverApi;
    const appendLog = typeof options.appendLog === "function" ? options.appendLog : function noop() {};

    if (!serverApi) throw new Error("serverApi is required for wasm adapter");

    const worker = new Worker("/wasm_worker.js");
    const pending = new Map();
    let nextRequestId = 1;
    let logSeq = 1;
    const localLogs = [];
    const sceneCache = new Map();

    function pushLog(level, message) {
      const entry = {
        id: logSeq,
        ts: nowIso(),
        level: level || "info",
        message: message || "",
      };
      logSeq += 1;
      localLogs.push(entry);
      if (localLogs.length > 1000) localLogs.splice(0, localLogs.length - 1000);
      appendLog(`wasm/${entry.level}: ${entry.message}`);
    }

    async function callWorker(type, payload) {
      const requestId = nextRequestId;
      nextRequestId += 1;
      return new Promise((resolve, reject) => {
        pending.set(requestId, { resolve, reject });
        worker.postMessage({
          type: "request",
          request_id: requestId,
          op: type,
          payload: payload || {},
        });
      });
    }

    worker.onmessage = (ev) => {
      const msg = ev.data || {};
      if (msg.type === "log") {
        pushLog(msg.level || "info", msg.message || "");
        return;
      }
      if (msg.type !== "response") return;

      const p = pending.get(msg.request_id);
      if (!p) return;
      pending.delete(msg.request_id);

      if (msg.ok) p.resolve(msg.result);
      else p.reject(makeError(msg.error || "worker request failed"));
    };

    worker.onerror = (ev) => {
      pushLog("error", `worker runtime error: ${ev.message || "unknown"}`);
    };

    window.addEventListener("beforeunload", () => {
      worker.terminate();
    });

    async function loadIntegrators() {
      try {
        const data = await fetchJson("/app/data/integrators.json");
        const list = Array.isArray(data.integrators) ? data.integrators : [];
        if (list.length > 0) return list;
      } catch (_) {
        // fallback below
      }
      try {
        return await serverApi.getIntegrators();
      } catch (_) {
        return DEFAULT_INTEGRATORS;
      }
    }

    async function loadPostFilters() {
      if (serverApi && typeof serverApi.getPostFilters === "function") {
        try {
          const list = await serverApi.getPostFilters();
          if (Array.isArray(list) && list.length > 0) return list;
        } catch (_) {
          // fallback below
        }
      }
      return DEFAULT_POST_FILTERS.slice();
    }

    async function loadResolutions() {
      try {
        return await serverApi.getResolutionPresets();
      } catch (_) {
        return DEFAULT_RESOLUTIONS;
      }
    }

    async function getSceneSourceHybrid(scene) {
      const sceneName = normalizeSceneName(scene);
      if (!safeSceneFile(sceneName)) throw makeError("invalid scene", 400);

      const localMap = loadLocalSceneMap();
      if (Object.prototype.hasOwnProperty.call(localMap, sceneName)) {
        return { scene: sceneName, source: localMap[sceneName] || "" };
      }

      if (sceneCache.has(sceneName)) {
        return { scene: sceneName, source: sceneCache.get(sceneName) || "" };
      }

      try {
        const source = await fetchText(`/scenes/${encodeURIComponent(sceneName)}`);
        sceneCache.set(sceneName, source);
        return { scene: sceneName, source };
      } catch (_) {
        // fallback below
      }

      const data = await serverApi.getSceneSource(sceneName);
      sceneCache.set(sceneName, data.source || "");
      return { scene: data.scene || sceneName, source: data.source || "" };
    }

    pushLog("info", "worker adapter initialized");

    return {
      mode: "wasm",
      async getLogsSince(sinceId) {
        const since = Number(sinceId) || 0;
        return localLogs.filter((e) => (e.id || 0) > since);
      },
      async getScenes() {
        const localMap = loadLocalSceneMap();
        const localNames = Object.keys(localMap);

        let serverNames = [];
        try {
          serverNames = await serverApi.getScenes();
        } catch (_) {
          serverNames = [];
        }
        const merged = mergeUniqueSorted([].concat(localNames, serverNames));
        const filtered = await Promise.all(merged.map(async (sceneName) => {
          try {
            const data = await getSceneSourceHybrid(sceneName);
            if (sceneDependsOnExternalFiles(data.source || "")) return null;
            return sceneName;
          } catch (_) {
            return null;
          }
        }));

        return filtered.filter((name) => !!name);
      },
      async getCameras(scene, variant) {
        if (!scene) return { cameras: [], cameraEntries: [], defaultCamera: "" };
        try {
          const fromServer = await serverApi.getCameras(scene, variant);
          if (fromServer && Array.isArray(fromServer.cameras) && fromServer.cameras.length >= 0) {
            return {
              cameras: fromServer.cameras || [],
              cameraEntries: Array.isArray(fromServer.cameraEntries) ? fromServer.cameraEntries : [],
              defaultCamera: fromServer.defaultCamera || "",
            };
          }
        } catch (_) {
          // fallback below
        }

        try {
          const data = await getSceneSourceHybrid(scene);
          const cameras = extractCameraNames(data.source || "");
          return {
            cameras,
            cameraEntries: cameras.map((name) => ({ name, type: "" })),
            defaultCamera: "",
          };
        } catch (_) {
          // fallback below
        }

        return { cameras: [], cameraEntries: [], defaultCamera: "" };
      },
      async getIntegrators() {
        return loadIntegrators();
      },
      async getPostFilters() {
        return loadPostFilters();
      },
      async getResolutionPresets() {
        return loadResolutions();
      },
      async getSceneSource(scene) {
        return getSceneSourceHybrid(scene);
      },
      async getEmptySceneTemplate() {
        try {
          if (typeof serverApi.getEmptySceneTemplate === "function") {
            return await serverApi.getEmptySceneTemplate();
          }
        } catch (_) {
          // fallback below
        }
        const data = await fetchJson("/api/scenes/template/empty");
        return data.source || "";
      },
      async getAbout() {
        try {
          const about = await serverApi.getAbout();
          return Object.assign({}, about, {
            backend: "xtracer_wasm_adapter",
            default_url: window.location.origin,
            scene_dir: "scenes/",
            static_assets: "/",
            third_party_licenses: Array.isArray(about.third_party_licenses)
              ? about.third_party_licenses
              : THIRD_PARTY_LICENSES,
          });
        } catch (_) {
          const currentYear = Math.max(2010, new Date().getFullYear());
          return {
            name: "XTRACER WEB",
            version: "",
            author_name: "Nikolaos Papadopoulos",
            author_email: "nikpapas@gmail.com",
            homepage: "https://www.4rknova.com",
            website: "https://github.com/4rknova/xtracer",
            copyright: `Copyright 2010-${currentYear} (c) Nikolaos Papadopoulos`,
            license: "Unavailable",
            third_party_licenses: THIRD_PARTY_LICENSES,
            backend: "xtracer_wasm_adapter",
            default_url: window.location.origin,
            scene_dir: "scenes/",
            static_assets: "/",
          };
        }
      },
      async startRender(params) {
        const p = Object.assign({}, params || {});
        if (!p.scene_source && p.scene) {
          try {
            const data = await getSceneSourceHybrid(p.scene);
            p.scene_source = data.source || "";
          } catch (err) {
            pushLog("warn", `scene source unavailable for wasm render (${err.message})`);
          }
        }
        const result = await callWorker("startRender", { params: p });
        return result.job_id;
      },
      async getJob(jobId) {
        return callWorker("getJob", { job_id: jobId });
      },
      async getJobImage(jobId, opts) {
        const result = await callWorker("getJobImage", {
          job_id: jobId,
          opts: opts || {},
        });
        if (!result || !result.bytes || !result.bytes.byteLength) return null;
        return new Blob([result.bytes], { type: result.mime || "image/png" });
      },
      async getJobExport(jobId, format, opts) {
        const result = await callWorker("getJobExport", {
          job_id: jobId,
          format: format || "png",
          opts: opts || {},
        });
        if (!result || !result.bytes || !result.bytes.byteLength) {
          throw makeError("export not available", 404);
        }
        return new Blob([result.bytes], { type: result.mime || "application/octet-stream" });
      },
      async saveScene(name, source, overwrite) {
        const sceneName = normalizeSceneName(name);
        if (!safeSceneFile(sceneName)) throw makeError("invalid scene name", 400);

        const localMap = loadLocalSceneMap();
        let serverNames = [];
        try {
          serverNames = await serverApi.getScenes();
        } catch (_) {
          serverNames = [];
        }
        const exists = Object.prototype.hasOwnProperty.call(localMap, sceneName)
          || serverNames.indexOf(sceneName) >= 0;

        if (exists && !overwrite) {
          throw makeError("scene already exists", 409);
        }

        localMap[sceneName] = source || "";
        saveLocalSceneMap(localMap);
        sceneCache.set(sceneName, source || "");
        pushLog("info", `scene saved locally: ${sceneName}`);
        return sceneName;
      },
    };
  };
}());
