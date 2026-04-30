function clamp(value, lo, hi) {
  return Math.min(hi, Math.max(lo, value));
}

function v3(x, y, z) {
  return [Number(x) || 0, Number(y) || 0, Number(z) || 0];
}

function v3add(a, b) {
  return [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
}

function v3sub(a, b) {
  return [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
}

function v3scale(a, s) {
  return [a[0] * s, a[1] * s, a[2] * s];
}

function v3dot(a, b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

function v3cross(a, b) {
  return [
    a[1] * b[2] - a[2] * b[1],
    a[2] * b[0] - a[0] * b[2],
    a[0] * b[1] - a[1] * b[0],
  ];
}

function v3len(a) {
  return Math.sqrt(v3dot(a, a));
}

function v3norm(a, fallback) {
  const l = v3len(a);
  if (!Number.isFinite(l) || l < 1e-8) return fallback ? [...fallback] : [0, 0, 1];
  return [a[0] / l, a[1] / l, a[2] / l];
}

// Rodrigues' rotation formula: rotate v around a unit axis by radians.
function rotateAroundAxis(v, axisUnit, radians) {
  const c = Math.cos(radians);
  const s = Math.sin(radians);
  const term1 = v3scale(v, c);
  const term2 = v3scale(v3cross(axisUnit, v), s);
  const term3 = v3scale(axisUnit, v3dot(axisUnit, v) * (1 - c));
  return v3add(v3add(term1, term2), term3);
}
