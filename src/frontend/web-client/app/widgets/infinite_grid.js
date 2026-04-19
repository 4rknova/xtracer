(function (global) {
  const widgets = global.XTracerWidgets || (global.XTracerWidgets = {});

  // Reads a CSS custom property from an element, returns fallback if empty.
  function cssVar(el, name, fallback) {
    const v = getComputedStyle(el).getPropertyValue(name).trim();
    return v || fallback;
  }

  /**
   * createInfiniteGrid(options) → HTMLElement
   *
   * Pannable / zoomable canvas grid.  Works as a standalone timeline background.
   *
   * options:
   *   height        {number}   CSS height in px (default 220)
   *   baseStep      {number}   World-unit size of one minor cell (default 80)
   *   majorEvery    {number}   Major line every N minor cells (default 5)
   *   className     {string}
   *   labelUnit     {number}   World units per displayed tick label (default = baseStep)
   *   labelSuffix   {string}   Appended to each label, e.g. "s" or "f" (default "")
   *   showPlayhead  {boolean}  Draw a highlighted vertical line at world x=0 (default true)
   *   showLabels    {boolean}  Draw X-axis time labels (default true)
   *   showAxisLine  {boolean}  Draw a horizontal baseline at world y=0 (default true)
   */
  function createInfiniteGrid(options) {
    const opts = options || {};
    const baseStep   = opts.baseStep   || 80;
    const majorEvery = opts.majorEvery || 5;
    const labelUnit  = opts.labelUnit  || baseStep;
    const suffix     = opts.labelSuffix || "";
    const showPH     = opts.showPlayhead !== false;
    const showLabels = opts.showLabels  !== false;
    const showAxis   = opts.showAxisLine !== false;
    const height     = opts.height || 220;

    const wrapper = document.createElement("div");
    wrapper.className = `xui-grid-view${opts.className ? ` ${opts.className}` : ""}`;
    wrapper.style.height = height + "px";

    const canvas = document.createElement("canvas");
    canvas.className = "xui-grid-view__canvas";
    wrapper.appendChild(canvas);

    // State
    let tx = opts.initialTx !== undefined ? opts.initialTx : (opts.height || 220) / 2;
    let ty = opts.initialTy !== undefined ? opts.initialTy : (opts.height || 220) / 2;
    let scale = opts.initialScale || 1;
    let rafId = 0;
    let needsRedraw = true;

    function scheduleRedraw() {
      if (needsRedraw) return;
      needsRedraw = true;
      rafId = requestAnimationFrame(paint);
    }

    function paint() {
      needsRedraw = false;
      const dpr  = window.devicePixelRatio || 1;
      const cssW = wrapper.clientWidth;
      const cssH = wrapper.clientHeight;
      if (cssW <= 0 || cssH <= 0) return;

      if (canvas.width !== cssW * dpr || canvas.height !== cssH * dpr) {
        canvas.width  = cssW * dpr;
        canvas.height = cssH * dpr;
        canvas.style.width  = cssW + "px";
        canvas.style.height = cssH + "px";
      }

      const ctx = canvas.getContext("2d");
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, cssW, cssH);

      // ── Background ────────────────────────────────────────────────────
      ctx.fillStyle = cssVar(wrapper, "--panel", "#111315");
      ctx.fillRect(0, 0, cssW, cssH);

      const step      = baseStep * scale;
      const majorStep = step * majorEvery;

      if (step >= 5) {
        const ox = ((tx % step) + step) % step;
        const oy = ((ty % step) + step) % step;

        // ── Minor grid ─────────────────────────────────────────────────
        ctx.beginPath();
        ctx.strokeStyle = "rgba(120,145,170,0.09)";
        ctx.lineWidth = 1;
        for (let x = ox; x <= cssW; x += step) {
          const col = Math.round((x - tx) / step);
          if (col % majorEvery !== 0) { ctx.moveTo(x, 0); ctx.lineTo(x, cssH); }
        }
        for (let y = oy; y <= cssH; y += step) {
          const row = Math.round((y - ty) / step);
          if (row % majorEvery !== 0) { ctx.moveTo(0, y); ctx.lineTo(cssW, y); }
        }
        ctx.stroke();

        // ── Major grid ─────────────────────────────────────────────────
        const mox = ((tx % majorStep) + majorStep) % majorStep;
        const moy = ((ty % majorStep) + majorStep) % majorStep;
        ctx.beginPath();
        ctx.strokeStyle = "rgba(120,145,170,0.20)";
        ctx.lineWidth = 1;
        for (let x = mox; x <= cssW; x += majorStep) { ctx.moveTo(x, 0); ctx.lineTo(x, cssH); }
        for (let y = moy; y <= cssH; y += majorStep) { ctx.moveTo(0, y); ctx.lineTo(cssW, y); }
        ctx.stroke();
      }

      // ── Axis baseline (world y=0) ─────────────────────────────────────
      if (showAxis) {
        const originY = ty;
        if (originY >= 0 && originY <= cssH) {
          ctx.strokeStyle = "rgba(120,145,170,0.35)";
          ctx.lineWidth = 1;
          ctx.beginPath();
          ctx.moveTo(0, originY);
          ctx.lineTo(cssW, originY);
          ctx.stroke();
        }
      }

      // ── X-axis labels ─────────────────────────────────────────────────
      if (showLabels && step >= 18) {
        const labelStep = majorStep;
        const lox = ((tx % labelStep) + labelStep) % labelStep;
        const accentRgb = cssVar(wrapper, "--accent-rgb", "100,160,220");
        ctx.font = `10px IBM Plex Mono, "Fira Code", monospace`;
        ctx.textBaseline = "top";

        for (let x = lox - labelStep; x <= cssW + labelStep; x += labelStep) {
          const worldX = (x - tx) / scale;
          const tick   = Math.round(worldX / labelUnit * majorEvery);
          const label  = String(tick) + suffix;
          const isOrigin = tick === 0;

          ctx.fillStyle = isOrigin
            ? `rgba(${accentRgb},0.80)`
            : "rgba(120,145,170,0.45)";
          ctx.fillText(label, x + 3, 4);

          // Tick mark on axis
          const axisY = Math.max(0, Math.min(cssH, ty));
          ctx.strokeStyle = isOrigin
            ? `rgba(${accentRgb},0.55)`
            : "rgba(120,145,170,0.30)";
          ctx.lineWidth = 1;
          ctx.beginPath();
          ctx.moveTo(x, axisY - 4);
          ctx.lineTo(x, axisY + 4);
          ctx.stroke();
        }
      }

      // ── Playhead (world x=0) ──────────────────────────────────────────
      if (showPH) {
        const phX = tx;
        if (phX >= -1 && phX <= cssW + 1) {
          const accentRgb = cssVar(wrapper, "--accent-rgb", "100,160,220");
          ctx.strokeStyle = `rgba(${accentRgb},0.70)`;
          ctx.lineWidth   = 1.5;
          ctx.beginPath();
          ctx.moveTo(phX, 0);
          ctx.lineTo(phX, cssH);
          ctx.stroke();

          // Playhead diamond marker at top
          ctx.fillStyle = `rgba(${accentRgb},0.85)`;
          ctx.beginPath();
          ctx.moveTo(phX, 4);
          ctx.lineTo(phX + 5, 10);
          ctx.lineTo(phX, 16);
          ctx.lineTo(phX - 5, 10);
          ctx.closePath();
          ctx.fill();
        }
      }
    }

    // ── Interaction ───────────────────────────────────────────────────────
    let dragging = false;
    let dragStartX = 0, dragStartY = 0;
    let dragOriginTx = 0, dragOriginTy = 0;

    canvas.addEventListener("mousedown", (e) => {
      dragging = true;
      dragStartX = e.clientX; dragStartY = e.clientY;
      dragOriginTx = tx; dragOriginTy = ty;
      canvas.classList.add("is-grabbing");
      e.preventDefault();
    });

    window.addEventListener("mouseup", () => {
      if (dragging) { dragging = false; canvas.classList.remove("is-grabbing"); }
    });

    window.addEventListener("mousemove", (e) => {
      if (!dragging) return;
      tx = dragOriginTx + (e.clientX - dragStartX);
      ty = dragOriginTy + (e.clientY - dragStartY);
      scheduleRedraw();
    });

    canvas.addEventListener("wheel", (e) => {
      e.preventDefault();
      const zoomIn  = e.deltaY < 0;
      const factor  = zoomIn ? 1.12 : 1 / 1.12;
      const rect    = canvas.getBoundingClientRect();
      const mx      = e.clientX - rect.left;
      const my      = e.clientY - rect.top;
      tx    = mx - (mx - tx) * factor;
      ty    = my - (my - ty) * factor;
      scale = Math.max(0.08, Math.min(16, scale * factor));
      scheduleRedraw();
    }, { passive: false });

    canvas.addEventListener("touchstart", (e) => {
      if (e.touches.length !== 1) return;
      dragging = true;
      dragStartX = e.touches[0].clientX; dragStartY = e.touches[0].clientY;
      dragOriginTx = tx; dragOriginTy = ty;
    }, { passive: true });

    canvas.addEventListener("touchmove", (e) => {
      if (!dragging || e.touches.length !== 1) return;
      tx = dragOriginTx + (e.touches[0].clientX - dragStartX);
      ty = dragOriginTy + (e.touches[0].clientY - dragStartY);
      scheduleRedraw();
      e.preventDefault();
    }, { passive: false });

    canvas.addEventListener("touchend", () => { dragging = false; }, { passive: true });

    // ── Resize ────────────────────────────────────────────────────────────
    const ro = new ResizeObserver(() => scheduleRedraw());
    ro.observe(wrapper);

    // ── Initial paint ─────────────────────────────────────────────────────
    requestAnimationFrame(() => {
      // Centre the playhead initially
      tx = wrapper.clientWidth * 0.25;
      ty = wrapper.clientHeight * 0.5;
      needsRedraw = true;
      paint();
    });

    wrapper._gridDestroy = () => {
      cancelAnimationFrame(rafId);
      ro.disconnect();
    };

    return wrapper;
  }

  widgets.createInfiniteGrid = createInfiniteGrid;
})(window);
