(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createFact(label, value, valueClassName) {
    if (!value) return null;
    return dom.el("div", {
      className: "xui-dependency__fact",
      children: [
        dom.el("span", {
          className: "xui-dependency__fact-label",
          text: label,
        }),
        dom.el("span", {
          className: `xui-dependency__fact-value${valueClassName ? ` ${valueClassName}` : ""}`,
          text: value,
        }),
      ],
    });
  }

  function createDependencyItem(options) {
    const opts = options || {};
    const facts = [
      createFact("Used in", opts.usedIn, "xui-dependency__usage"),
      createFact("License", opts.license, "xui-dependency__license"),
    ].filter(Boolean);

    return dom.el("article", {
      className: `xui-dependency${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        dom.el("div", {
          className: "xui-dependency__main",
          children: [
            dom.el("h4", {
              className: "xui-dependency__name",
              text: opts.name || "Unknown",
            }),
            opts.description ? dom.el("p", {
              className: "xui-dependency__description",
              text: opts.description,
            }) : null,
            facts.length ? dom.el("div", {
              className: "xui-dependency__facts",
              children: facts,
            }) : null,
          ],
        }),
        opts.url ? dom.el("a", {
          className: "xui-dependency__link",
          attrs: {
            href: opts.url,
            target: "_blank",
            rel: "noopener noreferrer",
          },
          children: [
            dom.el("span", {
              className: "xui-dependency__link-label",
              text: "Repository",
            }),
            dom.el("span", {
              className: "xui-dependency__link-value",
              text: opts.url,
            }),
          ],
        }) : null,
      ],
    });
  }

  function renderDependencyList(target, items) {
    if (!(target instanceof HTMLElement)) return null;
    const list = Array.isArray(items) ? items : [];
    return dom.mount(target, list.map((item) => createDependencyItem({
      name: item && item.name ? String(item.name) : "Unknown",
      description: item && item.description ? String(item.description) : "",
      usedIn: item && item.used_in ? String(item.used_in) : "Unknown",
      license: item && item.license ? String(item.license) : "Unknown",
      url: item && item.url ? String(item.url) : "",
    })));
  }

  widgets.createDependencyItem = createDependencyItem;
  widgets.renderDependencyList = renderDependencyList;
})(window);
