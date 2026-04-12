(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createTabContainer(options) {
    const opts = options || {};
    const tabs = opts.tabs || [];
    let activeId = (tabs[0] || {}).id || "";

    const tabNav = dom.el("nav", {
      className: "xui-tab-container-nav",
      attrs: { role: "tablist" },
    });

    const tabBody = dom.el("div", { className: "xui-tab-container-body" });

    function activate(id) {
      activeId = id;
      const btns = tabNav.querySelectorAll(".xui-tab-btn");
      btns.forEach((btn) => {
        const isActive = btn.dataset.tab === id;
        btn.classList.toggle("is-active", isActive);
        btn.setAttribute("aria-selected", String(isActive));
      });
      const panes = tabBody.querySelectorAll(".xui-tab-pane");
      panes.forEach((pane) => {
        pane.hidden = pane.dataset.tab !== id;
      });
        // Reset scroll position of nearest scrollable ancestor
      let scrollable = tabBody.parentElement;
      while (scrollable && scrollable !== document.body) {
        const { overflowY } = getComputedStyle(scrollable);
        if (overflowY === "auto" || overflowY === "scroll") {
          scrollable.scrollTop = 0;
          break;
        }
        scrollable = scrollable.parentElement;
      }
      if (typeof opts.onChange === "function") opts.onChange(id);
    }

    tabs.forEach((tab) => {
      const btn = dom.el("button", {
        className: `xui-tab-btn${tab.id === activeId ? " is-active" : ""}`,
        attrs: {
          type: "button",
          role: "tab",
          "aria-selected": tab.id === activeId ? "true" : "false",
          "data-tab": tab.id,
        },
        text: tab.label,
      });
      btn.addEventListener("click", () => activate(tab.id));
      tabNav.appendChild(btn);

      const pane = dom.el("div", {
        className: "xui-tab-pane",
        attrs: { role: "tabpanel", "data-tab": tab.id },
        children: tab.content ? [tab.content] : [],
      });
      pane.hidden = tab.id !== activeId;
      tabBody.appendChild(pane);
    });

    // Arrow key navigation
    tabNav.addEventListener("keydown", (e) => {
      if (e.key !== "ArrowRight" && e.key !== "ArrowLeft") return;
      const btns = [...tabNav.querySelectorAll(".xui-tab-btn")];
      const idx = btns.indexOf(document.activeElement);
      if (idx === -1) return;
      e.preventDefault();
      const next = e.key === "ArrowRight"
        ? btns[(idx + 1) % btns.length]
        : btns[(idx - 1 + btns.length) % btns.length];
      next.focus();
      activate(next.dataset.tab);
    });

    return dom.el("div", {
      className: `xui-tab-container${opts.className ? ` ${opts.className}` : ""}`,
      children: [tabNav, tabBody],
    });
  }

  widgets.createTabContainer = createTabContainer;
})(window);
