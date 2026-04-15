Add a new procedural mesh generator to xtracer.

## What you're doing

Generators are invoked from scene files as `source = gen(my_generator)`. Adding one requires edits to exactly three files plus an optional declaration in a fourth.

---

## File map

| File | Role |
|------|------|
| `src/xtcore/proto.h` | Token `#define XTPROTO_LTRL_MY_GEN "my_gen"` |
| `lib/nmesh/extras.h` | Function declaration in `nmesh::generator::` namespace |
| `lib/nmesh/extras.cc` | Function implementation |
| `src/xtcore/parseutil.cc` | Dispatch `else if` block + any mesh post-processing helpers |

---

## Step 1 — proto.h

Add a literal token near line 248 (after `XTPROTO_LTRL_CITY`):

```cpp
#define XTPROTO_LTRL_MY_GEN "my_gen" /* string */ /* One-line description */
```

If your generator takes parameters that don't already have tokens, add them too:

```cpp
#define XTPROTO_PROP_MY_PARAM "my_param" /* scalar_t */ /* Description */
```

Existing reusable tokens: `XTPROTO_PROP_RESOLUTION`, `XTPROTO_PROP_SEED`, `XTPROTO_PROP_DIMENSIONS`, `XTPROTO_PROP_RADIUS`, `XTPROTO_PROP_TURNS`, `XTPROTO_PROP_OCTAVES`, `XTPROTO_PROP_LACUNARITY`, `XTPROTO_PROP_GAIN`, `XTPROTO_PROP_HEIGHT_SAMPLER`.

---

## Step 2 — extras.h

Declare in the `nmesh::generator` namespace (after the last entry, before the closing braces):

```cpp
void my_gen(object_t *obj, size_t resolution = 32, float my_param = 1.0f);
```

---

## Step 3 — extras.cc

### Data structures

```cpp
// All generators write into obj->attributes and obj->shapes.
// object_t:
//   attrib_t attributes { vector<float> v, n, uv }  — flat arrays, 3 floats per vertex/normal, 2 per uv
//   vector<shape_t> shapes { mesh_t mesh { vector<index_t> indices } }
//   index_t { int v, n, uv }  — always set v == n; set uv = -1 if no UV

// Two static helpers are available throughout extras.cc:
static int  append_vertex(object_t *obj, const Vec3 &p, const Vec3 &n, const uv_t *uv = 0);
static void append_quad(shape_t &shape, int a, int b, int c, int d, bool has_uv = false);
static void append_triangle(shape_t &shape, int a, int b, int c, bool has_uv = false);
// append_vertex returns the new vertex index
// append_quad emits two triangles: (a,b,c) and (a,c,d)
```

### Minimal implementation template

```cpp
void my_gen(object_t *obj, size_t resolution, float my_param)
{
    if (!obj) return;

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    // Build vertices — call append_vertex, collect returned indices
    // Build faces   — call append_quad / append_triangle with those indices

    // Example: single flat quad
    const Vec3 n(0.0f, 1.0f, 0.0f);
    const int i0 = append_vertex(obj, Vec3(-1, 0, -1), n);
    const int i1 = append_vertex(obj, Vec3( 1, 0, -1), n);
    const int i2 = append_vertex(obj, Vec3( 1, 0,  1), n);
    const int i3 = append_vertex(obj, Vec3(-1, 0,  1), n);
    append_quad(out, i0, i1, i2, i3, false);
}
```

### For flat-shaded (faceted) geometry

Do NOT share vertices between triangles. For each triangle, compute the face normal and use it for all three vertices:

```cpp
Vec3 edge1 = vb - va, edge2 = vc - va;
Vec3 face_n = edge1.cross(edge2);
if (face_n.length() > 1e-8f) face_n.normalize();
else face_n = Vec3(0, 1, 0);
const int ia = append_vertex(obj, va, face_n);
const int ib = append_vertex(obj, vb, face_n);
const int ic = append_vertex(obj, vc, face_n);
append_triangle(out, ia, ib, ic, false);
```

### Coordinate system

Right-handed, Y-up. `Vec3` is `nmath::Vector3f`. Use `nmath_sin`, `nmath_cos` (from `<nmath/maths.h>`) for trig.

---

## Step 4 — parseutil.cc dispatch

Add an `else if` block **before** the final `else Log::handle()...` line (around line 1420), after the `city` block:

```cpp
else if (!token.compare(XTPROTO_LTRL_MY_GEN)) {
    int i = deserialize_numi(p ? p->get_property_by_name(XTPROTO_PROP_RESOLUTION) : 0, 32);
    if (i < 2) i = 2;
    if (i > 1024) i = 1024;
    float my_param = (float)deserialize_numf(p ? p->get_property_by_name(XTPROTO_PROP_MY_PARAM) : 0, 1.0f);
    nmesh::generator::my_gen(&obj, (size_t)i, my_param);
}
```

**Terrain-style generators** (plane + heightfield): follow the pattern used by `lowpoly_terrain` — call `nmesh::generator::plane`, then `apply_heightfield_to_mesh`, then any mesh post-processing.

**Helper deserialization functions available:**

```cpp
int   deserialize_numi(const ncf::NCF *p, int default_val);
float deserialize_numf(const ncf::NCF *p, float default_val);
nmath::Vector3f deserialize_vec3(const ncf::NCF *p, const char *name, nmath::Vector3f default_val);
```

---

## Step 5 — scene file usage

```
geometry = {
    my_shape = {
        type       = mesh
        source     = gen(my_gen)
        resolution = 48
        my_param   = 2.0
        modifiers  = {
            translation = vec3(0, 0, 0)
            rotation    = vec3(0, 0, 0)
            scale       = vec3(1, 1, 1)
        }
    }
}
```

For terrain-style generators, the `height_sampler` block is also available:

```
height_sampler = {
    type            = scenery_heightfield
    seed            = 1337
    scale           = 0.28
    octaves         = 4
    lacunarity      = 2.0
    gain            = 0.52
    ridge_strength  = 0.82
    mountain_strength = 0.90
    valley_strength = 0.40
}
```

---

## Build

```
make
```

From the repo root. The generator is compiled into `lib/nmesh/extras.cc`; the dispatch is in `src/xtcore/parseutil.cc`.

---

## Checklist

- [ ] Token added to `proto.h`
- [ ] Declaration added to `extras.h`
- [ ] Implementation in `extras.cc` — no vertex sharing between triangles if flat shading is needed
- [ ] Dispatch `else if` added to `parseutil.cc` before the final `else Log` line
- [ ] Scene file uses `source = gen(my_gen)` with correct parameter names
- [ ] Builds without warnings
