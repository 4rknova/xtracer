(function () {
    'use strict';

    const runBtn     = document.getElementById('runBtn');
    const statusBar  = document.getElementById('furnaceStatus');
    const statusText = document.getElementById('furnaceStatusText');
    const summaryEl  = document.getElementById('furnaceSummary');
    const resultsEl  = document.getElementById('furnaceResults');

    // ---- integrators -------------------------------------------------------

    const INTEGRATORS = [
        'raytracer',
        'pathtracer',
        'pathtracer_mis',
    ];

    // ---- test group definitions --------------------------------------------

    const GROUPS = [
        {
            id: 'renderer',
            label: 'Lambert Renderer',
            desc: {
                expect: 'White (albedo=1) must be brighter than gray (albedo=0.5) with a ratio near 0.5, both channels balanced within 0.08.',
                pitfall: 'The raytracer integrator only casts shadow rays and misses indirect bounces, yielding black.',
            },
            check: checkRenderer,
            cases: [
                { key: 'white', label: 'white (albedo=1)' },
                { key: 'gray',  label: 'gray (albedo=0.5)' },
            ],
        },
        {
            id: 'thin_dielectric',
            label: 'Thin Dielectric',
            desc: {
                expect: 'A thin slab transmits through both surfaces; the result should be near 1.0 because Fresnel reflections at entry and exit are accounted for on both sides.',
                pitfall: 'Computing only one surface interface, forgetting back-face transmission, or double-applying Fresnel all drive the mean away from 1.0.',
            },
            check: checkThinDielectric,
            cases: [
                { key: 'clear',   label: 'clear (r=0)' },
                { key: 'frosted', label: 'frosted (r=0.24)' },
            ],
        },
        {
            id: 'rough_dielectric',
            label: 'Rough Dielectric',
            desc: {
                expect: 'In a white furnace a dielectric must return close to 1.0 — reflected plus transmitted energy equals incident energy. Both smooth (r=0.04) and rough (r=0.28) variants should look nearly white.',
                pitfall: 'Dropping the transmission lobe, incorrect Fresnel weighting, or missing the refracted ray contribution all cause the surface to appear artificially dark.',
            },
            check: checkRoughDielectric,
            cases: [
                { key: 'clear',   label: 'clear (r=0.04)' },
                { key: 'frosted', label: 'frosted (r=0.28)' },
            ],
        },
        {
            id: 'absorbing_rough_dielectric',
            label: 'Absorbing Rough Dielectric',
            desc: {
                expect: 'Transmitted tint must be positive, finite, and ordered b > g > r — the material absorbs red the most and blue the least.',
                pitfall: 'A sign error in the absorption coefficient inverts the channel order. Numerical issues in Beer-Lambert can produce non-finite values or push a channel to zero.',
            },
            check: checkAbsorbingRoughDielectric,
            isColor: true,
            cases: [
                { key: 'absorbed', label: 'absorbed tint' },
            ],
        },
        {
            id: 'principled_clearcoat',
            label: 'Principled Clearcoat',
            desc: {
                expect: 'The coated surface must not be darker than the bare base — clearcoat adds a specular lobe on top and should not attenuate the underlying diffuse contribution.',
                pitfall: 'Treating clearcoat as an opaque mask that multiplies down the base BRDF weight instead of compositing it additively causes the coated variant to darken.',
            },
            check: checkPrincipledClearcoat,
            cases: [
                { key: 'base',   label: 'base (cc=0)' },
                { key: 'coated', label: 'coated (cc=1)' },
            ],
        },
        {
            id: 'principled_anisotropy',
            label: 'Principled Anisotropy',
            desc: {
                expect: 'Anisotropy reshapes the specular highlight but must not change total reflected energy. Both isotropic (a=0) and anisotropic (a=0.85) should remain in a reasonable brightness range.',
                pitfall: 'Anisotropic GGX importance sampling that undersamples tangent-aligned directions at high anisotropy values produces dark bands and measurable energy loss.',
            },
            check: checkPrincipledAnisotropy,
            cases: [
                { key: 'isotropic',   label: 'isotropic (a=0)' },
                { key: 'anisotropic', label: 'anisotropic (a=0.85)' },
            ],
        },
        {
            id: 'subsurface',
            label: 'Subsurface Scattering',
            desc: {
                expect: 'Energy must be conserved across all depth variants. Thicker media attenuate more (thin.mean > thick.mean). The thin variant should show a b > g > r tint driven by the scattering radius parameters.',
                pitfall: 'Double-counting the surface BSDF when blending with the subsurface lobe, or a non-energy-conserving split between surface and subsurface contributions, causes values above 1.0 or incorrect tint ordering.',
            },
            check: checkSubsurface,
            cases: [
                { key: 'mostly_diffuse', label: 'mostly diffuse (sss=0.15 t=0.15)' },
                { key: 'thin',           label: 'thin (sss=0.75 t=0.20)' },
                { key: 'thick',          label: 'thick (sss=0.75 t=0.90)' },
            ],
        },
        {
            id: 'sheen',
            label: 'Sheen',
            desc: {
                expect: 'Sheen is a retroreflective micro-fiber lobe that adds glancing-angle energy. A high-sheen surface must not be significantly darker than a low-sheen surface.',
                pitfall: 'Compositing sheen as a multiplicative weight on the base lobe instead of adding it causes the surface to darken with increasing sheen strength.',
            },
            check: checkSheen,
            cases: [
                { key: 'low',  label: 'low (sh=0.05)' },
                { key: 'high', label: 'high (sh=0.85)' },
            ],
        },
        {
            id: 'thin_translucent',
            label: 'Thin Translucent',
            desc: {
                expect: 'Thin translucent uses a slab approximation for forward scattering. Thicker slabs attenuate more (thin.mean > thick.mean). The thin variant\'s green channel must dominate (g > r and g > b).',
                pitfall: 'Incorrect transmission direction for thin geometry, wrong colour channel order in the scattering coefficients, or failing to account for both reflected and transmitted contributions all cause failures.',
            },
            check: checkThinTranslucent,
            cases: [
                { key: 'low',   label: 'low (tr=0.18 t=0.10)' },
                { key: 'thin',  label: 'thin (tr=0.82 t=0.18)' },
                { key: 'thick', label: 'thick (tr=0.82 t=0.70)' },
            ],
        },
    ];

    // ---- check logic (mirrors white_furnace_test.cc verify_* functions) ----

    function pass()       { return { pass: true }; }
    function fail(reason) { return { pass: false, reason }; }
    function f(v)         { return (v == null) ? '—' : v.toFixed(4); }
    function chSpread(s)  { return Math.max(Math.abs(s.r-s.g), Math.abs(s.r-s.b), Math.abs(s.g-s.b)); }
    function radialOk(s, minEdgeRatio, maxRadialSpan) {
        return s && s.edge_ratio >= minEdgeRatio && s.radial_span <= maxRadialSpan;
    }

    function checkRenderer(d, intId) {
        const { white, gray } = d;
        if (!white || !gray) return fail('render failed');
        if (intId === 'raytracer') return fail('expected — this scene has no explicit lights; energy reaches the sphere only through indirect bounces from the environment. The raytracer only shoots shadow rays toward explicit lights, so every surface hit returns black regardless of albedo. The material is white but the integrator cannot gather any radiance to reflect.');
        if (white.samples < 50 || gray.samples < 50) return fail('insufficient hit samples');
        if (!(white.mean > gray.mean)) return fail(`white not > gray (${f(white.mean)} <= ${f(gray.mean)})`);
        const ratio = gray.mean / Math.max(white.mean, 1e-6);
        if (ratio < 0.35 || ratio > 0.65) return fail(`gray/white ratio ${f(ratio)} outside [0.35, 0.65]`);
        const ws = chSpread(white), gs = chSpread(gray);
        if (ws > 0.08 || gs > 0.08) return fail(`channel imbalance (white=${f(ws)} gray=${f(gs)})`);
        if (white.mean < 0.05 || white.mean > 1.1 || gray.mean < 0.02 || gray.mean > 0.9)
            return fail(`means out of range (white=${f(white.mean)} gray=${f(gray.mean)})`);
        return pass();
    }

    function checkRoughDielectric(d) {
        // Single-scatter GGX energy loss at grazing: clear ~0.83, frosted ~0.76.
        const minEdgeRatio = 0.72;
        const maxRadialSpan = 0.28;
        const { clear, frosted } = d;
        if (!clear || !frosted) return fail('render failed');
        if (clear.samples < 50 || frosted.samples < 50) return fail('insufficient hit samples');
        if (clear.mean < 0.15 || frosted.mean < 0.12) return fail(`too dark (clear=${f(clear.mean)} frosted=${f(frosted.mean)})`);
        if (Math.abs(clear.r-clear.g)>0.1 || Math.abs(clear.r-clear.b)>0.1 ||
            Math.abs(frosted.r-frosted.g)>0.1 || Math.abs(frosted.r-frosted.b)>0.1)
            return fail('channel imbalance too high');
        if (!radialOk(clear, minEdgeRatio, maxRadialSpan) || !radialOk(frosted, minEdgeRatio, maxRadialSpan))
            return fail(`radial bias too high (clear ratio=${f(clear.edge_ratio)} span=${f(clear.radial_span)}; frosted ratio=${f(frosted.edge_ratio)} span=${f(frosted.radial_span)})`);
        return pass();
    }

    function checkAbsorbingRoughDielectric(d) {
        const { absorbed } = d;
        if (!absorbed) return fail('material sample failed');
        if (!isFinite(absorbed.r)||!isFinite(absorbed.g)||!isFinite(absorbed.b)) return fail('non-finite tint');
        if (absorbed.r<=0||absorbed.g<=0||absorbed.b<=0) return fail('non-positive tint');
        if (!(absorbed.b > absorbed.g && absorbed.g > absorbed.r))
            return fail(`tint ordering invalid (r=${f(absorbed.r)} g=${f(absorbed.g)} b=${f(absorbed.b)})`);
        return pass();
    }

    function checkPrincipledClearcoat(d) {
        const { base, coated } = d;
        if (!base || !coated) return fail('render failed');
        if (base.samples < 50 || coated.samples < 50) return fail('insufficient hit samples');
        if (base.mean < 0.05 || coated.mean < 0.05 || coated.mean > 1.1)
            return fail(`out of range (base=${f(base.mean)} coated=${f(coated.mean)})`);
        if (coated.mean + 0.1 < base.mean)
            return fail(`clearcoat darkens (base=${f(base.mean)} coated=${f(coated.mean)})`);
        return pass();
    }

    function checkPrincipledAnisotropy(d) {
        const { isotropic, anisotropic } = d;
        if (!isotropic || !anisotropic) return fail('render failed');
        if (isotropic.samples < 50 || anisotropic.samples < 50) return fail('insufficient hit samples');
        if (isotropic.mean < 0.05 || anisotropic.mean < 0.05 || anisotropic.mean > 1.1)
            return fail(`out of range (iso=${f(isotropic.mean)} aniso=${f(anisotropic.mean)})`);
        return pass();
    }

    function checkThinDielectric(d) {
        const minEdgeRatio = 0.90;
        const maxRadialSpan = 0.12;
        const { clear, frosted } = d;
        if (!clear || !frosted) return fail('render failed');
        if (clear.samples < 50 || frosted.samples < 50) return fail('insufficient hit samples');
        if (clear.mean < 0.2 || frosted.mean < 0.18 || clear.mean > 1.1 || frosted.mean > 1.1)
            return fail(`out of range (clear=${f(clear.mean)} frosted=${f(frosted.mean)})`);
        if (!radialOk(clear, minEdgeRatio, maxRadialSpan) || !radialOk(frosted, minEdgeRatio, maxRadialSpan))
            return fail(`radial bias too high (clear ratio=${f(clear.edge_ratio)} span=${f(clear.radial_span)}; frosted ratio=${f(frosted.edge_ratio)} span=${f(frosted.radial_span)})`);
        return pass();
    }

    function checkSubsurface(d) {
        const { mostly_diffuse, thin, thick } = d;
        if (!mostly_diffuse || !thin || !thick) return fail('render failed');
        if (mostly_diffuse.samples < 50 || thin.samples < 50 || thick.samples < 50)
            return fail('insufficient hit samples');
        if (mostly_diffuse.mean < 0.05 || thin.mean < 0.05 || thick.mean < 0.02 ||
            mostly_diffuse.mean > 1.1   || thin.mean > 1.1   || thick.mean > 1.1)
            return fail(`out of range (low=${f(mostly_diffuse.mean)} thin=${f(thin.mean)} thick=${f(thick.mean)})`);
        if (!(thin.mean > thick.mean))
            return fail(`thickness attenuation invalid (thin=${f(thin.mean)} thick=${f(thick.mean)})`);
        if (!(thin.b > thin.g && thin.g > thin.r))
            return fail(`radius tint ordering invalid (r=${f(thin.r)} g=${f(thin.g)} b=${f(thin.b)})`);
        return pass();
    }

    function checkSheen(d) {
        const { low, high } = d;
        if (!low || !high) return fail('render failed');
        if (low.samples < 50 || high.samples < 50) return fail('insufficient hit samples');
        if (low.mean < 0.05 || high.mean < 0.05 || low.mean > 1.1 || high.mean > 1.1)
            return fail(`out of range (low=${f(low.mean)} high=${f(high.mean)})`);
        if (high.mean + 0.15 < low.mean)
            return fail(`sheen darkens (low=${f(low.mean)} high=${f(high.mean)})`);
        return pass();
    }

    function checkThinTranslucent(d) {
        const { low, thin, thick } = d;
        if (!low || !thin || !thick) return fail('render failed');
        if (low.samples < 50 || thin.samples < 50 || thick.samples < 50)
            return fail('insufficient hit samples');
        if (low.mean < 0.05 || thin.mean < 0.05 || thick.mean < 0.02 ||
            low.mean > 1.1   || thin.mean > 1.1   || thick.mean > 1.1)
            return fail(`out of range (low=${f(low.mean)} thin=${f(thin.mean)} thick=${f(thick.mean)})`);
        if (!(thin.mean > thick.mean))
            return fail(`thickness attenuation invalid (thin=${f(thin.mean)} thick=${f(thick.mean)})`);
        if (!(thin.g > thin.r && thin.g > thin.b))
            return fail(`green tint invalid (r=${f(thin.r)} g=${f(thin.g)} b=${f(thin.b)})`);
        return pass();
    }

    // ---- summary tracking --------------------------------------------------

    let totalChecks = 0;
    let passedChecks = 0;

    function updateSummary() {
        const completed = GROUPS.reduce((n, g) => n + g._total, 0);
        const failed    = totalChecks - passedChecks;
        summaryEl.textContent = `${passedChecks}\u2009/\u2009${totalChecks} passed`;

        const progressWrap = document.getElementById('furnaceProgressWrap');
        const progressFill = document.getElementById('furnaceProgressFill');
        if (progressWrap && progressFill && totalChecks > 0) {
            progressWrap.hidden = false;
            progressFill.style.width = `${(completed / totalChecks * 100).toFixed(1)}%`;
        }

        if (totalChecks > 0 && failed === 0) {
            statusBar.className = 'furnace-bar furnace-bar--pass';
        } else if (failed > 0) {
            statusBar.className = 'furnace-bar furnace-bar--fail';
        }
    }

    // ---- DOM builders ------------------------------------------------------

    const rowMap      = new Map(); // key: `${group.id}__${intId}` → <tr>
    const groupTagMap = new Map(); // key: group.id → tag <span>

    function updateGroupTag(group) {
        const tag = groupTagMap.get(group.id);
        if (!tag) return;
        const failed = group._total - group._pass;
        const allDone = group._total === INTEGRATORS.length;
        if (group._total === 0) {
            tag.className = 'xui-tag furnace-group-tag';
            tag.textContent = '';
            return;
        }
        if (allDone && failed === 0) {
            tag.className = 'xui-tag xui-tag--success furnace-group-tag';
            tag.textContent = 'All pass';
        } else if (failed > 0) {
            tag.className = 'xui-tag xui-tag--error furnace-group-tag';
            tag.textContent = `${failed} fail`;
        } else {
            tag.className = 'xui-tag furnace-group-tag';
            tag.textContent = `${group._pass}\u2009/\u2009${group._total}`;
        }
    }

    function buildSkeleton() {
        resultsEl.innerHTML = '';
        rowMap.clear();
        groupTagMap.clear();
        totalChecks = 0;
        passedChecks = 0;
        summaryEl.textContent = '';

        for (const group of GROUPS) {
            group._pass  = 0;
            group._total = 0;

            const section = document.createElement('div');
            section.className = 'furnace-group xui-card';

            // card head
            const head = document.createElement('div');
            head.className = 'xui-card__head xui-card__head--panel';

            const title = document.createElement('h3');
            title.className = 'xui-card__title';
            title.textContent = group.label;
            head.appendChild(title);

            const tag = document.createElement('span');
            tag.className = 'xui-tag furnace-group-tag';
            groupTagMap.set(group.id, tag);
            head.appendChild(tag);

            section.appendChild(head);

            if (group.desc) {
                const info = document.createElement('div');
                info.className = 'furnace-group-info';
                const desc = document.createElement('p');
                desc.className = 'furnace-group-desc';
                desc.innerHTML =
                    `<span class="furnace-desc-expect">${group.desc.expect}</span> `
                    + `<span class="furnace-desc-pitfall"><strong>Pitfall:</strong> ${group.desc.pitfall}</span>`;
                info.appendChild(desc);
                section.appendChild(info);
            }

            const table = document.createElement('table');
            table.className = 'furnace-table';

            const thead = document.createElement('thead');
            const hrow = document.createElement('tr');
            for (const h of ['Integrator', ...group.cases.map(c => c.label), 'Status', 'Detail']) {
                const th = document.createElement('th');
                th.textContent = h;
                hrow.appendChild(th);
            }
            thead.appendChild(hrow);
            table.appendChild(thead);

            const tbody = document.createElement('tbody');
            for (const intId of INTEGRATORS) {
                const row = document.createElement('tr');
                row.className = 'furnace-row--pending';

                const intCell = document.createElement('td');
                intCell.className = 'furnace-cell-integrator';
                intCell.textContent = intId;
                row.appendChild(intCell);

                const span = group.cases.length + 2;
                const placeholder = document.createElement('td');
                placeholder.colSpan = span;
                placeholder.className = 'furnace-cell-pending';
                placeholder.innerHTML = '<span class="furnace-spinner"></span>';
                row.appendChild(placeholder);

                rowMap.set(`${group.id}__${intId}`, row);
                tbody.appendChild(row);
                totalChecks++;
            }
            table.appendChild(tbody);
            section.appendChild(table);
            resultsEl.appendChild(section);
        }
    }

    function fillRow(group, intId, data) {
        const row = rowMap.get(`${group.id}__${intId}`);
        if (!row) return;

        const cases = (data && data.cases) ? data.cases : {};
        const result = group.check(cases, intId);
        if (result.pass) passedChecks++;

        row.className = result.pass ? 'furnace-row--pass' : 'furnace-row--fail';

        // Remove placeholder
        while (row.children.length > 1) row.removeChild(row.lastChild);

        for (const c of group.cases) {
            const td = document.createElement('td');
            td.className = 'furnace-cell-val';
            const d = cases[c.key];
            if (!d) {
                td.textContent = '—';
            } else if (group.isColor) {
                td.innerHTML = `<span class="furnace-rgb">r=${f(d.r)}&nbsp;g=${f(d.g)}&nbsp;b=${f(d.b)}</span>`;
            } else {
                if (d.img) {
                    const img = document.createElement('img');
                    img.src = d.img;
                    img.className = 'furnace-thumb';
                    img.alt = c.key;
                    td.appendChild(img);
                }
                const stats = document.createElement('div');
                stats.innerHTML = `<span class="furnace-mean">${f(d.mean)}</span>`
                    + `<span class="furnace-rgb">&nbsp;(r=${f(d.r)}&nbsp;g=${f(d.g)}&nbsp;b=${f(d.b)})</span>`;
                if (d.edge_ratio != null && d.radial_span != null) {
                    stats.innerHTML += `<span class="furnace-rgb">&nbsp;edge/center=${f(d.edge_ratio)}&nbsp;span=${f(d.radial_span)}</span>`;
                }
                td.appendChild(stats);
            }
            row.appendChild(td);
        }

        group._total++;
        if (result.pass) group._pass++;
        updateGroupTag(group);

        const statusCell = document.createElement('td');
        statusCell.className = 'furnace-cell-status';
        statusCell.innerHTML = result.pass
            ? '<span class="xui-tag xui-tag--success">PASS</span>'
            : '<span class="xui-tag xui-tag--error">FAIL</span>';
        row.appendChild(statusCell);

        const detailCell = document.createElement('td');
        detailCell.className = 'furnace-cell-detail';
        detailCell.textContent = result.pass ? '' : (result.reason || '');
        row.appendChild(detailCell);

        updateSummary();
    }

    function markRowError(group, intId, err) {
        const row = rowMap.get(`${group.id}__${intId}`);
        if (!row) return;
        row.className = 'furnace-row--fail';
        while (row.children.length > 1) row.removeChild(row.lastChild);
        const td = document.createElement('td');
        td.colSpan = group.cases.length + 2;
        td.className = 'furnace-cell-detail';
        td.textContent = `Error: ${err.message}`;
        row.appendChild(td);
        updateSummary();
    }

    // ---- run ---------------------------------------------------------------

    function setStatus(cls, text) {
        statusBar.className = 'furnace-bar ' + cls;
        statusText.textContent = text;
    }

    async function runTests() {
        runBtn.disabled = true;
        setStatus('furnace-bar--running', `Running ${GROUPS.length * INTEGRATORS.length} tests…`);
        buildSkeleton();

        const promises = [];
        for (const group of GROUPS) {
            for (const intId of INTEGRATORS) {
                promises.push(
                    fetch(`/api/tests/furnace/${group.id}/${intId}`)
                        .then(r => { if (!r.ok) throw new Error(`HTTP ${r.status}`); return r.json(); })
                        .then(data => fillRow(group, intId, data))
                        .catch(err => markRowError(group, intId, err))
                );
            }
        }

        await Promise.allSettled(promises);

        const failed = totalChecks - passedChecks;
        if (failed === 0) {
            setStatus('furnace-bar--pass', `All ${totalChecks} checks passed.`);
        } else {
            setStatus('furnace-bar--fail', `${failed} of ${totalChecks} checks failed.`);
        }
        runBtn.disabled = false;
    }

    runBtn.addEventListener('click', runTests);
}());
