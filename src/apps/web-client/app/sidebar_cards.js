(function (global) {
  const widgets = window.XTracerWidgets;
  const dom = widgets.dom;

  function htmlToFragment(html) {
    const template = document.createElement("template");
    template.innerHTML = html.trim();
    return template.content;
  }

  function createSidebarModalHeader() {
    const closeBtn = widgets.createIconButton({
      className: "controls-modal-close",
      variant: "ghost",
      label: "Close controls",
      title: "Close controls",
      icon: dom.svgIcon(
        "M6 6l12 12M18 6L6 18",
        "0 0 24 24"
      ),
    });
    closeBtn.id = "controlsModalCloseBtn";

    return dom.el("div", {
      className: "controls-modal-head",
      children: [
        dom.el("div", {
          className: "controls-modal-heading",
          children: [
            dom.el("p", {
              className: "controls-modal-kicker",
              text: "Controls",
            }),
            dom.el("h2", {
              className: "controls-modal-title",
              attrs: { id: "controlsModalTitle" },
              text: "Render Settings",
            }),
          ],
        }),
        closeBtn,
      ],
    });
  }

  function createSidebarCard(config) {
    const card = window.XTracerWidgets.createCollapsibleCard({
      className: "control-section control-card",
      title: config.title || "",
      note: config.note || "",
      open: config.open !== false,
      bodyClass: `control-section-body${config.bodyClass ? ` ${config.bodyClass}` : ""}`,
      body: htmlToFragment(config.bodyHTML || ""),
    });
    card.id = config.id;
    if (config.hidden) card.hidden = true;
    const titleEl = card.querySelector(".xui-card__title");
    if (titleEl) titleEl.classList.add("control-card-title");
    const noteEl = card.querySelector(".xui-card__note");
    if (noteEl) noteEl.classList.add("control-card-note");
    return card;
  }

  const CARD_DEFS = [
    {
      id: "activeSceneControlsCard",
      title: "Active Scene",
      note: "Scene, camera, variant",
      icon: "M3 5h18v14H3z M7 5v14 M17 5v14 M3 9h18 M3 15h18",
      open: true,
      bodyHTML: `
        <section class="settings-section workspace-info-section active-scene-sidebar">
          <div id="activeSceneCardScene" class="active-scene-header"></div>
          <p id="activeSceneCardDescription" class="active-scene-description">-</p>
          <div id="activeSceneCardSource" class="active-scene-row"></div>
          <div class="active-scene-row active-scene-camera-row">
            <span class="active-scene-row-label">Camera</span>
            <select id="activeSceneCameraSelect" class="xui-select active-scene-camera-select" aria-label="Active camera"></select>
          </div>
          <div id="activeSceneCardVariant" class="active-scene-row"></div>
          <p id="activeSceneCardVariantDescription" class="active-scene-variant-description"></p>
        </section>
      `,
    },
    {
      id: "integratorControlsCard",
      title: "Integrator",
      note: "Renderer and settings",
      icon: "M9 2h6v2h2a1 1 0 0 1 1 1v2h2v6h-2v2a1 1 0 0 1-1 1h-2v2H9v-2H7a1 1 0 0 1-1-1v-2H4V7h2V5a1 1 0 0 1 1-1h2V2z M9 9v6h6V9H9z",
      open: true,
      bodyHTML: `
        <label class="xui-field">
          <span class="xui-field__label">Integrator</span>
          <select id="integrator" class="xui-select"></select>
        </label>
        <div class="row">
          <label class="xui-field">
            <span class="xui-field__label">Tile Size</span>
            <select id="tile_size" class="xui-select">
              <option value="auto">Auto</option>
              <option value="8">8</option>
              <option value="32" selected>32</option>
              <option value="64">64</option>
            </select>
          </label>
          <label class="xui-field">
            <span class="xui-field__label">Tile Order</span>
            <select id="tile_order" class="xui-select">
              <option value="random">Random</option>
              <option value="scanline">Scanline</option>
              <option value="radial_in">Radial In</option>
              <option value="radial_out">Radial Out</option>
              <option value="spiral_in">Spiral In</option>
              <option value="spiral_out">Spiral Out</option>
            </select>
          </label>
          <label class="xui-field">
            <span class="xui-field__label-stack">
              <span id="threadsLabelText" class="xui-field__label">Threads</span>
              <span id="threadsLabelSubtext" class="xui-field__sub">11 max</span>
            </span>
            <input id="threads" class="xui-input" type="number" min="0" max="256" value="0">
          </label>
        </div>
        <label class="xui-field">
          <span class="xui-field__label">Render Mode</span>
          <select id="renderMode" class="xui-select">
            <option value="direct">Direct</option>
            <option value="progressive" selected>Progressive</option>
            <option value="incremental">Incremental</option>
            <option value="interactive">Interactive</option>
          </select>
        </label>
        <div id="integratorControlsSection" hidden>
          <div id="integratorControls"></div>
        </div>
        <label class="xui-switch"><span class="xui-switch__label">Clear preview on render</span><input id="clearPreviewOnRender" type="checkbox"></label>
        <label class="xui-switch"><span class="xui-switch__label">Draft mode</span><input id="draftMode" type="checkbox"></label>
      `,
    },
    {
      id: "frameControlsCard",
      title: "Frame",
      note: "Output dimensions",
      icon: "M8 3H5a2 2 0 0 0-2 2v3 M21 8V5a2 2 0 0 0-2-2h-3 M3 16v3a2 2 0 0 0 2 2h3 M16 21h3a2 2 0 0 0 2-2v-3",
      open: true,
      bodyHTML: `
        <section class="frame-section">
          <h4 class="frame-section-title">Resolution Presets</h4>
          <div class="resolution-mode-seg" role="group" aria-label="Resolution mode filter">
            <button id="resolutionModeFilterAll" class="resolution-seg-btn active" type="button" data-mode="all" aria-pressed="true">All</button>
            <button id="resolutionModeFilterSquare" class="resolution-seg-btn" type="button" data-mode="square" aria-pressed="false">Square</button>
            <button id="resolutionModeFilterPortrait" class="resolution-seg-btn" type="button" data-mode="portrait" aria-pressed="false">Portrait</button>
            <button id="resolutionModeFilterLandscape" class="resolution-seg-btn" type="button" data-mode="landscape" aria-pressed="false">Landscape</button>
          </div>
          <div class="resolution-preset-table" role="group" aria-label="Resolution presets">
            <div id="resolutionPresetList" class="resolution-preset-list" role="listbox" aria-label="Resolution preset list"></div>
          </div>
          <button id="resolutionCustomRow" class="resolution-custom-chip" type="button" data-value="custom" aria-label="Custom dimensions" aria-selected="false">Custom</button>
          <select id="resolutionPreset" hidden aria-hidden="true" tabindex="-1"></select>
        </section>
        <section class="frame-section">
          <h4 class="frame-section-title">Custom Dimensions</h4>
          <div class="custom-dim-row">
            <label class="custom-dim-label" for="width">W</label>
            <input id="width" class="xui-input custom-dim-input" type="number" min="32" max="8192" value="500">
            <span class="custom-dim-sep" aria-hidden="true">×</span>
            <label class="custom-dim-label" for="height">H</label>
            <input id="height" class="xui-input custom-dim-input" type="number" min="32" max="8192" value="500">
          </div>
          <div id="customDimStats" class="custom-dim-stats"></div>
        </section>
      `,
    },
    {
      id: "textEditorControlsCard",
      title: "Text Editor",
      note: "Load and save source",
      icon: "M10 20l4-16 M18 9l3 3-3 3 M6 9l-3 3 3 3",
      open: true,
      hidden: true,
      bodyHTML: `
        <button id="loadSceneBtn" type="button">Load Selected Scene</button>
        <button id="saveSceneBtn" type="button">Save</button>
        <p id="editorOpStatus" class="editor-op-status" hidden></p>
      `,
    },
    {
      id: "qualityControlsCard",
      title: "Quality",
      note: "Sampling and bounce depth",
      icon: "M12 2l3 7h7l-5.5 4 2 7L12 16l-6.5 4 2-7L2 9h7z",
      open: true,
      bodyHTML: `
        <div class="row">
          <label class="sample-presets-group">
            <span>Samples</span>
            <div class="aa-presets-stack">
              <div class="aa-pill-row" role="group" aria-label="Sample presets">
                <button class="aa-pill xui-pill samples-pill" type="button" data-samples="2">2x</button>
                <button class="aa-pill xui-pill samples-pill" type="button" data-samples="4">4x</button>
                <button class="aa-pill xui-pill samples-pill" type="button" data-samples="10">10x</button>
                <button class="aa-pill xui-pill samples-pill" type="button" data-samples="100">100x</button>
              </div>
              <input id="samples" class="aa-custom-input" type="number" min="1" max="1024" value="1" aria-label="Custom samples">
            </div>
          </label>
          <label class="aa-presets-group">
            <span>Anti-Aliasing</span>
            <div class="aa-presets-stack">
              <div class="aa-pill-row" role="group" aria-label="AA presets">
                <button class="aa-pill xui-pill" type="button" data-aa="2">2x</button>
                <button class="aa-pill xui-pill" type="button" data-aa="4">4x</button>
                <button class="aa-pill xui-pill" type="button" data-aa="10">10x</button>
                <button class="aa-pill xui-pill" type="button" data-aa="25">25x</button>
              </div>
              <input id="aa" class="aa-custom-input" type="number" min="1" max="16" value="1" aria-label="Custom anti-aliasing">
            </div>
          </label>
          <label class="xui-field"><span class="xui-field__label">Sample Distribution</span><select id="sample_distribution" class="xui-select"><option value="grid">Grid Aligned</option><option value="random">Monte Carlo</option></select></label>
          <label class="xui-field"><span class="xui-field__label">Ray Depth</span><input id="rdepth" class="xui-input" type="number" min="1" max="4096" value="15"></label>
        </div>
      `,
    },
    {
      id: "toneMappingControlsCard",
      title: "Tone Mapping",
      note: "Preview and output mapping",
      icon: "M12 2v3 M12 19v3 M4.22 4.22l2.12 2.12 M17.66 17.66l2.12 2.12 M2 12h3 M19 12h3 M4.22 19.78l2.12-2.12 M17.66 6.34l2.12-2.12 M12 7a5 5 0 1 0 0 10A5 5 0 0 0 12 7z",
      open: true,
      bodyHTML: `
        <label class="xui-field">
          <span class="xui-field__label">Tone Mapping</span>
          <select id="toneMapping" class="xui-select">
            <option value="aces">ACES (Fitted)</option>
            <option value="agx">AgX</option>
            <option value="khronos_pbr">Khronos PBR Neutral</option>
            <option value="hable">Hable (Uncharted 2)</option>
            <option value="uchimura">Uchimura (Gran Turismo)</option>
            <option value="lottes">Lottes</option>
            <option value="reinhard">Reinhard</option>
            <option value="reinhard_luma">Reinhard (Luma)</option>
            <option value="cineon">Cineon</option>
            <option value="exponential">Exponential</option>
            <option value="mantiuk_2006">Mantiuk 2006</option>
            <option value="none">None</option>
          </select>
        </label>
        <div class="row" id="toneMappingParamsRow" hidden>
          <label id="toneMappingExposureControl" class="xui-field"><span class="xui-field__label">TM Exposure</span><input id="toneMappingExposure" class="xui-input" type="number" min="0.001" max="64" step="0.1" value="1.0"></label>
          <label id="toneMappingWhitePointControl" class="xui-field"><span class="xui-field__label">TM White Point</span><input id="toneMappingWhitePoint" class="xui-input" type="number" min="0.001" max="64" step="0.1" value="1.0"></label>
          <label id="toneMappingMantiukContrastControl" class="xui-field"><span class="xui-field__label">TM Contrast</span><input id="toneMappingMantiukContrast" class="xui-input" type="number" min="0" max="1" step="0.01" value="0.1"></label>
          <label id="toneMappingMantiukSaturationControl" class="xui-field"><span class="xui-field__label">TM Saturation</span><input id="toneMappingMantiukSaturation" class="xui-input" type="number" min="0" max="2" step="0.01" value="0.8"></label>
          <label id="toneMappingMantiukDetailControl" class="xui-field"><span class="xui-field__label">TM Detail</span><input id="toneMappingMantiukDetail" class="xui-input" type="number" min="1" max="99" step="1" value="1"></label>
        </div>
      `,
    },
    {
      id: "exportControlsCard",
      title: "Export",
      note: "Preview and export",
      icon: "M12 16l-4-4h3V4h2v8h3l-4 4z M4 20h16",
      open: true,
      bodyHTML: `
        <div id="interactivePreviewControls" hidden>
          <label for="interactivePreviewSpeed" class="xui-field">
            <span class="xui-field__label">Interactive Speed</span>
            <div class="interactive-speed-row">
              <input id="interactivePreviewSpeed" type="range" min="0.2" max="5.0" step="0.1" value="1.0">
              <code id="interactivePreviewSpeedValue" class="interactive-speed-value">1.0x</code>
            </div>
          </label>
          <button id="interactivePreviewSaveCameraBtn" class="action-btn xui-button xui-button--primary interactive-save-camera-btn is-disabled" type="button" disabled aria-disabled="true" title="Save current interactive camera as a new scene camera">Save Interactive Camera</button>
        </div>
        <label for="previewSampling" class="xui-field"><span class="xui-field__label">Preview Sampling</span><select id="previewSampling" class="xui-select preview-sampling"><option value="smooth" selected>Smooth</option><option value="nearest">Nearest</option></select></label>
        <label for="exportFormat" class="xui-field"><span class="xui-field__label">Export Format</span><select id="exportFormat" class="xui-select export-format"><option value="png">PNG</option><option value="jpg">JPG</option><option value="bmp">BMP</option><option value="tga">TGA</option><option value="exr">EXR</option><option value="hdr">HDR</option></select></label>
        <button id="download" class="reset-view-btn xui-button xui-button--secondary is-disabled export-download-link" type="button" disabled aria-disabled="true" aria-label="Export Render" title="Export Render">Export Render</button>
      `,
    },
    {
      id: "sceneLoadingControlsCard",
      title: "Loading Status",
      note: "Scene load activity",
      icon: "M22 12h-4l-3 9L9 3l-3 9H2",
      open: true,
      bodyHTML: `
        <div class="scene-loading-meta">
          <p class="scene-loading-row"><span class="scene-loading-key">State</span><span id="sceneLoadState" class="scene-loading-value scene-load-state-pill">idle</span></p>
          <p class="scene-loading-row"><span class="scene-loading-key">Message</span><span id="sceneLoadMessage" class="scene-loading-value">No active scene load.</span></p>
          <p class="scene-loading-row"><span class="scene-loading-key">Job</span><span id="sceneLoadJobId" class="scene-loading-value">-</span></p>
          <p class="scene-loading-row"><span class="scene-loading-key">Elapsed</span><span id="sceneLoadElapsed" class="scene-loading-value">00:00</span></p>
        </div>
      `,
    },
  ];

  function renderSidebarCards(target) {
    if (!(target instanceof HTMLElement)) return;
    target.innerHTML = "";
    target.appendChild(createSidebarModalHeader());
    const modalBody = document.createElement("div");
    modalBody.className = "controls-modal-body";
    modalBody.hidden = true;
    target.appendChild(modalBody);
    CARD_DEFS.forEach((config) => {
      target.appendChild(createSidebarCard(config));
    });
  }

  function upgradePillRows(presets) {
    const samplePresets = (presets && presets.samples) || [2, 4, 10, 100];
    const aaPresets = (presets && presets.aa) || [2, 4, 10, 25];

    const sampleRow = document.querySelector('.aa-pill-row[aria-label="Sample presets"]');
    if (sampleRow) {
      sampleRow.innerHTML = samplePresets.map(v =>
        `<button class="aa-pill xui-pill samples-pill" type="button" data-samples="${v}">${v}x</button>`
      ).join("");
    }

    const aaRow = document.querySelector('.aa-pill-row[aria-label="AA presets"]');
    if (aaRow) {
      aaRow.innerHTML = aaPresets.map(v =>
        `<button class="aa-pill xui-pill" type="button" data-aa="${v}">${v}x</button>`
      ).join("");
    }
  }

  global.XTracerSidebarCards = {
    renderSidebarCards,
    upgradePillRows,
    getCardDefs: () => CARD_DEFS,
  };
})(window);
