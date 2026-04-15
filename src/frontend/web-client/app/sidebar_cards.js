(function (global) {
  function htmlToFragment(html) {
    const template = document.createElement("template");
    template.innerHTML = html.trim();
    return template.content;
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
      open: true,
      bodyHTML: `
        <section class="settings-section workspace-info-section active-scene-sidebar">
          <div id="activeSceneCardScene" class="active-scene-header"></div>
          <p id="activeSceneCardDescription" class="active-scene-description">-</p>
          <div id="activeSceneCardSource" class="active-scene-row"></div>
          <div id="activeSceneCardCamera" class="active-scene-row"></div>
          <div id="activeSceneCardVariant" class="active-scene-row"></div>
          <p id="activeSceneCardVariantDescription" class="active-scene-variant-description"></p>
        </section>
      `,
    },
    {
      id: "integratorControlsCard",
      title: "Integrator",
      note: "Renderer and settings",
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
        <label class="xui-field xui-field--checkbox">
          <span class="xui-field__check-row">
            <span class="xui-field__check-label">Clear preview on render</span>
            <input id="clearPreviewOnRender" type="checkbox">
          </span>
        </label>
      `,
    },
    {
      id: "frameControlsCard",
      title: "Frame",
      note: "Output dimensions",
      open: true,
      bodyHTML: `
        <section class="frame-section">
          <h4 class="frame-section-title">Resolution Presets</h4>
          <div class="resolution-mode-filter-row" role="group" aria-label="Resolution mode filter">
            <button id="resolutionModeFilterAll" class="aa-pill xui-pill active" type="button" data-mode="all" aria-pressed="true">All</button>
            <button id="resolutionModeFilterSquare" class="aa-pill xui-pill" type="button" data-mode="square" aria-pressed="false">Square</button>
            <button id="resolutionModeFilterPortrait" class="aa-pill xui-pill" type="button" data-mode="portrait" aria-pressed="false">Portrait</button>
            <button id="resolutionModeFilterLandscape" class="aa-pill xui-pill" type="button" data-mode="landscape" aria-pressed="false">Landscape</button>
          </div>
          <div class="resolution-preset-table" role="group" aria-label="Resolution presets">
            <div class="resolution-preset-head" aria-hidden="true">
              <span>Name</span>
              <span>Mode</span>
              <span>Aspect</span>
              <span>W</span>
              <span>H</span>
            </div>
            <div id="resolutionPresetList" class="resolution-preset-list" role="listbox" aria-label="Resolution preset list"></div>
          </div>
          <select id="resolutionPreset" hidden aria-hidden="true" tabindex="-1"></select>
        </section>
        <section class="frame-section">
          <h4 class="frame-section-title">Custom Dimensions</h4>
          <div class="row">
            <label class="xui-field">
              <span class="xui-field__label">Width</span>
              <input id="width" class="xui-input" type="number" min="32" max="8192" value="500">
            </label>
            <label class="xui-field">
              <span class="xui-field__label">Height</span>
              <input id="height" class="xui-input" type="number" min="32" max="8192" value="500">
            </label>
          </div>
        </section>
      `,
    },
    {
      id: "visualControlsCard",
      title: "3D View",
      note: "Camera, projection, viewport",
      open: true,
      hidden: true,
      bodyClass: "visual-controls-card-body",
      bodyHTML: `
        <label for="visualCamera" class="xui-field">
          <span class="xui-field__label">Camera</span>
          <select id="visualCamera" class="xui-select visual-camera-inline" aria-label="Visual camera"></select>
        </label>
        <label for="visualProjection" class="xui-field">
          <span class="xui-field__label">Projection</span>
          <select id="visualProjection" class="xui-select visual-projection-inline" aria-label="Visual projection">
            <option value="perspective">Perspective</option>
            <option value="isometric">Isometric</option>
          </select>
        </label>
        <label for="visualSceneScale" class="xui-field">
          <span class="xui-field__label">Scene Scale</span>
          <input id="visualSceneScale" class="xui-input visual-scene-scale-inline" type="number" min="0.01" max="100" step="0.1" value="1" aria-label="3D scene scale multiplier">
        </label>
        <button id="visualLoadBtn" class="action-btn xui-button xui-button--primary visual-load-btn" type="button" title="Reload visual scene">Load Visual</button>
        <label class="xui-field xui-field--checkbox" title="Show or hide visual grid">
          <span class="xui-field__check-row">
            <span class="xui-field__check-label">Grid</span>
            <input id="visualShowGrid" type="checkbox" checked>
          </span>
        </label>
        <label class="xui-field xui-field--checkbox" title="Show scene-wide BVH boxes">
          <span class="xui-field__check-row">
            <span class="xui-field__check-label">Global BVH</span>
            <input id="visualShowGlobalBvh" type="checkbox">
          </span>
        </label>
        <label class="xui-field xui-field--checkbox" title="Show mesh-local BVH boxes">
          <span class="xui-field__check-row">
            <span class="xui-field__check-label">Mesh BVH</span>
            <input id="visualShowMeshBvh" type="checkbox">
          </span>
        </label>
        <section class="visual-controls-info" aria-label="3D controls help">
          <h4 class="visual-controls-title">Controls</h4>
          <div class="visual-controls-list">
            <div class="visual-controls-item"><span class="visual-controls-key"><kbd>Left Click</kbd> / <kbd>Tap</kbd></span><small class="control-hint">Select object</small></div>
            <div class="visual-controls-item"><span class="visual-controls-key"><kbd>Left Drag</kbd> / <kbd>1-Finger Drag</kbd></span><small class="control-hint">Orbit camera</small></div>
            <div class="visual-controls-item"><span class="visual-controls-key"><kbd>Right Drag</kbd> / <kbd>Middle Drag</kbd> / <kbd>2-Finger Drag</kbd></span><small class="control-hint">Pan camera</small></div>
            <div class="visual-controls-item"><span class="visual-controls-key"><kbd>Wheel</kbd> / <kbd>Pinch</kbd></span><small class="control-hint">Zoom</small></div>
            <div class="visual-controls-item"><span class="visual-controls-key"><kbd>Ctrl</kbd> + <kbd>Left Drag</kbd></span><small class="control-hint">Move selected object</small></div>
            <div class="visual-controls-item"><span class="visual-controls-key"><kbd>F</kbd></span><small class="control-hint">Focus selected object</small></div>
            <div class="visual-controls-item"><span class="visual-controls-key"><kbd>M</kbd></span><small class="control-hint">Jump to selected scene camera</small></div>
            <div class="visual-controls-item"><span class="visual-controls-key">Axis gizmo click</span><small class="control-hint">Snap view to axis</small></div>
          </div>
        </section>
      `,
    },
    {
      id: "textEditorControlsCard",
      title: "Text Editor",
      note: "Load and save source",
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
      id: "postFiltersControlsCard",
      title: "Post Filters",
      note: "Filter chain order",
      open: true,
      bodyHTML: `
        <label class="xui-field xui-field--checkbox" for="postFiltersEnabled">
          <span class="xui-field__check-row">
            <span class="xui-field__check-label">Enable Post Filters</span>
            <input id="postFiltersEnabled" type="checkbox" checked>
          </span>
        </label>
        <div class="post-filters-toolbar">
          <select id="postFilterType" class="xui-select" aria-label="Filter to add">
            <option value="desaturate">Desaturate</option>
            <option value="chromatic_aberration">Chromatic Aberration</option>
            <option value="vignette">Vignette</option>
            <option value="film_grain">Film Grain</option>
            <option value="denoise">Bilateral Denoise</option>
            <option value="fxaa">FXAA</option>
            <option value="sharpen">Sharpen</option>
            <option value="brightness">Brightness</option>
            <option value="contrast">Contrast</option>
            <option value="raindrops_lens">Raindrops on Lens</option>
          </select>
          <button id="postFilterAddBtn" class="action-btn xui-button xui-button--primary post-filter-add-btn" type="button" title="Add to chain" aria-label="Add to chain"><span aria-hidden="true">+</span></button>
          <button id="postFiltersRecalcBtn" class="action-btn xui-button xui-button--primary post-filter-recalc-btn" type="button" title="Recalculate post filters preview">Recalculate</button>
        </div>
        <div id="postFiltersChain" class="post-filters-chain" aria-live="polite"></div>
      `,
    },
    {
      id: "exportControlsCard",
      title: "Export",
      note: "Preview and export",
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
    {
      id: "logsControlsCard",
      title: "Log Filters",
      note: "Live log filters",
      open: true,
      hidden: true,
      bodyHTML: `
        <section class="log-filter-list" aria-labelledby="logFilterHeading">
          <div class="log-filter-list-head">
            <h3 id="logFilterHeading">Message Types</h3>
            <span class="log-filter-list-note">Live visibility</span>
          </div>
          <div class="log-filter-list-grid" aria-label="Log message type filters">
            <label class="log-filter-item log-filter-item-debug"><span class="log-filter-item-main"><span class="log-filter-item-dot" aria-hidden="true"></span><span class="log-filter-item-label">Debug</span></span><input id="logFilterDebug" type="checkbox"></label>
            <label class="log-filter-item log-filter-item-message"><span class="log-filter-item-main"><span class="log-filter-item-dot" aria-hidden="true"></span><span class="log-filter-item-label">Message</span></span><input id="logFilterMessage" type="checkbox" checked></label>
            <label class="log-filter-item log-filter-item-warning"><span class="log-filter-item-main"><span class="log-filter-item-dot" aria-hidden="true"></span><span class="log-filter-item-label">Warning</span></span><input id="logFilterWarning" type="checkbox" checked></label>
            <label class="log-filter-item log-filter-item-error"><span class="log-filter-item-main"><span class="log-filter-item-dot" aria-hidden="true"></span><span class="log-filter-item-label">Error</span></span><input id="logFilterError" type="checkbox" checked></label>
            <label class="log-filter-item log-filter-item-ui"><span class="log-filter-item-main"><span class="log-filter-item-dot" aria-hidden="true"></span><span class="log-filter-item-label">UI</span></span><input id="logFilterUi" type="checkbox" checked></label>
          </div>
        </section>
      `,
    },
    {
      id: "workspaceControlsCard",
      title: "Workspace",
      note: "Workspace status",
      open: true,
      hidden: true,
      bodyHTML: `
        <section class="settings-section workspace-info-section">
          <div class="workspace-info-group">
            <p id="workspaceActiveHint" class="workspace-active-hint">Active workspace: -</p>
            <p id="workspaceCountHint" class="workspace-active-hint">Workspaces: 0</p>
          </div>
        </section>
      `,
    },
    {
      id: "JobsControlsCard",
      title: "Jobs",
      note: "Server active/queued jobs",
      open: true,
      hidden: true,
      bodyHTML: `
        <section class="settings-section workspace-info-section settings-jobs-section">
          <div
            id="settingsJobsThreadGraph"
            class="settings-jobs-graph"
            role="img"
            aria-label="Threads in use over the last minute"
          ></div>
          <p id="settingsJobsUpdated" class="workspace-active-hint">
            <span class="workspace-active-label">Updated</span>
            <code class="workspace-active-value">-</code>
          </p>
          <p id="settingsJobsThreadsUsage" class="workspace-active-hint">
            <span class="workspace-active-label">Threads In Use</span>
            <code class="workspace-active-value">-</code>
          </p>
          <div id="settingsJobsList" class="settings-jobs-list" aria-live="polite"></div>
        </section>
      `,
    },
  ];

  function renderSidebarCards(target) {
    if (!(target instanceof HTMLElement)) return;
    target.innerHTML = "";
    CARD_DEFS.forEach((config) => {
      target.appendChild(createSidebarCard(config));
    });
  }

  global.XTracerSidebarCards = {
    renderSidebarCards,
  };
})(window);
