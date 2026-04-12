#include <algorithm>
#include <cstdint>
#include <cmath>

#include <nmath/vector.h>

#include "city.h"

namespace nmesh {
    namespace generator {

namespace {

typedef nmath::Vector3f Vec3;

static const Vec3 UP(0, 1, 0);

/* ---- PRNG ---- */

static uint32_t lcg(uint32_t &state)
{
    state = state * 1664525u + 1013904223u;
    return state;
}

static float randf(uint32_t &state)
{
    return (float)(lcg(state) >> 8) / (float)(1 << 24);
}

/* ---- Vertex / index helpers ---- */

static int append_vertex_uv(object_t *obj, const Vec3 &p, const Vec3 &n, float u, float v)
{
    const int idx = (int)(obj->attributes.v.size() / 3);
    obj->attributes.v.push_back(p.x);
    obj->attributes.v.push_back(p.y);
    obj->attributes.v.push_back(p.z);
    obj->attributes.n.push_back(n.x);
    obj->attributes.n.push_back(n.y);
    obj->attributes.n.push_back(n.z);
    obj->attributes.uv.push_back(u);
    obj->attributes.uv.push_back(v);
    return idx;
}

static void append_triangle_uv(shape_t &shape, int a, int b, int c)
{
    index_t ia; ia.v = a; ia.n = a; ia.uv = a;
    index_t ib; ib.v = b; ib.n = b; ib.uv = b;
    index_t ic; ic.v = c; ic.n = c; ic.uv = c;
    shape.mesh.indices.push_back(ia);
    shape.mesh.indices.push_back(ib);
    shape.mesh.indices.push_back(ic);
}

/* Adds a quad from four CCW world-space corners and an explicit normal. */
static void add_quad4(object_t *obj, shape_t &out,
                      const Vec3 &p0, const Vec3 &p1,
                      const Vec3 &p2, const Vec3 &p3,
                      const Vec3 &n)
{
    int i0 = append_vertex_uv(obj, p0, n, 0, 0);
    int i1 = append_vertex_uv(obj, p1, n, 1, 0);
    int i2 = append_vertex_uv(obj, p2, n, 1, 1);
    int i3 = append_vertex_uv(obj, p3, n, 0, 1);
    append_triangle_uv(out, i0, i1, i2);
    append_triangle_uv(out, i0, i2, i3);
}

/* Horizontal quad at fixed y, normal = +UP. */
static void add_hquad(object_t *obj, shape_t &out,
                      float x0, float z0, float x1, float z1, float y = 0)
{
    add_quad4(obj, out,
        Vec3(x0, y, z0), Vec3(x1, y, z0),
        Vec3(x1, y, z1), Vec3(x0, y, z1), UP);
}

/* ---- Facade (detailed wall with windows / doors) ---- */

struct Face {
    Vec3 bl;     /* bottom-left 3D world position of this face */
    Vec3 right;  /* unit horizontal direction along the face   */
    Vec3 normal; /* outward unit normal                        */
};

/*
 * Generates one detailed facade.
 *
 * Face-local coords: u along face.right, v along UP.
 * pt(u,v,d) = face.bl + face.right*u + UP*v - face.normal*d
 *
 * Winding conventions (verified with cross-product):
 *   wall panel / back:  p(u0,v0) → p(u1,v0) → p(u1,v1) → p(u0,v1), normal = face.normal
 *   left  reveal (u=iu0): (u,v0,0) → (u,v0,d) → (u,v1,d) → (u,v1,0),   normal = +face.right
 *   right reveal (u=iu1): (u,v0,d) → (u,v0,0) → (u,v1,0) → (u,v1,d),   normal = -face.right
 *   top   reveal (v=iv1): (u0,v,d) → (u1,v,d) → (u1,v,0) → (u0,v,0),   normal = -UP
 *   sill  reveal (v=iv0): (u1,v,d) → (u0,v,d) → (u0,v,0) → (u1,v,0),   normal = +UP
 */
static void add_facade(
    object_t *obj, shape_t &out,
    const Face &face,
    float fw, float fh,
    float floor_h, float bay_w,
    float win_wr, float win_hr, float inset,
    bool has_door, uint32_t &state)
{
    const Vec3 nrm  = face.normal;
    const Vec3 rgt  = face.right;
    const Vec3 nrgt = -face.right;

    auto pt = [&](float u, float v, float d) -> Vec3 {
        return face.bl + rgt * u + UP * v - nrm * d;
    };
    auto wall_q = [&](float u0, float v0, float u1, float v1) {
        add_quad4(obj, out, pt(u0,v0,0), pt(u1,v0,0), pt(u1,v1,0), pt(u0,v1,0), nrm);
    };

    const int nb  = std::max(1, (int)std::round(fw / bay_w));
    const int nf  = std::max(1, (int)std::round(fh / floor_h));
    const float abw = fw / (float)nb;
    const float afh = fh / (float)nf;

    int door_bay = -1;
    if (has_door) {
        door_bay = (int)(randf(state) * (float)nb);
        if (door_bay >= nb) door_bay = nb - 1;
    }

    for (int fi = 0; fi < nf; ++fi) {
        const float fv0 = fi * afh;
        const float fv1 = fv0 + afh;

        for (int bi = 0; bi < nb; ++bi) {
            const float bu0 = bi * abw;
            const float bu1 = bu0 + abw;
            const bool is_door = (fi == 0) && (bi == door_bay);

            float iu0, iv0, iu1, iv1;
            bool  no_sill;

            if (is_door) {
                const float dw   = abw * win_wr * 0.65f;
                const float hgap = (abw - dw) * 0.5f;
                iu0 = bu0 + hgap;
                iu1 = bu1 - hgap;
                iv0 = fv0;
                iv1 = fv0 + afh * 0.85f;
                no_sill = true;
            } else {
                const float mu    = abw * (1.0f - win_wr) * 0.5f;
                const float mv_b  = afh * (1.0f - win_hr) * 0.45f;
                const float mv_t  = afh * (1.0f - win_hr) * 0.55f;
                iu0 = bu0 + mu;
                iu1 = bu1 - mu;
                iv0 = fv0 + mv_b;
                iv1 = fv1 - mv_t;
                no_sill = false;
            }

            /* Skip degenerate openings on very narrow faces */
            if ((iu1 - iu0) < 1e-4f || (iv1 - iv0) < 1e-4f) {
                wall_q(bu0, fv0, bu1, fv1);
                continue;
            }

            /* Wall panels surrounding the opening */
            wall_q(bu0, fv0, iu0, fv1);
            wall_q(iu1, fv0, bu1, fv1);
            wall_q(iu0, iv1, iu1, fv1);
            if (!no_sill) wall_q(iu0, fv0, iu1, iv0);

            /* Opening back (recessed) */
            add_quad4(obj, out,
                pt(iu0,iv0,inset), pt(iu1,iv0,inset),
                pt(iu1,iv1,inset), pt(iu0,iv1,inset), nrm);

            /* Reveals */
            /* Left (at u=iu0), normal = +right */
            add_quad4(obj, out,
                pt(iu0,iv0,0), pt(iu0,iv0,inset),
                pt(iu0,iv1,inset), pt(iu0,iv1,0), rgt);
            /* Right (at u=iu1), normal = -right */
            add_quad4(obj, out,
                pt(iu1,iv0,inset), pt(iu1,iv0,0),
                pt(iu1,iv1,0), pt(iu1,iv1,inset), nrgt);
            /* Top lintel (at v=iv1), normal = -UP */
            add_quad4(obj, out,
                pt(iu0,iv1,inset), pt(iu1,iv1,inset),
                pt(iu1,iv1,0), pt(iu0,iv1,0), -UP);
            /* Bottom sill (at v=iv0), normal = +UP; skip for door */
            if (!no_sill) {
                add_quad4(obj, out,
                    pt(iu1,iv0,inset), pt(iu0,iv0,inset),
                    pt(iu0,iv0,0), pt(iu1,iv0,0), UP);
            }
        }
    }
}

/* ---- Building (top cap + 4 detailed facades) ---- */

static void add_building(
    object_t *obj, shape_t &out,
    float x0, float z0, float x1, float z1, float h,
    float floor_h, float bay_w,
    float win_wr, float win_hr, float inset,
    uint32_t &state)
{
    const float fw_x = x1 - x0;
    const float fw_z = z1 - z0;

    /* Clamp inset so it can't punch through a thin building */
    const float eff_inset = std::min(inset, std::min(fw_x, fw_z) * 0.25f);

    /* Pick which face has the door */
    const int door_face = (int)(randf(state) * 4.0f) % 4;

    /* Roof cap */
    add_quad4(obj, out,
        Vec3(x0,h,z0), Vec3(x1,h,z0), Vec3(x1,h,z1), Vec3(x0,h,z1), UP);

    /* +Z face */
    {
        Face f; f.bl = Vec3(x0,0,z1); f.right = Vec3(1,0,0); f.normal = Vec3(0,0,1);
        add_facade(obj, out, f, fw_x, h, floor_h, bay_w, win_wr, win_hr, eff_inset,
                   door_face == 0, state);
    }
    /* -Z face */
    {
        Face f; f.bl = Vec3(x1,0,z0); f.right = Vec3(-1,0,0); f.normal = Vec3(0,0,-1);
        add_facade(obj, out, f, fw_x, h, floor_h, bay_w, win_wr, win_hr, eff_inset,
                   door_face == 1, state);
    }
    /* +X face */
    {
        Face f; f.bl = Vec3(x1,0,z0); f.right = Vec3(0,0,1); f.normal = Vec3(1,0,0);
        add_facade(obj, out, f, fw_z, h, floor_h, bay_w, win_wr, win_hr, eff_inset,
                   door_face == 2, state);
    }
    /* -X face */
    {
        Face f; f.bl = Vec3(x0,0,z1); f.right = Vec3(0,0,-1); f.normal = Vec3(-1,0,0);
        add_facade(obj, out, f, fw_z, h, floor_h, bay_w, win_wr, win_hr, eff_inset,
                   door_face == 3, state);
    }
}

/* ---- Ground: road surface + raised pavement slabs + curbs ---- */

/*
 * Road is at Y=0.  Pavement is a raised slab at Y=pavement_height that
 * runs from the block edge outward by pavement_width into the road strip,
 * forming a kerb/curb visible from any lighting direction.
 *
 * For each block:
 *   - pavement slab (top face, +Y)
 *   - 4 curb side faces (vertical quads along each edge of the slab)
 *
 * Road surface: generated as one quad per road strip (horizontal and
 * vertical strips between blocks, plus corner intersections), excluding
 * the pavement footprints.
 *
 * For simplicity we generate the road as one large ground plane, then the
 * pavement slabs sit on top of it — the slight height difference creates
 * a visible step/shadow even with a single material.
 */
static void add_ground(
    object_t *obj, shape_t &out,
    int blocks_x, int blocks_z,
    float ox, float oz,
    float step_x, float step_z,
    float road_width, float block_size,
    float total_w, float total_d,
    float pav_h, float pav_w)
{
    /* Full road surface at Y=0 */
    add_hquad(obj, out, ox, oz, ox + total_w, oz + total_d, 0.0f);

    /* Clamp pavement width so it doesn't invade the block area */
    const float eff_pw = std::min(pav_w, road_width * 0.45f);
    if (pav_h <= 0.0f || eff_pw <= 0.0f) return;

    for (int bz = 0; bz < blocks_z; ++bz) {
        for (int bx = 0; bx < blocks_x; ++bx) {
            /* Block corners in world space */
            const float bx0 = ox + road_width + bx * step_x;
            const float bz0 = oz + road_width + bz * step_z;
            const float bx1 = bx0 + block_size;
            const float bz1 = bz0 + block_size;

            /* Pavement slab extends eff_pw outside the block edge */
            const float px0 = bx0 - eff_pw;
            const float pz0 = bz0 - eff_pw;
            const float px1 = bx1 + eff_pw;
            const float pz1 = bz1 + eff_pw;
            const float ph  = pav_h;

            /* Pavement top face */
            add_hquad(obj, out, px0, pz0, px1, pz1, ph);

            /* Curb sides */
            /* -Z curb (south) */
            add_quad4(obj, out,
                Vec3(px1,0,pz0), Vec3(px0,0,pz0),
                Vec3(px0,ph,pz0), Vec3(px1,ph,pz0), Vec3(0,0,-1));
            /* +Z curb (north) */
            add_quad4(obj, out,
                Vec3(px0,0,pz1), Vec3(px1,0,pz1),
                Vec3(px1,ph,pz1), Vec3(px0,ph,pz1), Vec3(0,0,1));
            /* -X curb (west) */
            add_quad4(obj, out,
                Vec3(px0,0,pz0), Vec3(px0,0,pz1),
                Vec3(px0,ph,pz1), Vec3(px0,ph,pz0), Vec3(-1,0,0));
            /* +X curb (east) */
            add_quad4(obj, out,
                Vec3(px1,0,pz1), Vec3(px1,0,pz0),
                Vec3(px1,ph,pz0), Vec3(px1,ph,pz1), Vec3(1,0,0));
        }
    }
}

} /* namespace */

/* ---- Public entry point ---- */

void city(object_t *obj,
          int   seed,
          int   blocks_x,
          int   blocks_z,
          float block_size,
          float road_width,
          float building_height_min,
          float building_height_max,
          float lot_padding,
          int   buildings_per_block_x,
          int   buildings_per_block_z,
          float floor_height,
          float bay_width,
          float window_width_ratio,
          float window_height_ratio,
          float window_inset,
          float pavement_height,
          float pavement_width)
{
    if (!obj) return;

    blocks_x = std::max(1, blocks_x);
    blocks_z = std::max(1, blocks_z);
    block_size   = std::max(0.01f, block_size);
    road_width   = std::max(0.001f, road_width);
    building_height_min = std::max(0.001f, building_height_min);
    building_height_max = std::max(building_height_min, building_height_max);
    lot_padding  = std::max(0.0f, lot_padding);
    buildings_per_block_x = std::max(1, buildings_per_block_x);
    buildings_per_block_z = std::max(1, buildings_per_block_z);
    floor_height = std::max(0.01f, floor_height);
    bay_width    = std::max(0.01f, bay_width);
    window_width_ratio  = std::max(0.1f, std::min(0.95f, window_width_ratio));
    window_height_ratio = std::max(0.1f, std::min(0.95f, window_height_ratio));
    window_inset = std::max(0.0f, window_inset);
    pavement_height = std::max(0.0f, pavement_height);
    pavement_width  = std::max(0.0f, pavement_width);

    shape_t shape;
    obj->shapes.push_back(shape);
    shape_t &out = obj->shapes.back();

    const float step_x  = block_size + road_width;
    const float step_z  = block_size + road_width;
    const float total_w = blocks_x * step_x + road_width;
    const float total_d = blocks_z * step_z + road_width;
    const float ox      = -total_w * 0.5f;
    const float oz      = -total_d * 0.5f;

    /* Ground: road surface + pavement slabs + curbs */
    add_ground(obj, out,
               blocks_x, blocks_z, ox, oz,
               step_x, step_z, road_width, block_size,
               total_w, total_d,
               pavement_height, pavement_width);

    /* Buildings */
    const float lot_w = block_size / (float)buildings_per_block_x;
    const float lot_d = block_size / (float)buildings_per_block_z;

    for (int bz = 0; bz < blocks_z; ++bz) {
        for (int bx = 0; bx < blocks_x; ++bx) {
            const float block_x0 = ox + road_width + bx * step_x;
            const float block_z0 = oz + road_width + bz * step_z;

            uint32_t state = (uint32_t)seed
                ^ ((uint32_t)bx * 73856093u)
                ^ ((uint32_t)bz * 19349663u);
            lcg(state);

            for (int lz = 0; lz < buildings_per_block_z; ++lz) {
                for (int lx = 0; lx < buildings_per_block_x; ++lx) {
                    float x0 = block_x0 + lx * lot_w + lot_padding;
                    float z0 = block_z0 + lz * lot_d + lot_padding;
                    float x1 = block_x0 + (lx + 1) * lot_w - lot_padding;
                    float z1 = block_z0 + (lz + 1) * lot_d - lot_padding;
                    if (x1 <= x0 || z1 <= z0) continue;

                    const float t = randf(state);
                    const float h = building_height_min
                                  + t * (building_height_max - building_height_min);

                    /* Random footprint variation */
                    const float fw = x1 - x0;
                    const float fd = z1 - z0;
                    const float sx = randf(state) * 0.2f;
                    const float sz = randf(state) * 0.2f;
                    x0 += sx * fw;  x1 -= sx * fw;
                    z0 += sz * fd;  z1 -= sz * fd;
                    if (x1 <= x0 || z1 <= z0) continue;

                    add_building(obj, out, x0, z0, x1, z1, h,
                                 floor_height, bay_width,
                                 window_width_ratio, window_height_ratio,
                                 window_inset, state);
                }
            }
        }
    }
}

    } /* namespace generator */
} /* namespace nmesh */
