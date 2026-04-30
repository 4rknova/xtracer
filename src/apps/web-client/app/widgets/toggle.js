(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createToggle(options) {
    const opts = options || {};
    const input = dom.el("input", {
      attrs: { type: "checkbox", id: opts.id || null },
      props: { checked: !!opts.checked, disabled: !!opts.disabled },
    });
    if (typeof opts.onChange === "function") input.addEventListener("change", opts.onChange);
    return dom.el("label", {
      className: `xui-switch${opts.className ? ` ${opts.className}` : ""}`,
      attrs: opts.title ? { title: opts.title } : {},
      children: [
        opts.label ? dom.el("span", { className: "xui-switch__label", text: opts.label }) : null,
        input,
      ],
    });
  }

  widgets.createToggle = createToggle;
})(window);
