(function (global) {
  const widgets = global.XTracerWidgets || {};
  const dom = widgets.dom;
  const root = document.getElementById("showcaseRoot");
  const THEME_STORAGE_KEY = "xtracer-theme";
  const DARK_PALETTE_STORAGE_KEY = "xtracer-dark-palette";
  const LIGHT_PALETTE_STORAGE_KEY = "xtracer-light-palette";
  const DARK_PALETTES = [
    { value: "slate", label: "Slate", accent: "#3d65a3", soft: "#1d2e48" },
    { value: "crimson", label: "Crimson", accent: "#9a4350", soft: "#3a1f27" },
    { value: "graphite", label: "Graphite", accent: "#5a6474", soft: "#252c36" },
    { value: "emerald", label: "Emerald", accent: "#2f8a67", soft: "#153428" },
    { value: "ember", label: "Ember", accent: "#b45f3a", soft: "#41231a" },
    { value: "arctic", label: "Arctic", accent: "#4d7ea8", soft: "#1f3143" },
  ];
  const LIGHT_PALETTES = [
    { value: "coastal", label: "Coastal", accent: "#264f7d", soft: "#b9cde4" },
    { value: "amber", label: "Amber", accent: "#8a5a1e", soft: "#ead2ad" },
    { value: "sage", label: "Sage", accent: "#2d6a4f", soft: "#b9decd" },
    { value: "rose", label: "Rose", accent: "#995a6b", soft: "#efd1d9" },
    { value: "sky", label: "Sky", accent: "#2b6f9b", soft: "#b8dbee" },
    { value: "olive", label: "Olive", accent: "#5f712b", soft: "#dce5b8" },
  ];
  const SECTION_META = [
    { id: "overview", label: "Overview", num: "01" },
    { id: "theme-coverage", label: "Theme Coverage", num: "02" },
    { id: "tokens", label: "Tokens", num: "03" },
    { id: "theme-colors", label: "Theme Colors", num: "04" },
    { id: "widgets", label: "Widgets", num: "05" },
    { id: "compositions", label: "Compositions", num: "06" },
    { id: "adoption", label: "Adoption Status", num: "07" },
  ];
  let activeSectionId = SECTION_META[0].id;
  let sectionObserver = null;
  let manualNavTargetId = "";
  let manualNavUntil = 0;

  function currentShowcaseState() {
    return {
      theme: localStorage.getItem(THEME_STORAGE_KEY) || document.documentElement.getAttribute("data-theme") || "system",
      darkPalette: localStorage.getItem(DARK_PALETTE_STORAGE_KEY) || document.documentElement.getAttribute("data-dark-palette") || "slate",
      lightPalette: localStorage.getItem(LIGHT_PALETTE_STORAGE_KEY) || document.documentElement.getAttribute("data-light-palette") || "coastal",
    };
  }

  function syncShowcaseThemeAttributes(state) {
    const next = state || currentShowcaseState();
    document.documentElement.setAttribute("data-theme", next.theme);
    document.documentElement.setAttribute("data-dark-palette", next.darkPalette);
    document.documentElement.setAttribute("data-light-palette", next.lightPalette);
  }

  function applyShowcaseTheme(next) {
    const state = { ...currentShowcaseState(), ...(next || {}) };
    syncShowcaseThemeAttributes(state);
    localStorage.setItem(THEME_STORAGE_KEY, state.theme);
    localStorage.setItem(DARK_PALETTE_STORAGE_KEY, state.darkPalette);
    localStorage.setItem(LIGHT_PALETTE_STORAGE_KEY, state.lightPalette);
    render();
  }

  function section(id, title, intro, children) {
    const meta = SECTION_META.find((s) => s.id === id);
    return dom.el("section", {
      className: "showcase-section",
      attrs: { id },
      children: [
        dom.el("div", {
          className: "showcase-section-head",
          children: [
            meta ? dom.el("p", { className: "showcase-section-num", text: meta.num }) : null,
            dom.el("h2", { text: title }),
            intro ? dom.el("p", { className: "showcase-copy", text: intro }) : null,
          ],
        }),
        ...(Array.isArray(children) ? children : [children]),
      ],
    });
  }

  function demoCard(title, description, children, options) {
    const opts = options || {};
    return dom.el("section", {
      className: `showcase-specimen${opts.className ? ` ${opts.className}` : ""}`,
      children: [
        dom.el("div", {
          className: "showcase-specimen-head",
          children: [
            dom.el("h3", { text: title }),
            description ? dom.el("p", { text: description }) : null,
          ],
        }),
        dom.el("div", {
          className: opts.bodyClassName || "showcase-demo-row",
          children,
        }),
      ],
    });
  }

  function tokenSwatch(name, value) {
    return dom.el("div", {
      className: "showcase-token-swatch",
      children: [
        dom.el("div", {
          className: "showcase-token-chip",
          attrs: { style: `background:${value}` },
        }),
        dom.el("strong", { text: name }),
        dom.el("span", { className: "showcase-meta", text: value }),
      ],
    });
  }

  function paletteSwatch(title, palette, tone) {
    return dom.el("section", {
      className: "showcase-specimen showcase-palette-card",
      children: [
        dom.el("div", {
          className: "showcase-palette-head",
          children: [
            dom.el("div", {
              children: [
                dom.el("h3", { text: title }),
              ],
            }),
            widgets.createTag({ label: tone, tone: tone === "dark" ? "info" : "warning" }),
          ],
        }),
        dom.el("div", {
          className: "showcase-palette-pair",
          children: [
            tokenSwatch("Accent", palette.accent),
            tokenSwatch("Soft", palette.soft),
          ],
        }),
      ],
    });
  }

  function buildSidebar(state) {
    return dom.el("aside", {
      className: "showcase-sidebar",
      children: [
        dom.el("div", {
          className: "showcase-sidebar-card",
          children: [
            dom.el("img", {
              className: "showcase-sidebar-logo",
              attrs: { src: "/res/logo.svg", alt: "XTRACER" },
            }),
            dom.el("p", { className: "showcase-sidebar-kicker", text: "Widget Library Showcase" }),
            dom.el("p", {
              className: "showcase-copy showcase-sidebar-copy",
              text: "Reference surface for the shared web widget library and current visual language.",
            }),
            dom.el("div", {
              className: "showcase-sidebar-stats",
              children: [
                widgets.createStatHint({ label: "Theme", value: state.theme }),
                widgets.createStatHint({ label: "Light", value: state.lightPalette }),
                widgets.createStatHint({ label: "Dark", value: state.darkPalette }),
              ],
            }),
          ],
        }),
        dom.el("nav", {
          className: "showcase-sidebar-card showcase-sidebar-nav",
          attrs: { "aria-label": "Showcase sections" },
          children: [
            dom.el("p", { className: "showcase-sidebar-label", text: "Sections" }),
            dom.el("div", {
              className: "showcase-sidebar-links",
              children: SECTION_META.map((item) => dom.el("a", {
                className: `showcase-sidebar-link${item.id === activeSectionId ? " is-active" : ""}`,
                attrs: {
                  href: `#${item.id}`,
                  "aria-current": item.id === activeSectionId ? "location" : null,
                },
                children: [
                  dom.el("span", { className: "showcase-sidebar-link-num", text: item.num }),
                  dom.el("span", { text: item.label }),
                ],
              })),
            }),
            dom.el("button", {
              className: "showcase-back-top",
              attrs: { type: "button", "aria-label": "Scroll to top" },
              children: [
                dom.el("span", { attrs: { "aria-hidden": "true" }, text: "↑" }),
                dom.el("span", { text: "Back to top" }),
              ],
            }),
          ],
        }),
      ],
    });
  }

  function buildOverview() {
    return section(
      "overview",
      "Overview",
      "The widget library uses plain JavaScript DOM builders and layered CSS. The showcase should mirror production UI patterns instead of inventing a separate design system.",
      dom.el("div", {
        className: "showcase-grid showcase-grid--wide",
        children: [
          demoCard("How To Read This Page", "Use the sidebar to jump between library layers and composition examples.", [
            widgets.createTag({ label: "Plain JS", tone: "info" }),
            widgets.createTag({ label: "Layered CSS", tone: "neutral" }),
            widgets.createTag({ label: "Production-aligned", tone: "success" }),
          ]),
          demoCard("Library Scope", "The current library focuses on repeated primitives rather than full feature panels.", [
            widgets.createStatHint({ label: "Primitives", value: "Buttons, pills, tags, fields, cards, stats" }),
            widgets.createStatHint({ label: "Adoption", value: "Workspace, jobs, preview, filters, integrator UI" }),
          ]),
          demoCard("At a Glance", "Library and theme coverage summary.", [
            widgets.createStatHint({ label: "Sections", value: String(SECTION_META.length) }),
            widgets.createStatHint({ label: "Palettes", value: String(DARK_PALETTES.length + LIGHT_PALETTES.length) + " (" + LIGHT_PALETTES.length + " light · " + DARK_PALETTES.length + " dark)" }),
            widgets.createStatHint({ label: "Theme modes", value: "System · Light · Dark" }),
          ]),
        ],
      })
    );
  }

  function buildThemeCoverage(state) {
    return section(
      "theme-coverage",
      "Theme Coverage",
      "Switch the page between system, light, and dark modes and inspect the same root token model used by the main app.",
      [
        dom.el("div", {
          className: "showcase-controls-bar",
          children: [
            dom.el("span", { className: "showcase-controls-bar-label", text: "Theme mode" }),
            widgets.createToolbar({
              className: "showcase-theme-toolbar",
              children: [
                widgets.createPill({ label: "System", selectable: true, active: state.theme === "system", onClick: () => applyShowcaseTheme({ theme: "system" }) }),
                widgets.createPill({ label: "Light", selectable: true, active: state.theme === "light", onClick: () => applyShowcaseTheme({ theme: "light" }) }),
                widgets.createPill({ label: "Dark", selectable: true, active: state.theme === "dark", onClick: () => applyShowcaseTheme({ theme: "dark" }) }),
              ],
            }),
          ],
        }),
        demoCard("Theme State", "Palette selection and live token state.", [
          dom.el("div", {
            className: "showcase-grid",
            children: [
              widgets.createSelectField({
                label: "Light Palette",
                value: state.lightPalette,
                options: LIGHT_PALETTES.map((item) => ({ value: item.value, label: item.label })),
                onChange: (event) => applyShowcaseTheme({ lightPalette: event.target.value }),
              }),
              widgets.createSelectField({
                label: "Dark Palette",
                value: state.darkPalette,
                options: DARK_PALETTES.map((item) => ({ value: item.value, label: item.label })),
                onChange: (event) => applyShowcaseTheme({ darkPalette: event.target.value }),
              }),
            ],
          }),
          dom.el("div", {
            className: "showcase-demo-row",
            children: [
              widgets.createStatHint({ label: "Mode", value: state.theme }),
              widgets.createStatHint({ label: "Light", value: state.lightPalette }),
              widgets.createStatHint({ label: "Dark", value: state.darkPalette }),
            ],
          }),
          dom.el("div", {
            className: "showcase-token-grid showcase-token-grid--compact",
            children: [
              tokenSwatch("Background", "var(--bg)"),
              tokenSwatch("Panel", "var(--panel)"),
              tokenSwatch("Accent", "var(--accent)"),
              tokenSwatch("Accent Soft", "var(--accent-soft)"),
            ],
          }),
        ], { bodyClassName: "showcase-demo-stack" }),
      ]
    );
  }

  function buildTokens() {
    return section(
      "tokens",
      "Tokens",
      "These are the foundational tokens the widget layer depends on. Theme switching should modify these values rather than individual component rules.",
      dom.el("div", {
        className: "showcase-token-grid",
        children: [
          tokenSwatch("Accent", "var(--accent)"),
          tokenSwatch("Accent Soft", "var(--accent-soft)"),
          tokenSwatch("Panel", "var(--panel)"),
          tokenSwatch("Background", "var(--bg)"),
          tokenSwatch("Field", "var(--field-bg)"),
          tokenSwatch("Preview", "var(--preview-bg)"),
        ],
      })
    );
  }

  function buildThemeColors() {
    return section(
      "theme-colors",
      "Theme Colors",
      "Palette coverage for both light and dark theme families.",
      dom.el("div", {
        className: "showcase-grid",
        children: [
          ...LIGHT_PALETTES.map((palette) => paletteSwatch(palette.label, palette, "light")),
          ...DARK_PALETTES.map((palette) => paletteSwatch(palette.label, palette, "dark")),
        ],
      })
    );
  }

  function buildWidgets() {
    return section(
      "widgets",
      "Widgets",
      "Primitive controls and containers. These should align visually with the app’s existing language and be reused across dynamic surfaces.",
      dom.el("div", {
        className: "showcase-grid showcase-grid--widgets",
        children: [
          demoCard("Buttons", "Primary, secondary, ghost, danger, and icon actions.", [
            widgets.createButton({ label: "Render", variant: "primary" }),
            widgets.createButton({ label: "Queue", variant: "secondary" }),
            widgets.createButton({ label: "Ghost", variant: "ghost" }),
            widgets.createButton({ label: "Abort", variant: "danger" }),
            widgets.createIconButton({ title: "Move up", icon: widgets.dom.svgIcon("M8 12V4M8 4L5.4 6.6M8 4l2.6 2.6") }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-wrap" }),
          demoCard("Pills & Tags", "Selectable filters and semantic state badges.", [
            widgets.createPill({ label: "Landscape", selectable: true, active: true }),
            widgets.createPill({ label: "Portrait", selectable: true }),
            widgets.createTag({ label: "Running", tone: "success" }),
            widgets.createTag({ label: "Queued", tone: "warning" }),
            widgets.createTag({ label: "Error", tone: "error" }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-wrap" }),
          demoCard("Fields", "Form wrappers for numeric, select, boolean, and custom controls. createField is the base factory; createNumberField / createSelectField / createCheckboxField are typed wrappers built on top of it.", [
            widgets.createField({
              label: "Scene path",
              control: dom.el("input", { className: "xui-input", attrs: { type: "text", placeholder: "scenes/my_scene.ncf" } }),
              help: "Base factory — slot any control element.",
            }),
            widgets.createNumberField({ label: "Width", value: 1280, min: 32, max: 8192 }),
            widgets.createSelectField({
              label: "Integrator",
              value: "pathtracer",
              options: [
                { value: "pathtracer", label: "Pathtracer" },
                { value: "ao", label: "Ambient Occlusion" },
              ],
            }),
            widgets.createCheckboxField({ label: "Enable post filters", checked: true }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack showcase-demo-stack--form" }),
          demoCard("Toggle", "On/off switch control. createToggle produces a standalone xui-switch; createCheckboxField also emits xui-switch markup when used in a form context.", [
            widgets.createToggle({ label: "Enable feature", checked: true }),
            widgets.createToggle({ label: "Dark mode sync" }),
            widgets.createToggle({ label: "Disabled option", checked: true, disabled: true }),
            widgets.createToggle({ label: "Auto-scroll logs", checked: false, title: "Scroll to newest entry" }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack showcase-demo-stack--form" }),
          demoCard("Create Field", "Integrated name-and-create control for inline creation flows.", [
            typeof widgets.createCreateField === "function"
              ? widgets.createCreateField({
                inputId: "showcaseCreateFieldInput",
                buttonId: "showcaseCreateFieldBtn",
                placeholder: "Workspace name",
                inputLabel: "Workspace name for new workspace",
                buttonTitle: "Create workspace",
                buttonLabel: "Create workspace",
              })
              : dom.el("div", { text: "Create field widget unavailable." }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Stats & Progress", "Display primitives reused by settings, jobs, and preview stats. createStatHint creates a new element; renderStatHint upgrades an existing one in place.", [
            widgets.createSurface({
              className: "showcase-compact-surface",
              body: widgets.createStack({
                density: "compact",
                children: [
                  widgets.createStatHint({ label: "Threads In Use", value: "8 / 12" }),
                  (() => {
                    const node = dom.el("p");
                    widgets.renderStatHint(node, { label: "Active Workspace", value: "ws_main" });
                    return node;
                  })(),
                  widgets.createProgressBar({ label: "Render Progress", value: 0.64, valueText: "64%" }),
                ],
              }),
            }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Cards & Empty State", "Containers and empty messaging. createAccordionCard is an alias of createCollapsibleCard.", [
            widgets.createStack({
              density: "compact",
              children: [
                widgets.createPanelCard({
                  title: "Workspace",
                  note: "Status snapshot",
                  body: widgets.createStatHint({ label: "Active", value: "ws_main" }),
                }),
                widgets.createCollapsibleCard({
                  title: "Tone Mapping",
                  note: "Expandable controls",
                  open: false,
                  body: widgets.createSelectField({
                    label: "Mode",
                    value: "aces",
                    options: [
                      { value: "aces", label: "ACES" },
                      { value: "reinhard", label: "Reinhard" },
                    ],
                  }),
                }),
                widgets.createEmptyState({
                  title: "No jobs in queue",
                  message: "Start a render to populate the job list.",
                }),
              ],
            }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Panel Header", "Structured pane header widget for settings and about surfaces.", [
            typeof widgets.createPanelHeader === "function"
              ? widgets.createSurface({
                className: "showcase-compact-surface",
                body: widgets.createStack({
                  density: "compact",
                  children: [
                    widgets.createPanelHeader({
                      title: "Third-Party Licenses",
                      meta: widgets.createTag({ label: "About", tone: "info" }),
                    }),
                    dom.el("p", {
                      className: "showcase-copy",
                      text: "Used for section heads that need a stronger title treatment plus optional metadata.",
                    }),
                  ],
                }),
              })
              : dom.el("div", { text: "Panel header widget unavailable." }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Scene Cards", "Composite browser cards built from the shared widget layer.", [
            widgets.createStack({
              density: "compact",
              children: [
                widgets.createSceneCard({
                  sceneFile: "materials/cornell_box.ncf",
                  title: "Warm Cornell study",
                  sourceOrigin: "disk",
                  cameraCount: 3,
                  variantCount: 2,
                  hasVariants: true,
                  selected: true,
                }),
                widgets.createSceneCard({
                  sceneFile: "workspace/hero_draft.ncf",
                  title: "Hero draft",
                  sourceOrigin: "workspace",
                  dependsExternal: true,
                  cameraCount: 1,
                  variantCount: 0,
                  active: true,
                }),
              ],
            }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Camera & Variant Cards", "Companion browser cards for scene cameras and variants.", [
            widgets.createStack({
              density: "compact",
              children: [
                widgets.createCameraCard({
                  value: "cam_close",
                  label: "Hero Close-Up",
                  description: "Perspective camera",
                  selected: true,
                }),
                widgets.createVariantCard({
                  value: "fog_high",
                  label: "Fog High",
                  description: "Dense volumetric atmosphere with colder bounce light.",
                  active: true,
                }),
              ],
            }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Job Rows", "Settings job list entries with status, metrics, progress, and actions.", [
            widgets.createStack({
              density: "compact",
              children: [
                widgets.createJobRow({
                  id: "job_144",
                  state: "running",
                  pills: [widgets.createTag({ label: "running", tone: "success", className: "settings-job-state-pill state-running" })],
                  workspaceLabel: "ws draft_hero",
                  metrics: [
                    dom.el("span", { className: "settings-job-meta-pill", text: "8 threads" }),
                    dom.el("span", { className: "settings-job-meta-pill", text: "02:14" }),
                    dom.el("span", { className: "settings-job-meta-pill", text: "64.0%" }),
                  ],
                  progress: widgets.createProgressBar({ value: 0.64, className: "settings-job-progress" }),
                  subtext: "atrium_pathtrace.ncf · pathtracer",
                  controls: [
                    widgets.createIconButton({ title: "Abort", variant: "ghost", className: "settings-job-abort-btn", icon: widgets.dom.svgIcon("M5 5l6 6M11 5L5 11") }),
                  ],
                }),
                widgets.createJobRow({
                  id: "job_145",
                  state: "queued",
                  pills: [widgets.createTag({ label: "queued", tone: "warning", className: "settings-job-state-pill state-queued" })],
                  workspaceLabel: "ws batch_a",
                  metrics: [
                    dom.el("span", { className: "settings-job-meta-pill", text: "4 threads" }),
                    dom.el("span", { className: "settings-job-meta-pill", text: "-" }),
                    dom.el("span", { className: "settings-job-meta-pill", text: "0.0%" }),
                  ],
                  progress: widgets.createProgressBar({ value: 0, className: "settings-job-progress" }),
                  subtext: "fog_lab_draft.ncf · ao",
                  controls: [
                    dom.el("div", {
                      className: "settings-job-queue-controls",
                      children: [
                        widgets.createIconButton({ title: "Move up", variant: "ghost", className: "settings-job-queue-btn", icon: widgets.dom.svgIcon("M8 12V4M8 4L5.4 6.6M8 4l2.6 2.6") }),
                        widgets.createIconButton({ title: "Move down", variant: "ghost", className: "settings-job-queue-btn", icon: widgets.dom.svgIcon("M8 4v8M8 12l-2.6-2.6M8 12l2.6-2.6") }),
                      ],
                    }),
                    widgets.createIconButton({ title: "Abort", variant: "ghost", className: "settings-job-abort-btn", icon: widgets.dom.svgIcon("M5 5l6 6M11 5L5 11") }),
                  ],
                }),
              ],
            }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Dependencies", "Dependency cards used by the About pane.", [
            typeof widgets.createDependencyItem === "function"
              ? widgets.createStack({
                density: "compact",
                children: [
                  widgets.createDependencyItem({
                    name: "cgltf",
                    description: "Single-file glTF 2.0 loader used for importing compact scene assets into the renderer.",
                    usedIn: "xtcore",
                    license: "MIT",
                    url: "https://github.com/jkuhlmann/cgltf",
                  }),
                  widgets.createDependencyItem({
                    name: "crow",
                    description: "C++ HTTP and WebSocket server framework used by the web backend.",
                    usedIn: "xtracer-web",
                    license: "BSD-3-Clause",
                    url: "https://github.com/CrowCpp/Crow",
                  }),
                ],
              })
              : dom.el("div", { text: "Dependency widget unavailable." }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Workspace Cards", "Larger workspace containers for card and list views.", [
            widgets.createStack({
              density: "compact",
              children: [
                widgets.createWorkspaceCard({
                  active: true,
                  head: dom.el("header", {
                    className: "workspace-item-head",
                    children: [
                      dom.el("h3", { className: "workspace-item-title", text: "ws_main" }),
                      widgets.createTag({ label: "This Client", tone: "info", className: "workspace-item-state" }),
                      widgets.createTag({ label: "Active", tone: "success", className: "workspace-item-state" }),
                    ],
                  }),
                  preview: dom.el("div", {
                    className: "workspace-item-preview",
                    children: [
                      dom.el("div", {
                        className: "workspace-item-preview-empty xui-empty-state",
                        children: [
                          dom.el("div", { className: "xui-empty-state__message", text: "No render yet" }),
                        ],
                      }),
                    ],
                  }),
                  meta: dom.el("dl", {
                    className: "workspace-item-meta",
                    children: [
                      ["Scene", "atrium_pathtrace.ncf"],
                      ["Clients", "2"],
                      ["Drafts", "3"],
                      ["Job", "job_144"],
                    ].map(([key, value]) => dom.el("div", {
                      className: "workspace-item-meta-row",
                      children: [
                        dom.el("dt", { text: key }),
                        dom.el("dd", { text: value }),
                      ],
                    })),
                  }),
                  actions: dom.el("div", {
                    className: "workspace-item-actions",
                    children: [
                      widgets.createButton({ label: "Active", variant: "secondary", className: "workspace-item-action-btn", disabled: true }),
                      widgets.createButton({ label: "Delete", variant: "ghost", className: "workspace-item-action-btn" }),
                    ],
                  }),
                }),
                widgets.createWorkspaceCard({
                  listMode: true,
                  previewColumn: dom.el("div", {
                    className: "workspace-item-preview-col",
                    children: [
                      dom.el("div", {
                        className: "workspace-item-preview",
                        children: [
                          dom.el("div", {
                            className: "workspace-item-preview-loading",
                            children: [
                              dom.el("span", { className: "workspace-item-preview-spinner" }),
                            ],
                          }),
                        ],
                      }),
                      widgets.createTag({ label: "Rendering", tone: "success", className: "workspace-item-list-state" }),
                    ],
                  }),
                  main: dom.el("div", {
                    className: "workspace-item-list-main",
                    children: [
                      dom.el("header", {
                        className: "workspace-item-head",
                        children: [
                          dom.el("h3", { className: "workspace-item-title", text: "batch_a" }),
                          widgets.createTag({ label: "Idle", tone: "neutral", className: "workspace-item-state" }),
                        ],
                      }),
                      dom.el("p", { className: "workspace-item-list-scene", text: "fog_lab_draft.ncf" }),
                      dom.el("div", {
                        className: "workspace-item-meta-strip",
                        children: [
                          ["Users", "1"],
                          ["Drafts", "5"],
                          ["Job", "job_145"],
                        ].map(([key, value]) => dom.el("span", {
                          className: "workspace-item-meta-chip",
                          children: [
                            dom.el("span", { className: "workspace-item-meta-chip-key", text: key }),
                            dom.el("span", { className: "workspace-item-meta-chip-value", text: value }),
                          ],
                        })),
                      }),
                    ],
                  }),
                  actions: dom.el("div", {
                    className: "workspace-item-actions",
                    children: [
                      widgets.createButton({ label: "Use", variant: "primary", className: "workspace-item-action-btn" }),
                      widgets.createButton({ label: "Delete", variant: "ghost", className: "workspace-item-action-btn" }),
                    ],
                  }),
                }),
              ],
            }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Toolbar & Key Hints", "Compact action bars and interaction hints.", [
            widgets.createSurface({
              className: "showcase-compact-surface",
              body: widgets.createStack({
                density: "compact",
                children: [
                  widgets.createToolbar({
                    children: [
                      widgets.createButton({ label: "Apply", variant: "primary" }),
                      widgets.createButton({ label: "Reset", variant: "ghost" }),
                    ],
                  }),
                  widgets.createKeyHint({ keys: ["Ctrl", "R"], label: "Reload visual scene" }),
                ],
              }),
            }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Menus & Surfaces", "Larger reusable containers and action menus.", [
            widgets.createStack({
              density: "compact",
              children: [
                widgets.createSurface({
                  title: "Main Menu",
                  note: "App navigation widget",
                  className: "showcase-main-menu-surface",
                  body: typeof widgets.createMainTabs === "function"
                    ? widgets.createMainTabs()
                    : dom.el("div", { text: "Main menu widget unavailable." }),
                }),
                widgets.createSurface({
                  title: "Quick Actions",
                  note: "Shared surface container",
                  body: widgets.createStack({
                    density: "compact",
                    children: [
                      widgets.createStatHint({ label: "Scope", value: "Scene Browser" }),
                      widgets.createToolbar({
                        children: [
                          widgets.createButton({ label: "Open", variant: "secondary" }),
                          widgets.createButton({ label: "Delete", variant: "danger" }),
                        ],
                      }),
                    ],
                  }),
                }),
                widgets.createMenu({
                  className: "showcase-inline-menu",
                  children: [
                    widgets.createMenuItem({ label: "Set Active" }),
                    widgets.createMenuItem({ label: "Refetch" }),
                    widgets.createMenuItem({ label: "Reload" }),
                    widgets.createMenuDivider(),
                    widgets.createMenuItem({ label: "Delete", danger: true }),
                  ],
                }),
              ],
            }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Modal", "Confirmation dialog with backdrop, focus management, Escape key, and Promise-based API. Pass danger: true to invert default focus to the cancel button.", [
            (() => {
              const confirmBtn = widgets.createButton({ label: "Confirm Modal", variant: "secondary" });
              confirmBtn.addEventListener("click", () => {
                if (typeof widgets.showModal === "function") {
                  widgets.showModal({
                    title: "Confirm Action",
                    body: "This will apply the selected settings. Continue?",
                    confirmLabel: "Apply",
                  });
                }
              });
              const dangerBtn = widgets.createButton({ label: "Delete Modal", variant: "danger" });
              dangerBtn.addEventListener("click", () => {
                if (typeof widgets.showModal === "function") {
                  widgets.showModal({
                    title: "Delete Scene",
                    body: "Delete \"atrium_pathtrace.ncf\"? This cannot be undone.",
                    confirmLabel: "Delete",
                    danger: true,
                  });
                }
              });
              const wrap = dom.el("div", { className: "showcase-demo-row", children: [confirmBtn, dangerBtn] });
              return wrap;
            })(),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Toast", "Ephemeral status notifications with tone variants and auto-dismiss. showToast({ message, tone, duration }) appends to a host container fixed to the bottom-right corner.", [
            (() => {
              const tones = ["success", "error", "warning", "info", "neutral"];
              const wrap = dom.el("div", { className: "showcase-demo-wrap" });
              tones.forEach((tone) => {
                const btn = widgets.createButton({ label: tone, variant: "ghost" });
                btn.addEventListener("click", () => {
                  if (typeof widgets.showToast === "function") {
                    widgets.showToast({ message: `This is a ${tone} toast message.`, tone });
                  }
                });
                wrap.appendChild(btn);
              });
              return wrap;
            })(),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Tab Container", "Generic tab UI with keyboard navigation (arrow keys), ARIA roles, and onChange callback. createTabContainer({ tabs: [{id, label, content}] }).", [
            typeof widgets.createTabContainer === "function"
              ? widgets.createTabContainer({
                tabs: [
                  {
                    id: "info",
                    label: "Info",
                    content: widgets.createStack({
                      density: "compact",
                      children: [
                        widgets.createStatHint({ label: "Integrator", value: "Pathtracer" }),
                        widgets.createStatHint({ label: "Threads", value: "8 / 12" }),
                      ],
                    }),
                  },
                  {
                    id: "actions",
                    label: "Actions",
                    content: dom.el("div", {
                      className: "showcase-demo-wrap",
                      children: [
                        widgets.createButton({ label: "Apply", variant: "primary" }),
                        widgets.createButton({ label: "Reset", variant: "ghost" }),
                      ],
                    }),
                  },
                  {
                    id: "log",
                    label: "Log",
                    content: dom.el("p", { className: "showcase-copy", text: "No events yet." }),
                  },
                ],
              })
              : dom.el("div", { text: "Tab container widget unavailable." }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Dropdown", "Enhanced select control wrapping native <select> with keyboard nav and custom styling. enhanceSelects(root) is the batch variant that enhances all eligible selects under a root element.", [
            typeof widgets.enhanceSelect === "function"
              ? (() => {
                const container = dom.el("div", { className: "showcase-dropdown-host" });
                const select = dom.el("select", {
                  className: "xui-select",
                  children: [
                    dom.el("option", { attrs: { value: "pathtracer" }, text: "Pathtracer" }),
                    dom.el("option", { attrs: { value: "ao" }, text: "Ambient Occlusion" }),
                    dom.el("option", { attrs: { value: "direct" }, text: "Direct Light" }),
                  ],
                });
                container.appendChild(select);
                widgets.enhanceSelect(select);
                return container;
              })()
              : dom.el("div", { text: "Dropdown widget unavailable." }),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
          demoCard("Sampling Switch", "Toggle between smooth (bilinear) and nearest-neighbour canvas sampling. Used in the render preview and gallery toolbars. Arrow keys navigate between modes.", [
            (() => {
              const wrap = dom.el("div", { className: "showcase-demo-wrap" });
              const smoothSwitch = widgets.createSamplingSwitch({
                value: "smooth",
                onChange: (mode) => widgets.syncSamplingSwitch(smoothSwitch, mode),
              });
              const nearestSwitch = widgets.createSamplingSwitch({
                value: "nearest",
                onChange: (mode) => widgets.syncSamplingSwitch(nearestSwitch, mode),
              });
              wrap.appendChild(smoothSwitch);
              wrap.appendChild(nearestSwitch);
              return wrap;
            })(),
          ], { className: "showcase-specimen--widgets", bodyClassName: "showcase-demo-stack" }),
        ],
      })
    );
  }

  function buildTimelineDemo() {
    if (typeof widgets.createInfiniteGrid !== "function") return null;
    const wrapper = dom.el("div", { className: "showcase-specimen showcase-specimen--full" });
    const head = dom.el("div", {
      className: "showcase-specimen-head",
      children: [
        dom.el("h3", { text: "Infinite Grid / Timeline" }),
        dom.el("p", { text: "Pannable and zoomable canvas grid. Drag to pan, scroll to zoom. The playhead marks world x=0." }),
      ],
    });
    wrapper.appendChild(head);

    const controls = dom.el("div", {
      className: "showcase-demo-row showcase-demo-row--toolbar",
      children: [
        dom.el("span", { className: "showcase-meta", text: "Label suffix" }),
        (function () {
          const sel = document.createElement("select");
          sel.className = "xui-select";
          [["", "none"], ["f", "frames"], ["s", "seconds"], ["ms", "ms"]].forEach(([v, l]) => {
            const o = document.createElement("option");
            o.value = v; o.textContent = l;
            sel.appendChild(o);
          });
          return sel;
        })(),
        dom.el("span", { className: "showcase-meta", text: "Major every" }),
        (function () {
          const sel = document.createElement("select");
          sel.className = "xui-select";
          [4, 5, 8, 10].forEach((n) => {
            const o = document.createElement("option");
            o.value = n; o.textContent = n; o.selected = n === 5;
            sel.appendChild(o);
          });
          return sel;
        })(),
      ],
    });
    wrapper.appendChild(controls);

    let grid = null;
    function buildGrid() {
      if (grid && grid._gridDestroy) grid._gridDestroy();
      if (grid && grid.parentNode) grid.parentNode.removeChild(grid);
      const suffixSel = controls.querySelectorAll("select")[0];
      const majorSel  = controls.querySelectorAll("select")[1];
      grid = widgets.createInfiniteGrid({
        height: 240,
        baseStep: 60,
        majorEvery: majorSel ? parseInt(majorSel.value, 10) : 5,
        labelSuffix: suffixSel ? suffixSel.value : "",
        showPlayhead: true,
        showLabels: true,
        showAxisLine: true,
      });
      const hint = dom.el("span", { className: "xui-grid-view__hint", text: "drag · scroll" });
      grid.appendChild(hint);
      wrapper.appendChild(grid);
    }

    controls.querySelectorAll("select").forEach((sel) => {
      sel.addEventListener("change", buildGrid);
    });

    // Defer first build until layout is ready
    requestAnimationFrame(buildGrid);

    return wrapper;
  }

  function buildCompositions() {
    return section(
      "compositions",
      "Compositions",
      "Feature-owned layouts built from primitives, mirroring patterns already used in the app.",
      dom.el("div", {
        className: "showcase-grid",
        children: [
          buildTimelineDemo(),
          dom.el("section", {
            className: "showcase-specimen",
            children: [
              dom.el("div", { className: "showcase-specimen-head", children: [dom.el("h3", { text: "Sidebar Control Block" })] }),
              widgets.createCollapsibleCard({
                title: "Frame",
                note: "Output dimensions",
                body: widgets.createToolbar({
                  children: [
                    widgets.createPill({ label: "Square", selectable: true }),
                    widgets.createPill({ label: "Landscape", selectable: true, active: true }),
                    widgets.createNumberField({ label: "Width", value: 1920 }),
                  ],
                }),
              }),
            ],
          }),
          dom.el("section", {
            className: "showcase-specimen",
            children: [
              dom.el("div", { className: "showcase-specimen-head", children: [dom.el("h3", { text: "Settings Job Row" })] }),
              widgets.createPanelCard({
                body: [
                  widgets.createToolbar({
                    children: [
                      dom.el("code", { text: "job_144" }),
                      widgets.createTag({ label: "running", tone: "success" }),
                      widgets.createIconButton({ title: "Abort", icon: widgets.dom.svgIcon("M5 5l6 6M11 5L5 11") }),
                    ],
                  }),
                  widgets.createStatHint({ label: "Scene", value: "kitchen.scn" }),
                  widgets.createProgressBar({ value: 0.42, label: "Progress", valueText: "42.0%" }),
                ],
              }),
            ],
          }),
          dom.el("section", {
            className: "showcase-specimen",
            children: [
              dom.el("div", { className: "showcase-specimen-head", children: [dom.el("h3", { text: "Scene Browser Cards" })] }),
              widgets.createStack({
                density: "compact",
                children: [
                  widgets.createSceneCard({
                    sceneFile: "scene/atrium_pathtrace.ncf",
                    title: "Atrium daylight",
                    sourceOrigin: "disk",
                    hasVariants: true,
                    variantCount: 4,
                    cameraCount: 2,
                    selected: true,
                  }),
                  widgets.createSceneCard({
                    sceneFile: "scene/fog_lab_draft.ncf",
                    title: "Fog lab iteration",
                    sourceOrigin: "workspace",
                    dependsExternal: true,
                    variantCount: 1,
                    hasVariants: true,
                    cameraCount: 5,
                    active: true,
                  }),
                ],
              }),
            ],
          }),
        ],
      })
    );
  }

  function buildAdoption() {
    return section(
      "adoption",
      "Adoption Status",
      "Most repeated primitives route through the widget layer. Sidebar card bodies remain static HTML strings; modal and toast are wired into delete flows; the tab container drives the About pane.",
      dom.el("div", {
        className: "showcase-grid showcase-grid--wide",
        children: [
          demoCard("Fully Adopted", "Surfaces using only widget primitives.", [
            widgets.createStatHint({ label: "Sidebar Cards", value: "createCollapsibleCard" }),
            widgets.createStatHint({ label: "Jobs Panel", value: "createJobRow, renderStatHint" }),
            widgets.createStatHint({ label: "Scene Browser", value: "createSceneCard, createCameraCard, createVariantCard" }),
            widgets.createStatHint({ label: "Post Filters", value: "createPill, createTag, createIconButton, createField" }),
            widgets.createStatHint({ label: "Delete Flows", value: "showModal (scene + workspace), showToast (workspace)" }),
            widgets.createStatHint({ label: "About Pane", value: "createTabContainer — Overview / License / Third-Party tabs" }),
          ]),
          demoCard("Remaining Work", "Areas with inline DOM not yet converted.", [
            widgets.createStatHint({ label: "sidebar_cards.js bodies", value: "Card bodies built from static HTML strings via htmlToFragment" }),
          ]),
        ],
      })
    );
  }

  function render() {
    if (!root || !dom) return;
    const state = currentShowcaseState();
    syncShowcaseThemeAttributes(state);
    dom.mount(root, dom.el("div", {
      className: "showcase-layout",
      children: [
        buildSidebar(state),
        dom.el("div", {
          className: "showcase-main",
          children: [
            buildOverview(),
            buildThemeCoverage(state),
            buildTokens(),
            buildThemeColors(),
            buildWidgets(),
            buildCompositions(),
            buildAdoption(),
          ],
        }),
      ],
    }));
    if (widgets && typeof widgets.enhanceSelects === "function") {
      widgets.enhanceSelects(root);
    }
    bindSidebarNavigation();
    bindSidebarKeyNav();
    attachSectionObserver();
  }

  function updateActiveSection(nextId) {
    if (!nextId || nextId === activeSectionId) return;
    activeSectionId = nextId;
    const links = document.querySelectorAll(".showcase-sidebar-link");
    links.forEach((link) => {
      const isActive = link.getAttribute("href") === `#${nextId}`;
      link.classList.toggle("is-active", isActive);
      if (isActive) {
        link.setAttribute("aria-current", "location");
      } else {
        link.removeAttribute("aria-current");
      }
    });
  }

  function bindSidebarNavigation() {
    const links = document.querySelectorAll(".showcase-sidebar-link");
    links.forEach((link) => {
      link.addEventListener("click", (event) => {
        const href = String(link.getAttribute("href") || "");
        if (!href.startsWith("#")) return;
        const nextId = href.slice(1);
        const target = document.getElementById(nextId);
        if (!target) return;
        event.preventDefault();
        manualNavTargetId = nextId;
        manualNavUntil = Date.now() + 900;
        updateActiveSection(nextId);
        if (global.history && typeof global.history.replaceState === "function") {
          global.history.replaceState(null, "", href);
        } else {
          global.location.hash = href;
        }
        target.scrollIntoView({ behavior: "smooth", block: "start" });
      });
    });
    const backTop = document.querySelector(".showcase-back-top");
    if (backTop) {
      backTop.addEventListener("click", () => {
        window.scrollTo({ top: 0, behavior: "smooth" });
      });
    }
  }

  function bindSidebarKeyNav() {
    const nav = document.querySelector(".showcase-sidebar-nav");
    if (!nav) return;
    nav.addEventListener("keydown", (event) => {
      if (event.key !== "ArrowDown" && event.key !== "ArrowUp") return;
      const links = [...nav.querySelectorAll(".showcase-sidebar-link")];
      const idx = links.indexOf(document.activeElement);
      if (idx === -1) return;
      event.preventDefault();
      const next = event.key === "ArrowDown"
        ? links[(idx + 1) % links.length]
        : links[(idx - 1 + links.length) % links.length];
      next.focus();
    });
  }

  function attachSectionObserver() {
    if (sectionObserver) {
      sectionObserver.disconnect();
      sectionObserver = null;
    }
    const sections = SECTION_META
      .map((item) => document.getElementById(item.id))
      .filter(Boolean);
    if (!sections.length || !("IntersectionObserver" in global)) return;
    sectionObserver = new IntersectionObserver((entries) => {
      if (manualNavTargetId && Date.now() < manualNavUntil) {
        updateActiveSection(manualNavTargetId);
        return;
      }
      if (manualNavTargetId && Date.now() >= manualNavUntil) {
        manualNavTargetId = "";
      }
      const visible = entries
        .filter((entry) => entry.isIntersecting)
        .sort((a, b) => b.intersectionRatio - a.intersectionRatio);
      if (visible.length) {
        updateActiveSection(visible[0].target.id);
      }
    }, {
      root: null,
      rootMargin: "-18% 0px -55% 0px",
      threshold: [0.2, 0.4, 0.6],
    });
    sections.forEach((sectionNode) => sectionObserver.observe(sectionNode));
  }

  syncShowcaseThemeAttributes(currentShowcaseState());
  render();
})(window);
