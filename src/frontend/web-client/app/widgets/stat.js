(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createStatHint(options) {
    const opts = options || {};
    return dom.el(opts.tagName || "p", {
      className: `xui-stat${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        dom.el("span", { className: "xui-stat__label", text: opts.label || "" }),
        dom.el(opts.valueTagName || "code", { className: "xui-stat__value", text: opts.value !== undefined ? opts.value : "-" }),
      ],
    });
  }

  function renderStatHint(target, options) {
    if (!target) return null;
    const opts = options || {};
    target.classList.add("xui-stat");
    if (opts.className) {
      String(opts.className).split(/\s+/).filter(Boolean).forEach((name) => target.classList.add(name));
    }
    return dom.mount(target, [
      dom.el("span", { className: "xui-stat__label", text: opts.label || "" }),
      dom.el(opts.valueTagName || "code", {
        className: "xui-stat__value",
        text: opts.value !== undefined ? opts.value : "-",
      }),
    ]);
  }

  widgets.createStatHint = createStatHint;
  widgets.renderStatHint = renderStatHint;
})(window);
