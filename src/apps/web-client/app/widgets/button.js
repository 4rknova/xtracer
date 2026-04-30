(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createButton(options) {
    const opts = options || {};
    const variant = opts.variant || "secondary";
    const node = dom.el("button", {
      className: `xui-button xui-button--${variant}${opts.busy ? " is-busy" : ""}${opts.className ? ` ${opts.className}` : ""}`,
      attrs: {
        type: opts.type || "button",
        "aria-disabled": opts.disabled ? "true" : null,
      },
      props: {
        disabled: !!opts.disabled,
      },
      children: opts.children !== undefined ? opts.children : opts.label,
    });
    if (opts.title) node.title = opts.title;
    if (typeof opts.onClick === "function") node.addEventListener("click", opts.onClick);
    return node;
  }

  function createIconButton(options) {
    const opts = options || {};
    const variant = opts.variant || "ghost";
    const node = dom.el("button", {
      className: `xui-icon-button xui-icon-button--${variant}${opts.className ? ` ${opts.className}` : ""}`,
      attrs: {
        type: opts.type || "button",
        "aria-label": opts.label || opts.title || "Icon button",
        title: opts.title || null,
        "aria-disabled": opts.disabled ? "true" : null,
      },
      props: {
        disabled: !!opts.disabled,
      },
      children: opts.icon || opts.children || null,
    });
    if (typeof opts.onClick === "function") node.addEventListener("click", opts.onClick);
    return node;
  }

  widgets.createButton = createButton;
  widgets.createIconButton = createIconButton;
})(window);
