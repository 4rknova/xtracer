(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});

  function appendChildren(node, children) {
    if (!node || children === undefined || children === null) return node;
    const list = Array.isArray(children) ? children : [children];
    list.forEach((child) => {
      if (child === undefined || child === null || child === false) return;
      if (Array.isArray(child)) {
        appendChildren(node, child);
        return;
      }
      if (child instanceof Node) {
        node.appendChild(child);
        return;
      }
      node.appendChild(document.createTextNode(String(child)));
    });
    return node;
  }

  function el(tag, options) {
    const opts = options || {};
    const node = document.createElement(tag);
    if (opts.className) node.className = opts.className;
    if (Array.isArray(opts.classes)) {
      opts.classes.filter(Boolean).forEach((name) => node.classList.add(name));
    }
    if (opts.attrs) {
      Object.keys(opts.attrs).forEach((key) => {
        const value = opts.attrs[key];
        if (value === undefined || value === null || value === false) return;
        node.setAttribute(key, value === true ? "" : String(value));
      });
    }
    if (opts.dataset) {
      Object.keys(opts.dataset).forEach((key) => {
        const value = opts.dataset[key];
        if (value === undefined || value === null) return;
        node.dataset[key] = String(value);
      });
    }
    if (opts.text !== undefined && opts.text !== null) {
      node.textContent = String(opts.text);
    }
    if (opts.html !== undefined && opts.html !== null) {
      node.innerHTML = String(opts.html);
    }
    if (opts.props) {
      Object.keys(opts.props).forEach((key) => {
        node[key] = opts.props[key];
      });
    }
    if (opts.children !== undefined) appendChildren(node, opts.children);
    return node;
  }

  function clear(node) {
    if (!node) return node;
    while (node.firstChild) node.removeChild(node.firstChild);
    return node;
  }

  function mount(node, child) {
    if (!node) return null;
    clear(node);
    appendChildren(node, child);
    return node;
  }

  function svgIcon(pathData, viewBox) {
    const ns = "http://www.w3.org/2000/svg";
    const svg = document.createElementNS(ns, "svg");
    svg.setAttribute("viewBox", viewBox || "0 0 16 16");
    svg.setAttribute("aria-hidden", "true");
    const path = document.createElementNS(ns, "path");
    path.setAttribute("fill", "none");
    path.setAttribute("stroke", "currentColor");
    path.setAttribute("stroke-width", "1.8");
    path.setAttribute("stroke-linecap", "round");
    path.setAttribute("stroke-linejoin", "round");
    path.setAttribute("d", pathData);
    svg.appendChild(path);
    return svg;
  }

  widgets.dom = { appendChildren, el, clear, mount, svgIcon };
})(window);
