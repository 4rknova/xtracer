(function () {
'use strict';

// ---------------------------------------------------------------------------
// mat4 (column-major)
// ---------------------------------------------------------------------------
const m4 = {
  identity() {
    return new Float32Array([1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]);
  },
  perspective(fovy, asp, near, far) {
    const f = 1.0 / Math.tan(fovy / 2);
    const nf = 1 / (near - far);
    const m = new Float32Array(16);
    m[0] = f/asp; m[5] = f;
    m[10] = (far+near)*nf; m[11] = -1;
    m[14] = 2*far*near*nf;
    return m;
  },
  lookAt(eye, center, up) {
    const ex = eye[0], ey = eye[1], ez = eye[2];
    let zx = ex-center[0], zy = ey-center[1], zz = ez-center[2];
    let zl = Math.sqrt(zx*zx+zy*zy+zz*zz)||1; zx/=zl; zy/=zl; zz/=zl;
    let xx = up[1]*zz-up[2]*zy, xy = up[2]*zx-up[0]*zz, xz = up[0]*zy-up[1]*zx;
    let xl = Math.sqrt(xx*xx+xy*xy+xz*xz)||1; xx/=xl; xy/=xl; xz/=xl;
    const yx = zy*xz-zz*xy, yy = zz*xx-zx*xz, yz = zx*xy-zy*xx;
    const m = new Float32Array(16);
    m[0]=xx; m[1]=yx; m[2]=zx; m[3]=0;
    m[4]=xy; m[5]=yy; m[6]=zy; m[7]=0;
    m[8]=xz; m[9]=yz; m[10]=zz; m[11]=0;
    m[12]=-(xx*ex+xy*ey+xz*ez);
    m[13]=-(yx*ex+yy*ey+yz*ez);
    m[14]=-(zx*ex+zy*ey+zz*ez);
    m[15]=1;
    return m;
  },
  multiply(a, b) {
    const m = new Float32Array(16);
    for (let r = 0; r < 4; r++)
      for (let c = 0; c < 4; c++)
        m[c*4+r] = a[0*4+r]*b[c*4+0] + a[1*4+r]*b[c*4+1] + a[2*4+r]*b[c*4+2] + a[3*4+r]*b[c*4+3];
    return m;
  },
  nm3(m) {
    const a00=m[0],a01=m[1],a02=m[2],
          a10=m[4],a11=m[5],a12=m[6],
          a20=m[8],a21=m[9],a22=m[10];
    const b00=a11*a22-a12*a21, b01=a12*a20-a10*a22, b02=a10*a21-a11*a20;
    const det = a00*b00 + a01*b01 + a02*b02 || 1;
    const inv = new Float32Array(9);
    inv[0]=b00/det; inv[1]=(a02*a21-a01*a22)/det; inv[2]=(a01*a12-a02*a11)/det;
    inv[3]=b01/det; inv[4]=(a00*a22-a02*a20)/det; inv[5]=(a02*a10-a00*a12)/det;
    inv[6]=b02/det; inv[7]=(a01*a20-a00*a21)/det; inv[8]=(a00*a11-a01*a10)/det;
    return inv;
  }
};

// ---------------------------------------------------------------------------
// WebGL
// ---------------------------------------------------------------------------
const VERT = `
attribute vec3 aPos;
attribute vec3 aNorm;
uniform mat4 uMVP;
uniform mat3 uNM;
uniform bool uWire;
varying vec3 vNorm;
varying vec3 vPos;
void main(){
  vNorm = normalize(uNM * aNorm);
  vPos  = aPos;
  gl_Position = uMVP * vec4(aPos, 1.0);
}`;

const FRAG = `
precision mediump float;
varying vec3 vNorm;
varying vec3 vPos;
uniform vec3 uColor;
uniform bool uWire;
void main(){
  if(uWire){
    gl_FragColor = vec4(uColor * 0.9 + 0.1, 1.0);
  } else {
    vec3 L = normalize(vec3(0.8, 1.2, 1.0));
    float d = max(dot(vNorm, L), 0.0);
    float fill = max(dot(vNorm, normalize(vec3(-0.5,-0.3,-0.8))), 0.0) * 0.18;
    vec3 col = uColor * (0.15 + d * 0.78 + fill);
    gl_FragColor = vec4(col, 1.0);
  }
}`;

function createCtx(canvas, w, h) {
  canvas.width = w; canvas.height = h;
  const gl = canvas.getContext('webgl', { preserveDrawingBuffer: true, antialias: true });
  if (!gl) return null;

  function compileShader(src, type) {
    const s = gl.createShader(type);
    gl.shaderSource(s, src); gl.compileShader(s);
    return s;
  }
  const prog = gl.createProgram();
  gl.attachShader(prog, compileShader(VERT, gl.VERTEX_SHADER));
  gl.attachShader(prog, compileShader(FRAG, gl.FRAGMENT_SHADER));
  gl.linkProgram(prog);
  gl.useProgram(prog);

  const aPos  = gl.getAttribLocation(prog, 'aPos');
  const aNorm = gl.getAttribLocation(prog, 'aNorm');
  const uMVP  = gl.getUniformLocation(prog, 'uMVP');
  const uNM   = gl.getUniformLocation(prog, 'uNM');
  const uCol  = gl.getUniformLocation(prog, 'uColor');
  const uWire = gl.getUniformLocation(prog, 'uWire');

  const posBuf  = gl.createBuffer();
  const normBuf = gl.createBuffer();
  const lineBuf = gl.createBuffer();

  return { gl, prog, aPos, aNorm, uMVP, uNM, uCol, uWire, posBuf, normBuf, lineBuf };
}

function uploadMesh(ctx, positions, normals) {
  const { gl, posBuf, normBuf, lineBuf } = ctx;
  const pos  = new Float32Array(positions);
  const nrm  = new Float32Array(normals);

  gl.bindBuffer(gl.ARRAY_BUFFER, posBuf);
  gl.bufferData(gl.ARRAY_BUFFER, pos, gl.STATIC_DRAW);
  gl.bindBuffer(gl.ARRAY_BUFFER, normBuf);
  gl.bufferData(gl.ARRAY_BUFFER, nrm, gl.STATIC_DRAW);

  // Build wireframe line positions: duplicate edges of each triangle
  const triCount = positions.length / 9;
  const linePos = new Float32Array(triCount * 6 * 3);
  for (let t = 0; t < triCount; t++) {
    const base = t * 9;
    const out  = t * 18;
    const edges = [[0,1],[1,2],[2,0]];
    for (let e = 0; e < 3; e++) {
      const a = edges[e][0]*3, b = edges[e][1]*3;
      linePos.set([pos[base+a], pos[base+a+1], pos[base+a+2]], out + e*6);
      linePos.set([pos[base+b], pos[base+b+1], pos[base+b+2]], out + e*6+3);
    }
  }
  gl.bindBuffer(gl.ARRAY_BUFFER, lineBuf);
  gl.bufferData(gl.ARRAY_BUFFER, linePos, gl.STATIC_DRAW);

  ctx._vertCount    = positions.length / 3;
  ctx._lineVertCount= linePos.length / 3;
}

function getPreviewBgRGB() {
  const raw = getComputedStyle(document.documentElement).getPropertyValue('--preview-bg').trim();
  const hex = raw.startsWith('#') ? raw : null;
  if (hex && hex.length === 7) {
    return [
      parseInt(hex.slice(1, 3), 16) / 255,
      parseInt(hex.slice(3, 5), 16) / 255,
      parseInt(hex.slice(5, 7), 16) / 255,
    ];
  }
  if (hex && hex.length === 4) {
    return [
      parseInt(hex[1] + hex[1], 16) / 255,
      parseInt(hex[2] + hex[2], 16) / 255,
      parseInt(hex[3] + hex[3], 16) / 255,
    ];
  }
  return [0.067, 0.075, 0.082];
}

function drawScene(ctx, color, az, el, dist, wire, w, h) {
  const { gl, aPos, aNorm, uMVP, uNM, uCol, uWire, posBuf, normBuf, lineBuf } = ctx;
  gl.viewport(0, 0, w, h);
  const [cr, cg, cb] = getPreviewBgRGB();
  gl.clearColor(cr, cg, cb, 1);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  gl.enable(gl.DEPTH_TEST);

  const eye = [
    dist * Math.cos(el) * Math.sin(az),
    dist * Math.sin(el),
    dist * Math.cos(el) * Math.cos(az)
  ];
  const proj = m4.perspective(0.8, w/h, 0.05, 50);
  const view = m4.lookAt(eye, [0,0,0], [0,1,0]);
  const model = m4.identity();
  const mv = m4.multiply(view, model);
  const mvp = m4.multiply(proj, mv);
  const nm = m4.nm3(mv);

  gl.uniformMatrix4fv(uMVP, false, mvp);
  gl.uniformMatrix3fv(uNM, false, nm);
  gl.uniform3fv(uCol, color);
  gl.uniform1i(uWire, wire ? 1 : 0);

  if (wire) {
    gl.bindBuffer(gl.ARRAY_BUFFER, lineBuf);
    gl.enableVertexAttribArray(aPos);
    gl.vertexAttribPointer(aPos, 3, gl.FLOAT, false, 0, 0);
    // use position as normal approximation for wire (not lit)
    gl.disableVertexAttribArray(aNorm);
    gl.vertexAttrib3f(aNorm, 0, 1, 0);
    gl.drawArrays(gl.LINES, 0, ctx._lineVertCount);
  } else {
    gl.bindBuffer(gl.ARRAY_BUFFER, posBuf);
    gl.enableVertexAttribArray(aPos);
    gl.vertexAttribPointer(aPos, 3, gl.FLOAT, false, 0, 0);
    gl.bindBuffer(gl.ARRAY_BUFFER, normBuf);
    gl.enableVertexAttribArray(aNorm);
    gl.vertexAttribPointer(aNorm, 3, gl.FLOAT, false, 0, 0);
    gl.drawArrays(gl.TRIANGLES, 0, ctx._vertCount);
  }
}

// ---------------------------------------------------------------------------
// SHAPES registry  (server-native param names)
// ---------------------------------------------------------------------------
const SHAPES = [
  {
    id: 'icosphere', label: 'Sphere', group: 'Primitives',
    color: [0.38, 0.62, 0.92],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int', min: 4, max: 64, step: 4, def: 32 }
    ]
  },
  {
    id: 'disc', label: 'Disc', group: 'Primitives',
    color: [0.70, 0.55, 0.90],
    params: [
      { name: 'resolution',    label: 'Resolution',    type: 'int',   min: 8,   max: 64,  step: 4,   def: 32  },
      { name: 'inner_radius',  label: 'Inner Radius',  type: 'float', min: 0.0, max: 0.9, step: 0.05, def: 0.0 },
      { name: 'outer_radius',  label: 'Outer Radius',  type: 'float', min: 0.1, max: 3.0, step: 0.1,  def: 1.0 }
    ]
  },
  {
    id: 'capped_cylinder', label: 'Cylinder', group: 'Primitives',
    color: [0.55, 0.80, 0.55],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int', min: 8, max: 64, step: 4, def: 32 }
    ]
  },
  {
    id: 'cone', label: 'Cone', group: 'Primitives',
    color: [0.92, 0.70, 0.38],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int', min: 8, max: 64, step: 4, def: 32 }
    ]
  },
  {
    id: 'capsule', label: 'Capsule', group: 'Primitives',
    color: [0.55, 0.75, 0.90],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int', min: 12, max: 64, step: 4, def: 32 }
    ]
  },
  {
    id: 'ring', label: 'Torus', group: 'Primitives',
    color: [0.92, 0.42, 0.55],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int',   min: 16,  max: 96,  step: 8,    def: 32   },
      { name: 'radius',     label: 'Radius',     type: 'float', min: 0.2, max: 3.0, step: 0.1,  def: 1.0  },
      { name: 'height',     label: 'Height',     type: 'float', min: 0.1, max: 2.0, step: 0.05, def: 0.64 },
      { name: 'thickness',  label: 'Thickness',  type: 'float', min: 0.0, max: 0.9, step: 0.05, def: 0.3  }
    ]
  },
  {
    id: 'torus_knot', label: 'Torus Knot', group: 'Primitives',
    color: [0.45, 0.82, 0.82],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int', min: 24, max: 192, step: 8, def: 64 }
    ]
  },
  {
    id: 'tetrahedron', label: 'Tetrahedron', group: 'Polyhedra',
    color: [0.95, 0.80, 0.35],
    params: []
  },
  {
    id: 'cube', label: 'Cube', group: 'Polyhedra',
    color: [0.68, 0.50, 0.35],
    params: []
  },
  {
    id: 'octahedron', label: 'Octahedron', group: 'Polyhedra',
    color: [0.38, 0.70, 0.55],
    params: []
  },
  {
    id: 'icosahedron', label: 'Icosahedron', group: 'Polyhedra',
    color: [0.65, 0.40, 0.82],
    params: []
  },
  {
    id: 'dodecahedron', label: 'Dodecahedron', group: 'Polyhedra',
    color: [0.50, 0.65, 0.90],
    params: []
  },
  {
    id: 'mobius_strip', label: 'Möbius Strip', group: 'Special',
    color: [0.92, 0.55, 0.75],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int',   min: 24,  max: 128, step: 8,    def: 64   },
      { name: 'radius',     label: 'Radius',     type: 'float', min: 0.2, max: 2.0, step: 0.1,  def: 1.0  },
      { name: 'width',      label: 'Width',      type: 'float', min: 0.1, max: 1.5, step: 0.05, def: 0.64 }
    ]
  },
  {
    id: 'superellipsoid', label: 'Superellipsoid', group: 'Special',
    color: [0.40, 0.75, 0.75],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int',   min: 8,   max: 64,  step: 4,    def: 32 },
      { name: 'e1',         label: 'E1 (lat)',   type: 'float', min: 0.1, max: 2.5, step: 0.05, def: 0.3 },
      { name: 'e2',         label: 'E2 (lng)',   type: 'float', min: 0.1, max: 2.5, step: 0.05, def: 0.3 }
    ]
  },
  {
    id: 'star', label: 'Star', group: 'Special',
    color: [0.95, 0.85, 0.35],
    params: [
      { name: 'resolution',   label: 'Resolution',   type: 'int',   min: 8,   max: 64,  step: 4,    def: 32  },
      { name: 'points',       label: 'Points',       type: 'int',   min: 3,   max: 16,  step: 1,    def: 5   },
      { name: 'outer_radius', label: 'Outer Radius', type: 'float', min: 0.3, max: 2.0, step: 0.05, def: 1.0 },
      { name: 'inner_radius', label: 'Inner Radius', type: 'float', min: 0.1, max: 1.5, step: 0.05, def: 0.4 },
      { name: 'height',       label: 'Height',       type: 'float', min: 0.05, max: 1.0, step: 0.05, def: 0.2 }
    ]
  },
  {
    id: 'menger_sponge', label: 'Menger Sponge', group: 'Fractals',
    color: [0.82, 0.72, 0.62],
    params: [
      { name: 'resolution', label: 'Iterations', type: 'int', min: 1, max: 3, step: 1, def: 2 }
    ]
  },
  {
    id: 'sierpinski_tetrahedron', label: 'Sierpinski Tetra', group: 'Fractals',
    color: [0.70, 0.60, 0.85],
    params: [
      { name: 'resolution', label: 'Iterations', type: 'int', min: 1, max: 4, step: 1, def: 3 }
    ]
  },
  {
    id: 'rock', label: 'Rock', group: 'Organic',
    color: [0.62, 0.62, 0.58],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int',   min: 8,   max: 128, step: 8,    def: 48   },
      { name: 'roughness',  label: 'Roughness',  type: 'float', min: 0.0, max: 2.0, step: 0.05, def: 0.35 },
      { name: 'seed',       label: 'Seed',       type: 'int',   min: 1,   max: 9999, step: 1,   def: 1337 },
      { name: 'octaves',    label: 'Octaves',    type: 'int',   min: 1,   max: 8,   step: 1,    def: 4    }
    ]
  },
  {
    id: 'shell_spiral', label: 'Shell Spiral', group: 'Organic',
    color: [0.90, 0.78, 0.55],
    params: [
      { name: 'resolution',  label: 'Resolution',  type: 'int',   min: 16,  max: 256,  step: 16,   def: 64   },
      { name: 'turns',       label: 'Turns',       type: 'float', min: 0.5, max: 12.0, step: 0.5,  def: 4.0  },
      { name: 'growth',      label: 'Growth',      type: 'float', min: 0.05, max: 1.0, step: 0.05, def: 0.22 },
      { name: 'tube_radius', label: 'Tube Radius', type: 'float', min: 0.02, max: 0.5, step: 0.02, def: 0.14 }
    ]
  },
  {
    id: 'crystal', label: 'Crystal', group: 'Organic',
    color: [0.72, 0.85, 0.95],
    params: [
      { name: 'resolution', label: 'Resolution', type: 'int',   min: 8,   max: 64,  step: 4,    def: 16  },
      { name: 'count',      label: 'Count',      type: 'int',   min: 1,   max: 12,  step: 1,    def: 5   },
      { name: 'radius',     label: 'Radius',     type: 'float', min: 0.1, max: 2.0, step: 0.1,  def: 0.8 },
      { name: 'height',     label: 'Height',     type: 'float', min: 0.2, max: 4.0, step: 0.1,  def: 1.5 },
      { name: 'tip_height', label: 'Tip Height', type: 'float', min: 0.1, max: 2.0, step: 0.1,  def: 0.6 },
      { name: 'seed',       label: 'Seed',       type: 'int',   min: 1,   max: 9999, step: 1,   def: 1337 }
    ]
  },
  {
    id: 'gear', label: 'Gear', group: 'Mechanical',
    color: [0.72, 0.72, 0.78],
    params: [
      { name: 'resolution',   label: 'Resolution',    type: 'int',   min: 8,   max: 64,  step: 4,    def: 32   },
      { name: 'tooth_count',  label: 'Teeth',         type: 'int',   min: 3,   max: 32,  step: 1,    def: 12   },
      { name: 'tooth_depth',  label: 'Tooth Depth',   type: 'float', min: 0.02, max: 0.5, step: 0.02, def: 0.1 },
      { name: 'inner_radius', label: 'Inner Radius',  type: 'float', min: 0.05, max: 0.8, step: 0.05, def: 0.2 },
      { name: 'outer_radius', label: 'Outer Radius',  type: 'float', min: 0.2, max: 2.0, step: 0.05, def: 0.5 },
      { name: 'height',       label: 'Height',        type: 'float', min: 0.05, max: 1.0, step: 0.05, def: 0.2 }
    ]
  },
  {
    id: 'spring', label: 'Spring', group: 'Mechanical',
    color: [0.60, 0.80, 0.65],
    params: [
      { name: 'resolution',    label: 'Resolution',    type: 'int',   min: 8,   max: 64,  step: 4,    def: 32   },
      { name: 'coils',         label: 'Coils',         type: 'float', min: 1.0, max: 16.0, step: 0.5,  def: 6.0  },
      { name: 'wire_radius',   label: 'Wire Radius',   type: 'float', min: 0.01, max: 0.3, step: 0.01, def: 0.05 },
      { name: 'spring_radius', label: 'Spring Radius', type: 'float', min: 0.1, max: 2.0, step: 0.05, def: 0.3  },
      { name: 'height',        label: 'Height',        type: 'float', min: 0.1, max: 4.0, step: 0.1,  def: 1.2  }
    ]
  },
  {
    id: 'tree', label: 'Tree', group: 'Organic',
    color: [0.42, 0.62, 0.32],
    params: [
      { name: 'resolution',    label: 'Resolution',    type: 'int',   min: 4,   max: 32,  step: 4,    def: 10   },
      { name: 'depth',         label: 'Depth',         type: 'int',   min: 1,   max: 6,   step: 1,    def: 4    },
      { name: 'branch_count',  label: 'Branches',      type: 'int',   min: 2,   max: 6,   step: 1,    def: 3    },
      { name: 'branch_angle',  label: 'Branch Angle',  type: 'float', min: 0.1, max: 1.5, step: 0.05, def: 0.6  },
      { name: 'trunk_height',  label: 'Trunk Height',  type: 'float', min: 0.2, max: 4.0, step: 0.1,  def: 1.2  },
      { name: 'trunk_radius',  label: 'Trunk Radius',  type: 'float', min: 0.02, max: 0.5, step: 0.01, def: 0.08 },
      { name: 'seed',          label: 'Seed',          type: 'int',   min: 1,   max: 9999, step: 1,   def: 1337 }
    ]
  },
  {
    id: 'coral', label: 'Coral', group: 'Organic',
    color: [0.92, 0.55, 0.45],
    params: [
      { name: 'resolution',    label: 'Resolution',    type: 'int',   min: 4,   max: 32,  step: 4,    def: 10   },
      { name: 'depth',         label: 'Depth',         type: 'int',   min: 1,   max: 6,   step: 1,    def: 4    },
      { name: 'branch_count',  label: 'Branches',      type: 'int',   min: 2,   max: 6,   step: 1,    def: 4    },
      { name: 'branch_angle',  label: 'Branch Angle',  type: 'float', min: 0.1, max: 1.5, step: 0.05, def: 0.7  },
      { name: 'height',        label: 'Height',        type: 'float', min: 0.2, max: 4.0, step: 0.1,  def: 1.0  },
      { name: 'branch_radius', label: 'Branch Radius', type: 'float', min: 0.01, max: 0.2, step: 0.01, def: 0.05 },
      { name: 'seed',          label: 'Seed',          type: 'int',   min: 1,   max: 9999, step: 1,   def: 1337 }
    ]
  }
];

// ---------------------------------------------------------------------------
// NCF generator
// ---------------------------------------------------------------------------
function escHtml(s) {
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}
function fmtF(v) {
  if (Math.abs(v) < 0.001 && v !== 0) return v.toExponential(3);
  return parseFloat(v.toFixed(4)).toString();
}

function generateNCF(def, params, name) {
  const ind = '    ';
  const objName = (name && name.trim()) ? name.trim() : def.id;
  const entries = [
    ['source', `<span class="ncf-fn">gen</span>(<span class="ncf-val">${escHtml(def.id)}</span>)`]
  ];
  for (const pd of def.params) {
    const val = params[pd.name];
    let valHtml;
    if (pd.type === 'int') {
      valHtml = `<span class="ncf-val">${Math.round(val)}</span>`;
    } else {
      valHtml = `<span class="ncf-val">${fmtF(Number(val))}</span>`;
    }
    entries.push([pd.name, valHtml]);
  }
  const maxLen = Math.max(...entries.map(([k]) => k.length));
  const lines = [`<span class="ncf-key">${escHtml(objName)}</span> = {`];
  for (const [k, valHtml] of entries) {
    const pad = ' '.repeat(maxLen - k.length + 1);
    lines.push(`${ind}<span class="ncf-key">${escHtml(k)}</span>${pad}= ${valHtml}`);
  }
  lines.push('}');
  return lines.join('\n');
}

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------
function buildFormBody(gen, params) {
  const parts = [`gen=${encodeURIComponent(gen)}`];
  for (const [k, v] of Object.entries(params)) {
    parts.push(`${encodeURIComponent(k)}=${encodeURIComponent(v)}`);
  }
  return parts.join('&');
}

function fetchGeometry(genId, params) {
  return fetch('/api/geometry/generate', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: buildFormBody(genId, params)
  }).then(r => r.json());
}

// ---------------------------------------------------------------------------
// App state
// ---------------------------------------------------------------------------
let currentShape  = null;
let currentParams = {};
let displayMode   = 'solid';
let orbit         = { az: 0.6, el: 0.35, dist: 3.0 };
let mainCtx       = null;
let thumbCtx      = null;
let pendingGenId  = null;
const thumbSize   = 64;

// ---------------------------------------------------------------------------
// WebGL contexts
// ---------------------------------------------------------------------------
function initWebGL() {
  const thumbCanvas = document.createElement('canvas');
  thumbCtx = createCtx(thumbCanvas, thumbSize, thumbSize);

  const mainCanvas = document.getElementById('mainCanvas');
  mainCtx = createCtx(mainCanvas, mainCanvas.offsetWidth || 400, mainCanvas.offsetHeight || 300);
}

// ---------------------------------------------------------------------------
// Thumbnail generation queue
// ---------------------------------------------------------------------------
const thumbQueue = [];
let thumbBusy = false;

function enqueueThumbs() {
  for (const def of SHAPES) thumbQueue.push(def.id);
  processThumbQueue();
}

function processThumbQueue() {
  if (thumbBusy || !thumbQueue.length || !thumbCtx) return;
  thumbBusy = true;
  const id = thumbQueue.shift();
  const def = SHAPES.find(s => s.id === id);
  if (!def) { thumbBusy = false; processThumbQueue(); return; }

  const params = buildDefaultParams(def);
  fetchGeometry(def.id, params)
    .then(data => {
      if (!data.positions || !data.normals) return;
      requestAnimationFrame(() => {
        try {
          uploadMesh(thumbCtx, data.positions, data.normals);
          drawScene(thumbCtx, def.color, 0.6, 0.35, 3.0, false, thumbSize, thumbSize);
          const pixels = new Uint8Array(thumbSize * thumbSize * 4);
          thumbCtx.gl.readPixels(0, 0, thumbSize, thumbSize, thumbCtx.gl.RGBA, thumbCtx.gl.UNSIGNED_BYTE, pixels);
          const flipped = new Uint8ClampedArray(thumbSize * thumbSize * 4);
          for (let y = 0; y < thumbSize; y++) {
            const srcRow = (thumbSize - 1 - y) * thumbSize * 4;
            const dstRow = y * thumbSize * 4;
            flipped.set(pixels.subarray(srcRow, srcRow + thumbSize * 4), dstRow);
          }
          const img = new ImageData(flipped, thumbSize, thumbSize);
          const thumb2d = document.getElementById('thumb-' + id);
          if (thumb2d) {
            const ctx2d = thumb2d.getContext('2d');
            ctx2d.putImageData(img, 0, 0);
          }
        } catch(e) { console.error('thumb error', id, e); }
      });
    })
    .catch(e => console.error('thumb fetch error', id, e))
    .finally(() => { thumbBusy = false; processThumbQueue(); });
}

// ---------------------------------------------------------------------------
// Main render
// ---------------------------------------------------------------------------
function setGenerateState(loading) {
  const btn = document.getElementById('generateBtn');
  if (!btn) return;
  btn.disabled = loading;
  btn.textContent = loading ? 'Generating…' : 'Generate';
  btn.classList.toggle('loading', loading);
}

function renderFromData(data) {
  if (!mainCtx || !data || !data.positions) return;
  const canvas = document.getElementById('mainCanvas');
  const w = canvas.clientWidth * window.devicePixelRatio;
  const h = canvas.clientHeight * window.devicePixelRatio;
  canvas.width = w; canvas.height = h;
  uploadMesh(mainCtx, data.positions, data.normals);
  drawScene(mainCtx, currentShape.color, orbit.az, orbit.el, orbit.dist, displayMode === 'wire', w, h);
  document.getElementById('previewTris').textContent = (data.triangles || 0).toLocaleString() + ' tris';
}

function redraw() {
  if (!mainCtx || !mainCtx._vertCount) return;
  const canvas = document.getElementById('mainCanvas');
  const w = canvas.clientWidth * window.devicePixelRatio;
  const h = canvas.clientHeight * window.devicePixelRatio;
  canvas.width = w; canvas.height = h;
  drawScene(mainCtx, currentShape.color, orbit.az, orbit.el, orbit.dist, displayMode === 'wire', w, h);
}

function generate() {
  if (!currentShape) return;
  const genId = pendingGenId = currentShape.id;
  setGenerateState(true);
  document.getElementById('generateBtn').classList.remove('dirty');

  fetchGeometry(genId, currentParams)
    .then(data => {
      if (pendingGenId !== genId) return;
      if (data.error) {
        console.error('generate error:', data.error);
        return;
      }
      renderFromData(data);
    })
    .catch(e => console.error('generate fetch error', e))
    .finally(() => {
      if (pendingGenId === genId) setGenerateState(false);
    });
}

// ---------------------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------------------
function buildDefaultParams(def) {
  const p = {};
  def.params.forEach(pd => { p[pd.name] = pd.def; });
  return p;
}

function formatVal(v, type) {
  if (type === 'int') return Math.round(v).toString();
  return parseFloat(parseFloat(v).toFixed(4)).toString();
}

// ---------------------------------------------------------------------------
// Sidebar
// ---------------------------------------------------------------------------
function buildSidebar(filter) {
  const sidebar = document.getElementById('sidebar');
  const groups = {};
  for (const def of SHAPES) {
    if (filter && !def.label.toLowerCase().includes(filter.toLowerCase()) && !def.id.toLowerCase().includes(filter.toLowerCase())) continue;
    if (!groups[def.group]) groups[def.group] = [];
    groups[def.group].push(def);
  }
  sidebar.innerHTML = '';
  for (const [groupName, defs] of Object.entries(groups)) {
    const sec = document.createElement('div');
    sec.className = 'sp-section';
    const head = document.createElement('div');
    head.className = 'sp-section-head';
    head.textContent = groupName;
    sec.appendChild(head);
    for (const def of defs) {
      const item = document.createElement('div');
      item.className = 'sp-item' + (currentShape && currentShape.id === def.id ? ' active' : '');
      item.dataset.id = def.id;
      const canvas = document.createElement('canvas');
      canvas.id = 'thumb-' + def.id;
      canvas.width = thumbSize; canvas.height = thumbSize;
      canvas.className = 'sp-thumb';
      const label = document.createElement('span');
      label.className = 'sp-label';
      label.textContent = def.label;
      item.appendChild(canvas);
      item.appendChild(label);
      item.addEventListener('click', () => selectShape(def.id));
      sec.appendChild(item);
    }
    sidebar.appendChild(sec);
  }
}

// ---------------------------------------------------------------------------
// Params panel
// ---------------------------------------------------------------------------
function buildParams(def) {
  const body     = document.getElementById('paramsBody');
  const title    = document.getElementById('paramsTitle');
  const subtitle = document.getElementById('paramsSubtitle');
  title.textContent    = def.label;
  subtitle.textContent = def.group;
  document.getElementById('ncfBtn').classList.remove('sp-hidden');
  document.getElementById('generateBtn').classList.remove('sp-hidden');
  document.getElementById('generateBtn').classList.remove('dirty');

  body.innerHTML = '';
  for (const pd of def.params) {
    const row = document.createElement('div');
    row.className = 'sp-param-row';

    const lbl = document.createElement('label');
    lbl.className = 'sp-param-label';
    lbl.textContent = pd.label;

    const val = currentParams[pd.name];

    const valDisplay = document.createElement('span');
    valDisplay.className = 'sp-param-value';
    valDisplay.textContent = formatVal(val, pd.type);

    const slider = document.createElement('input');
    slider.type = 'range';
    slider.className = 'sp-param-slider';
    slider.min = pd.min; slider.max = pd.max;
    slider.step = pd.step;
    slider.value = val;

    slider.addEventListener('input', () => {
      const nv = pd.type === 'int' ? Math.round(parseFloat(slider.value)) : parseFloat(slider.value);
      currentParams[pd.name] = nv;
      valDisplay.textContent = formatVal(nv, pd.type);
      document.getElementById('generateBtn').classList.add('dirty');
      if (!ncfModal.hidden) refreshNCF();
    });

    row.appendChild(lbl);
    row.appendChild(valDisplay);
    row.appendChild(slider);
    body.appendChild(row);
  }

  if (def.params.length === 0) {
    const note = document.createElement('p');
    note.className = 'sp-params-empty';
    note.textContent = 'No parameters for this shape.';
    body.appendChild(note);
  }
}

// ---------------------------------------------------------------------------
// Select shape
// ---------------------------------------------------------------------------
function selectShape(id) {
  const def = SHAPES.find(s => s.id === id);
  if (!def) return;
  currentShape  = def;
  currentParams = buildDefaultParams(def);

  document.querySelectorAll('.sp-item').forEach(el => {
    el.classList.toggle('active', el.dataset.id === id);
  });

  document.getElementById('previewLabel').textContent = def.label;
  buildParams(def);
  generate();
}

// ---------------------------------------------------------------------------
// NCF modal
// ---------------------------------------------------------------------------
const ncfModal      = document.getElementById('ncfModal');
const ncfCode       = document.getElementById('ncfCode');
const ncfCopyBtn    = document.getElementById('ncfCopyBtn');
const ncfCloseBtn   = document.getElementById('ncfCloseBtn');
const ncfRefreshBtn = document.getElementById('ncfRefreshBtn');
const ncfNameInput  = document.getElementById('ncfNameInput');
const ncfBtn        = document.getElementById('ncfBtn');

function refreshNCF() {
  if (!currentShape) return;
  ncfCode.innerHTML = generateNCF(currentShape, currentParams, ncfNameInput.value);
}

ncfBtn.addEventListener('click', () => {
  if (!currentShape) return;
  if (!ncfNameInput.value) ncfNameInput.value = currentShape.id;
  refreshNCF();
  ncfModal.hidden = false;
});
ncfCloseBtn.addEventListener('click', () => { ncfModal.hidden = true; });
ncfModal.querySelector('.sp-modal-backdrop').addEventListener('click', () => { ncfModal.hidden = true; });
ncfRefreshBtn.addEventListener('click', refreshNCF);
ncfNameInput.addEventListener('input', refreshNCF);
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

// ---------------------------------------------------------------------------
// Generate button
// ---------------------------------------------------------------------------
document.getElementById('generateBtn').addEventListener('click', () => {
  generate();
});

// ---------------------------------------------------------------------------
// Orbit controls
// ---------------------------------------------------------------------------
(function() {
  const canvas = document.getElementById('mainCanvas');
  let dragging = false, lastX = 0, lastY = 0;

  canvas.addEventListener('mousedown', e => {
    dragging = true; lastX = e.clientX; lastY = e.clientY;
    canvas.classList.add('grabbing');
  });
  window.addEventListener('mouseup', () => { dragging = false; canvas.classList.remove('grabbing'); });
  window.addEventListener('mousemove', e => {
    if (!dragging) return;
    const dx = e.clientX - lastX, dy = e.clientY - lastY;
    orbit.az -= dx * 0.01;
    orbit.el = Math.max(-1.4, Math.min(1.4, orbit.el + dy * 0.01));
    lastX = e.clientX; lastY = e.clientY;
    redraw();
  });
  canvas.addEventListener('wheel', e => {
    e.preventDefault();
    orbit.dist = Math.max(0.5, Math.min(10, orbit.dist * (1 + e.deltaY * 0.001)));
    redraw();
  }, { passive: false });

  let lastTouchDist = 0;
  canvas.addEventListener('touchstart', e => {
    if (e.touches.length === 1) {
      dragging = true; lastX = e.touches[0].clientX; lastY = e.touches[0].clientY;
    } else if (e.touches.length === 2) {
      dragging = false;
      lastTouchDist = Math.hypot(e.touches[0].clientX-e.touches[1].clientX, e.touches[0].clientY-e.touches[1].clientY);
    }
    e.preventDefault();
  }, { passive: false });
  canvas.addEventListener('touchmove', e => {
    if (e.touches.length === 1 && dragging) {
      const dx = e.touches[0].clientX - lastX, dy = e.touches[0].clientY - lastY;
      orbit.az -= dx * 0.01;
      orbit.el = Math.max(-1.4, Math.min(1.4, orbit.el + dy * 0.01));
      lastX = e.touches[0].clientX; lastY = e.touches[0].clientY;
      redraw();
    } else if (e.touches.length === 2) {
      const d = Math.hypot(e.touches[0].clientX-e.touches[1].clientX, e.touches[0].clientY-e.touches[1].clientY);
      orbit.dist = Math.max(0.5, Math.min(10, orbit.dist * (lastTouchDist / d)));
      lastTouchDist = d;
      redraw();
    }
    e.preventDefault();
  }, { passive: false });
  canvas.addEventListener('touchend', () => { dragging = false; }, { passive: false });
})();

// ---------------------------------------------------------------------------
// Display mode toggle
// ---------------------------------------------------------------------------
document.querySelectorAll('.sp-chip[data-display]').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.sp-chip[data-display]').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    displayMode = btn.dataset.display;
    redraw();
  });
});

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------
document.getElementById('searchInput').addEventListener('input', e => {
  buildSidebar(e.target.value);
  enqueueThumbs();
});

// ---------------------------------------------------------------------------
// Resize
// ---------------------------------------------------------------------------
new ResizeObserver(() => redraw()).observe(document.getElementById('previewArea'));

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
initWebGL();
buildSidebar('');
enqueueThumbs();
selectShape('icosphere');

})();
