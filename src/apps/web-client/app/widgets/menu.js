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

  const TAB_ICONS = {
    scene: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8l-6-6zm0 1.5L18.5 8H14V3.5zM8 13h8v1.5H8V13zm0 3.5h5v1.5H8V16.5z"></path></svg>',
    render: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M8 5v14l11-7L8 5z"></path></svg>',
    workspaces: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M3 3h8v8H3V3zm10 0h8v8h-8V3zm0 10h8v8h-8v-8zm-10 0h8v8H3v-8z"></path></svg>',
    visual: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 0 0 0-1.41l-2.34-2.34a1 1 0 0 0-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z"></path></svg>',
    gallery: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M22 16V4a2 2 0 0 0-2-2H8a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2zm-11-4 2.53 3.21L17 11l5 7H8l3-6zM2 6v14a2 2 0 0 0 2 2h14v-2H4V6H2z"></path></svg>',
    settings: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M19.14 12.94c.04-.3.06-.62.06-.94s-.02-.64-.07-.94l2.03-1.58a.49.49 0 0 0 .12-.61l-1.92-3.32a.49.49 0 0 0-.59-.22l-2.39.96a7.4 7.4 0 0 0-1.62-.94l-.36-2.54a.48.48 0 0 0-.48-.41h-3.84a.48.48 0 0 0-.47.41l-.36 2.54a7.4 7.4 0 0 0-1.62.94l-2.39-.96a.49.49 0 0 0-.59.22L2.74 8.87a.48.48 0 0 0 .12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58a.49.49 0 0 0-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-.17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32a.48.48 0 0 0-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"/></svg>',
    about: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z"/></svg>',
    logs: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M20 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2zm0 14H4V8h16v10zm-2-1h-6v-2h6v2zM7.5 17l-1.41-1.41L8.67 13l-2.58-2.59L7.5 9l4 4-4 4z"/></svg>',
    jobs: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67V7z"/></svg>',
  };

  function makeTabIcon(mode) {
    const svg = TAB_ICONS[mode];
    if (!svg) return null;
    const wrap = dom.el("span", { className: "tab-icon" });
    wrap.innerHTML = svg;
    return wrap;
  }

  function createMainTabs() {
    const tabs = [
      {
        id: "tabWorkspaces",
        mode: "workspaces",
        active: true,
        children: [
          makeTabIcon("workspaces"),
          dom.el("span", { className: "tab-label tab-label-desktop", text: "Workspaces" }),
        ],
      },
      { id: "tabRender", label: "Render", mode: "render" },
      { id: "tabVisual", label: "Editor", mode: "visual" },
      { id: "tabGallery", label: "Gallery", mode: "gallery" },
      { id: "tabSettings", label: "Settings", mode: "settings" },
      { id: "tabAbout", label: "About", mode: "about" },
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
        children: tab.children || [
          makeTabIcon(tab.mode),
          dom.el("span", { className: "tab-label", text: tab.label }),
        ],
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
