(function () {
    'use strict';

    const canvas   = document.getElementById('samplingCanvas');
    const status   = document.getElementById('samplingStatus');
    const methodSel = document.getElementById('methodSelect');
    const countRange = document.getElementById('countRange');
    const countVal   = document.getElementById('countVal');
    const regenBtn   = document.getElementById('regenerateBtn');

    const paramRow   = document.getElementById('paramRow');
    const paramLabel = document.getElementById('paramLabel');
    const paramRange = document.getElementById('paramRange');
    const paramVal   = document.getElementById('paramVal');

    const paramXRow  = document.getElementById('paramXRow');
    const paramXRange = document.getElementById('paramXRange');
    const paramXVal  = document.getElementById('paramXVal');

    const paramYRow  = document.getElementById('paramYRow');
    const paramYRange = document.getElementById('paramYRange');
    const paramYVal  = document.getElementById('paramYVal');

    const expRow   = document.getElementById('expRow');
    const expRange = document.getElementById('expRange');
    const expVal   = document.getElementById('expVal');

    const coneRow   = document.getElementById('coneRow');
    const coneRange = document.getElementById('coneRange');
    const coneVal   = document.getElementById('coneVal');

    // --- Three.js scene setup ---
    const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
    renderer.setPixelRatio(window.devicePixelRatio);

    const scene  = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(45, 1, 0.01, 100);
    camera.position.set(2.2, 1.4, 2.2);
    camera.lookAt(0, 0, 0);

    // Hemisphere reference sphere (wireframe)
    const sphereGeo  = new THREE.SphereGeometry(1.0, 32, 32);
    const sphereMat  = new THREE.MeshBasicMaterial({ color: 0x334455, wireframe: true, opacity: 0.15, transparent: true });
    const sphereMesh = new THREE.Mesh(sphereGeo, sphereMat);
    scene.add(sphereMesh);

    // Equatorial disc
    const discGeo  = new THREE.CircleGeometry(1.0, 64);
    const discMat  = new THREE.MeshBasicMaterial({ color: 0x445566, opacity: 0.12, transparent: true, side: THREE.DoubleSide });
    const discMesh = new THREE.Mesh(discGeo, discMat);
    discMesh.rotation.x = Math.PI / 2;
    scene.add(discMesh);

    // Axes helper
    const axes = new THREE.AxesHelper(1.2);
    scene.add(axes);

    // Points geometry (replaced on each fetch)
    let pointsMesh = null;

    function buildPoints(positions, colors) {
        if (pointsMesh) {
            scene.remove(pointsMesh);
            pointsMesh.geometry.dispose();
            pointsMesh.material.dispose();
            pointsMesh = null;
        }
        const geo = new THREE.BufferGeometry();
        geo.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3));
        geo.setAttribute('color',    new THREE.Float32BufferAttribute(colors, 3));
        const mat = new THREE.PointsMaterial({ size: 0.018, vertexColors: true, sizeAttenuation: true });
        pointsMesh = new THREE.Points(geo, mat);
        scene.add(pointsMesh);
    }

    function colorForPoint(x, y, z) {
        // Y drives hue: top = warm, bottom = cool
        const t = (y + 1.0) * 0.5;
        const r = 0.18 + 0.82 * t;
        const g = 0.38 + 0.42 * (1.0 - t);
        const b = 1.0 - 0.45 * t;
        return [r, g, b];
    }

    // --- Orbit controls (minimal, no dep) ---
    let isDragging = false;
    let lastX = 0, lastY = 0;
    let theta = 0.7, phi = 0.55, radius = 3.5;

    function updateCamera() {
        camera.position.set(
            radius * Math.sin(phi) * Math.sin(theta),
            radius * Math.cos(phi),
            radius * Math.sin(phi) * Math.cos(theta)
        );
        camera.lookAt(0, 0, 0);
    }

    canvas.addEventListener('mousedown', e => { isDragging = true; lastX = e.clientX; lastY = e.clientY; });
    window.addEventListener('mouseup',   () => { isDragging = false; });
    window.addEventListener('mousemove', e => {
        if (!isDragging) return;
        const dx = e.clientX - lastX;
        const dy = e.clientY - lastY;
        lastX = e.clientX; lastY = e.clientY;
        theta -= dx * 0.007;
        phi    = Math.max(0.05, Math.min(Math.PI - 0.05, phi + dy * 0.007));
        updateCamera();
    });
    canvas.addEventListener('wheel', e => {
        e.preventDefault();
        radius = Math.max(1.5, Math.min(8.0, radius + e.deltaY * 0.005));
        updateCamera();
    }, { passive: false });

    // Touch support
    let lastTouchDist = null;
    canvas.addEventListener('touchstart', e => {
        if (e.touches.length === 1) { isDragging = true; lastX = e.touches[0].clientX; lastY = e.touches[0].clientY; }
        if (e.touches.length === 2) { lastTouchDist = Math.hypot(e.touches[0].clientX - e.touches[1].clientX, e.touches[0].clientY - e.touches[1].clientY); }
    });
    canvas.addEventListener('touchend', () => { isDragging = false; lastTouchDist = null; });
    canvas.addEventListener('touchmove', e => {
        e.preventDefault();
        if (e.touches.length === 1 && isDragging) {
            const dx = e.touches[0].clientX - lastX;
            const dy = e.touches[0].clientY - lastY;
            lastX = e.touches[0].clientX; lastY = e.touches[0].clientY;
            theta -= dx * 0.007;
            phi    = Math.max(0.05, Math.min(Math.PI - 0.05, phi + dy * 0.007));
            updateCamera();
        }
        if (e.touches.length === 2 && lastTouchDist !== null) {
            const d = Math.hypot(e.touches[0].clientX - e.touches[1].clientX, e.touches[0].clientY - e.touches[1].clientY);
            radius = Math.max(1.5, Math.min(8.0, radius - (d - lastTouchDist) * 0.01));
            lastTouchDist = d;
            updateCamera();
        }
    }, { passive: false });

    updateCamera();

    // --- Render loop ---
    function resize() {
        const w = canvas.parentElement.clientWidth;
        const h = canvas.parentElement.clientHeight;
        renderer.setSize(w, h);
        camera.aspect = w / h;
        camera.updateProjectionMatrix();
    }

    window.addEventListener('resize', resize);
    resize();

    function animate() {
        requestAnimationFrame(animate);
        renderer.render(scene, camera);
    }
    animate();

    // --- UI logic ---
    const PARAM_CONFIG = {
        uniform_sphere:              { params: [] },
        cosine_hemisphere:           { params: [] },
        power_cosine_lobe:           { params: ['exp'] },
        ggx_half_vector:             { params: ['roughness'] },
        ggx_half_vector_anisotropic: { params: ['alpha_x', 'alpha_y'] },
        uniform_cone:                { params: ['cone'] },
        nmath_sphere:                { params: [] },
        nmath_hemisphere:            { params: [] },
        nmath_diffuse:               { params: [] },
        nmath_lobe:                  { params: ['exp'] },
    };

    function updateParamVisibility() {
        const cfg = PARAM_CONFIG[methodSel.value] || { params: [] };
        const has = name => cfg.params.includes(name);

        paramRow.classList.toggle('sampling-hidden', !has('roughness'));
        expRow.classList.toggle('sampling-hidden',   !has('exp'));
        coneRow.classList.toggle('sampling-hidden',  !has('cone'));
        paramXRow.classList.toggle('sampling-hidden', !has('alpha_x'));
        paramYRow.classList.toggle('sampling-hidden', !has('alpha_y'));

        if (has('roughness')) { paramLabel.textContent = 'Roughness'; paramRange.min = '0.02'; paramRange.max = '1.0'; paramRange.step = '0.01'; }
    }

    function buildURL() {
        const method = methodSel.value;
        const count  = countRange.value;
        let url = `/api/sampling/samples?method=${encodeURIComponent(method)}&count=${count}`;

        const cfg = PARAM_CONFIG[method] || { params: [] };
        if (cfg.params.includes('roughness')) url += `&param=${paramRange.value}`;
        if (cfg.params.includes('exp'))       url += `&param=${expRange.value}`;
        if (cfg.params.includes('cone'))      url += `&param=${coneRange.value}`;
        if (cfg.params.includes('alpha_x'))   url += `&param_x=${paramXRange.value}`;
        if (cfg.params.includes('alpha_y'))   url += `&param_y=${paramYRange.value}`;
        return url;
    }

    let fetchController = null;

    function fetchAndDraw() {
        if (fetchController) fetchController.abort();
        fetchController = new AbortController();

        status.textContent = 'loading\u2026';
        regenBtn.disabled = true;

        fetch(buildURL(), { signal: fetchController.signal })
            .then(r => {
                if (!r.ok) throw new Error('HTTP ' + r.status);
                return r.json();
            })
            .then(data => {
                const samples = data.samples;
                const n = samples.length;
                const positions = new Float32Array(n * 3);
                const colors    = new Float32Array(n * 3);
                for (let i = 0; i < n; i++) {
                    const [x, y, z] = samples[i];
                    positions[i * 3]     = x;
                    positions[i * 3 + 1] = y;
                    positions[i * 3 + 2] = z;
                    const [r, g, b] = colorForPoint(x, y, z);
                    colors[i * 3]     = r;
                    colors[i * 3 + 1] = g;
                    colors[i * 3 + 2] = b;
                }
                buildPoints(positions, colors);
                status.textContent = n.toLocaleString() + ' samples';
                regenBtn.disabled = false;
            })
            .catch(err => {
                if (err.name !== 'AbortError') {
                    status.textContent = 'error: ' + err.message;
                    regenBtn.disabled = false;
                }
            });
    }

    countRange.addEventListener('input', () => { countVal.textContent = countRange.value; });
    paramRange.addEventListener('input', () => { paramVal.textContent  = parseFloat(paramRange.value).toFixed(2); });
    paramXRange.addEventListener('input', () => { paramXVal.textContent = parseFloat(paramXRange.value).toFixed(2); });
    paramYRange.addEventListener('input', () => { paramYVal.textContent = parseFloat(paramYRange.value).toFixed(2); });
    expRange.addEventListener('input', ()   => { expVal.textContent   = expRange.value; });
    coneRange.addEventListener('input', ()  => { coneVal.textContent  = parseFloat(coneRange.value).toFixed(2); });

    methodSel.addEventListener('change', () => { updateParamVisibility(); fetchAndDraw(); });
    regenBtn.addEventListener('click',   fetchAndDraw);

    updateParamVisibility();
    fetchAndDraw();
}());
