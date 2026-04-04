(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  let activeModal = null;

  function dismissModal() {
    if (!activeModal) return;
    activeModal.remove();
    activeModal = null;
  }

  function showModal(options) {
    if (activeModal) dismissModal();
    const opts = options || {};

    return new Promise((resolve) => {
      const backdrop = dom.el("div", {
        className: "xui-modal-backdrop",
        attrs: { "aria-hidden": "true" },
      });

      const cancelBtn = opts.cancelLabel !== false
        ? dom.el("button", {
          className: "xui-button xui-button--ghost",
          attrs: { type: "button" },
          text: opts.cancelLabel || "Cancel",
        })
        : null;

      const confirmBtn = dom.el("button", {
        className: `xui-button xui-button--${opts.danger ? "danger" : "primary"}`,
        attrs: { type: "button" },
        text: opts.confirmLabel || "Confirm",
      });

      const bodyChildren = typeof opts.body === "string"
        ? [dom.el("p", { text: opts.body })]
        : opts.body ? [opts.body] : [];

      const dialog = dom.el("div", {
        className: "xui-modal-dialog",
        attrs: {
          role: "dialog",
          "aria-modal": "true",
          "aria-label": opts.title || "Dialog",
          tabindex: "-1",
        },
        children: [
          dom.el("div", {
            className: "xui-modal-head",
            children: [
              dom.el("h2", { className: "xui-modal-title", text: opts.title || "" }),
            ],
          }),
          bodyChildren.length
            ? dom.el("div", { className: "xui-modal-body", children: bodyChildren })
            : null,
          dom.el("div", {
            className: "xui-modal-actions",
            children: [cancelBtn, confirmBtn],
          }),
        ],
      });

      const overlay = dom.el("div", {
        className: "xui-modal",
        attrs: { role: "presentation" },
        children: [backdrop, dialog],
      });

      function close(confirmed) {
        dismissModal();
        if (confirmed && typeof opts.onConfirm === "function") opts.onConfirm();
        if (!confirmed && typeof opts.onCancel === "function") opts.onCancel();
        resolve(confirmed);
      }

      backdrop.addEventListener("click", () => close(false));
      if (cancelBtn) cancelBtn.addEventListener("click", () => close(false));
      confirmBtn.addEventListener("click", () => close(true));
      overlay.addEventListener("keydown", (e) => {
        if (e.key === "Escape") close(false);
      });

      document.body.appendChild(overlay);
      activeModal = overlay;

      // Focus cancel for destructive actions, confirm otherwise
      const focusTarget = opts.danger && cancelBtn ? cancelBtn : confirmBtn;
      focusTarget.focus();
    });
  }

  widgets.showModal = showModal;
  widgets.dismissModal = dismissModal;
})(window);
