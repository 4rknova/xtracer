// Gallery — fetches and displays cached renders from /api/gallery

let galleryEntries = [];
let galleryDetailId = null;

function formatElapsed(ms) {
  if (!ms || ms <= 0) return "-";
  const s = ms / 1000;
  if (s < 60) return s.toFixed(1) + "s";
  const m = Math.floor(s / 60);
  const rem = (s - m * 60).toFixed(0).padStart(2, "0");
  return `${m}m ${rem}s`;
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
    if (entry.elapsed_ms) parts.push(formatElapsed(entry.elapsed_ms));
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
    img.src = galleryThumbUrl(entry.id);
    img.alt = entry.scene || entry.id;
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
      ["Render time", formatElapsed(entry.elapsed_ms)],
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
          if (img) img.src = galleryPassThumbUrl(entry.id, i);
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
});
