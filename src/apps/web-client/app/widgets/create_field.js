(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createCreateField(options) {
    const opts = options || {};
    const inputId = opts.inputId || null;
    const buttonId = opts.buttonId || null;
    const icon = opts.icon || dom.el("span", {
      className: "xui-create-field__plus",
      attrs: { "aria-hidden": "true" },
      text: "+",
    });

    return dom.el("div", {
      className: `xui-create-field${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        dom.el("input", {
          className: "xui-create-field__input xui-input",
          attrs: {
            id: inputId,
            type: opts.type || "text",
            maxlength: opts.maxlength || null,
            placeholder: opts.placeholder || null,
            "aria-label": opts.inputLabel || opts.placeholder || "Create name",
          },
        }),
        dom.el("button", {
          className: "xui-create-field__action",
          attrs: {
            id: buttonId,
            type: "button",
            title: opts.buttonTitle || "Create",
            "aria-label": opts.buttonLabel || opts.buttonTitle || "Create",
          },
          children: [
            icon,
            dom.el("span", {
              className: "sr-only",
              text: opts.buttonLabel || opts.buttonTitle || "Create",
            }),
          ],
        }),
      ],
    });
  }

  function renderCreateField(target, options) {
    if (!(target instanceof HTMLElement) || !target.parentNode) return null;
    const field = createCreateField(options);
    target.parentNode.replaceChild(field, target);
    return field;
  }

  widgets.createCreateField = createCreateField;
  widgets.renderCreateField = renderCreateField;
})(window);
