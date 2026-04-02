(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createProgressBar(options) {
    const opts = options || {};
    const value = Math.max(0, Math.min(1, Number(opts.value) || 0));
    const fill = dom.el("span", {
      className: "xui-progress__fill",
      attrs: { style: `width: ${(value * 100).toFixed(1)}%` },
    });
    return dom.el("div", {
      className: `xui-progress${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        opts.label ? dom.el("div", { className: "xui-stat", children: [dom.el("span", { className: "xui-stat__label", text: opts.label }), dom.el("span", { className: "xui-stat__value", text: opts.valueText || `${Math.round(value * 100)}%` })] }) : null,
        dom.el("div", { className: "xui-progress__track", children: fill }),
      ],
    });
  }

  widgets.createProgressBar = createProgressBar;
})(window);
