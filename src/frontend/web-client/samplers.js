(function () {
'use strict';

// ─────────────────────────────────────────────────────────────────────────────
// Math utilities
// ─────────────────────────────────────────────────────────────────────────────

const PI = Math.PI;
const im = Math.imul;

function u32(x) { return x >>> 0; }

function hash2(ix, iy) {
    let h = u32(im(u32(ix), 0x8da6b343) ^ im(u32(iy), 0xd8163841));
    h = u32(im(h ^ (h >>> 13), 0x85ebca6b));
    return ((h ^ (h >>> 16)) & 0x00ffffff) / 0x01000000;
}

function uhash3(x, y, seed) {
    let h = u32(u32(seed) ^ im(u32(x), 0x9e3779b9) ^ im(u32(y), 0x85ebca6b));
    h = u32(im(h ^ (h >>> 16), 0x45d9f3b));
    return (h ^ (h >>> 16)) >>> 0;
}

function h01(x, y, seed, ch) {
    return ((uhash3(x, y, seed) ^ im(u32(ch), 0xc2b2ae35)) & 0x00ffffff) / 0x01000000;
}

function smoothstep(t) { return t * t * (3 - 2 * t); }
function fract(v)      { return v - Math.floor(v); }
function clamp01(v)    { return v < 0 ? 0 : v > 1 ? 1 : v; }
function lerp(a, b, t) { return a + (b - a) * t; }

function valueNoise(x, y) {
    const ix = Math.floor(x), iy = Math.floor(y);
    const fx = x - ix, fy = y - iy;
    const sx = smoothstep(fx), sy = smoothstep(fy);
    const a = hash2(ix,     iy);
    const b = hash2(ix + 1, iy);
    const c = hash2(ix,     iy + 1);
    const d = hash2(ix + 1, iy + 1);
    return a + (b - a) * sx + (c - a) * sy + (a - b - c + d) * sx * sy;
}

function fbm(x, y, oct, lac, gain) {
    let v = 0, amp = 0.5, freq = 1;
    for (let i = 0; i < oct; i++) {
        v += amp * (2 * valueNoise(x * freq, y * freq) - 1);
        freq *= lac; amp *= gain;
    }
    return v;
}

function voronoiEdge(u, v, seed) {
    const iu = Math.floor(u), iv = Math.floor(v);
    let f1 = 1e30, f2 = 1e30;
    for (let dy = -2; dy <= 2; dy++) {
        for (let dx = -2; dx <= 2; dx++) {
            const cx = iu + dx, cy = iv + dy;
            const jx = h01(cx, cy, seed, 0);
            const jy = h01(cx, cy, seed, 1);
            const px = cx + jx - u, py = cy + jy - v;
            const d = px * px + py * py;
            if (d < f1) { f2 = f1; f1 = d; } else if (d < f2) { f2 = d; }
        }
    }
    return Math.sqrt(f2) - Math.sqrt(f1);
}

// Numerical 2D curl from gradient of value noise fbm
const EPS_CURL = 1e-3;
function curl(x, y, oct, lac, gain) {
    const dydx = (fbm(x + EPS_CURL, y, oct, lac, gain) - fbm(x - EPS_CURL, y, oct, lac, gain)) / (2 * EPS_CURL);
    const dydy = (fbm(x, y + EPS_CURL, oct, lac, gain) - fbm(x, y - EPS_CURL, oct, lac, gain)) / (2 * EPS_CURL);
    return [dydy, -dydx];
}

// Color utilities
function collerp(a, b, t) {
    return [lerp(a[0], b[0], t), lerp(a[1], b[1], t), lerp(a[2], b[2], t)];
}

function hexToLinear(hex) {
    const r = parseInt(hex.slice(1, 3), 16) / 255;
    const g = parseInt(hex.slice(3, 5), 16) / 255;
    const b = parseInt(hex.slice(5, 7), 16) / 255;
    return [r, g, b];
}

function linearToHex(col) {
    const tc = v => Math.max(0, Math.min(255, Math.round(v * 255)));
    return '#' + tc(col[0]).toString(16).padStart(2,'0')
               + tc(col[1]).toString(16).padStart(2,'0')
               + tc(col[2]).toString(16).padStart(2,'0');
}

// UV → direction for equirectangular sky rendering
function uvToDir(u, v) {
    const phi   = u * 2 * PI - PI;
    const theta = v * PI;
    const st = Math.sin(theta);
    return [st * Math.sin(phi), Math.cos(theta), st * Math.cos(phi)];
}

// ─────────────────────────────────────────────────────────────────────────────
// Sampler implementations  (u,v ∈ [0,1], params object → [r,g,b])
// ─────────────────────────────────────────────────────────────────────────────

function sampleColor(u, v, p) {
    return [p.r, p.g, p.b];
}

function sampleChecker(u, v, p) {
    const iu = Math.floor(u * p.scale_u + p.offset_u);
    const iv = Math.floor(v * p.scale_v + p.offset_v);
    return ((iu + iv) & 1) ? p.color_b : p.color_a;
}

function sampleGraphpaper(u, v, p) {
    const s  = p.scale > 1e-6 ? p.scale : 16;
    const fu = fract(u * s), fv = fract(v * s);
    const hw_minor = p.minor_width * 0.5;
    const hw_major = p.major_width * 0.5;
    const me = p.major_every > 0 ? p.major_every : 5;
    const iu = Math.floor(u * s), iv = Math.floor(v * s);
    const isMajorU = (((iu % me) + me) % me) === 0;
    const isMajorV = (((iv % me) + me) % me) === 0;
    const onMinorU = fu < hw_minor || fu > 1 - hw_minor;
    const onMinorV = fv < hw_minor || fv > 1 - hw_minor;
    const onMajorU = fu < hw_major || fu > 1 - hw_major;
    const onMajorV = fv < hw_major || fv > 1 - hw_major;
    if ((isMajorU && onMajorU) || (isMajorV && onMajorV)) return p.major;
    if (onMinorU || onMinorV) return p.minor;
    return p.base;
}

function sampleWeave(u, v, p) {
    const s  = p.scale > 1e-6 ? p.scale : 12;
    const bw = Math.max(0.02, Math.min(0.98, p.band_width));
    const fu = fract(u * s), fv = fract(v * s);
    const iu = Math.floor(u * s) & 1, iv = Math.floor(v * s) & 1;
    const in_warp  = fu < bw;
    const in_weft  = fv < bw;
    const on_top   = (iu === iv) ? in_warp : in_weft;
    const col_top  = (iu === iv) ? p.warp  : p.weft;
    const col_bot  = (iu === iv) ? p.weft  : p.warp;
    const t = smoothstep(clamp01((in_warp && in_weft) ? (on_top ? 1 : 0) : (in_warp ? 1 : in_weft ? 0 : 0.5)));
    if (!in_warp && !in_weft) return p.base;
    return collerp(col_bot, col_top, on_top ? 1 : 0);
}

function sampleFBMMarble(u, v, p) {
    const s   = p.scale > 1e-6 ? p.scale : 6;
    const oct = Math.max(1, p.octaves | 0);
    const lac = Math.max(1, p.lacunarity);
    const g   = clamp01(p.gain);
    const x = u * s, y = v * s;
    const n = fbm(x, y, oct, lac, g);
    const m = Math.sin((x + y) * p.vein_frequency + p.turbulence * n);
    const t = 0.5 * m + 0.5;
    const base = collerp(p.color_a, p.color_b, t);
    const sharp = p.vein_sharpness > 0 ? p.vein_sharpness : 1;
    const vein  = Math.pow(1 - Math.abs(m), sharp);
    const blend = clamp01(p.vein_strength * vein);
    return collerp(base, p.vein, blend);
}

function sampleVoronoiNormal(u, v, p) {
    const grid = Math.max(1, Math.ceil(Math.sqrt(Math.max(1, p.cells | 0))));
    const gx = u * grid, gy = v * grid;
    const ix = Math.floor(gx), iy = Math.floor(gy);
    let best_d2 = 1e30, bx = 0, by = 0;
    for (let dy = -1; dy <= 1; dy++) {
        for (let dx = -1; dx <= 1; dx++) {
            const cx = ((ix + dx) % grid + grid) % grid;
            const cy = ((iy + dy) % grid + grid) % grid;
            const jx = h01(cx, cy, p.seed, 0);
            const jy = h01(cx, cy, p.seed, 1);
            const sx = ix + dx + jx, sy = iy + dy + jy;
            const d2 = (gx - sx) ** 2 + (gy - sy) ** 2;
            if (d2 < best_d2) { best_d2 = d2; bx = cx; by = cy; }
        }
    }
    const maxDev = Math.max(0, Math.min(89, p.max_deviation));
    const theta = h01(bx, by, p.seed, 2) * maxDev * (PI / 180);
    const phi   = h01(bx, by, p.seed, 3) * 2 * PI;
    const st = Math.sin(theta);
    const nx = st * Math.cos(phi), ny = st * Math.sin(phi), nz = Math.cos(theta);
    return [(nx + 1) * 0.5, (ny + 1) * 0.5, (nz + 1) * 0.5];
}

function sampleGradient(u, v, p) {
    // Vertical gradient by v axis
    return collerp(p.a, p.b, v);
}

// --- Preetham sky helpers ---
function perez(A, B, C, D, E, theta, gamma) {
    const cosT = Math.cos(theta);
    const denom = Math.abs(cosT) < 1e-6 ? 1e-6 : cosT;
    return (1 + A * Math.exp(B / denom)) * (1 + C * Math.exp(D * gamma) + E * Math.cos(gamma) ** 2);
}
function preethamZenithY(T, ts) {
    const chi = (4 / 9 - T / 120) * (PI - 2 * ts);
    return (4.0453 * T - 4.9710) * Math.tan(chi) - 0.2155 * T + 2.4192;
}
function preethamZenithX(T, t) {
    return T*T*(0.00166*t**3 - 0.00375*t**2 + 0.00209*t)
         + T*(-0.02903*t**3 + 0.06377*t**2 - 0.03202*t + 0.00394)
         + (0.11693*t**3 - 0.21196*t**2 + 0.06052*t + 0.25886);
}
function preethamZenithY2(T, t) {
    return T*T*(0.00275*t**3 - 0.00610*t**2 + 0.00317*t)
         + T*(-0.04214*t**3 + 0.08970*t**2 - 0.04153*t + 0.00516)
         + (0.15346*t**3 - 0.26756*t**2 + 0.06670*t + 0.26688);
}
function xyYtoRGB(x, y, Y) {
    if (y < 1e-6) return [0, 0, 0];
    const X = x * Y / y, Z = (1 - x - y) * Y / y;
    return [
        Math.max(0,  3.2404542 * X - 1.5371385 * Y - 0.4985314 * Z),
        Math.max(0, -0.9692660 * X + 1.8760108 * Y + 0.0415560 * Z),
        Math.max(0,  0.0556434 * X - 0.2040259 * Y + 1.0572252 * Z),
    ];
}

function samplePreethamSky(u, v, p) {
    const dir = uvToDir(u, v);
    if (dir[1] < 0) return p.ground_color;
    const T  = Math.max(1.7, Math.min(10, p.turbidity));
    const sl = Math.sqrt(p.sun_direction[0]**2 + p.sun_direction[1]**2 + p.sun_direction[2]**2);
    if (sl < 1e-6) return p.ground_color;
    const sd = p.sun_direction.map(v => v / sl);
    const cosTs = Math.max(0, Math.min(1, sd[1]));
    const ts    = Math.acos(cosTs);
    const cosG  = clamp01(dir[0]*sd[0] + dir[1]*sd[1] + dir[2]*sd[2]);
    const gamma = Math.acos(cosG);
    const theta = Math.acos(clamp01(dir[1]));
    const AY = 0.1787*T-1.4630, BY = -0.3554*T+0.4275, CY = -0.0227*T+5.3251, DY = 0.1206*T-2.5771, EY = -0.0670*T+0.3703;
    const Ax = -0.0193*T-0.2592, Bx = -0.0665*T+0.0008, Cx = -0.0004*T+0.2125, Dx = -0.0641*T-0.8989, Ex = -0.0033*T+0.0452;
    const Ay = -0.0167*T-0.2608, By = -0.0950*T+0.0092, Cy = -0.0079*T+0.2102, Dy = -0.0441*T-1.6537, Ey = -0.0109*T+0.0529;
    const PzY = perez(AY,BY,CY,DY,EY, 0, ts);
    const Pzx = perez(Ax,Bx,Cx,Dx,Ex, 0, ts);
    const Pzy = perez(Ay,By,Cy,Dy,Ey, 0, ts);
    if (!PzY || !Pzx || !Pzy) return p.ground_color;
    const Yze = preethamZenithY(T, ts);
    const xze = preethamZenithX(T, ts);
    const yze = preethamZenithY2(T, ts);
    const Y = Yze * perez(AY,BY,CY,DY,EY, theta, gamma) / PzY;
    const x = xze * perez(Ax,Bx,Cx,Dx,Ex, theta, gamma) / Pzx;
    const y = yze * perez(Ay,By,Cy,Dy,Ey, theta, gamma) / Pzy;
    const exp = p.exposure > 1e-8 ? p.exposure : 0.04;
    const rgb = xyYtoRGB(x, y, Y * exp);
    const h = clamp01(dir[1] * 8);
    return collerp(p.ground_color, rgb, h);
}

// --- Hosek-Wilkie helpers ---
const HW_ABCDEFGHI = [
    [[0.10,-0.02],[-0.14,-0.03],[0.50,0.08],[-0.70,-0.10],[0.04,0.01],[0.06,0.01],[0.01,0.002],[-0.06,-0.01],[0.04,0.005]],
    [[0.08,-0.015],[-0.12,-0.03],[0.48,0.07],[-0.72,-0.10],[0.03,0.01],[0.05,0.01],[0.01,0.002],[-0.06,-0.01],[0.03,0.004]],
    [[0.05,-0.010],[-0.10,-0.02],[0.42,0.06],[-0.75,-0.09],[0.02,0.008],[0.04,0.008],[0.01,0.002],[-0.05,-0.01],[0.02,0.003]],
];
const ZEN_RGB = [[0.038,0.120],[0.055,0.140],[0.095,0.130]];
function hwF(A,B,C,D,E,F,G,H,I, theta, gamma) {
    const cosT = Math.cos(theta), sinT = Math.sin(theta);
    const cosG = Math.cos(gamma);
    const expB = Math.exp(B / (Math.abs(cosT) < 1e-6 ? 1e-6 : cosT));
    return (1 + A * expB) * (C + E * Math.exp(D * gamma) + F * cosG**2 + G * Math.exp(H * gamma) + I * sinT);
}
function hwCoeff(ch, coeff, T) {
    const t = (T - 1) / 9;
    return HW_ABCDEFGHI[ch][coeff][0] + HW_ABCDEFGHI[ch][coeff][1] * t;
}
function hwZenith(ch, T, alb) {
    const t = (T - 1) / 9;
    return (ZEN_RGB[ch][0] + (ZEN_RGB[ch][1] - ZEN_RGB[ch][0]) * t) * (1 + 0.5 * alb);
}

function sampleHosekWilkieSky(u, v, p) {
    const dir = uvToDir(u, v);
    if (dir[1] < 0) return p.ground_color;
    const T   = Math.max(1, Math.min(10, p.turbidity));
    const alb = clamp01(p.ground_albedo);
    const exp = p.exposure > 1e-8 ? p.exposure : 1;
    const sl = Math.sqrt(p.sun_direction[0]**2 + p.sun_direction[1]**2 + p.sun_direction[2]**2);
    if (sl < 1e-6) return p.ground_color;
    const sd = p.sun_direction.map(v => v / sl);
    const theta = Math.acos(clamp01(dir[1]));
    const cosG  = clamp01(dir[0]*sd[0] + dir[1]*sd[1] + dir[2]*sd[2]);
    const gamma = Math.acos(cosG);
    const sunTheta = Math.acos(clamp01(sd[1]));
    const rgb = [];
    for (let ch = 0; ch < 3; ch++) {
        const [A,B,C,D,E,F,G,H,I] = Array.from({length:9},(_,i)=>hwCoeff(ch,i,T));
        const Fz = hwF(A,B,C,D,E,F,G,H,I, 0, sunTheta);
        const Fv = hwF(A,B,C,D,E,F,G,H,I, theta, gamma);
        const Yz = hwZenith(ch, T, alb);
        rgb[ch] = Math.max(0, (Math.abs(Fz) < 1e-10 ? 0 : Yz * Fv / Fz) * exp);
    }
    const h = clamp01(dir[1] * 8);
    return collerp(p.ground_color, rgb, h);
}

function sampleRayleighSky(u, v, p) {
    const dir = uvToDir(u, v);
    if (dir[1] < 0) return p.ground_color;
    const sl = Math.sqrt(p.sun_direction[0]**2 + p.sun_direction[1]**2 + p.sun_direction[2]**2);
    if (sl < 1e-6) return p.ground_color;
    const sd = p.sun_direction.map(c => c / sl);
    const cosV  = clamp01(dir[1]);
    const cosG  = clamp01(dir[0]*sd[0] + dir[1]*sd[1] + dir[2]*sd[2]);
    const cosVc = Math.pow(Math.max(0, cosV), p.horizon_falloff);
    const beta  = p.beta_rayleigh.map(b => b * p.density);
    const scatter = beta.map(b => b * (1 + cosG * cosG) * cosVc);
    const sky = scatter.map((s, i) => p.sun_intensity[i] * s * 0.08);
    // Sun disk
    const angR = p.sun_disk_radius * PI / 180;
    const angG = p.sun_glow_radius * PI / 180;
    const gamma = Math.acos(cosG);
    const diskT = gamma < angR ? p.sun_disk_intensity : 0;
    const glowT = p.sun_glow_intensity * Math.pow(Math.max(0, 1 - gamma / angG), p.sun_glow_falloff);
    const sun = sky.map((s, i) => s + p.sun_intensity[i] * (diskT + glowT) * 0.01);
    const h = clamp01(dir[1] * 8);
    return collerp(p.ground_color, sun.map(c => clamp01(c)), h);
}

function sampleStars(u, v, p) {
    const dir = uvToDir(u, v);
    const gridScale = 256;
    const gu = u * gridScale, gv = v * gridScale;
    const iu = Math.floor(gu), iv = Math.floor(gv);
    const fu = gu - iu, fv = gv - iv;
    let result = [...p.background];
    for (let dy = -1; dy <= 1; dy++) {
        for (let dx = -1; dx <= 1; dx++) {
            const cx = iu + dx, cy = iv + dy;
            if (h01(cx, cy, p.seed, 0) > p.density) continue;
            const sx = dx + h01(cx, cy, p.seed, 1);
            const sy = dy + h01(cx, cy, p.seed, 2);
            const dist = Math.sqrt((fu - sx) ** 2 + (fv - sy) ** 2);
            const sz   = p.star_size + 1e-6;
            const gaus = Math.exp(-dist * dist / (2 * sz * sz));
            const br   = p.min_brightness + (p.max_brightness - p.min_brightness) * h01(cx, cy, p.seed, 3);
            const temp = h01(cx, cy, p.seed, 4);
            const sc   = [br * (0.8 + 0.2 * (1 - temp)), br * (0.85 + 0.1 * (1 - temp)), br * (0.9 + 0.1 * temp)];
            result = [result[0] + sc[0] * gaus, result[1] + sc[1] * gaus, result[2] + sc[2] * gaus];
        }
    }
    return result.map(c => Math.min(1, c));
}

function sampleSceneryHeightfield(u, v, p) {
    const s   = p.scale > 1e-6 ? p.scale : 0.25;
    const oct = Math.max(1, p.octaves | 0);
    const lac = Math.max(1, p.lacunarity);
    const g   = clamp01(p.gain);
    const x = u * (1 / s), y = v * (1 / s);
    // Ridge-modified fBm
    let value = 0, amp = 0.5, freq = 1;
    for (let i = 0; i < oct; i++) {
        const n = 2 * valueNoise(x * freq, y * freq) - 1;
        const ridge = 1 - Math.abs(n);
        value += amp * (lerp(n * 0.5 + 0.5, ridge * ridge, p.ridge_strength));
        freq *= lac; amp *= g;
    }
    // Apply mountain/valley shaping
    const t = clamp01(value);
    const shaped = t < 0.5
        ? t * (1 - p.valley_strength * (1 - 2 * t))
        : t + p.mountain_strength * (t - 0.5) * (t - 0.5);
    const v01 = clamp01(shaped);
    return [v01, v01, v01];
}

function sampleFBMWood(u, v, p) {
    const s = p.scale > 1e-6 ? p.scale : 4;
    const oct = Math.max(1, p.octaves | 0);
    const lac = Math.max(1, p.lacunarity);
    const g = clamp01(p.gain);
    const x = u * s, y = v * s;
    const distortion = p.turbulence * fbm(x, y, oct, lac, g);
    const dist = Math.sqrt(x * x + y * y);
    const ring = dist * p.ring_frequency + distortion;
    const t = clamp01(0.5 + 0.5 * Math.sin(ring));
    return collerp(p.color_a, p.color_b, t);
}

function sampleCurlNoise(u, v, p) {
    const s   = p.scale > 1e-6 ? p.scale : 3;
    const oct = Math.max(1, p.octaves | 0);
    const lac = Math.max(1, p.lacunarity);
    const g   = clamp01(p.gain);
    const x = u * s, y = v * s;
    const [cx, cy] = curl(x, y, oct, lac, g);
    const n = clamp01(0.5 + 0.5 * fbm(x + p.strength * cx, y + p.strength * cy, oct, lac, g));
    return collerp(p.color_a, p.color_b, n);
}

function sampleScratches(u, v, p) {
    const s  = p.scale > 1e-6 ? p.scale : 4;
    const pu = u * s, pv = v * s;
    const iu = Math.floor(pu), iv = Math.floor(pv);
    const w  = p.width * s + 1e-6;
    const n  = Math.max(1, p.density | 0);
    let minDist = 1e30;
    for (let dy = -1; dy <= 1; dy++) {
        for (let dx = -1; dx <= 1; dx++) {
            const cx = iu + dx, cy = iv + dy;
            for (let k = 0; k < n; k++) {
                const sk = p.seed >>> 0;
                const scx01 = h01(cx, cy, sk, k * 3 + 0);
                const scy01 = h01(cx, cy, sk, k * 3 + 1);
                const aj    = h01(cx, cy, sk, k * 3 + 2);
                const len   = h01(cx * 7, cy * 13, sk, k);
                const angle = p.angle + p.angle_jitter * (aj * 2 - 1) * PI;
                const cosA = Math.cos(angle), sinA = Math.sin(angle);
                const scx  = cx + scx01, scy = cy + scy01;
                const halfLen = (0.2 + 0.8 * len) * 0.5;
                const ex = pu - scx, ey = pv - scy;
                const along = ex * cosA + ey * sinA;
                const cl = Math.max(-halfLen, Math.min(halfLen, along));
                const rx = ex - cl * cosA, ry = ey - cl * sinA;
                const d = Math.sqrt(rx * rx + ry * ry);
                if (d < minDist) minDist = d;
            }
        }
    }
    const t = clamp01(1 - minDist / w);
    return collerp(p.color_base, p.color_scratch, t);
}

function sampleEdgeWear(u, v, p) {
    const s = p.scale > 1e-6 ? p.scale : 8;
    const edge = voronoiEdge(u * s, v * s, p.seed);
    const cov  = clamp01(p.coverage);
    const sp   = p.sharpness > 0.1 ? p.sharpness : 0.1;
    const raw  = clamp01(edge / (cov + 1e-6));
    const t    = 1 - Math.pow(raw, sp);
    return collerp(p.color_base, p.color_worn, t);
}

function sampleBrick(u, v, p) {
    const su = p.scale_u > 1e-6 ? p.scale_u : 8;
    const sv = p.scale_v > 1e-6 ? p.scale_v : 4;
    const bu = u * su, bv = v * sv;
    const row = Math.floor(bv);
    const offset = (row & 1) ? 0.5 : 0;
    const col = Math.floor(bu + offset);
    const fu = fract(bu + offset), fv = fract(bv);
    const hmu = clamp01(p.mortar_u) * 0.5;
    const hmv = clamp01(p.mortar_v) * 0.5;
    if (fu < hmu || fu > 1 - hmu || fv < hmv || fv > 1 - hmv) return p.color_mortar;
    const sk = p.seed >>> 0;
    const var_ = h01(row, col, sk, 0) * 2 - 1;
    const cv = p.color_variation * var_;
    return p.color_brick.map(c => clamp01(c + cv));
}

function sampleDots(u, v, p) {
    const s  = p.scale > 1e-6 ? p.scale : 8;
    const fu = fract(u * s) - 0.5;
    const fv = fract(v * s) - 0.5;
    const d  = Math.sqrt(fu * fu + fv * fv);
    const r  = clamp01(p.radius);
    const soft = clamp01(p.softness) + 1e-6;
    const t = clamp01((1 - (d - r) / soft));
    return collerp(p.color_bg, p.color_dot, t);
}

// Composite samplers — JS versions take a params object where children are
// pre-sampled using the hardcoded example child render fns below.

function sampleBlend(u, v, p) {
    const a = sampleChecker(u, v, { color_a: [0.9,0.9,0.9], color_b: [0.1,0.1,0.1], scale_u: 6, scale_v: 6, offset_u: 0, offset_v: 0 });
    const b = sampleFBMWood(u, v, { color_a: [0.72,0.48,0.22], color_b: [0.36,0.20,0.08], scale: 4, ring_frequency: 8, turbulence: 2, octaves: 4, lacunarity: 2, gain: 0.5 });
    return collerp(a, b, clamp01(p.t));
}

function sampleMixMasked(u, v, p) {
    const base    = sampleFBMWood(u, v, { color_a: [0.72,0.48,0.22], color_b: [0.36,0.20,0.08], scale: 4, ring_frequency: 8, turbulence: 2, octaves: 4, lacunarity: 2, gain: 0.5 });
    const overlay = sampleScratches(u, v, { color_base: [0.6,0.6,0.62], color_scratch: [0.95,0.95,0.97], scale: 4, density: 24, width: 0.008, angle: 0, angle_jitter: 0.3, seed: 42 });
    const maskCol = sampleEdgeWear(u, v, { color_base: [0,0,0], color_worn: [1,1,1], scale: 8, sharpness: 6, coverage: 0.5, seed: 1337 });
    const m = clamp01((maskCol[0] + maskCol[1] + maskCol[2]) / 3);
    const mode = p.mode;
    if (mode === 'multiply') {
        const mul = base.map((c,i) => c * overlay[i]);
        return collerp(base, mul, m);
    } else if (mode === 'add') {
        return base.map((c,i) => Math.min(1, c + overlay[i] * m));
    } else if (mode === 'screen') {
        const sc = base.map((c,i) => 1 - (1-c)*(1-overlay[i]));
        return collerp(base, sc, m);
    }
    return collerp(base, overlay, m);
}

function sampleTriplanar(u, v, p) {
    // Map UV to unit sphere for triplanar demo
    const phi = u * 2 * PI, theta = v * PI;
    const st = Math.sin(theta);
    const px = st * Math.cos(phi), py = Math.cos(theta), pz = st * Math.sin(phi);
    const s   = p.scale > 1e-6 ? p.scale : 1;
    const sp  = p.blend_sharpness > 0.1 ? p.blend_sharpness : 0.1;
    const child = { color_a: [0.9,0.9,0.9], color_b: [0.1,0.1,0.1], scale_u: 4, scale_v: 4, offset_u: 0, offset_v: 0 };
    const cx = sampleChecker(fract(py * s), fract(pz * s), child);
    const cy = sampleChecker(fract(px * s), fract(pz * s), child);
    const cz = sampleChecker(fract(px * s), fract(py * s), child);
    let wx = Math.pow(Math.abs(px), sp);
    let wy = Math.pow(Math.abs(py), sp);
    let wz = Math.pow(Math.abs(pz), sp);
    const wsum = wx + wy + wz + 1e-8;
    wx /= wsum; wy /= wsum; wz /= wsum;
    return [cx[0]*wx + cy[0]*wy + cz[0]*wz, cx[1]*wx + cy[1]*wy + cz[1]*wz, cx[2]*wx + cy[2]*wy + cz[2]*wz];
}

// ─────────────────────────────────────────────────────────────────────────────
// Sampler registry
// ─────────────────────────────────────────────────────────────────────────────

const SAMPLERS = [
  // ── Base ────────────────────────────────────────────────────────────────
  {
    id: 'color', label: 'color', group: 'Base',
    params: [
      { name: 'r', label: 'R', type: 'float', min: 0, max: 1, step: 0.01, def: 0.5 },
      { name: 'g', label: 'G', type: 'float', min: 0, max: 1, step: 0.01, def: 0.5 },
      { name: 'b', label: 'B', type: 'float', min: 0, max: 1, step: 0.01, def: 0.5 },
    ],
    render: sampleColor,
  },
  // ── Pattern ─────────────────────────────────────────────────────────────
  {
    id: 'checker', label: 'checker', group: 'Pattern',
    params: [
      { name: 'color_a', label: 'Color A', type: 'color', def: [0.92,0.92,0.92] },
      { name: 'color_b', label: 'Color B', type: 'color', def: [0.08,0.08,0.08] },
      { name: 'scale_u', label: 'Scale U', type: 'float', min: 0.5, max: 20, step: 0.5, def: 4 },
      { name: 'scale_v', label: 'Scale V', type: 'float', min: 0.5, max: 20, step: 0.5, def: 4 },
      { name: 'offset_u', label: 'Offset U', type: 'float', min: -2, max: 2, step: 0.05, def: 0 },
      { name: 'offset_v', label: 'Offset V', type: 'float', min: -2, max: 2, step: 0.05, def: 0 },
    ],
    render: sampleChecker,
  },
  {
    id: 'graphpaper', label: 'graphpaper', group: 'Pattern',
    params: [
      { name: 'base',  label: 'Background', type: 'color', def: [0.93,0.93,0.93] },
      { name: 'minor', label: 'Minor line', type: 'color', def: [0.71,0.83,0.89] },
      { name: 'major', label: 'Major line', type: 'color', def: [0.30,0.63,0.77] },
      { name: 'scale',       label: 'Scale',       type: 'float', min: 2, max: 64, step: 1, def: 16 },
      { name: 'minor_width', label: 'Minor width',  type: 'float', min: 0.005, max: 0.2, step: 0.005, def: 0.020 },
      { name: 'major_width', label: 'Major width',  type: 'float', min: 0.01,  max: 0.3, step: 0.01,  def: 0.050 },
      { name: 'major_every', label: 'Major every',  type: 'int',   min: 2,     max: 20,  step: 1,     def: 5 },
    ],
    render: sampleGraphpaper,
  },
  {
    id: 'weave', label: 'weave', group: 'Pattern',
    params: [
      { name: 'base', label: 'Base',       type: 'color', def: [0.14,0.13,0.12] },
      { name: 'warp', label: 'Warp',       type: 'color', def: [0.86,0.80,0.72] },
      { name: 'weft', label: 'Weft',       type: 'color', def: [0.28,0.62,0.68] },
      { name: 'scale',      label: 'Scale',      type: 'float', min: 2, max: 40, step: 1,    def: 12 },
      { name: 'band_width', label: 'Band width', type: 'float', min: 0.02, max: 0.98, step: 0.02, def: 0.66 },
    ],
    render: sampleWeave,
  },
  {
    id: 'brick', label: 'brick', group: 'Pattern',
    params: [
      { name: 'color_brick',  label: 'Brick',            type: 'color', def: [0.72,0.32,0.22] },
      { name: 'color_mortar', label: 'Mortar',           type: 'color', def: [0.72,0.70,0.66] },
      { name: 'scale_u',  label: 'Scale U',          type: 'float', min: 1, max: 24, step: 0.5, def: 8 },
      { name: 'scale_v',  label: 'Scale V',          type: 'float', min: 1, max: 24, step: 0.5, def: 4 },
      { name: 'mortar_u', label: 'Mortar U',         type: 'float', min: 0.01, max: 0.4, step: 0.01, def: 0.06 },
      { name: 'mortar_v', label: 'Mortar V',         type: 'float', min: 0.01, max: 0.4, step: 0.01, def: 0.10 },
      { name: 'color_variation', label: 'Variation', type: 'float', min: 0, max: 0.5, step: 0.01, def: 0.08 },
      { name: 'seed',     label: 'Seed',             type: 'int',   min: 0, max: 9999, step: 1, def: 42 },
    ],
    render: sampleBrick,
  },
  {
    id: 'dots', label: 'dots', group: 'Pattern',
    params: [
      { name: 'color_bg',  label: 'Background', type: 'color', def: [0.92,0.92,0.92] },
      { name: 'color_dot', label: 'Dot',        type: 'color', def: [0.10,0.10,0.10] },
      { name: 'scale',   label: 'Scale',   type: 'float', min: 1, max: 32, step: 0.5, def: 8 },
      { name: 'radius',  label: 'Radius',  type: 'float', min: 0.05, max: 0.49, step: 0.01, def: 0.35 },
      { name: 'softness',label: 'Softness',type: 'float', min: 0.002, max: 0.2, step: 0.005, def: 0.05 },
    ],
    render: sampleDots,
  },
  // ── Noise ────────────────────────────────────────────────────────────────
  {
    id: 'fbm_marble', label: 'fbm_marble', group: 'Noise',
    params: [
      { name: 'color_a', label: 'Color A',       type: 'color', def: [0.93,0.92,0.90] },
      { name: 'color_b', label: 'Color B',       type: 'color', def: [0.62,0.60,0.58] },
      { name: 'vein',    label: 'Vein',          type: 'color', def: [0.18,0.17,0.16] },
      { name: 'scale',          label: 'Scale',          type: 'float', min: 0.5, max: 20, step: 0.5, def: 6 },
      { name: 'vein_frequency', label: 'Vein frequency', type: 'float', min: 1, max: 30, step: 0.5, def: 9 },
      { name: 'turbulence',     label: 'Turbulence',     type: 'float', min: 0, max: 10, step: 0.1, def: 3.5 },
      { name: 'octaves',        label: 'Octaves',        type: 'int',   min: 1, max: 8, step: 1, def: 5 },
      { name: 'lacunarity',     label: 'Lacunarity',     type: 'float', min: 1, max: 4, step: 0.1, def: 2 },
      { name: 'gain',           label: 'Gain',           type: 'float', min: 0.1, max: 0.9, step: 0.05, def: 0.5 },
      { name: 'vein_strength',  label: 'Vein strength',  type: 'float', min: 0, max: 2, step: 0.05, def: 0.85 },
      { name: 'vein_sharpness', label: 'Vein sharpness', type: 'float', min: 0.5, max: 16, step: 0.5, def: 4 },
    ],
    render: sampleFBMMarble,
  },
  {
    id: 'fbm_wood', label: 'fbm_wood', group: 'Noise',
    params: [
      { name: 'color_a', label: 'Color A', type: 'color', def: [0.72,0.48,0.22] },
      { name: 'color_b', label: 'Color B', type: 'color', def: [0.36,0.20,0.08] },
      { name: 'scale',          label: 'Scale',          type: 'float', min: 0.5, max: 20, step: 0.5, def: 4 },
      { name: 'ring_frequency', label: 'Ring frequency', type: 'float', min: 1, max: 30, step: 0.5, def: 8 },
      { name: 'turbulence',     label: 'Turbulence',     type: 'float', min: 0, max: 8, step: 0.1, def: 2 },
      { name: 'octaves',        label: 'Octaves',        type: 'int',   min: 1, max: 8, step: 1, def: 4 },
      { name: 'lacunarity',     label: 'Lacunarity',     type: 'float', min: 1, max: 4, step: 0.1, def: 2 },
      { name: 'gain',           label: 'Gain',           type: 'float', min: 0.1, max: 0.9, step: 0.05, def: 0.5 },
    ],
    render: sampleFBMWood,
  },
  {
    id: 'curl_noise', label: 'curl_noise', group: 'Noise',
    params: [
      { name: 'color_a', label: 'Color A', type: 'color', def: [0.10,0.18,0.42] },
      { name: 'color_b', label: 'Color B', type: 'color', def: [0.82,0.90,0.98] },
      { name: 'scale',    label: 'Scale',    type: 'float', min: 0.5, max: 16, step: 0.5, def: 3 },
      { name: 'strength', label: 'Strength', type: 'float', min: 0, max: 4, step: 0.1, def: 1 },
      { name: 'octaves',  label: 'Octaves',  type: 'int',   min: 1, max: 8, step: 1, def: 4 },
      { name: 'lacunarity',label:'Lacunarity',type:'float', min: 1, max: 4, step: 0.1, def: 2 },
      { name: 'gain',     label: 'Gain',     type: 'float', min: 0.1, max: 0.9, step: 0.05, def: 0.5 },
    ],
    render: sampleCurlNoise,
  },
  // ── Surface Detail ───────────────────────────────────────────────────────
  {
    id: 'scratches', label: 'scratches', group: 'Surface Detail',
    params: [
      { name: 'color_base',   label: 'Base',    type: 'color', def: [0.60,0.60,0.62] },
      { name: 'color_scratch',label: 'Scratch', type: 'color', def: [0.90,0.90,0.92] },
      { name: 'scale',        label: 'Scale',       type: 'float', min: 0.5, max: 16, step: 0.5, def: 4 },
      { name: 'density',      label: 'Density',     type: 'int',   min: 1, max: 64, step: 1, def: 24 },
      { name: 'width',        label: 'Width',       type: 'float', min: 0.001, max: 0.05, step: 0.001, def: 0.008 },
      { name: 'angle',        label: 'Angle',       type: 'float', min: -3.14, max: 3.14, step: 0.05, def: 0 },
      { name: 'angle_jitter', label: 'Angle jitter',type: 'float', min: 0, max: 1, step: 0.05, def: 0.3 },
      { name: 'seed',         label: 'Seed',        type: 'int',   min: 0, max: 9999, step: 1, def: 42 },
    ],
    render: sampleScratches,
  },
  {
    id: 'edge_wear', label: 'edge_wear', group: 'Surface Detail',
    params: [
      { name: 'color_base', label: 'Base', type: 'color', def: [0.50,0.48,0.46] },
      { name: 'color_worn', label: 'Worn', type: 'color', def: [0.92,0.90,0.88] },
      { name: 'scale',    label: 'Scale',    type: 'float', min: 1, max: 30, step: 0.5, def: 8 },
      { name: 'sharpness',label: 'Sharpness',type: 'float', min: 0.5, max: 20, step: 0.5, def: 6 },
      { name: 'coverage', label: 'Coverage', type: 'float', min: 0.05, max: 1, step: 0.05, def: 0.5 },
      { name: 'seed',     label: 'Seed',     type: 'int',   min: 0, max: 9999, step: 1, def: 1337 },
    ],
    render: sampleEdgeWear,
  },
  // ── Height / Normal ──────────────────────────────────────────────────────
  {
    id: 'voronoi_normal', label: 'voronoi_normal', group: 'Height / Normal',
    params: [
      { name: 'cells',         label: 'Cells',         type: 'int',   min: 4, max: 512, step: 4, def: 64 },
      { name: 'max_deviation', label: 'Max deviation', type: 'float', min: 0, max: 89, step: 1, def: 12 },
      { name: 'seed',          label: 'Seed',          type: 'int',   min: 0, max: 9999, step: 1, def: 1337 },
    ],
    render: sampleVoronoiNormal,
  },
  {
    id: 'scenery_heightfield', label: 'scenery_heightfield', group: 'Height / Normal',
    params: [
      { name: 'seed',             label: 'Seed',            type: 'int',   min: 0, max: 9999, step: 1, def: 1337 },
      { name: 'scale',            label: 'Scale',           type: 'float', min: 0.05, max: 2, step: 0.05, def: 0.25 },
      { name: 'octaves',          label: 'Octaves',         type: 'int',   min: 1, max: 10, step: 1, def: 5 },
      { name: 'lacunarity',       label: 'Lacunarity',      type: 'float', min: 1, max: 4, step: 0.1, def: 2 },
      { name: 'gain',             label: 'Gain',            type: 'float', min: 0.05, max: 0.95, step: 0.05, def: 0.5 },
      { name: 'ridge_strength',   label: 'Ridge strength',  type: 'float', min: 0, max: 1, step: 0.05, def: 0.65 },
      { name: 'mountain_strength',label: 'Mountain strength',type:'float', min: 0, max: 1, step: 0.05, def: 0.75 },
      { name: 'valley_strength',  label: 'Valley strength', type: 'float', min: 0, max: 1, step: 0.05, def: 0.55 },
    ],
    render: sampleSceneryHeightfield,
  },
  // ── Sky / Environment ────────────────────────────────────────────────────
  {
    id: 'gradient', label: 'gradient', group: 'Sky / Environment', isEnv: true,
    params: [
      { name: 'a', label: 'Bottom', type: 'color', def: [1,1,1] },
      { name: 'b', label: 'Top',    type: 'color', def: [0.5,0.7,1] },
    ],
    render: sampleGradient,
  },
  {
    id: 'rayleigh_sky', label: 'rayleigh_sky', group: 'Sky / Environment', isEnv: true,
    params: [
      { name: 'sun_direction', label: 'Sun direction', type: 'vec3', def: [0.35,0.8,0.2] },
      { name: 'sun_intensity', label: 'Sun intensity', type: 'color', def: [0.4,0.37,0.3] },
      { name: 'beta_rayleigh', label: 'Beta Rayleigh', type: 'color', def: [0.18,0.35,0.80] },
      { name: 'ground_color',  label: 'Ground color',  type: 'color', def: [0.02,0.02,0.03] },
      { name: 'density',           label: 'Density',           type: 'float', min: 0.1, max: 5, step: 0.1, def: 1 },
      { name: 'horizon_falloff',   label: 'Horizon falloff',   type: 'float', min: 0.1, max: 5, step: 0.1, def: 1.5 },
      { name: 'sun_disk_radius',   label: 'Disk radius',       type: 'float', min: 0.1, max: 10, step: 0.1, def: 1 },
      { name: 'sun_disk_intensity',label: 'Disk intensity',    type: 'float', min: 0, max: 4, step: 0.1, def: 1 },
      { name: 'sun_glow_radius',   label: 'Glow radius',       type: 'float', min: 1, max: 40, step: 0.5, def: 8 },
      { name: 'sun_glow_intensity',label: 'Glow intensity',    type: 'float', min: 0, max: 2, step: 0.05, def: 0.35 },
      { name: 'sun_glow_falloff',  label: 'Glow falloff',      type: 'float', min: 0.5, max: 12, step: 0.5, def: 4 },
    ],
    render: sampleRayleighSky,
  },
  {
    id: 'preetham_sky', label: 'preetham_sky', group: 'Sky / Environment', isEnv: true,
    params: [
      { name: 'sun_direction', label: 'Sun direction', type: 'vec3', def: [0.35,0.8,0.2] },
      { name: 'ground_color',  label: 'Ground color',  type: 'color', def: [0.02,0.02,0.02] },
      { name: 'turbidity', label: 'Turbidity', type: 'float', min: 1.7, max: 10, step: 0.1, def: 3 },
      { name: 'exposure',  label: 'Exposure',  type: 'float', min: 0.005, max: 0.3, step: 0.005, def: 0.04 },
    ],
    render: samplePreethamSky,
  },
  {
    id: 'hosek_wilkie_sky', label: 'hosek_wilkie_sky', group: 'Sky / Environment', isEnv: true,
    params: [
      { name: 'sun_direction', label: 'Sun direction', type: 'vec3', def: [0.35,0.8,0.2] },
      { name: 'ground_color',  label: 'Ground color',  type: 'color', def: [0.02,0.02,0.02] },
      { name: 'turbidity',     label: 'Turbidity',     type: 'float', min: 1, max: 10, step: 0.1, def: 3 },
      { name: 'ground_albedo', label: 'Ground albedo', type: 'float', min: 0, max: 1, step: 0.05, def: 0.3 },
      { name: 'exposure',      label: 'Exposure',      type: 'float', min: 0.1, max: 5, step: 0.1, def: 1 },
    ],
    render: sampleHosekWilkieSky,
  },
  {
    id: 'stars', label: 'stars', group: 'Sky / Environment', isEnv: true,
    params: [
      { name: 'background', label: 'Background', type: 'color', def: [0,0,0.02] },
      { name: 'density',        label: 'Density',        type: 'float', min: 0.001, max: 0.1, step: 0.002, def: 0.015 },
      { name: 'min_brightness', label: 'Min brightness', type: 'float', min: 0, max: 1, step: 0.05, def: 0.4 },
      { name: 'max_brightness', label: 'Max brightness', type: 'float', min: 0, max: 1, step: 0.05, def: 1 },
      { name: 'star_size',      label: 'Star size',      type: 'float', min: 0.002, max: 0.08, step: 0.002, def: 0.015 },
      { name: 'seed',           label: 'Seed',           type: 'int',   min: 0, max: 9999, step: 1, def: 42 },
    ],
    render: sampleStars,
  },
  // ── Compositing ──────────────────────────────────────────────────────────
  {
    id: 'blend', label: 'blend', group: 'Compositing',
    note: 'Children: checker (a) + fbm_wood (b)',
    params: [
      { name: 't', label: 'Blend (t)', type: 'float', min: 0, max: 1, step: 0.01, def: 0.5 },
    ],
    render: sampleBlend,
  },
  {
    id: 'mix_masked', label: 'mix_masked', group: 'Compositing',
    note: 'Base: fbm_wood  ·  Overlay: scratches  ·  Mask: edge_wear',
    params: [
      { name: 'mode', label: 'Mode', type: 'select', options: ['lerp','multiply','add','screen'], def: 'lerp' },
    ],
    render: sampleMixMasked,
  },
  {
    id: 'triplanar', label: 'triplanar', group: 'Compositing', isEnv: true,
    note: 'Child: checker — rendered on unit sphere',
    params: [
      { name: 'scale',           label: 'Scale',           type: 'float', min: 0.1, max: 10, step: 0.1, def: 1 },
      { name: 'blend_sharpness', label: 'Blend sharpness', type: 'float', min: 0.5, max: 20, step: 0.5, def: 4 },
    ],
    render: sampleTriplanar,
  },
];

// ─────────────────────────────────────────────────────────────────────────────
// Rendering engine
// ─────────────────────────────────────────────────────────────────────────────

// Apply simple Reinhard tone mapping for HDR output (sky samplers).
function toneReinhard(c) { return c / (1 + c); }

function renderToCanvas(canvas, sampler, params, size, hdr) {
    canvas.width  = size;
    canvas.height = size;
    const ctx  = canvas.getContext('2d');
    const data = ctx.createImageData(size, size);
    const buf  = data.data;
    const fn   = sampler.render;
    const isEnv = !!sampler.isEnv;
    for (let py = 0; py < size; py++) {
        const v = isEnv ? py / size : 1 - py / size;
        for (let px = 0; px < size; px++) {
            const u = px / size;
            let [r, g, b] = fn(u, v, params);
            if (hdr) { r = toneReinhard(r); g = toneReinhard(g); b = toneReinhard(b); }
            const i = (py * size + px) * 4;
            buf[i]     = Math.max(0, Math.min(255, r * 255 + 0.5));
            buf[i + 1] = Math.max(0, Math.min(255, g * 255 + 0.5));
            buf[i + 2] = Math.max(0, Math.min(255, b * 255 + 0.5));
            buf[i + 3] = 255;
        }
    }
    ctx.putImageData(data, 0, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// UI
// ─────────────────────────────────────────────────────────────────────────────

let currentSampler = null;
let currentParams  = {};
let previewSize    = 256;
let hdrMode        = false;
let renderTimer    = null;

const mainCanvas   = document.getElementById('mainCanvas');
const previewLabel = document.getElementById('previewLabel');
const previewSizeEl= document.getElementById('previewSize');
const paramsTitle  = document.getElementById('paramsTitle');
const paramsSubtitle = document.getElementById('paramsSubtitle');
const paramsBody   = document.getElementById('paramsBody');
const sidebar      = document.getElementById('sidebar');
const searchInput  = document.getElementById('searchInput');

// Build default params for a sampler definition
function buildDefaultParams(samplerDef) {
    const p = {};
    for (const pd of samplerDef.params) {
        if (pd.type === 'color') {
            p[pd.name] = [...pd.def];
        } else if (pd.type === 'vec3') {
            p[pd.name] = [...pd.def];
        } else {
            p[pd.name] = pd.def;
        }
    }
    return p;
}

function scheduleRender(canvas, sampler, params, size) {
    if (renderTimer) clearTimeout(renderTimer);
    renderTimer = setTimeout(() => {
        renderToCanvas(canvas, sampler, params, size, hdrMode);
    }, 0);
}

// Render the main large canvas
function renderMain() {
    if (!currentSampler) return;
    renderToCanvas(mainCanvas, currentSampler, currentParams, previewSize, hdrMode);
    previewSizeEl.textContent = previewSize + ' × ' + previewSize;
}

// Build the parameters panel for the selected sampler
function buildParams(samplerDef) {
    paramsTitle.textContent   = samplerDef.label;
    paramsSubtitle.textContent = samplerDef.note || (samplerDef.isEnv ? 'Equirectangular projection' : 'UV flat quad');
    paramsBody.innerHTML = '';

    if (samplerDef.params.length === 0) {
        paramsBody.innerHTML = '<p class="sp-params-empty">No parameters.</p>';
        return;
    }

    for (const pd of samplerDef.params) {
        const row = document.createElement('div');
        row.className = 'sp-param';

        const label = document.createElement('span');
        label.className = 'sp-param-label';
        label.textContent = pd.label;
        row.appendChild(label);

        const ctrl = document.createElement('div');
        ctrl.className = 'sp-param-control';

        if (pd.type === 'float' || pd.type === 'int') {
            const range = document.createElement('input');
            range.type  = 'range';
            range.className = 'sp-range';
            range.min   = pd.min;
            range.max   = pd.max;
            range.step  = pd.step;
            range.value = currentParams[pd.name];

            const val = document.createElement('span');
            val.className = 'sp-range-val';
            val.textContent = formatVal(currentParams[pd.name], pd.type);

            range.addEventListener('input', () => {
                const v = pd.type === 'int' ? parseInt(range.value) : parseFloat(range.value);
                currentParams[pd.name] = v;
                val.textContent = formatVal(v, pd.type);
                scheduleRender(mainCanvas, currentSampler, currentParams, previewSize);
                refreshThumb(currentSampler.id);
            });

            ctrl.appendChild(range);
            ctrl.appendChild(val);

        } else if (pd.type === 'color') {
            const swatch = document.createElement('label');
            swatch.className = 'sp-color-swatch';
            swatch.style.background = linearToHex(currentParams[pd.name]);

            const picker = document.createElement('input');
            picker.type  = 'color';
            picker.value = linearToHex(currentParams[pd.name]);

            picker.addEventListener('input', () => {
                currentParams[pd.name] = hexToLinear(picker.value);
                swatch.style.background = picker.value;
                scheduleRender(mainCanvas, currentSampler, currentParams, previewSize);
                refreshThumb(currentSampler.id);
            });

            swatch.appendChild(picker);
            ctrl.appendChild(swatch);

            const hexLabel = document.createElement('span');
            hexLabel.className = 'sp-range-val';
            hexLabel.textContent = linearToHex(currentParams[pd.name]);
            picker.addEventListener('input', () => { hexLabel.textContent = picker.value; });
            ctrl.appendChild(hexLabel);

        } else if (pd.type === 'vec3') {
            // Show as three mini number inputs (x, y, z)
            const val = currentParams[pd.name];
            ['x','y','z'].forEach((axis, i) => {
                const inp = document.createElement('input');
                inp.type  = 'number';
                inp.className = 'sp-range-val';
                inp.style.cssText = 'width:3.4rem;border:1px solid var(--line);background:var(--field-bg);color:var(--ink);border-radius:3px;padding:1px 4px;font-family:inherit;font-size:0.72rem;';
                inp.step  = '0.05';
                inp.value = val[i].toFixed(2);
                inp.title = axis;
                inp.addEventListener('input', () => {
                    currentParams[pd.name][i] = parseFloat(inp.value) || 0;
                    scheduleRender(mainCanvas, currentSampler, currentParams, previewSize);
                    refreshThumb(currentSampler.id);
                });
                ctrl.appendChild(inp);
            });

        } else if (pd.type === 'select') {
            const sel = document.createElement('select');
            sel.className = 'xui-select sp-select';
            for (const opt of pd.options) {
                const o = document.createElement('option');
                o.value = opt; o.textContent = opt;
                if (opt === currentParams[pd.name]) o.selected = true;
                sel.appendChild(o);
            }
            sel.addEventListener('change', () => {
                currentParams[pd.name] = sel.value;
                scheduleRender(mainCanvas, currentSampler, currentParams, previewSize);
                refreshThumb(currentSampler.id);
            });
            ctrl.appendChild(sel);
        }

        row.appendChild(ctrl);
        paramsBody.appendChild(row);
    }
}

function formatVal(v, type) {
    if (type === 'int') return v.toString();
    if (Math.abs(v) < 0.01 && v !== 0) return v.toExponential(2);
    return v.toFixed(Math.abs(v) < 0.1 ? 3 : 2);
}

// Thumbnail canvas map: id → canvas element
const thumbMap = {};

function refreshThumb(id) {
    const def = SAMPLERS.find(s => s.id === id);
    if (!def || !thumbMap[id]) return;
    const p = (id === currentSampler?.id) ? currentParams : buildDefaultParams(def);
    renderToCanvas(thumbMap[id], def, p, 64, hdrMode);
}

function selectSampler(def) {
    // Deselect previous
    if (currentSampler) {
        const prev = document.querySelector(`.sp-thumb[data-id="${currentSampler.id}"]`);
        if (prev) prev.classList.remove('selected');
    }
    currentSampler = def;
    currentParams  = buildDefaultParams(def);

    const thumb = document.querySelector(`.sp-thumb[data-id="${def.id}"]`);
    if (thumb) thumb.classList.add('selected');

    previewLabel.textContent = def.label;
    buildParams(def);
    renderMain();

    document.getElementById('ncfBtn').classList.remove('sp-hidden');
    if (!document.getElementById('ncfModal').hidden) refreshNCF();
}

// Build the sidebar from SAMPLERS array
function buildSidebar(filter) {
    sidebar.innerHTML = '';
    const groups = {};
    for (const s of SAMPLERS) {
        if (filter && !s.label.includes(filter) && !s.group.toLowerCase().includes(filter)) continue;
        if (!groups[s.group]) groups[s.group] = [];
        groups[s.group].push(s);
    }

    for (const [groupName, samplers] of Object.entries(groups)) {
        const section = document.createElement('div');
        section.className = 'sp-group';

        const title = document.createElement('div');
        title.className = 'sp-group-title';
        title.textContent = groupName;
        section.appendChild(title);

        const grid = document.createElement('div');
        grid.className = 'sp-grid';

        for (const def of samplers) {
            const thumb = document.createElement('div');
            thumb.className = 'sp-thumb';
            thumb.dataset.id = def.id;
            if (currentSampler?.id === def.id) thumb.classList.add('selected');

            const canvas = document.createElement('canvas');
            canvas.width = canvas.height = 64;
            thumbMap[def.id] = canvas;

            const name = document.createElement('div');
            name.className = 'sp-thumb-name';
            name.textContent = def.label;

            thumb.appendChild(canvas);
            thumb.appendChild(name);
            thumb.addEventListener('click', () => selectSampler(def));
            grid.appendChild(thumb);
        }

        section.appendChild(grid);
        sidebar.appendChild(section);
    }
}

// Render thumbnails in a staggered queue so we don't block the main thread
function renderThumbnailQueue(defs, index) {
    if (index >= defs.length) return;
    const def = defs[index];
    const canvas = thumbMap[def.id];
    if (canvas) {
        const p = buildDefaultParams(def);
        renderToCanvas(canvas, def, p, 64, hdrMode);
    }
    requestAnimationFrame(() => renderThumbnailQueue(defs, index + 1));
}

// Preview size chips
document.querySelectorAll('.sp-chip[data-size]').forEach(chip => {
    chip.addEventListener('click', () => {
        document.querySelectorAll('.sp-chip[data-size]').forEach(c => c.classList.remove('active'));
        chip.classList.add('active');
        previewSize = parseInt(chip.dataset.size);
        renderMain();
    });
});

// Search
searchInput.addEventListener('input', () => {
    const q = searchInput.value.toLowerCase().trim();
    buildSidebar(q || null);
    renderThumbnailQueue(SAMPLERS.filter(s => !q || s.label.includes(q) || s.group.toLowerCase().includes(q)), 0);
});

// ─────────────────────────────────────────────────────────────────────────────
// NCF Generator
// ─────────────────────────────────────────────────────────────────────────────

const ncfModal      = document.getElementById('ncfModal');
const ncfCode       = document.getElementById('ncfCode');
const ncfCopyBtn    = document.getElementById('ncfCopyBtn');
const ncfCloseBtn   = document.getElementById('ncfCloseBtn');
const ncfRefreshBtn = document.getElementById('ncfRefreshBtn');
const ncfSlotInput  = document.getElementById('ncfSlotInput');
const ncfContextSel = document.getElementById('ncfContextSel');
const ncfBtn        = document.getElementById('ncfBtn');

function escHtml(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

function fmtF(v) {
    if (Math.abs(v) < 0.001 && v !== 0) return v.toExponential(3);
    return v.toFixed(3);
}

function generateNCF(def, params, slot, context) {
    const ind = '    ';
    let outerKey;
    if      (context === 'environment') outerKey = 'environment';
    else if (context === 'height')      outerKey = 'height_sampler';
    else                                outerKey = (slot && slot.trim()) ? slot.trim() : 'base_color';

    const entries = [['type', `<span class="ncf-val">${escHtml(def.id)}</span>`]];

    for (const pd of def.params) {
        const k   = pd.name;
        const val = params[k];
        let valHtml;
        if (pd.type === 'color') {
            valHtml = `<span class="ncf-fn">col3</span>(<span class="ncf-val">${fmtF(val[0])}, ${fmtF(val[1])}, ${fmtF(val[2])}</span>)`;
        } else if (pd.type === 'vec3') {
            valHtml = `<span class="ncf-fn">vec3</span>(<span class="ncf-val">${fmtF(val[0])}, ${fmtF(val[1])}, ${fmtF(val[2])}</span>)`;
        } else if (pd.type === 'select') {
            valHtml = `<span class="ncf-val">${escHtml(String(val))}</span>`;
        } else {
            valHtml = `<span class="ncf-val">${fmtF(Number(val))}</span>`;
        }
        entries.push([k, valHtml]);
    }

    const maxLen = Math.max(...entries.map(([k]) => k.length));
    const lines = [`<span class="ncf-key">${escHtml(outerKey)}</span> = {`];
    for (const [k, valHtml] of entries) {
        const pad = ' '.repeat(maxLen - k.length + 1);
        lines.push(`${ind}<span class="ncf-key">${escHtml(k)}</span>${pad}= ${valHtml}`);
    }
    lines.push('}');
    return lines.join('\n');
}

function refreshNCF() {
    if (!currentSampler) return;
    ncfCode.innerHTML = generateNCF(
        currentSampler, currentParams,
        ncfSlotInput.value,
        ncfContextSel.value
    );
}

ncfBtn.addEventListener('click', () => {
    if (!currentSampler) return;
    ncfContextSel.value = currentSampler.isEnv ? 'environment' : 'material';
    if (!ncfSlotInput.value) ncfSlotInput.value = currentSampler.id;
    refreshNCF();
    ncfModal.hidden = false;
});

ncfCloseBtn.addEventListener('click', () => { ncfModal.hidden = true; });
ncfModal.querySelector('.sp-modal-backdrop').addEventListener('click', () => { ncfModal.hidden = true; });
ncfRefreshBtn.addEventListener('click', refreshNCF);
ncfContextSel.addEventListener('change', refreshNCF);
ncfSlotInput.addEventListener('input', refreshNCF);

ncfCopyBtn.addEventListener('click', () => {
    navigator.clipboard.writeText(ncfCode.textContent).then(() => {
        ncfCopyBtn.classList.add('copied');
        ncfCopyBtn.textContent = 'Copied!';
        setTimeout(() => {
            ncfCopyBtn.classList.remove('copied');
            ncfCopyBtn.textContent = 'Copy';
        }, 2000);
    });
});

// Initial build
buildSidebar(null);
renderThumbnailQueue(SAMPLERS, 0);
selectSampler(SAMPLERS.find(s => s.id === 'checker'));

})();
