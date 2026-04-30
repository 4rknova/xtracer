(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  const SAMPLING_MODES = [
    {
      mode: "smooth",
      title: "Smooth sampling",
      path: "M12 4a8 8 0 1 0 8 8 8 8 0 0 0-8-8zm0 4a4 4 0 1 1-4 4 4 4 0 0 1 4-4z",
    },
    {
      mode: "nearest",
      title: "Nearest sampling",
      path: "M5 5h5v5H5zm0 9h5v5H5zm9-9h5v5h-5zm0 9h5v5h-5z",
    },
  ];

  // Creates a sampling mode toggle switch (smooth / nearest).
  // opts.value    – initial active mode ("smooth" | "nearest", default "smooth")
  // opts.onChange – function(mode) called when the user selects a mode
  // opts.label    – aria-label for the group element (default "Preview sampling")
  function createSamplingSwitch(options) {
    const opts = options || {};
    const value = opts.value === "nearest" ? "nearest" : "smooth";

    const node = dom.el("div", {
      className: "render-preview-sampling-switch",
      attrs: {
        role: "group",
        "aria-label": opts.label || "Preview sampling",
      },
      dataset: { value },
    });

    SAMPLING_MODES.forEach((item) => {
      const active = item.mode === value;
      const btn = dom.el("button", {
        className: `render-preview-sampling-btn${active ? " is-active" : ""}`,
        attrs: {
          type: "button",
          "aria-pressed": active ? "true" : "false",
          "aria-label": item.title,
          title: item.title,
        },
        dataset: { previewSampling: item.mode },
        children: dom.svgIcon(item.path, "0 0 24 24"),
      });
      btn.addEventListener("click", () => {
        if (typeof opts.onChange === "function") opts.onChange(item.mode);
      });
      btn.addEventListener("keydown", (evt) => {
        if (evt.key !== "ArrowLeft" && evt.key !== "ArrowRight") return;
        evt.preventDefault();
        const next = evt.key === "ArrowLeft" ? "smooth" : "nearest";
        if (typeof opts.onChange === "function") opts.onChange(next);
      });
      node.appendChild(btn);
    });

    return node;
  }

  // Updates the active state of an existing sampling switch container.
  // Works on any element that contains .render-preview-sampling-btn children.
  function syncSamplingSwitch(node, value) {
    if (!node) return;
    const v = value === "nearest" ? "nearest" : "smooth";
    node.dataset.value = v;
    node.querySelectorAll(".render-preview-sampling-btn").forEach((btn) => {
      const active = btn.dataset.previewSampling === v;
      btn.classList.toggle("is-active", active);
      btn.setAttribute("aria-pressed", active ? "true" : "false");
    });
  }

  widgets.createSamplingSwitch = createSamplingSwitch;
  widgets.syncSamplingSwitch = syncSamplingSwitch;
})(window);
