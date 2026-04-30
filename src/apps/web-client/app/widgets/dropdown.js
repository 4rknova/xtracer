(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;
  const enhanced = new WeakMap();
  const selectValueDescriptor = Object.getOwnPropertyDescriptor(global.HTMLSelectElement.prototype, "value");
  let activeDropdown = null;

  function shouldEnhanceSelect(select) {
    if (!(select instanceof HTMLSelectElement)) return false;
    if (select.multiple) return false;
    if (Number(select.size || 0) > 1) return false;
    if (select.hidden || select.closest("[hidden]")) return false;
    if (select.getAttribute("aria-hidden") === "true") return false;
    return true;
  }

  function closeActiveDropdown(except) {
    if (!activeDropdown || activeDropdown === except) return;
    const state = enhanced.get(activeDropdown);
    if (state) state.close();
  }

  function selectedOption(select) {
    const index = select.selectedIndex;
    if (index >= 0 && index < select.options.length) return select.options[index];
    return select.options.length ? select.options[0] : null;
  }

  function enhanceSelect(select) {
    if (!shouldEnhanceSelect(select)) return null;
    if (enhanced.has(select)) return enhanced.get(select);

    select.classList.add("xui-select", "xui-select-native");

    const wrapper = dom.el("div", {
      className: "xui-dropdown",
    });
    const trigger = dom.el("button", {
      className: "xui-dropdown__trigger",
      attrs: {
        type: "button",
        "aria-haspopup": "listbox",
        "aria-expanded": "false",
      },
      children: [
        dom.el("span", { className: "xui-dropdown__label" }),
        dom.el("span", { className: "xui-dropdown__chevron", attrs: { "aria-hidden": "true" } }),
      ],
    });
    const panel = dom.el("div", {
      className: "xui-dropdown__panel xui-menu",
      attrs: { role: "listbox" },
      props: { hidden: true },
    });

    select.parentNode.insertBefore(wrapper, select);
    wrapper.appendChild(select);
    wrapper.appendChild(trigger);
    document.body.appendChild(panel);

    let patchedValueSetter = false;

    function positionPanel() {
      const rect = trigger.getBoundingClientRect();
      const viewportHeight = Math.max(document.documentElement.clientHeight || 0, global.innerHeight || 0);
      panel.style.left = `${Math.round(rect.left)}px`;
      panel.style.top = `${Math.round(rect.bottom - 2)}px`;
      panel.style.width = `${Math.round(rect.width)}px`;
      const maxHeight = Math.max(140, viewportHeight - rect.bottom - 12);
      panel.style.maxHeight = `${Math.round(maxHeight)}px`;
    }

    function syncTrigger() {
      const option = selectedOption(select);
      const text = option ? String(option.textContent || option.label || option.value || "").trim() : "";
      const disabled = !!select.disabled;
      trigger.querySelector(".xui-dropdown__label").textContent = text || "Select";
      trigger.disabled = disabled;
      wrapper.classList.toggle("is-disabled", disabled);
      Array.from(panel.children).forEach((item) => {
        const value = item.getAttribute("data-value");
        const isSelected = value === String(select.value || "");
        item.classList.toggle("is-selected", isSelected);
        item.setAttribute("aria-selected", isSelected ? "true" : "false");
      });
    }

    function renderOptions() {
      dom.clear(panel);
      Array.from(select.options).forEach((option) => {
        const item = dom.el("button", {
          className: `xui-menu__item xui-dropdown__option${option.disabled ? " is-disabled" : ""}`,
          attrs: {
            type: "button",
            role: "option",
            "data-value": option.value,
            "aria-selected": option.selected ? "true" : "false",
            "aria-disabled": option.disabled ? "true" : null,
          },
          props: {
            disabled: !!option.disabled,
          },
          children: dom.el("span", { className: "xui-menu__item-label", text: option.textContent || option.label || option.value }),
        });
        item.addEventListener("click", () => {
          if (option.disabled) return;
          const prev = String(select.value || "");
          if (selectValueDescriptor && typeof selectValueDescriptor.set === "function") {
            selectValueDescriptor.set.call(select, option.value);
          } else {
            select.value = option.value;
          }
          syncTrigger();
          close();
          if (prev !== option.value) {
            select.dispatchEvent(new Event("input", { bubbles: true }));
            select.dispatchEvent(new Event("change", { bubbles: true }));
          }
        });
        panel.appendChild(item);
      });
      syncTrigger();
    }

    function open() {
      if (trigger.disabled) return;
      closeActiveDropdown(select);
      renderOptions();
      positionPanel();
      wrapper.classList.add("is-open");
      panel.hidden = false;
      trigger.setAttribute("aria-expanded", "true");
      activeDropdown = select;
    }

    function close() {
      wrapper.classList.remove("is-open");
      panel.hidden = true;
      trigger.setAttribute("aria-expanded", "false");
      if (activeDropdown === select) activeDropdown = null;
    }

    function toggle() {
      if (wrapper.classList.contains("is-open")) close();
      else open();
    }

    trigger.addEventListener("click", () => toggle());
    trigger.addEventListener("keydown", (evt) => {
      if (evt.key === "ArrowDown" || evt.key === "Enter" || evt.key === " ") {
        evt.preventDefault();
        open();
      } else if (evt.key === "Escape") {
        close();
      }
    });

    select.addEventListener("change", syncTrigger);

    const observer = new MutationObserver(() => {
      renderOptions();
    });
    observer.observe(select, { childList: true, subtree: true, attributes: true });

    if (selectValueDescriptor && typeof selectValueDescriptor.set === "function" && !select.__xuiPatchedValue) {
      Object.defineProperty(select, "value", {
        configurable: true,
        enumerable: selectValueDescriptor.enumerable,
        get() {
          return selectValueDescriptor.get.call(this);
        },
        set(next) {
          selectValueDescriptor.set.call(this, next);
          syncTrigger();
        },
      });
      select.__xuiPatchedValue = true;
      patchedValueSetter = true;
    }

    const state = {
      select,
      wrapper,
      trigger,
      panel,
      renderOptions,
      open,
      close,
      syncTrigger,
      positionPanel,
      destroy() {
        observer.disconnect();
        close();
        if (panel.parentNode) panel.parentNode.removeChild(panel);
        if (patchedValueSetter) delete select.__xuiPatchedValue;
      },
    };
    enhanced.set(select, state);
    renderOptions();
    return state;
  }

  function enhanceSelects(root) {
    const scope = root && root.querySelectorAll ? root : document;
    scope.querySelectorAll("select.xui-select, .xui-field select").forEach((select) => {
      enhanceSelect(select);
    });
  }

  document.addEventListener("click", (evt) => {
    if (!activeDropdown) return;
    const state = enhanced.get(activeDropdown);
    if (!state) return;
    if (evt.target && state.wrapper.contains(evt.target)) return;
    state.close();
  });

  document.addEventListener("keydown", (evt) => {
    if (evt.key !== "Escape" || !activeDropdown) return;
    const state = enhanced.get(activeDropdown);
    if (state) state.close();
  });

  global.addEventListener("resize", () => {
    if (!activeDropdown) return;
    const state = enhanced.get(activeDropdown);
    if (state) state.positionPanel();
  });
  document.addEventListener("scroll", () => {
    if (!activeDropdown) return;
    const state = enhanced.get(activeDropdown);
    if (state) state.positionPanel();
  }, true);

  widgets.enhanceSelect = enhanceSelect;
  widgets.enhanceSelects = enhanceSelects;
})(window);
