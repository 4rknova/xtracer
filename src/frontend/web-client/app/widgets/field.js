(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createField(options) {
    const opts = options || {};
    const label = opts.label ? dom.el("span", { className: "xui-field__label", text: opts.label }) : null;
    const help = opts.help ? dom.el("small", { className: "xui-field__help", text: opts.help }) : null;
    const error = opts.error ? dom.el("small", { className: "xui-field__error", text: opts.error }) : null;
    return dom.el("label", {
      className: `xui-field${opts.className ? ` ${opts.className}` : ""}`,
      children: [label, opts.control || null, help, error],
    });
  }

  function createNumberField(options) {
    const opts = options || {};
    const input = dom.el("input", {
      attrs: { type: "number", min: opts.min, max: opts.max, step: opts.step, placeholder: opts.placeholder },
      props: { value: opts.value !== undefined ? opts.value : "" },
    });
    if (typeof opts.onChange === "function") input.addEventListener("change", opts.onChange);
    return createField({ ...opts, control: input });
  }

  function createSelectField(options) {
    const opts = options || {};
    const select = dom.el("select", { className: "xui-select" });
    (opts.options || []).forEach((option) => {
      select.appendChild(dom.el("option", {
        attrs: { value: option.value },
        text: option.label,
        props: { selected: option.value === opts.value },
      }));
    });
    if (typeof opts.onChange === "function") select.addEventListener("change", opts.onChange);
    return createField({ ...opts, control: select });
  }

  function createCheckboxField(options) {
    const opts = options || {};
    const input = dom.el("input", {
      attrs: { type: "checkbox" },
      props: { checked: !!opts.checked },
    });
    if (typeof opts.onChange === "function") input.addEventListener("change", opts.onChange);
    return dom.el("label", {
      className: `xui-field xui-field--checkbox${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        dom.el("span", {
          className: "xui-field__check-row",
          children: [
            dom.el("span", { className: "xui-field__check-label", text: opts.label || "" }),
            input,
          ],
        }),
        opts.help ? dom.el("small", { className: "xui-field__help", text: opts.help }) : null,
      ],
    });
  }

  widgets.createField = createField;
  widgets.createNumberField = createNumberField;
  widgets.createSelectField = createSelectField;
  widgets.createCheckboxField = createCheckboxField;
})(window);
