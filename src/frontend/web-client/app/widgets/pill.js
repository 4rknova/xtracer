(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createPill(options) {
    const opts = options || {};
    const isActive = !!opts.active;
    const node = dom.el("button", {
      className: `xui-pill${isActive ? " is-active" : ""}${opts.className ? ` ${opts.className}` : ""}`,
      attrs: {
        type: opts.type || "button",
        "aria-pressed": opts.pressable ? (isActive ? "true" : "false") : null,
        "aria-selected": opts.selectable ? (isActive ? "true" : "false") : null,
        "aria-disabled": opts.disabled ? "true" : null,
      },
      props: {
        disabled: !!opts.disabled,
      },
      children: opts.children !== undefined ? opts.children : opts.label,
    });
    if (typeof opts.onClick === "function") node.addEventListener("click", opts.onClick);
    return node;
  }

  widgets.createPill = createPill;
})(window);
