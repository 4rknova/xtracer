(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createPanelHeader(options) {
    const opts = options || {};
    return dom.el("div", {
      className: `xui-panel-head${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        dom.el("div", {
          className: "xui-panel-head__main",
          children: [
            dom.el("h3", {
              className: "xui-panel-head__title",
              text: opts.title || "",
            }),
            opts.subtitle ? dom.el("p", {
              className: "xui-panel-head__subtitle",
              text: opts.subtitle,
            }) : null,
          ],
        }),
        opts.meta ? dom.el("div", {
          className: "xui-panel-head__meta",
          children: opts.meta,
        }) : null,
      ],
    });
  }

  function upgradePanelHeaders(root) {
    const scope = root instanceof HTMLElement || root instanceof Document ? root : document;
    scope.querySelectorAll("[data-widget-panel-head]").forEach((node) => {
      if (!(node instanceof HTMLElement) || node.dataset.widgetPanelHeadUpgraded === "true") return;
      const titleNode = node.querySelector(":scope > h1, :scope > h2, :scope > h3, :scope > h4");
      const title = titleNode ? String(titleNode.textContent || "").trim() : "";
      const meta = [];
      Array.from(node.childNodes).forEach((child) => {
        if (child === titleNode) return;
        if (child.nodeType === Node.TEXT_NODE && !String(child.textContent || "").trim()) return;
        meta.push(child);
      });
      dom.mount(node, createPanelHeader({
        title,
        meta,
      }));
      node.dataset.widgetPanelHeadUpgraded = "true";
    });
  }

  widgets.createPanelHeader = createPanelHeader;
  widgets.upgradePanelHeaders = upgradePanelHeaders;
})(window);
