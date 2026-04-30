(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createEmptyState(options) {
    const opts = options || {};
    return dom.el("section", {
      className: `xui-empty-state${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        dom.el("h3", { className: "xui-empty-state__title", text: opts.title || "Nothing to show" }),
        dom.el("p", { className: "xui-empty-state__message", text: opts.message || "" }),
        opts.actions || null,
      ],
    });
  }

  widgets.createEmptyState = createEmptyState;
})(window);
