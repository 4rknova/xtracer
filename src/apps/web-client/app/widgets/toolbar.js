(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createToolbar(options) {
    const opts = options || {};
    return dom.el("div", {
      className: `xui-toolbar${opts.className ? ` ${opts.className}` : ""}`,
      children: opts.children || [],
    });
  }

  widgets.createToolbar = createToolbar;
})(window);
