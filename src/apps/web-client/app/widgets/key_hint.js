(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createKeyHint(options) {
    const opts = options || {};
    const keys = Array.isArray(opts.keys) ? opts.keys : [opts.key || ""];
    return dom.el("span", {
      className: `xui-key-hint${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        keys.filter(Boolean).map((key) => dom.el("kbd", { text: key })),
        opts.label ? dom.el("span", { text: opts.label }) : null,
      ],
    });
  }

  widgets.createKeyHint = createKeyHint;
})(window);
