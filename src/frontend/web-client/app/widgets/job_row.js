(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createJobRow(options) {
    const opts = options || {};
    const state = String(opts.state || "unknown").toLowerCase();
    const item = dom.el("article", {
      className: `settings-job-item state-${state}${opts.className ? ` ${opts.className}` : ""}`,
    });

    const head = dom.el("div", {
      className: "settings-job-head",
      children: [
        dom.el("code", {
          className: "settings-job-id",
          text: opts.id || "-",
        }),
        dom.el("div", {
          className: "settings-job-pills",
          children: opts.pills || null,
        }),
      ],
    });

    const meta = dom.el("div", {
      className: "settings-job-meta",
      children: [
        dom.el("span", { text: opts.workspaceLabel || "-" }),
        dom.el("div", {
          className: "settings-job-metrics",
          children: opts.metrics || null,
        }),
      ],
    });

    const footer = dom.el("div", {
      className: "settings-job-footer",
      children: [
        dom.el("div", {
          className: "settings-job-sub",
          text: opts.subtext || "",
        }),
        (opts.controls && opts.controls.length)
          ? dom.el("div", {
            className: "settings-job-controls",
            children: opts.controls,
          })
          : null,
      ],
    });

    item.appendChild(head);
    if (opts.meta !== false) item.appendChild(meta);
    if (opts.progress) item.appendChild(opts.progress);
    item.appendChild(footer);
    return item;
  }

  widgets.createJobRow = createJobRow;
})(window);
