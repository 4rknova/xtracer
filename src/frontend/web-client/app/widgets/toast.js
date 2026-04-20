(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  let host = null;

  function getHost() {
    if (!host || !document.body.contains(host)) {
      host = dom.el("div", {
        className: "xui-toast-host",
        attrs: { "aria-live": "polite", "aria-atomic": "false" },
      });
      document.body.appendChild(host);
    }
    return host;
  }

  function showToast(options) {
    const opts = typeof options === "string" ? { message: options } : (options || {});
    const tone = opts.tone || "neutral";
    const duration = opts.duration !== undefined ? opts.duration : 4000;

    let dismissed = false;

    const closeSvg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    closeSvg.setAttribute("viewBox", "0 0 16 16");
    closeSvg.setAttribute("aria-hidden", "true");
    const line1 = document.createElementNS("http://www.w3.org/2000/svg", "line");
    line1.setAttribute("x1", "3"); line1.setAttribute("y1", "3");
    line1.setAttribute("x2", "13"); line1.setAttribute("y2", "13");
    const line2 = document.createElementNS("http://www.w3.org/2000/svg", "line");
    line2.setAttribute("x1", "13"); line2.setAttribute("y1", "3");
    line2.setAttribute("x2", "3"); line2.setAttribute("y2", "13");
    closeSvg.appendChild(line1);
    closeSvg.appendChild(line2);

    const closeBtn = dom.el("button", {
      className: "xui-toast-close",
      attrs: { type: "button", "aria-label": "Dismiss" },
    });
    closeBtn.appendChild(closeSvg);

    const toast = dom.el("div", {
      className: `xui-toast xui-toast--${tone}`,
      attrs: { role: "status" },
      children: [
        dom.el("span", { className: "xui-toast-message", text: opts.message || "" }),
        closeBtn,
      ],
    });

    function dismiss() {
      if (dismissed) return;
      dismissed = true;
      toast.classList.add("is-leaving");
      setTimeout(() => toast.remove(), 220);
    }

    closeBtn.addEventListener("click", dismiss);
    if (duration > 0) setTimeout(dismiss, duration);

    getHost().appendChild(toast);
    return { dismiss };
  }

  widgets.showToast = showToast;
})(window);
