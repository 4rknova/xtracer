(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});
  const dom = widgets.dom;

  function createWorkspaceCard(options) {
    const opts = options || {};
    const listMode = !!opts.listMode;
    const node = dom.el("article", {
      className: `workspace-item xui-workspace-card${opts.className ? ` ${opts.className}` : ""}`,
      children: listMode
        ? [opts.previewColumn || null, opts.main || null, opts.actions || null]
        : [opts.head || null, opts.preview || null, opts.meta || null, opts.actions || null],
    });
    if (opts.active) node.classList.add("is-active");
    if (opts.rendering) node.classList.add("is-rendering");
    if (listMode) node.classList.add("workspace-item-list-compact");
    return node;
  }

  widgets.createWorkspaceCard = createWorkspaceCard;
})(window);
