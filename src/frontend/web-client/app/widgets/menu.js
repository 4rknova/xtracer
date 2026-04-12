(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createMenuItem(options) {
    const opts = options || {};
    const node = dom.el("button", {
      className: `xui-menu__item${opts.danger ? " is-danger" : ""}${opts.className ? ` ${opts.className}` : ""}`,
      attrs: {
        type: opts.type || "button",
        role: opts.role || "menuitem",
        id: opts.id || null,
        "aria-disabled": opts.disabled ? "true" : null,
        "aria-label": opts.ariaLabel || null,
        "aria-pressed": opts.pressed === undefined ? null : (opts.pressed ? "true" : "false"),
      },
      dataset: opts.dataset || null,
      props: { disabled: !!opts.disabled },
      children: opts.children !== undefined ? opts.children : [
        opts.icon ? dom.el("span", { className: "xui-menu__item-icon", children: opts.icon }) : null,
        dom.el("span", { className: "xui-menu__item-label", text: opts.label || "" }),
        opts.meta ? dom.el("span", { className: "xui-menu__item-meta", text: opts.meta }) : null,
      ],
    });
    if (typeof opts.onClick === "function") node.addEventListener("click", opts.onClick);
    return node;
  }

  function createMenuDivider() {
    return dom.el("div", {
      className: "xui-menu__divider",
      attrs: { role: "separator" },
    });
  }

  function createMenu(options) {
    const opts = options || {};
    return dom.el(opts.tagName || "div", {
      className: `xui-menu${opts.className ? ` ${opts.className}` : ""}`,
      attrs: {
        role: opts.role || "menu",
        id: opts.id || null,
        "aria-label": opts.label || null,
      },
      children: opts.children || null,
    });
  }

  function createMainTabs() {
    const tabs = [
      { id: "tabScene", label: "Scene", mode: "scene", active: true },
      { id: "tabRender", label: "Render", mode: "render" },
      {
        id: "tabWorkspaces",
        label: "Workspaces",
        mode: "workspaces",
        children: [
          dom.el("span", { className: "tab-label tab-label-desktop", text: "Workspaces" }),
          dom.el("span", { className: "tab-label tab-label-mobile", text: "Spaces" }),
        ],
      },
      { id: "tabVisual", label: "Editor", mode: "visual" },
      { id: "tabGallery", label: "Gallery", mode: "gallery" },
    ];
    return createMenu({
      tagName: "nav",
      id: "mainTabs",
      role: "navigation",
      label: "Primary sections",
      className: "tabs main-menu",
      children: tabs.map((tab) => createMenuItem({
        id: tab.id,
        role: null,
        type: "button",
        ariaLabel: tab.ariaLabel || null,
        className: `tab main-menu__item${tab.active ? " active" : ""}`,
        dataset: { mode: tab.mode },
        children: tab.children || dom.el("span", { className: "tab-label", text: tab.label }),
      })),
    });
  }

  function renderMainTabs(target) {
    if (!(target instanceof HTMLElement) || !target.parentNode) return null;
    const menu = createMainTabs();
    target.parentNode.replaceChild(menu, target);
    return menu;
  }

  widgets.createMenu = createMenu;
  widgets.createMenuItem = createMenuItem;
  widgets.createMenuDivider = createMenuDivider;
  widgets.createMainTabs = createMainTabs;
  widgets.renderMainTabs = renderMainTabs;
})(window);
