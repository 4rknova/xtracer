(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  let activeModal = null;

  function dismissModal(confirmed) {
    if (!activeModal) return;
    activeModal.close(confirmed === true);
  }

  function showModal(options) {
    if (activeModal) dismissModal();
    const opts = options || {};

    return new Promise((resolve) => {
      let closed = false;
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

      const confirmBtn = opts.confirmLabel === false
        ? null
        : dom.el("button", {
          className: `xui-button xui-button--${opts.danger ? "danger" : "primary"}`,
          attrs: { type: "button" },
          text: opts.confirmLabel || "Confirm",
        });

      const rawBody = opts.body !== undefined ? opts.body : opts.message;
      const bodyChildren = typeof rawBody === "string"
        ? [dom.el("p", { text: rawBody })]
        : Array.isArray(rawBody)
          ? rawBody
          : rawBody ? [rawBody] : [];
      const actionChildren = [cancelBtn, confirmBtn].filter(Boolean);

      const dialog = dom.el("div", {
        className: `xui-modal-dialog${opts.dialogClassName ? ` ${opts.dialogClassName}` : ""}`,
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
            ? dom.el("div", {
              className: `xui-modal-body${opts.bodyClassName ? ` ${opts.bodyClassName}` : ""}`,
              children: bodyChildren,
            })
            : null,
          actionChildren.length
            ? dom.el("div", {
              className: "xui-modal-actions",
              children: actionChildren,
            })
            : null,
        ],
      });

      const overlay = dom.el("div", {
        className: `xui-modal${opts.overlayClassName ? ` ${opts.overlayClassName}` : ""}`,
        attrs: { role: "presentation" },
        children: [backdrop, dialog],
      });

      function close(confirmed) {
        if (closed) return;
        closed = true;
        if (overlay.parentNode) overlay.remove();
        if (activeModal && activeModal.overlay === overlay) activeModal = null;
        if (confirmed && typeof opts.onConfirm === "function") opts.onConfirm();
        if (!confirmed && typeof opts.onCancel === "function") opts.onCancel();
        resolve(confirmed);
      }

      backdrop.addEventListener("click", () => close(false));
      if (cancelBtn) cancelBtn.addEventListener("click", () => close(false));
      if (confirmBtn) confirmBtn.addEventListener("click", () => close(true));
      overlay.addEventListener("keydown", (e) => {
        if (e.key === "Escape") close(false);
      });

      document.body.appendChild(overlay);
      activeModal = { overlay, close };

      // Focus cancel for destructive actions, confirm otherwise
      const focusTarget = opts.danger && cancelBtn ? cancelBtn : (confirmBtn || cancelBtn || dialog);
      focusTarget.focus();
    });
  }

  widgets.showModal = showModal;
  widgets.dismissModal = dismissModal;
})(window);
