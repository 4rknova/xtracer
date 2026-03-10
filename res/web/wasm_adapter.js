(function () {
  "use strict";

  const LOCAL_SCENES_KEY = "xtracer-wasm-local-scenes-v1";
  const DEFAULT_INTEGRATORS = [
    { id: "pathtracer", label: "Pathtracer (Brute Force)" },
    { id: "pathtracer_is", label: "Pathtracer (IS)" },
    { id: "photon_mapping", label: "Photon Mapping" },
    { id: "depth", label: "Depth" },
    { id: "stencil", label: "Stencil" },
    { id: "normal", label: "Normal" },
    { id: "uv", label: "UV" },
    { id: "emission", label: "Emission" },
    { id: "ao", label: "Ambient Occlusion" },
  ];
  const DEFAULT_RESOLUTIONS = [
    { id: 0, description: "VGA/SD", width: 640, height: 480 },
    { id: 1, description: "HD", width: 1280, height: 720 },
    { id: 2, description: "FHD", width: 1920, height: 1080 },
    { id: 3, description: "QHD/WQHD", width: 2560, height: 1440 },
    { id: 4, description: "4K UHD", width: 3840, height: 2160 },
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
    let staticScenes = null;

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

    async function loadStaticSceneIndex() {
      if (staticScenes) return staticScenes;
      try {
        const data = await fetchJson("/scenes/index.json");
        staticScenes = mergeUniqueSorted(data.scenes || []);
      } catch (err) {
        pushLog("warn", `static scene index unavailable (${err.message})`);
        staticScenes = [];
      }
      return staticScenes;
    }

    async function loadIntegrators() {
      try {
        const data = await fetchJson("/integrators.json");
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

    async function loadResolutions() {
      try {
        const data = await fetchJson("/resolutions.json");
        const list = Array.isArray(data.presets) ? data.presets : [];
        if (list.length > 0) return list;
      } catch (_) {
        // fallback below
      }
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
        const staticNames = await loadStaticSceneIndex();

        let serverNames = [];
        try {
          serverNames = await serverApi.getScenes();
        } catch (_) {
          serverNames = [];
        }
        const merged = mergeUniqueSorted([].concat(staticNames, localNames, serverNames));
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
      async getCameras(scene) {
        if (!scene) return [];
        try {
          const data = await getSceneSourceHybrid(scene);
          const cameras = extractCameraNames(data.source || "");
          if (cameras.length > 0) return cameras;
        } catch (_) {
          // fallback below
        }

        try {
          return await serverApi.getCameras(scene);
        } catch (_) {
          return [];
        }
      },
      async getIntegrators() {
        return loadIntegrators();
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
          });
        } catch (_) {
          let licenseText = "BSD 3-Clause";
          const currentYear = Math.max(2010, new Date().getFullYear());
          try {
            licenseText = await fetchText("/license.txt");
          } catch (_) {
            // Keep short fallback text if static license file is missing.
          }
          return {
            name: "XTRACER WEB",
            version: "",
            author_name: "Nikos Papadopoulos",
            author_email: "nikpapas@gmail.com",
            homepage: "https://www.4rknova.com",
            website: "https://github.com/4rknova/xtracer",
            copyright: `Copyright 2010-${currentYear} (c) Nikos Papadopoulos`,
            license: licenseText,
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
      async saveScene(name, source, overwrite) {
        const sceneName = normalizeSceneName(name);
        if (!safeSceneFile(sceneName)) throw makeError("invalid scene name", 400);

        const localMap = loadLocalSceneMap();
        const staticNames = await loadStaticSceneIndex();
        const exists = Object.prototype.hasOwnProperty.call(localMap, sceneName)
          || staticNames.indexOf(sceneName) >= 0;

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
