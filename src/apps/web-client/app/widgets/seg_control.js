(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createSegControl(options) {
    const opts = options || {};
    const items = opts.items || [];
    const firstActive = items.find((i) => i.active);
    let activeId = firstActive ? firstActive.id : (items[0] || {}).id;

    const btns = [];

    function activate(id) {
      activeId = id;
      btns.forEach((btn, i) => {
        const on = items[i].id === id;
        btn.classList.toggle("active", on);
        btn.setAttribute("aria-selected", on ? "true" : "false");
      });
      if (typeof opts.onChange === "function") opts.onChange(id);
    }

    items.forEach((item) => {
      const on = item.id === activeId;
      const btn = dom.el("button", {
        className: "seg-btn" + (on ? " active" : ""),
        attrs: { type: "button", role: "tab", "aria-selected": on ? "true" : "false" },
        text: item.label,
      });
      btn.addEventListener("click", () => activate(item.id));
      btns.push(btn);
    });

    const attrs = { role: "tablist" };
    if (opts.ariaLabel) attrs["aria-label"] = opts.ariaLabel;

    return dom.el("div", {
      className: "seg-control" + (opts.className ? " " + opts.className : ""),
      attrs,
      children: btns,
    });
  }

  widgets.createSegControl = createSegControl;
})(window);
