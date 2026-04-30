(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createDefaultSceneIcon() {
    const ns = "http://www.w3.org/2000/svg";
    const svg = document.createElementNS(ns, "svg");
    svg.setAttribute("viewBox", "0 0 24 28");
    svg.setAttribute("aria-hidden", "true");
    svg.classList.add("scene-file-icon");

    const body = document.createElementNS(ns, "path");
    body.setAttribute("d", "M6.2 1.5h7.8L19 6.5V24a2.4 2.4 0 0 1-2.4 2.4H6.2A2.2 2.2 0 0 1 4 24.2V3.7a2.2 2.2 0 0 1 2.2-2.2z");
    body.setAttribute("fill", "none");
    body.setAttribute("stroke", "currentColor");
    body.setAttribute("stroke-width", "1.35");
    body.setAttribute("stroke-linejoin", "round");
    body.setAttribute("stroke-linecap", "round");
    svg.appendChild(body);

    const fold = document.createElementNS(ns, "path");
    fold.setAttribute("d", "M14 1.5v4.9H19");
    fold.setAttribute("fill", "none");
    fold.setAttribute("stroke", "currentColor");
    fold.setAttribute("stroke-width", "1.35");
    fold.setAttribute("stroke-linecap", "round");
    fold.setAttribute("stroke-linejoin", "round");
    svg.appendChild(fold);

    const text = document.createElementNS(ns, "text");
    text.setAttribute("x", "12");
    text.setAttribute("y", "12.4");
    text.setAttribute("text-anchor", "middle");
    text.setAttribute("dominant-baseline", "middle");
    text.textContent = ".ncf";
    svg.appendChild(text);
    return svg;
  }

  function createSceneBadge(label, className) {
    return dom.el("span", {
      className: `scene-file-ext${className ? ` ${className}` : ""}`,
      text: label,
    });
  }

  function createSceneStat(text) {
    return dom.el("span", {
      className: "scene-file-stat",
      text,
    });
  }

  function applyInteractiveState(node, opts) {
    if (!node) return node;
    if (typeof opts.onClick === "function") node.addEventListener("click", opts.onClick);
    if (typeof opts.onDoubleClick === "function") node.addEventListener("dblclick", opts.onDoubleClick);
    if (typeof opts.onContextMenu === "function") node.addEventListener("contextmenu", opts.onContextMenu);
    if (typeof opts.onKeyDown === "function") node.addEventListener("keydown", opts.onKeyDown);
    return node;
  }

  function createSceneCard(options) {
    const opts = options || {};
    const tagName = opts.tagName || "button";
    const sceneFile = String(opts.sceneFile || "").trim();
    const title = String(opts.title || "").trim();
    const sourceOrigin = String(opts.sourceOrigin || "").trim().toLowerCase();
    const dependsExternal = !!opts.dependsExternal;
    const hasVariants = !!opts.hasVariants;
    const variantCount = Number(opts.variantCount || 0);
    const cameraCount = Number(opts.cameraCount || 0);
    const isSelected = !!opts.selected;
    const isActive = !!opts.active;

    const badges = [];
    if (sourceOrigin) {
      badges.push(createSceneBadge(
        sourceOrigin === "workspace" ? "DRAFT" : "DISK",
        `scene-file-source scene-file-source--${sourceOrigin === "workspace" ? "workspace" : "disk"}`
      ));
    }
    if (dependsExternal) badges.push(createSceneBadge("EXT"));
    if (hasVariants) badges.push(createSceneBadge("VAR", "scene-file-var"));

    const stats = [
      createSceneStat(`${cameraCount || 0} camera${cameraCount === 1 ? "" : "s"}`),
      createSceneStat(variantCount > 0 ? `${variantCount} variant${variantCount === 1 ? "" : "s"}` : "base only"),
    ];

    const node = dom.el(tagName, {
      className: `scene-file-item xui-scene-card${opts.className ? ` ${opts.className}` : ""}`,
      attrs: {
        type: tagName === "button" ? "button" : null,
        role: opts.role || "option",
        "aria-selected": isSelected ? "true" : "false",
      },
      dataset: {
        scene: sceneFile || null,
      },
      children: [
        opts.icon || createDefaultSceneIcon(),
        dom.el("span", {
          className: "scene-file-meta",
          children: [
            dom.el("span", {
              className: "scene-file-name",
              text: sceneFile,
            }),
            title && title !== sceneFile ? dom.el("span", {
              className: "scene-file-title",
              text: title,
            }) : null,
            badges.length ? dom.el("span", {
              className: "scene-file-badges",
              children: badges,
            }) : null,
            dom.el("span", {
              className: "scene-file-stats",
              children: stats,
            }),
          ],
        }),
      ],
    });

    if (isSelected) node.classList.add("is-selected");
    if (isActive) node.classList.add("is-active");
    return applyInteractiveState(node, opts);
  }

  function createCameraCard(options) {
    const opts = options || {};
    const value = String(opts.value || "").trim();
    const label = String(opts.label || value || "-").trim() || "-";
    const description = String(opts.description || "").trim();
    const isSelected = !!opts.selected;
    const isActive = !!opts.active;
    const node = dom.el(opts.tagName || "button", {
      className: `scene-file-item camera-file-item xui-camera-card${opts.className ? ` ${opts.className}` : ""}`,
      attrs: {
        type: (opts.tagName || "button") === "button" ? "button" : null,
        role: opts.role || "option",
        "aria-selected": isSelected ? "true" : "false",
      },
      dataset: { camera: value || null },
      children: [
        dom.el("span", {
          className: "scene-file-header-row",
          children: [
            opts.icon || null,
            dom.el("span", { className: "scene-file-name", text: label }),
          ],
        }),
        dom.el("span", { className: "scene-file-title camera-file-description", text: description }),
      ],
    });
    if (isSelected) node.classList.add("is-selected");
    if (isActive) node.classList.add("is-active");
    return applyInteractiveState(node, opts);
  }

  function createVariantCard(options) {
    const opts = options || {};
    const value = String(opts.value || "").trim();
    const label = String(opts.label || "").trim() || "(base)";
    const description = String(opts.description || "").trim();
    const isBase = !!opts.base;
    const isSelected = !!opts.selected;
    const isActive = !!opts.active;
    const node = dom.el(opts.tagName || "button", {
      className: `scene-file-item camera-file-item variant-file-item xui-variant-card${opts.className ? ` ${opts.className}` : ""}`,
      attrs: {
        type: (opts.tagName || "button") === "button" ? "button" : null,
        role: opts.role || "option",
        "aria-selected": isSelected ? "true" : "false",
      },
      dataset: { variant: value || "" },
      children: [
        dom.el("span", {
          className: "scene-file-header-row",
          children: [
            dom.el("span", {
              className: "scene-file-name",
              children: [
                opts.icon || null,
                dom.el("span", { className: "scene-file-name-text", text: label }),
              ],
            }),
          ],
        }),
        description ? dom.el("span", { className: "scene-file-title variant-file-description", text: description }) : null,
      ],
    });
    if (isSelected) node.classList.add("is-selected");
    if (isActive) node.classList.add("is-active");
    if (isBase) node.classList.add("is-base");
    node.classList.add(description ? "has-description" : "no-description");
    return applyInteractiveState(node, opts);
  }

  widgets.createSceneCard = createSceneCard;
  widgets.createCameraCard = createCameraCard;
  widgets.createVariantCard = createVariantCard;
})(window);
