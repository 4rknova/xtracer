(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createTag(options) {
    const opts = options || {};
    const tone = opts.tone || "neutral";
    return dom.el("span", {
      className: `xui-tag xui-tag--${tone}${opts.className ? ` ${opts.className}` : ""}`,
      text: opts.text || opts.label || "",
    });
  }

  widgets.createTag = createTag;
})(window);
