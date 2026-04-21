"use strict";

(function (global) {
  var widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  var dom = widgets.dom;

  function createMenuItem(options) {
    var opts = options || {};
    var node = dom.el("button", {
      className: "xui-menu__item".concat(opts.danger ? " is-danger" : "").concat(opts.className ? " ".concat(opts.className) : ""),
      attrs: {
        type: opts.type || "button",
        role: opts.role || "menuitem",
        id: opts.id || null,
        "aria-disabled": opts.disabled ? "true" : null,
        "aria-label": opts.ariaLabel || null,
        "aria-pressed": opts.pressed === undefined ? null : opts.pressed ? "true" : "false"
      },
      dataset: opts.dataset || null,
      props: {
        disabled: !!opts.disabled
      },
      children: opts.children !== undefined ? opts.children : [opts.icon ? dom.el("span", {
        className: "xui-menu__item-icon",
        children: opts.icon
      }) : null, dom.el("span", {
        className: "xui-menu__item-label",
        text: opts.label || ""
      }), opts.meta ? dom.el("span", {
        className: "xui-menu__item-meta",
        text: opts.meta
      }) : null]
    });
    if (typeof opts.onClick === "function") node.addEventListener("click", opts.onClick);
    return node;
  }

  function createMenuDivider() {
    return dom.el("div", {
      className: "xui-menu__divider",
      attrs: {
        role: "separator"
      }
    });
  }

  function createMenu(options) {
    var opts = options || {};
    return dom.el(opts.tagName || "div", {
      className: "xui-menu".concat(opts.className ? " ".concat(opts.className) : ""),
      attrs: {
        role: opts.role || "menu",
        id: opts.id || null,
        "aria-label": opts.label || null
      },
      children: opts.children || null
    });
  }

  var TAB_ICONS = {
    scene: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8l-6-6zm0 1.5L18.5 8H14V3.5zM8 13h8v1.5H8V13zm0 3.5h5v1.5H8V16.5z"></path></svg>',
    render: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M8 5v14l11-7L8 5z"></path></svg>',
    workspaces: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M3 3h8v8H3V3zm10 0h8v8h-8V3zm0 10h8v8h-8v-8zm-10 0h8v8H3v-8z"></path></svg>',
    visual: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04a1 1 0 0 0 0-1.41l-2.34-2.34a1 1 0 0 0-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z"></path></svg>',
    gallery: '<svg viewBox="0 0 24 24" fill="currentColor" aria-hidden="true"><path d="M22 16V4a2 2 0 0 0-2-2H8a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2zm-11-4 2.53 3.21L17 11l5 7H8l3-6zM2 6v14a2 2 0 0 0 2 2h14v-2H4V6H2z"></path></svg>'
  };

  function makeTabIcon(mode) {
    var svg = TAB_ICONS[mode];
    if (!svg) return null;
    var wrap = dom.el("span", {
      className: "tab-icon"
    });
    wrap.innerHTML = svg;
    return wrap;
  }

  function createMainTabs() {
    var tabs = [{
      id: "tabWorkspaces",
      mode: "workspaces",
      active: true,
      children: [makeTabIcon("workspaces"), dom.el("span", {
        className: "tab-label tab-label-desktop",
        text: "Workspaces"
      })]
    }, {
      id: "tabRender",
      label: "Render",
      mode: "render"
    }, {
      id: "tabVisual",
      label: "Editor",
      mode: "visual"
    }, {
      id: "tabGallery",
      label: "Gallery",
      mode: "gallery"
    }];
    return createMenu({
      tagName: "nav",
      id: "mainTabs",
      role: "navigation",
      label: "Primary sections",
      className: "tabs main-menu",
      children: tabs.map(function (tab) {
        return createMenuItem({
          id: tab.id,
          role: null,
          type: "button",
          ariaLabel: tab.ariaLabel || null,
          className: "tab main-menu__item".concat(tab.active ? " active" : ""),
          dataset: {
            mode: tab.mode
          },
          children: tab.children || [makeTabIcon(tab.mode), dom.el("span", {
            className: "tab-label",
            text: tab.label
          })]
        });
      })
    });
  }

  function renderMainTabs(target) {
    if (!(target instanceof HTMLElement) || !target.parentNode) return null;
    var menu = createMainTabs();
    target.parentNode.replaceChild(menu, target);
    return menu;
  }

  widgets.createMenu = createMenu;
  widgets.createMenuItem = createMenuItem;
  widgets.createMenuDivider = createMenuDivider;
  widgets.createMainTabs = createMainTabs;
  widgets.renderMainTabs = renderMainTabs;
})(window);