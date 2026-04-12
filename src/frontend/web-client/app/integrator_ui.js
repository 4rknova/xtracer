function selectedExportFormat() {
  const controls = (typeof el !== "undefined" && el) ? el : null;
  const raw = String(controls && controls.exportFormat && controls.exportFormat.value ? controls.exportFormat.value : "png").toLowerCase();
  if (raw === "png" || raw === "jpg" || raw === "bmp" || raw === "tga" || raw === "exr" || raw === "hdr") return raw;
  return "png";
}

function updateDownloadUi() {
  const hasExportApi = hasBackendMethod(api, "getJobExport");
  const hasCompletedJob = !!lastCompletedJobId;
  const fmt = selectedExportFormat().toUpperCase();
  const busy = !!exportRequestInFlight;
  const enabled = hasExportApi && hasCompletedJob && !renderActive && !busy;
  const shell = el.download ? el.download.closest(".render-toolbar-save-shell") : null;
  syncRenderExportButtonDecor(el.download, fmt, busy ? "Saving" : "Save");
  if (shell) {
    shell.classList.toggle("is-ready", enabled);
    shell.classList.toggle("is-busy", busy);
    shell.classList.toggle("is-disabled", !enabled);
    shell.dataset.exportFormat = fmt;
  }
  el.download.classList.toggle("is-ready", enabled);
  el.download.classList.toggle("is-busy", busy);
  if (enabled) {
    el.download.disabled = false;
    el.download.setAttribute("aria-disabled", "false");
    el.download.classList.remove("is-disabled");
    el.download.setAttribute("title", `Save ${fmt} export`);
    el.download.setAttribute("aria-label", `Save ${fmt} export`);
  } else {
    el.download.disabled = true;
    el.download.setAttribute("aria-disabled", "true");
    el.download.classList.add("is-disabled");
    if (busy) {
      el.download.setAttribute("title", `Saving ${fmt} export`);
      el.download.setAttribute("aria-label", `Saving ${fmt} export`);
    } else if (!hasExportApi) {
      el.download.setAttribute("title", "Save unavailable on this backend");
      el.download.setAttribute("aria-label", "Save unavailable on this backend");
    } else if (renderActive) {
      el.download.setAttribute("title", `Wait for the current render to finish before saving ${fmt}`);
      el.download.setAttribute("aria-label", `Wait for the current render to finish before saving ${fmt}`);
    } else if (!hasCompletedJob) {
      el.download.setAttribute("title", `Complete a render to save ${fmt}`);
      el.download.setAttribute("aria-label", `Complete a render to save ${fmt}`);
    } else {
      el.download.setAttribute("title", `Save ${fmt} export`);
      el.download.setAttribute("aria-label", `Save ${fmt} export`);
    }
  }
}

function addOption(select, value, label) {
  const opt = document.createElement("option");
  opt.value = value;
  opt.textContent = label || value;
  select.appendChild(opt);
}

function parseIntegratorNumber(raw, fallback) {
  const n = Number(raw);
  return Number.isFinite(n) ? n : fallback;
}

function getIntegratorControlValue(state, ctrl) {
  const id = ctrl && ctrl.id ? ctrl.id : "";
  if (!id) return "";
  if (Object.prototype.hasOwnProperty.call(state, id)) return String(state[id] ?? "");
  if (ctrl.default !== undefined && ctrl.default !== null) return String(ctrl.default);
  return "";
}

function isIntegratorControlVisible(ctrl, state) {
  const vw = ctrl && ctrl.visible_when ? ctrl.visible_when : null;
  if (!vw || !vw.id) return true;
  return getIntegratorControlValue(state, { id: vw.id, default: "" }) === String(vw.value ?? "");
}

function renderIntegratorControls() {
  const selected = el.integrator.value || "";
  const info = integratorById.get(selected) || null;
  const controls = info && Array.isArray(info.controls) ? info.controls : [];
  const dom = window.XTracerWidgets.dom;

  el.integratorControls.innerHTML = "";
  if (!controls.length) {
    el.integratorControlsSection.hidden = true;
    return;
  }

  el.integratorControlsSection.hidden = false;
  const saved = { ...(integratorControlState.get(selected) || {}) };

  controls.forEach((ctrl) => {
    const id = ctrl.id || "";
    if (!id) return;
    if (!Object.prototype.hasOwnProperty.call(saved, id) && ctrl.default !== undefined) {
      saved[id] = String(ctrl.default);
    }
  });
  integratorControlState.set(selected, saved);

  controls.forEach((ctrl) => {
    const id = ctrl.id || "";
    if (!id) return;
    if (!isIntegratorControlVisible(ctrl, saved)) return;

    let input = null;
    if (ctrl.type === "enum") {
      input = dom.el("select", {
        className: "xui-select",
        dataset: { ioptId: id, ioptType: "enum" },
        children: (Array.isArray(ctrl.options) ? ctrl.options : []).map((opt) =>
          dom.el("option", { attrs: { value: String(opt.value ?? "") }, text: opt.label || opt.value || "" })
        ),
      });
    } else if (ctrl.type === "bool") {
      input = dom.el("select", {
        className: "xui-select",
        dataset: { ioptId: id, ioptType: "bool" },
        children: [
          dom.el("option", { attrs: { value: "false" }, text: "False" }),
          dom.el("option", { attrs: { value: "true" }, text: "True" }),
        ],
      });
    } else {
      input = dom.el("input", {
        className: "xui-input",
        attrs: {
          type: "number",
          step: ctrl.type === "int" ? (ctrl.step || "1") : (ctrl.step || "0.01"),
          min: (ctrl.min !== undefined && ctrl.min !== null && ctrl.min !== "") ? String(ctrl.min) : undefined,
          max: (ctrl.max !== undefined && ctrl.max !== null && ctrl.max !== "") ? String(ctrl.max) : undefined,
        },
        dataset: { ioptId: id, ioptType: ctrl.type || "string" },
      });
    }

    input.value = String(getIntegratorControlValue(saved, ctrl));
    input.addEventListener("change", () => {
      const curr = integratorControlState.get(selected) || {};
      curr[id] = input.value;
      integratorControlState.set(selected, curr);
      renderIntegratorControls();
      queueWorkspaceSettingsSave();
    });

    el.integratorControls.appendChild(window.XTracerWidgets.createField({
      label: ctrl.label || id,
      help: ctrl.description || "",
      control: input,
      className: "integrator-control",
    }));
  });
}

function gatherIntegratorOptionParams() {
  const out = {};
  const selected = el.integrator.value || "";
  const nodes = el.integratorControls.querySelectorAll("[data-iopt-id]");
  const save = integratorControlState.get(selected) || {};

  nodes.forEach((node) => {
    const id = node.dataset.ioptId || "";
    if (!id) return;
    const type = node.dataset.ioptType || "string";
    const raw = String(node.value ?? "").trim();
    if (!raw) return;

    let normalized = raw;
    if (type === "int") normalized = String(Math.round(parseIntegratorNumber(raw, 0)));
    else if (type === "float") normalized = String(parseIntegratorNumber(raw, 0));
    else if (type === "bool") normalized = (raw === "1" || raw === "true") ? "true" : "false";

    save[id] = normalized;
    out[`iopt.${id}`] = normalized;
  });

  integratorControlState.set(selected, save);
  return out;
}
