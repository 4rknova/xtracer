(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createStack(options) {
    const opts = options || {};
    const density = opts.density || "default";
    return dom.el(opts.tagName || "div", {
      className: `xui-stack xui-stack--${density}${opts.className ? ` ${opts.className}` : ""}`,
      children: opts.children || null,
    });
  }

  function createSurface(options) {
    const opts = options || {};
    const head = (opts.title || opts.note || opts.actions)
      ? dom.el("div", {
          className: "xui-surface__head",
          children: [
            dom.el("div", {
              className: "xui-surface__meta",
              children: [
                opts.title ? dom.el("h3", { className: "xui-surface__title", text: opts.title }) : null,
                opts.note ? dom.el("p", { className: "xui-surface__note", text: opts.note }) : null,
              ],
            }),
            opts.actions ? dom.el("div", { className: "xui-surface__actions", children: opts.actions }) : null,
          ],
        })
      : null;

    return dom.el(opts.tagName || "section", {
      className: `xui-surface${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        head,
        dom.el("div", {
          className: "xui-surface__body",
          children: opts.body || opts.children || null,
        }),
      ],
    });
  }

  widgets.createStack = createStack;
  widgets.createSurface = createSurface;
})(window);
