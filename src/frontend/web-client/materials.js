'use strict';

// ── Catalog (populated from /api/materials) ───────────────────────────────────

let MATERIALS = [];
let addToSceneRequestSeq = 0;
const pendingAddToSceneRequests = new Map();

async function loadCatalog() {
  const res = await fetch('/api/materials');
  if (!res.ok) throw new Error(`material catalog fetch failed (${res.status})`);
  const data = await res.json();
  MATERIALS = (data.materials || []).map(m => ({
    id: m.id,
    name: m.name,
    category: m.category,
    description: m.description,
    previewColor: m.preview_color,
    ncf: m.ncf,
  }));
}

// ── Category display order ────────────────────────────────────────────────────

const CATEGORY_ORDER = ['Metal', 'Dielectric', 'Principled', 'Diffuse', 'Procedural', 'Subsurface', 'Emissive'];

function materialThumbnailUrl(id) {
  return `/res/lib/materials/${encodeURIComponent(id)}.png`;
}

// ── API helpers ───────────────────────────────────────────────────────────────

async function apiPost(path, params) {
  const body = new URLSearchParams(params);
  const res = await fetch(path, { method: 'POST', body });
  if (!res.ok) {
    const text = await res.text().catch(() => '');
    throw new Error(`${path} failed (${res.status}): ${text}`);
  }
  return res.json();
}

async function submitRender(materialId, width, height, samples) {
  return apiPost(`/api/materials/${encodeURIComponent(materialId)}/preview`, {
    width,
    height,
    samples,
    tile_size: '32',
    tm: 'aces',
  });
}

async function pollJob(jobId, onProgress) {
  for (;;) {
    await new Promise(r => setTimeout(r, 600));
    const res = await fetch(`/api/jobs/${jobId}`);
    if (!res.ok) throw new Error('job poll failed');
    const snap = await res.json();
    const state = String(snap.state || '').toLowerCase();
    if (onProgress) onProgress(snap.progress || 0, snap.state);
    if (state === 'done') return snap;
    if (state === 'error' || state === 'aborted') {
      throw new Error(`render ${state}: ${snap.error || ''}`);
    }
  }
}

// ── Render queue ──────────────────────────────────────────────────────────────

const renderQueue = [];
let activeRenders = 0;
const MAX_CONCURRENT = 1;

function enqueueRender(task) {
  renderQueue.push(task);
  drainQueue();
}

function drainQueue() {
  if (activeRenders >= MAX_CONCURRENT || renderQueue.length === 0) return;
  const task = renderQueue.shift();
  activeRenders++;
  task().finally(() => {
    activeRenders--;
    drainQueue();
  });
}

// ── Raw RGBA → blob URL ───────────────────────────────────────────────────────

async function fetchJobImageUrl(jobId) {
  const res = await fetch(`/api/jobs/${jobId}/image`);
  if (!res.ok) throw new Error(`image fetch failed (${res.status})`);
  const width  = parseInt(res.headers.get('X-XTracer-Width')  || '0', 10);
  const height = parseInt(res.headers.get('X-XTracer-Height') || '0', 10);
  if (!width || !height) throw new Error('invalid image dimensions');
  const buf = await res.arrayBuffer();
  const canvas = document.createElement('canvas');
  canvas.width = width;
  canvas.height = height;
  const ctx = canvas.getContext('2d');
  ctx.putImageData(new ImageData(new Uint8ClampedArray(buf), width, height), 0, 0);
  return new Promise((resolve, reject) => {
    canvas.toBlob(blob => blob ? resolve(URL.createObjectURL(blob)) : reject(new Error('toBlob failed')), 'image/png');
  });
}

// ── Detail render cache ───────────────────────────────────────────────────────

const previewImageCache = new Map(); // render key → blob URL
const previewRenderInflight = new Map(); // render key → Promise<blob URL>
let detailRenderToken = 0;

function previewRenderKey(mat, width, height, samples) {
  return `${mat.id}:${width}x${height}:${samples}`;
}

function queuePreviewRender(mat, width, height, samples, onProgress, force = false) {
  const key = previewRenderKey(mat, width, height, samples);
  if (force) {
    previewRenderInflight.delete(key);
    previewImageCache.delete(key);
  }
  if (previewImageCache.has(key)) {
    return Promise.resolve(previewImageCache.get(key));
  }
  if (previewRenderInflight.has(key)) {
    return previewRenderInflight.get(key);
  }

  const promise = new Promise((resolve, reject) => {
    enqueueRender(async () => {
      try {
        const { job_id } = await submitRender(mat.id, width, height, samples);
        await pollJob(job_id, onProgress || null);
        const url = await fetchJobImageUrl(job_id);
        previewImageCache.set(key, url);
        resolve(url);
      } catch (e) {
        reject(e);
      } finally {
        previewRenderInflight.delete(key);
      }
    });
  });

  previewRenderInflight.set(key, promise);
  return promise;
}

// ── Detail render ─────────────────────────────────────────────────────────────

let selectedId = null;
let detailQuality = 'fast';

function detailWidth()   { return detailQuality === 'hq' ? 512 : 320; }
function detailHeight()  { return detailQuality === 'hq' ? 512 : 320; }
function detailSamples() { return detailQuality === 'hq' ? 128 : 48; }

const el = {};

function isEmbeddedInEditor() {
  return !!(window.parent && window.parent !== window);
}

function setButtonBusy(btn, busy, label) {
  if (!btn) return;
  btn.disabled = !!busy;
  if (label) btn.textContent = label;
}

function flashButtonLabel(btn, label, restoreLabel, timeout = 1600) {
  if (!btn) return;
  btn.textContent = label;
  window.setTimeout(() => {
    btn.textContent = restoreLabel;
  }, timeout);
}

function bindAddToSceneMessaging() {
  const btn = document.getElementById('addToSceneBtn');
  if (!btn) return;
  if (!isEmbeddedInEditor()) {
    btn.disabled = true;
    btn.title = 'Available when opened inside the scene editor';
    return;
  }

  window.addEventListener('message', (ev) => {
    const data = ev && ev.data;
    if (!data || data.type !== 'xtracer-material-add-to-scene-result') return;
    if (ev.origin && ev.origin !== window.location.origin) return;
    const pending = pendingAddToSceneRequests.get(String(data.requestId || ''));
    if (!pending) return;
    pendingAddToSceneRequests.delete(String(data.requestId || ''));
    window.clearTimeout(pending.timeoutId);
    if (data.ok) {
      btn.title = '';
      setButtonBusy(btn, false, 'Add to Current Scene');
      flashButtonLabel(btn, 'Added', 'Add to Current Scene');
    } else {
      setButtonBusy(btn, false, 'Add to Current Scene');
      flashButtonLabel(btn, 'Add Failed', 'Add to Current Scene', 2200);
      btn.title = String(data.error || 'Failed to add material');
    }
  });
}

async function renderDetail(mat, force = false) {
  const renderToken = ++detailRenderToken;
  el.previewEmpty.hidden = true;
  el.previewImage.hidden = true;
  el.previewSpinner.hidden = false;
  el.previewSpinnerLabel.textContent = 'Rendering…';
  el.previewProgressBar.style.width = '0%';
  el.previewProgress.hidden = false;
  el.previewMeta.hidden = false;
  el.previewName.textContent = mat.name;
  el.previewCategory.textContent = mat.category;
  el.previewDesc.textContent = mat.description;

  try {
    const url = await queuePreviewRender(mat, detailWidth(), detailHeight(), detailSamples(), (progress, state) => {
      if (renderToken !== detailRenderToken) return;
      const pct = Math.round(progress * 100);
      el.previewProgressBar.style.width = `${pct}%`;
      el.previewSpinnerLabel.textContent = state && state.toLowerCase() === 'preparing' ? 'Preparing…' : `${pct}%`;
    }, force);

    if (renderToken !== detailRenderToken) return;
    el.previewImage.src = url;
    el.previewImage.hidden = false;
    el.previewSpinner.hidden = true;
    el.previewProgress.hidden = true;
  } catch (e) {
    if (renderToken !== detailRenderToken) return;
    el.previewSpinner.hidden = true;
    el.previewProgress.hidden = true;
    el.previewEmpty.textContent = `Render failed: ${e.message || e}`;
    el.previewEmpty.hidden = false;
  }
}

// ── Sidebar rendering ─────────────────────────────────────────────────────────

const cardEls = new Map(); // id → card element

function buildSidebar(filter) {
  const sidebar = document.getElementById('sidebar');
  sidebar.innerHTML = '';

  const byCategory = new Map();
  for (const cat of CATEGORY_ORDER) byCategory.set(cat, []);

  const q = (filter || '').trim().toLowerCase();
  for (const mat of MATERIALS) {
    if (q && !mat.name.toLowerCase().includes(q) && !mat.category.toLowerCase().includes(q)) continue;
    if (!byCategory.has(mat.category)) byCategory.set(mat.category, []);
    byCategory.get(mat.category).push(mat);
  }

  cardEls.clear();

  let firstVisible = true;
  for (const cat of [...CATEGORY_ORDER, ...[...byCategory.keys()].filter(k => !CATEGORY_ORDER.includes(k))]) {
    const mats = byCategory.get(cat) || [];
    if (mats.length === 0) continue;

    const group = document.createElement('div');
    group.className = 'sp-group';

    const title = document.createElement('div');
    title.className = 'sp-group-title';
    title.textContent = cat;
    group.appendChild(title);

    const grid = document.createElement('div');
    grid.className = 'mat-grid';
    group.appendChild(grid);

    for (const mat of mats) {
      const card = document.createElement('div');
      card.className = 'mat-card sp-thumb';
      if (mat.id === selectedId) card.classList.add('selected');
      card.dataset.id = mat.id;
      card.title = mat.name;

      const thumb = document.createElement('div');
      thumb.className = 'mat-thumb-wrap';
      thumb.style.background = mat.previewColor;

      const img = document.createElement('img');
      img.className = 'mat-thumb-img is-pending';
      img.alt = mat.name;
      img.loading = 'lazy';
      img.decoding = 'async';
      img.addEventListener('error', () => {
        img.classList.add('is-pending');
      }, { once: true });
      img.addEventListener('load', () => {
        img.classList.remove('is-pending');
      }, { once: true });
      img.src = materialThumbnailUrl(mat.id);

      thumb.appendChild(img);

      const name = document.createElement('div');
      name.className = 'sp-thumb-name mat-card-name';
      name.textContent = mat.name;

      card.appendChild(thumb);
      card.appendChild(name);
      grid.appendChild(card);
      cardEls.set(mat.id, card);

      card.addEventListener('click', () => selectMaterial(mat));

      // Auto-select the first visible material on initial load
      if (firstVisible && !selectedId) {
        firstVisible = false;
        requestAnimationFrame(() => selectMaterial(mat));
      }
    }

    sidebar.appendChild(group);
  }
}

function selectMaterial(mat) {
  if (selectedId === mat.id) return;
  if (selectedId) {
    const prev = cardEls.get(selectedId);
    if (prev) prev.classList.remove('selected');
  }
  selectedId = mat.id;
  const card = cardEls.get(mat.id);
  if (card) card.classList.add('selected');
  renderDetail(mat);
}

// ── NCF modal ─────────────────────────────────────────────────────────────────

function openNcfModal(mat) {
  const modal = document.getElementById('ncfModal');
  const nameInput = document.getElementById('ncfNameInput');
  const codeEl = document.getElementById('ncfCode');

  nameInput.value = mat.id;

  function refresh() {
    const name = (nameInput.value.trim() || mat.id).replace(/\s+/g, '_');
    codeEl.textContent = `    ${name} = {\n        ${mat.ncf.trim()}\n    }`;
  }

  refresh();
  nameInput.oninput = refresh;

  document.getElementById('ncfCopyBtn').onclick = () => {
    navigator.clipboard.writeText(codeEl.textContent).catch(() => {});
    const btn = document.getElementById('ncfCopyBtn');
    btn.textContent = 'Copied!';
    setTimeout(() => { btn.textContent = 'Copy'; }, 1500);
  };

  document.getElementById('ncfCloseBtn').onclick = () => { modal.hidden = true; };
  modal.querySelector('.sp-modal-backdrop').onclick = () => { modal.hidden = true; };
  modal.hidden = false;
}

// ── Boot ──────────────────────────────────────────────────────────────────────

function bindElements() {
  el.previewEmpty        = document.getElementById('previewEmpty');
  el.previewImage        = document.getElementById('previewImage');
  el.previewSpinner      = document.getElementById('previewSpinner');
  el.previewSpinnerLabel = document.getElementById('previewSpinnerLabel');
  el.previewProgress     = document.getElementById('previewProgress');
  el.previewProgressBar  = document.getElementById('previewProgressBar');
  el.previewMeta         = document.getElementById('previewMeta');
  el.previewName         = document.getElementById('previewName');
  el.previewCategory     = document.getElementById('previewCategory');
  el.previewDesc         = document.getElementById('previewDesc');
  el.addToSceneBtn       = document.getElementById('addToSceneBtn');
}

function showLoadError(msg) {
  const sidebar = document.getElementById('sidebar');
  sidebar.innerHTML = `<p style="padding:var(--xui-space-4);color:var(--muted);font-size:0.75rem">${msg}</p>`;
}

document.addEventListener('DOMContentLoaded', async () => {
  bindElements();

  try {
    await loadCatalog();
  } catch (e) {
    showLoadError(`Could not load material catalog: ${e.message}`);
    return;
  }

  buildSidebar('');

  document.getElementById('searchInput').addEventListener('input', e => {
    buildSidebar(e.target.value);
  });

  document.querySelectorAll('.sp-size-chips [data-quality]').forEach(btn => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('.sp-size-chips [data-quality]').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      detailQuality = btn.dataset.quality;
      const mat = MATERIALS.find(m => m.id === selectedId);
      if (mat) renderDetail(mat);
    });
  });

  document.getElementById('ncfBtn').addEventListener('click', () => {
    const mat = MATERIALS.find(m => m.id === selectedId);
    if (mat) openNcfModal(mat);
  });

  bindAddToSceneMessaging();
  if (el.addToSceneBtn) {
    el.addToSceneBtn.addEventListener('click', () => {
      const mat = MATERIALS.find(m => m.id === selectedId);
      if (!mat) return;
      if (!isEmbeddedInEditor()) {
        flashButtonLabel(el.addToSceneBtn, 'Open In Editor', 'Add to Current Scene', 2200);
        return;
      }
      const requestId = `material_add_${Date.now()}_${++addToSceneRequestSeq}`;
      el.addToSceneBtn.title = '';
      setButtonBusy(el.addToSceneBtn, true, 'Adding...');
      const timeoutId = window.setTimeout(() => {
        pendingAddToSceneRequests.delete(requestId);
        setButtonBusy(el.addToSceneBtn, false, 'Add to Current Scene');
        flashButtonLabel(el.addToSceneBtn, 'Timed Out', 'Add to Current Scene', 2200);
      }, 8000);
      pendingAddToSceneRequests.set(requestId, { timeoutId });
      window.parent.postMessage({
        type: 'xtracer-material-add-to-scene',
        requestId,
        materialId: mat.id,
        ncf: mat.ncf,
      }, window.location.origin);
    });
  }

  document.getElementById('reRenderBtn').addEventListener('click', () => {
    const mat = MATERIALS.find(m => m.id === selectedId);
    if (mat) renderDetail(mat, true);
  });
});
