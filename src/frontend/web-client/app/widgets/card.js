(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createCardHead(options) {
    const opts = options || {};
    return dom.el("div", {
      className: "xui-card__head",
      children: [
        dom.el("h3", { className: "xui-card__title", text: opts.title || "" }),
        opts.note ? dom.el("span", { className: "xui-card__note", text: opts.note }) : null,
      ],
    });
  }

  function createPanelCard(options) {
    const opts = options || {};
    const head = (opts.title || opts.note) ? createCardHead(opts) : null;
    if (head) head.classList.add("xui-card__head--panel");
    return dom.el("section", {
      className: `xui-card${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        head,
        dom.el("div", { className: "xui-card__body", children: opts.body || opts.children || null }),
      ],
    });
  }

  function createCollapsibleCard(options) {
    const opts = options || {};
    const details = dom.el("details", {
      className: `xui-card xui-card--accordion xui-card--collapsible${opts.className ? ` ${opts.className}` : ""}`,
      props: { open: opts.open !== false },
    });
    const summary = dom.el("summary", {
      children: createCardHead(opts),
    });
    const body = dom.el("div", {
      className: "xui-card__body",
      children: opts.body || opts.children || null,
    });
    details.appendChild(summary);
    details.appendChild(body);
    if (typeof opts.onToggle === "function") {
      details.addEventListener("toggle", () => {
        opts.onToggle(details.open, details);
      });
    }
    return details;
  }

  function createAccordionCard(options) {
    return createCollapsibleCard(options);
  }

  widgets.createPanelCard = createPanelCard;
  widgets.createCollapsibleCard = createCollapsibleCard;
  widgets.createAccordionCard = createAccordionCard;
})(window);
