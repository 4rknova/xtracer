#define PUGIXML_HEADER_ONLY
#include "converter.h"
#include <pugixml.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

static std::string path_stem(const std::string& path) {
    size_t sep = path.find_last_of("/\\");
    std::string name = (sep != std::string::npos) ? path.substr(sep + 1) : path;
    size_t dot = name.find_last_of('.');
    return (dot != std::string::npos) ? name.substr(0, dot) : name;
}

static std::string path_basename(const std::string& path) {
    size_t sep = path.find_last_of("/\\");
    return (sep != std::string::npos) ? path.substr(sep + 1) : path;
}

static std::string path_join(const std::string& dir, const std::string& rel) {
    if (dir.empty()) return rel;
    if (rel.empty()) return dir;
    // If rel is already absolute, return it as-is
    if (rel[0] == '/' || rel[0] == '\\') return rel;
    return dir + "/" + rel;
}

// ─── math helpers ─────────────────────────────────────────────────────────────

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

static Vec3 parse_vec3(const std::string& s) {
    Vec3 v;
    std::string t = s;
    std::replace(t.begin(), t.end(), ',', ' ');
    std::sscanf(t.c_str(), "%f %f %f", &v.x, &v.y, &v.z);
    return v;
}

static float v3len(Vec3 v) { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }

// Row-major 3×3 rotation matrix → ZYX Euler angles in radians (xtracer passes to cos/sin directly)
static Vec3 mat3_to_euler_zyx(const float m[9]) {
    float sy = std::sqrt(m[0]*m[0] + m[3]*m[3]);
    float rx, ry, rz;
    if (sy > 1e-6f) {
        rx = std::atan2( m[7], m[8]);
        ry = std::atan2(-m[6], sy);
        rz = std::atan2( m[3], m[0]);
    } else {
        rx = std::atan2(-m[5], m[4]);
        ry = std::atan2(-m[6], sy);
        rz = 0;
    }
    return { rx, ry, rz };
}

struct Transform {
    Vec3 translate {0,0,0};
    Vec3 rotate    {0,0,0};   // ZYX Euler, radians
    Vec3 scale     {1,1,1};
    float rot3[9]  = {};      // raw 3×3 rotation (row-major), set when parsed from <matrix>
    bool has_rot3  = false;
    bool has_translate = false;
    bool has_rotate    = false;
    bool has_scale     = false;
    // lookat form (for sensors)
    bool has_lookat = false;
    Vec3 origin, target, up {0,1,0};
};

// ─── NCF writer ───────────────────────────────────────────────────────────────

class NCFWriter {
    std::ostream& out_;
    int depth_;

    void ind() { for (int i = 0; i < depth_; ++i) out_ << "    "; }

public:
    explicit NCFWriter(std::ostream& o, int initial_depth = 0)
        : out_(o), depth_(initial_depth) {}

    void blank()   { out_ << "\n"; }
    void comment(const std::string& s) { ind(); out_ << "# " << s << "\n"; }

    void kv(const std::string& key, const std::string& val) {
        ind(); out_ << key << " = " << val << "\n";
    }

    void begin(const std::string& name) {
        ind(); out_ << name << " = {\n";
        ++depth_;
    }

    void end() {
        --depth_;
        ind(); out_ << "}\n";
    }

    // key = { k=v, k=v } on a single line
    void inline_block(const std::string& key,
                      std::initializer_list<std::pair<const char*, std::string>> kvs) {
        ind(); out_ << key << " = {";
        bool first = true;
        for (auto& [k, v] : kvs) {
            if (!first) out_ << ",";
            out_ << " " << k << " = " << v;
            first = false;
        }
        out_ << " }\n";
    }
};

// ─── string helpers ───────────────────────────────────────────────────────────

static std::string sanitize(const std::string& s) {
    std::string r = s;
    for (char& c : r)
        if (!std::isalnum((unsigned char)c) && c != '_') c = '_';
    if (!r.empty() && std::isdigit((unsigned char)r[0])) r = "_" + r;
    return r;
}

static std::string ff(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%g", v);
    return buf;
}

static std::string col3(float r, float g, float b) {
    return "col3(" + ff(r) + ", " + ff(g) + ", " + ff(b) + ")";
}

static std::string vec3(Vec3 v) {
    return "vec3(" + ff(v.x) + ", " + ff(v.y) + ", " + ff(v.z) + ")";
}

// ─── converter ────────────────────────────────────────────────────────────────

class Converter {
    const ConvertOptions& opts_;
    std::ostream& err_;

    std::unordered_map<std::string, std::string> defaults_;
    std::unordered_map<std::string, pugi::xml_node> id_map_;

    int shape_count_ = 0;
    int mat_count_   = 0;
    std::unordered_set<std::string> emitted_mats_;
    std::vector<ResourceEntry>* resources_ = nullptr;

    // ── defaults & ref resolution ─────────────────────────────────────────────

    std::string resolve(std::string s) const {
        size_t p = 0;
        while ((p = s.find('$', p)) != std::string::npos) {
            size_t e = p + 1;
            while (e < s.size() && (std::isalnum((unsigned char)s[e]) || s[e] == '_')) ++e;
            std::string var = s.substr(p + 1, e - p - 1);
            auto it = defaults_.find(var);
            if (it != defaults_.end()) { s.replace(p, e - p, it->second); }
            else ++p;
        }
        return s;
    }

    pugi::xml_node deref(pugi::xml_node n) const {
        if (strcmp(n.name(), "ref") == 0) {
            const char* id = n.attribute("id").value();
            auto it = id_map_.find(id);
            if (it != id_map_.end()) return it->second;
        }
        return n;
    }

    // Find a child with a given `name` attribute (Mitsuba parameter convention)
    pugi::xml_node named(pugi::xml_node parent, const char* param) const {
        for (auto c : parent.children())
            if (strcmp(c.attribute("name").value(), param) == 0)
                return deref(c);
        return {};
    }

    // ── typed accessors ───────────────────────────────────────────────────────

    float get_float(pugi::xml_node p, const char* name, float def) const {
        auto c = named(p, name);
        if (!c) return def;
        try { return std::stof(resolve(c.attribute("value").value())); }
        catch (...) { return def; }
    }

    std::string get_string(pugi::xml_node p, const char* name, const char* def = "") const {
        auto c = named(p, name);
        return c ? resolve(c.attribute("value").value()) : def;
    }

    bool get_bool(pugi::xml_node p, const char* name, bool def) const {
        auto c = named(p, name);
        if (!c) return def;
        std::string v = resolve(c.attribute("value").value());
        return v == "true" || v == "1";
    }

    bool has_param(pugi::xml_node parent, const char* name) const {
        for (auto c : parent.children())
            if (strcmp(c.attribute("name").value(), name) == 0) return true;
        return false;
    }

    Vec3 get_color(pugi::xml_node parent, const char* name, Vec3 def = {0.8f,0.8f,0.8f}) const {
        for (auto c : parent.children()) {
            if (strcmp(c.attribute("name").value(), name) != 0) continue;
            std::string tag = c.name();
            if (tag == "rgb" || tag == "srgb" || tag == "spectrum") {
                std::string v = resolve(c.attribute("value").value());
                std::string t = v;
                std::replace(t.begin(), t.end(), ',', ' ');
                Vec3 col;
                int n = std::sscanf(t.c_str(), "%f %f %f", &col.x, &col.y, &col.z);
                if (n == 1) col.y = col.z = col.x;
                if (tag == "srgb") {
                    auto g = [](float x){ return std::pow(std::max(x, 0.0f), 2.2f); };
                    col = { g(col.x), g(col.y), g(col.z) };
                }
                return col;
            }
            if (tag == "float" || tag == "integer") {
                try {
                    float v = std::stof(resolve(c.attribute("value").value()));
                    return {v, v, v};
                } catch (...) {}
            }
        }
        return def;
    }

    Vec3 get_point(pugi::xml_node parent, const char* name, Vec3 def = {0,0,0}) const {
        for (auto c : parent.children()) {
            if (strcmp(c.attribute("name").value(), name) != 0) continue;
            std::string v = c.attribute("value").value();
            if (!v.empty()) return parse_vec3(resolve(v));
            Vec3 r = def;
            if (c.attribute("x")) r.x = std::stof(c.attribute("x").value());
            if (c.attribute("y")) r.y = std::stof(c.attribute("y").value());
            if (c.attribute("z")) r.z = std::stof(c.attribute("z").value());
            return r;
        }
        return def;
    }

    // ── transform parsing ─────────────────────────────────────────────────────

    Transform parse_transform(pugi::xml_node parent, const char* param = "to_world") const {
        Transform t;
        auto tf = named(parent, param);
        if (!tf) return t;

        for (auto c : tf.children()) {
            std::string tag = c.name();

            if (tag == "lookat") {
                t.has_lookat = true;
                auto read_pt = [&](const char* attr, const char* xa, const char* ya, const char* za, Vec3 fallback) -> Vec3 {
                    std::string v = c.attribute(attr).value();
                    if (!v.empty()) return parse_vec3(resolve(v));
                    Vec3 r = fallback;
                    if (c.attribute(xa)) r.x = std::stof(c.attribute(xa).value());
                    if (c.attribute(ya)) r.y = std::stof(c.attribute(ya).value());
                    if (c.attribute(za)) r.z = std::stof(c.attribute(za).value());
                    return r;
                };
                t.origin = read_pt("origin", "ox", "oy", "oz", {0,0,5});
                t.target = read_pt("target", "tx", "ty", "tz", {0,0,0});
                t.up     = read_pt("up",     "ux", "uy", "uz", {0,1,0});
            }
            else if (tag == "translate") {
                t.has_translate = true;
                std::string v = c.attribute("value").value();
                if (!v.empty()) { t.translate = parse_vec3(resolve(v)); }
                else {
                    if (c.attribute("x")) t.translate.x = std::stof(c.attribute("x").value());
                    if (c.attribute("y")) t.translate.y = std::stof(c.attribute("y").value());
                    if (c.attribute("z")) t.translate.z = std::stof(c.attribute("z").value());
                }
            }
            else if (tag == "scale") {
                t.has_scale = true;
                std::string v = c.attribute("value").value();
                if (!v.empty()) {
                    std::string tmp = v;
                    std::replace(tmp.begin(), tmp.end(), ',', ' ');
                    float x, y, z;
                    int n = std::sscanf(tmp.c_str(), "%f %f %f", &x, &y, &z);
                    t.scale = (n == 1) ? Vec3{x, x, x} : Vec3{x, y, z};
                } else {
                    if (c.attribute("x")) t.scale.x = std::stof(c.attribute("x").value());
                    if (c.attribute("y")) t.scale.y = std::stof(c.attribute("y").value());
                    if (c.attribute("z")) t.scale.z = std::stof(c.attribute("z").value());
                }
            }
            else if (tag == "rotate") {
                t.has_rotate = true;
                const float D2R = 3.14159265359f / 180.0f;
                float angle = (c.attribute("angle") ? std::stof(c.attribute("angle").value()) : 0.f) * D2R;
                float ax = 0, ay = 0, az = 0;
                if (c.attribute("x")) ax = std::stof(c.attribute("x").value());
                if (c.attribute("y")) ay = std::stof(c.attribute("y").value());
                if (c.attribute("z")) az = std::stof(c.attribute("z").value());
                if (c.attribute("axis")) {
                    Vec3 a = parse_vec3(c.attribute("axis").value());
                    ax = a.x; ay = a.y; az = a.z;
                }
                // Pure axis rotations map cleanly; general axis → approximate
                if      (ax != 0 && ay == 0 && az == 0) t.rotate.x += angle * ax;
                else if (ay != 0 && ax == 0 && az == 0) t.rotate.y += angle * ay;
                else if (az != 0 && ax == 0 && ay == 0) t.rotate.z += angle * az;
                else {
                    if (opts_.verbose)
                        err_ << "Warning: general axis-angle rotation not fully supported\n";
                    // Distribute onto dominant component
                    float len = std::sqrt(ax*ax + ay*ay + az*az);
                    if (len > 1e-6f) {
                        t.rotate.x += angle * ax / len;
                        t.rotate.y += angle * ay / len;
                        t.rotate.z += angle * az / len;
                    }
                }
            }
            else if (tag == "matrix") {
                // Row-major 4×4
                std::string v = c.attribute("value").value();
                float m[16] = {};
                std::istringstream ss(v);
                for (float& f : m) ss >> f;

                t.has_translate = true;
                t.translate = { m[3], m[7], m[11] };

                float sx = v3len({m[0], m[4], m[8]});
                float sy = v3len({m[1], m[5], m[9]});
                float sz = v3len({m[2], m[6], m[10]});
                if (sx > 1e-6f && sy > 1e-6f && sz > 1e-6f) {
                    t.has_scale = true;
                    t.scale = { sx, sy, sz };
                } else { sx = sy = sz = 1; }

                float r[9] = {
                    m[0]/sx, m[1]/sy, m[2]/sz,
                    m[4]/sx, m[5]/sy, m[6]/sz,
                    m[8]/sx, m[9]/sy, m[10]/sz
                };
                std::copy(r, r+9, t.rot3);
                t.has_rot3 = true;
                Vec3 euler = mat3_to_euler_zyx(r);
                if (euler.x != 0 || euler.y != 0 || euler.z != 0) {
                    t.has_rotate = true;
                    t.rotate = euler;
                }

                // Extract lookat from camera-to-world matrix.
                // Mitsuba 3 cameras look along +Z in camera space (not -Z like OpenGL).
                // Column 1 = up, column 2 = forward (+Z), column 3 = position.
                t.has_lookat = true;
                t.origin = t.translate;
                float up_len = v3len({m[1], m[5], m[9]});
                t.up = (up_len > 1e-6f)
                    ? Vec3{m[1]/up_len, m[5]/up_len, m[9]/up_len}
                    : Vec3{0.f, 1.f, 0.f};
                float fwd_len = v3len({m[2], m[6], m[10]});
                Vec3 fwd = (fwd_len > 1e-6f)
                    ? Vec3{m[2]/fwd_len, m[6]/fwd_len, m[10]/fwd_len}
                    : Vec3{0.f, 0.f, 1.f};
                t.target = Vec3{t.origin.x + fwd.x, t.origin.y + fwd.y, t.origin.z + fwd.z};
            }
        }
        return t;
    }

    // ── asset path rewriting ──────────────────────────────────────────────────

    // Returns the path to use in the .scn, and records the resource for copying.
    std::string rewrite_texture(const std::string& src) {
        if (!resources_ || opts_.dest_dir.empty() || src.empty()) return src;
        std::string fname = path_basename(src);
        std::string abs   = path_join(opts_.xml_dir, src);
        // Deduplicate by (abs, type)
        for (auto& r : *resources_)
            if (r.src_path == abs && r.type == "texture") return "resources/textures/" + fname;
        resources_->push_back({abs, "texture", fname});
        return "resources/textures/" + fname;
    }

    std::string rewrite_geometry(const std::string& src) {
        if (!resources_ || opts_.dest_dir.empty() || src.empty()) return src;
        std::string fname = path_basename(src);
        std::string abs   = path_join(opts_.xml_dir, src);
        for (auto& r : *resources_)
            if (r.src_path == abs && r.type == "geometry") return "resources/geometry/" + fname;
        resources_->push_back({abs, "geometry", fname});
        return "resources/geometry/" + fname;
    }

    // ── sampler emission ──────────────────────────────────────────────────────

    // Emits one sampler for a given Mitsuba parameter name.
    // Returns false if the parameter was not found (caller emits fallback).
    bool emit_sampler(NCFWriter& w, pugi::xml_node parent,
                      const char* param, const std::string& key) {
        for (auto c : parent.children()) {
            if (strcmp(c.attribute("name").value(), param) != 0) continue;

            // Resolve refs
            pugi::xml_node node = (strcmp(c.name(), "ref") == 0) ? deref(c) : c;
            std::string tag = node.name();

            if (tag == "rgb" || tag == "srgb" || tag == "spectrum" ||
                tag == "float" || tag == "integer") {
                Vec3 col = get_color(parent, param);
                w.inline_block(key, {{"type","color"}, {"value", col3(col.x, col.y, col.z)}});
                return true;
            }

            if (tag == "texture") {
                std::string ttype = node.attribute("type").value();

                if (ttype == "bitmap") {
                    std::string src = rewrite_texture(resolve(get_string(node, "filename")));
                    w.inline_block(key, {{"type","texture"}, {"source", src}, {"filtering","bilinear"}});
                    return true;
                }

                if (ttype == "checkerboard") {
                    Vec3 a = get_color(node, "color0", {0.4f,0.4f,0.4f});
                    Vec3 b = get_color(node, "color1", {0.2f,0.2f,0.2f});
                    w.begin(key);
                    w.kv("type", "checker");
                    w.inline_block("a", {{"type","color"}, {"value", col3(a.x, a.y, a.z)}});
                    w.inline_block("b", {{"type","color"}, {"value", col3(b.x, b.y, b.z)}});
                    w.end();
                    return true;
                }

                if (ttype == "scale") {
                    // Unwrap inner texture; scale value is lost
                    if (opts_.verbose)
                        err_ << "Warning: 'scale' texture wrapper not supported; inner used\n";
                    for (auto ic : node.children()) {
                        pugi::xml_node inner = (strcmp(ic.name(), "ref") == 0) ? deref(ic) : ic;
                        if (strcmp(inner.name(), "texture") == 0) {
                            std::string src = resolve(get_string(inner, "filename"));
                            if (!src.empty()) {
                                w.inline_block(key, {{"type","texture"}, {"source",src}, {"filtering","bilinear"}});
                                return true;
                            }
                        }
                    }
                }

                // Unknown texture type — grey fallback
                if (opts_.verbose)
                    err_ << "Warning: unsupported texture type '" << ttype << "', using grey\n";
                w.inline_block(key, {{"type","color"}, {"value","col3(0.5, 0.5, 0.5)"}});
                return true;
            }
        }
        return false;
    }

    void emit_sampler_or(NCFWriter& w, pugi::xml_node parent, const char* param,
                         const std::string& key, Vec3 fallback = {0.8f,0.8f,0.8f}) {
        if (!emit_sampler(w, parent, param, key))
            w.inline_block(key, {{"type","color"}, {"value", col3(fallback.x, fallback.y, fallback.z)}});
    }

    void emit_emissive_sampler(NCFWriter& w, pugi::xml_node emitter) {
        Vec3 r = get_color(emitter, "radiance", {1,1,1});
        float s = get_float(emitter, "scale", 1.0f);
        r = { r.x*s, r.y*s, r.z*s };
        w.inline_block("emissive", {{"type","color"}, {"value", col3(r.x, r.y, r.z)}});
    }

    // ── BSDF → material ───────────────────────────────────────────────────────

    void emit_material(NCFWriter& w, pugi::xml_node bsdf, const std::string& name,
                       pugi::xml_node emitter) {
        std::string type = bsdf.attribute("type").value();

        // Unwrap transparent wrappers
        if (type == "twosided" || type == "mask") {
            for (auto c : bsdf.children()) {
                std::string ct = c.name();
                pugi::xml_node inner = (ct == "ref" || ct == "bsdf") ? deref(c) : pugi::xml_node{};
                if (inner && strcmp(inner.name(), "bsdf") == 0) {
                    emit_material(w, inner, name, emitter);
                    return;
                }
            }
        }

        // Bump/normal map: emit inner BSDF with normal sampler threaded in
        if (type == "bumpmap" || type == "normalmap") {
            pugi::xml_node inner_bsdf;
            for (auto c : bsdf.children()) {
                pugi::xml_node d = deref(c);
                if (strcmp(d.name(), "bsdf") == 0) { inner_bsdf = d; break; }
            }
            if (inner_bsdf) {
                // TODO: inject normal map into the inner material
                emit_material(w, inner_bsdf, name, emitter);
                return;
            }
        }

        w.begin(name);

        auto emit_props_diffuse = [&](const char* reflParam = "reflectance") {
            w.begin("properties");
            w.begin("samplers");
            emit_sampler_or(w, bsdf, reflParam, "diffuse");
            if (emitter) emit_emissive_sampler(w, emitter);
            w.end(); w.end();
        };

        if (type == "diffuse" || type == "roughdiffuse") {
            w.kv("type", "lambert");
            emit_props_diffuse();
        }
        else if (type == "conductor" || type == "roughconductor") {
            w.kv("type", "principled");
            float alpha = get_float(bsdf, "alpha", type == "conductor" ? 0.01f : 0.1f);

            // Fresnel R0 per channel: ((n-1)^2 + k^2) / ((n+1)^2 + k^2)
            auto r0_ch = [](float n, float k) -> float {
                float d = (n+1.f)*(n+1.f) + k*k;
                return d > 1e-6f ? ((n-1.f)*(n-1.f) + k*k) / d : 0.f;
            };

            bool spec_is_tex = false;
            for (auto c : bsdf.children()) {
                if (strcmp(c.attribute("name").value(), "specular_reflectance") != 0) continue;
                pugi::xml_node node = (strcmp(c.name(), "ref") == 0) ? deref(c) : c;
                if (strcmp(node.name(), "texture") == 0) { spec_is_tex = true; break; }
            }

            Vec3 base_color;
            float ior_val = 1.5f;
            if (!spec_is_tex && has_param(bsdf, "eta") && has_param(bsdf, "k")) {
                Vec3 eta_v = get_color(bsdf, "eta", {1.5f, 1.5f, 1.5f});
                Vec3 k_v   = get_color(bsdf, "k",   {0.f,  0.f,  0.f});
                Vec3 spec  = get_color(bsdf, "specular_reflectance", {1.f, 1.f, 1.f});
                base_color = {
                    r0_ch(eta_v.x, k_v.x) * spec.x,
                    r0_ch(eta_v.y, k_v.y) * spec.y,
                    r0_ch(eta_v.z, k_v.z) * spec.z
                };
                // Luminance-weighted average of eta for the IOR slot
                ior_val = 0.2126f*eta_v.x + 0.7152f*eta_v.y + 0.0722f*eta_v.z;
            } else {
                Vec3 spec = get_color(bsdf, "specular_reflectance", {1.f, 1.f, 1.f});
                base_color = spec;
                ior_val = get_float(bsdf, "eta", 1.5f);
            }

            w.begin("properties");
            w.begin("samplers");
            if (spec_is_tex)
                emit_sampler(w, bsdf, "specular_reflectance", "base_color");
            else
                w.inline_block("base_color", {{"type","color"}, {"value", col3(base_color.x, base_color.y, base_color.z)}});
            if (emitter) emit_emissive_sampler(w, emitter);
            w.end();
            w.begin("scalars");
            w.kv("metallic",  "1.0");
            w.kv("roughness", ff(alpha));
            w.kv("ior",       ff(ior_val));
            w.end(); w.end();
        }
        else if (type == "dielectric" || type == "smoothdielectric") {
            w.kv("type", "dielectric");
            float ior = get_float(bsdf, "int_ior", get_float(bsdf, "ior", 1.5f));
            // transparency = luminance of specular_transmittance (Fresnel is internal)
            Vec3 spec_t = get_color(bsdf, "specular_transmittance", {1.f, 1.f, 1.f});
            float trans = 0.2126f*spec_t.x + 0.7152f*spec_t.y + 0.0722f*spec_t.z;
            w.begin("properties");
            if (emitter) { w.begin("samplers"); emit_emissive_sampler(w, emitter); w.end(); }
            w.begin("scalars");
            w.kv("ior",          ff(ior));
            w.kv("transparency", ff(trans));
            w.end(); w.end();
        }
        else if (type == "roughdielectric") {
            w.kv("type", "rough_dielectric");
            float ior   = get_float(bsdf, "int_ior", get_float(bsdf, "ior", 1.5f));
            float alpha = get_float(bsdf, "alpha", 0.1f);
            // transparency = luminance of specular_transmittance (Fresnel is internal)
            Vec3 spec_t = get_color(bsdf, "specular_transmittance", {1.f, 1.f, 1.f});
            float trans = 0.2126f*spec_t.x + 0.7152f*spec_t.y + 0.0722f*spec_t.z;
            w.begin("properties");
            w.begin("samplers");
            emit_sampler_or(w, bsdf, "specular_transmittance", "transmission", {1,1,1});
            if (emitter) emit_emissive_sampler(w, emitter);
            w.end();
            w.begin("scalars");
            w.kv("ior",          ff(ior));
            w.kv("roughness",    ff(alpha));
            w.kv("transparency", ff(trans));
            w.end(); w.end();
        }
        else if (type == "thindielectric") {
            w.kv("type", "thin_dielectric");
            float ior = get_float(bsdf, "int_ior", get_float(bsdf, "ior", 1.5f));
            Vec3 spec_t = get_color(bsdf, "specular_transmittance", {1.f, 1.f, 1.f});
            float trans = 0.2126f*spec_t.x + 0.7152f*spec_t.y + 0.0722f*spec_t.z;
            w.begin("properties");
            w.begin("samplers");
            emit_sampler_or(w, bsdf, "specular_transmittance", "transmission", {1,1,1});
            if (emitter) emit_emissive_sampler(w, emitter);
            w.end();
            w.begin("scalars");
            w.kv("ior",          ff(ior));
            w.kv("transparency", ff(trans));
            w.end(); w.end();
        }
        else if (type == "plastic" || type == "roughplastic") {
            w.kv("type", "principled");
            float alpha = get_float(bsdf, "alpha", type == "plastic" ? 0.01f : 0.1f);
            float ior   = get_float(bsdf, "int_ior", get_float(bsdf, "ior", 1.5f));
            w.begin("properties");
            w.begin("samplers");
            emit_sampler_or(w, bsdf, "diffuse_reflectance", "base_color");
            if (emitter) emit_emissive_sampler(w, emitter);
            w.end();
            w.begin("scalars");
            w.kv("metallic",  "0.0");
            w.kv("roughness", ff(alpha));
            w.kv("ior",       ff(ior));
            w.end(); w.end();
        }
        else if (type == "principled" || type == "principledbsdf") {
            w.kv("type", "principled");
            float alpha    = get_float(bsdf, "roughness", 0.5f);
            float metallic = get_float(bsdf, "metallic",  0.0f);
            float ior      = get_float(bsdf, "ior",       1.5f);
            float coat     = get_float(bsdf, "clearcoat", 0.0f);
            w.begin("properties");
            w.begin("samplers");
            emit_sampler_or(w, bsdf, "base_color", "base_color");
            if (emitter) emit_emissive_sampler(w, emitter);
            w.end();
            w.begin("scalars");
            w.kv("roughness", ff(alpha));
            w.kv("metallic",  ff(metallic));
            w.kv("ior",       ff(ior));
            if (coat > 0.0f) w.kv("clearcoat", ff(coat));
            w.end(); w.end();
        }
        else if (type == "null" || type == "interface") {
            w.kv("type", "boundary");
            // boundary has no properties
        }
        else if (type == "phong") {
            w.kv("type", "phong");
            w.begin("properties");
            w.begin("samplers");
            emit_sampler_or(w, bsdf, "diffuse_reflectance",  "diffuse");
            emit_sampler_or(w, bsdf, "specular_reflectance", "specular", {0.2f,0.2f,0.2f});
            if (emitter) emit_emissive_sampler(w, emitter);
            w.end();
            w.begin("scalars");
            w.kv("exponent", ff(get_float(bsdf, "exponent", 32.0f)));
            w.end(); w.end();
        }
        else {
            if (opts_.verbose)
                err_ << "Warning: unknown BSDF type '" << type << "', using lambert\n";
            w.kv("type", "lambert");
            emit_props_diffuse();
        }

        w.end(); // name block
    }

    // ── sensor → camera ───────────────────────────────────────────────────────

    void emit_sensor(NCFWriter& w, pugi::xml_node sensor) {
        std::string type = sensor.attribute("type").value();
        Transform t = parse_transform(sensor);

        std::string name = sensor.attribute("id").value();
        if (name.empty()) name = "default";
        name = sanitize(name);

        w.begin(name);

        if (type == "perspective" || type == "thinlens") {
            w.kv("type", "thin-lens");
            w.kv("fov", ff(get_float(sensor, "fov", 45.0f)));
            if (t.has_lookat) {
                w.kv("position", vec3(t.origin));
                w.kv("target",   vec3(t.target));
                w.kv("up",       vec3(t.up));
            } else {
                w.kv("position", "vec3(0, 0, 5)");
                w.kv("target",   "vec3(0, 0, 0)");
                w.kv("up",       "vec3(0, 1, 0)");
            }
            if (type == "thinlens") {
                float ap = get_float(sensor, "aperture_radius",
                           get_float(sensor, "aperture", 0.0f));
                float fd = get_float(sensor, "focus_distance", 1.0f);
                if (ap > 0.0f) {
                    w.kv("aperture", ff(ap));
                    w.kv("flength",  ff(fd));
                }
            }
        }
        else if (type == "spherical") {
            w.kv("type", "erp");
            w.kv("position",    t.has_lookat ? vec3(t.origin) : "vec3(0, 0, 0)");
            w.kv("orientation", t.has_rotate ? vec3(t.rotate)  : "vec3(0, 0, 0)");
        }
        else {
            if (opts_.verbose)
                err_ << "Warning: unsupported sensor type '" << type << "', using thin-lens\n";
            w.kv("type",     "thin-lens");
            w.kv("fov",      "45");
            w.kv("position", "vec3(0, 0, 5)");
            w.kv("target",   "vec3(0, 0, 0)");
            w.kv("up",       "vec3(0, 1, 0)");
        }

        w.end();
    }

    // ── shape → geometry + material ──────────────────────────────────────────

    struct ShapeOut { std::string geo_name, mat_name; };

    ShapeOut emit_shape(NCFWriter& geo_w, NCFWriter& mat_w, pugi::xml_node shape) {
        std::string type = shape.attribute("type").value();
        std::string id   = shape.attribute("id").value();
        std::string geo_name = id.empty() ? ("shape_" + std::to_string(shape_count_)) : sanitize(id);
        ++shape_count_;

        Transform t = parse_transform(shape);
        bool flip   = get_bool(shape, "flip_normals", false);

        // Find BSDF: inline <bsdf>, bare <ref id="X"/> (no name attr = BSDF slot), or deref'd ref
        pugi::xml_node bsdf;
        std::string bsdf_ref_id; // set when shape references a previously-defined named BSDF
        for (auto c : shape.children()) {
            const char* ctag = c.name();
            if (strcmp(ctag, "bsdf") == 0) { bsdf = c; break; }
            if (strcmp(ctag, "ref") == 0) {
                // A <ref> with no `name` attribute is the shape's BSDF slot
                std::string rname = c.attribute("name").value();
                if (rname.empty()) {
                    bsdf_ref_id = c.attribute("id").value();
                    pugi::xml_node d = deref(c);
                    if (strcmp(d.name(), "bsdf") == 0) bsdf = d;
                    break;
                }
            }
        }

        // Find inline area emitter
        pugi::xml_node emitter;
        for (auto c : shape.children())
            if (strcmp(c.name(), "emitter") == 0) { emitter = c; break; }

        // Resolve material name and emit material block
        std::string mat_name;
        if (!bsdf_ref_id.empty() && !emitter) {
            // References an already-emitted named BSDF — just alias it
            mat_name = sanitize(bsdf_ref_id);
        } else if (!bsdf && emitter) {
            // Pure area light — emissive material only
            mat_name = geo_name + "_mat";
            mat_w.begin(mat_name);
            mat_w.kv("type", "emissive");
            mat_w.begin("properties");
            mat_w.begin("samplers");
            emit_emissive_sampler(mat_w, emitter);
            mat_w.end(); mat_w.end(); mat_w.end();
        } else if (bsdf) {
            // Inline BSDF (possibly with area emitter)
            std::string bid = bsdf.attribute("id").value();
            // Use the ref id if we came via a ref, otherwise the bsdf's own id
            mat_name = !bsdf_ref_id.empty() ? sanitize(bsdf_ref_id)
                     : !bid.empty()         ? sanitize(bid)
                     :                        geo_name + "_mat";
            if (emitted_mats_.find(mat_name) == emitted_mats_.end()) {
                emit_material(mat_w, bsdf, mat_name, emitter);
                emitted_mats_.insert(mat_name);
            }
        } else {
            mat_name = geo_name + "_mat";
            mat_w.begin(mat_name);
            mat_w.kv("type", "lambert");
            mat_w.begin("properties"); mat_w.begin("samplers");
            mat_w.inline_block("diffuse", {{"type","color"}, {"value","col3(0.8, 0.8, 0.8)"}});
            mat_w.end(); mat_w.end(); mat_w.end();
        }

        // Emit geometry block
        geo_w.begin(geo_name);

        auto emit_mods = [&](Vec3 base_scale = {1,1,1}) {
            bool any = t.has_translate || t.has_rotate || t.has_scale || flip;
            if (!any) return;
            geo_w.begin("modifiers");
            if (t.has_rotate)    geo_w.kv("rotation",    vec3(t.rotate));
            if (t.has_scale) {
                Vec3 s = { t.scale.x * base_scale.x,
                           t.scale.y * base_scale.y,
                           t.scale.z * base_scale.z };
                geo_w.kv("scale", vec3(s));
            }
            if (t.has_translate) geo_w.kv("translation", vec3(t.translate));
            if (flip)            geo_w.kv("flip_normals", "true");
            geo_w.end();
        };

        if (type == "sphere") {
            Vec3 center = get_point(shape, "center");
            float radius = get_float(shape, "radius", 1.0f);
            // Fold world transform into sphere parameters directly
            if (t.has_translate) {
                center.x += t.translate.x;
                center.y += t.translate.y;
                center.z += t.translate.z;
            }
            if (t.has_scale)
                radius *= (t.scale.x + t.scale.y + t.scale.z) / 3.0f;

            geo_w.kv("type",     "sphere");
            geo_w.kv("position", vec3(center));
            geo_w.kv("radius",   ff(radius));
            if (flip) geo_w.kv("flip_normals", "true");
        }
        else if (type == "cylinder") {
            Vec3 p0 = get_point(shape, "p0", {0,-1,0});
            Vec3 p1 = get_point(shape, "p1", {0, 1,0});
            float radius = get_float(shape, "radius", 1.0f);
            // Build translation from midpoint, scale from half-length and radius
            Vec3 mid = { (p0.x+p1.x)*.5f, (p0.y+p1.y)*.5f, (p0.z+p1.z)*.5f };
            float hh = v3len({p1.x-p0.x, p1.y-p0.y, p1.z-p0.z}) * 0.5f;

            geo_w.kv("type",   "mesh");
            geo_w.kv("source", "gen(capped_cylinder)");
            geo_w.begin("modifiers");
            geo_w.kv("scale",       vec3({radius, hh, radius}));
            geo_w.kv("translation", vec3(mid));
            if (t.has_rotate)    geo_w.kv("rotation", vec3(t.rotate));
            if (flip)            geo_w.kv("flip_normals", "true");
            geo_w.end();
        }
        else if (type == "rectangle") {
            geo_w.kv("type",   "mesh");
            geo_w.kv("source", "gen(plane)");
            // xtracer gen(plane): XZ plane [-0.5,0.5], normal +Y
            // Mitsuba rectangle:  XY plane [-1,1],    normal +Z
            // Fix: swap rot cols 1↔2 so xtracer-Y→Mitsuba-Z(normal), xtracer-Z→Mitsuba-Y(depth)
            //      scale x2 in plane dims (1×1 → 2×2)
            if (t.has_rot3) {
                const float *r = t.rot3;
                // Build corrected rotation: [col0 | -col2 | col1] (negate normal col for det=+1)
                float rc[9] = {
                    r[0], -r[2],  r[1],   // row 0
                    r[3], -r[5],  r[4],   // row 1
                    r[6], -r[8],  r[7]    // row 2
                };
                Vec3 euler_rect = mat3_to_euler_zyx(rc);
                // Scale: map each gen(plane) tangent axis to its dominant world axis after rc.
                // col0 of rc = world dir of gen X, col2 = world dir of gen Z; col1(normal) = 1.
                Vec3 s = {1.f, 1.f, 1.f};
                {   float ax=std::abs(rc[0]), ay=std::abs(rc[3]), az=std::abs(rc[6]);
                    if (ax >= ay && ax >= az)      s.x = t.scale.x * 2.f;
                    else if (ay >= ax && ay >= az) s.y = t.scale.x * 2.f;
                    else                           s.z = t.scale.x * 2.f;
                }
                {   float ax=std::abs(rc[2]), ay=std::abs(rc[5]), az=std::abs(rc[8]);
                    if (ax >= ay && ax >= az)      s.x = t.scale.y * 2.f;
                    else if (ay >= ax && ay >= az) s.y = t.scale.y * 2.f;
                    else                           s.z = t.scale.y * 2.f;
                }
                bool any = t.has_translate || t.has_scale || t.has_rotate;
                if (any) {
                    geo_w.begin("modifiers");
                    if (euler_rect.x != 0 || euler_rect.y != 0 || euler_rect.z != 0)
                        geo_w.kv("rotation",    vec3(euler_rect));
                    if (t.has_scale) geo_w.kv("scale",       vec3(s));
                    if (t.has_translate) geo_w.kv("translation", vec3(t.translate));
                    geo_w.kv("flip_normals", "true");
                    geo_w.end();
                }
            } else {
                emit_mods({2, 1, 2});
            }
        }
        else if (type == "disk") {
            geo_w.kv("type",   "mesh");
            geo_w.kv("source", "gen(disc)");
            emit_mods();
        }
        else if (type == "cube") {
            geo_w.kv("type",   "mesh");
            geo_w.kv("source", "gen(hexahedron)");
            emit_mods();
        }
        else if (type == "obj" || type == "ply" || type == "serialized") {
            geo_w.kv("type", "mesh");
            std::string fn = resolve(get_string(shape, "filename"));
            if (fn.empty()) fn = "mesh." + (type == "ply" ? std::string("ply") : std::string("obj"));
            geo_w.kv("source", rewrite_geometry(fn));
            emit_mods();
        }
        else {
            if (opts_.verbose)
                err_ << "Warning: unsupported shape type '" << type << "', using sphere\n";
            geo_w.kv("type",     "sphere");
            geo_w.kv("position", "vec3(0, 0, 0)");
            geo_w.kv("radius",   "1");
        }

        geo_w.end();
        return { geo_name, mat_name };
    }

    // ── environment emitter ───────────────────────────────────────────────────

    void emit_environment(NCFWriter& w, pugi::xml_node emitter) {
        std::string type = emitter.attribute("type").value();
        w.begin("environment");

        if (type == "envmap") {
            std::string src = rewrite_texture(resolve(get_string(emitter, "filename")));
            w.kv("type", "erp");
            w.begin("config");
            w.kv("source", src);
            w.end();
        }
        else if (type == "constant") {
            Vec3 r = get_color(emitter, "radiance", {1,1,1});
            float s = get_float(emitter, "scale", 1.0f);
            r = {r.x*s, r.y*s, r.z*s};
            w.kv("type", "color");
            w.begin("config");
            w.kv("value", col3(r.x, r.y, r.z));
            w.end();
        }
        else if (type == "sunsky" || type == "sky") {
            w.kv("type", "hosek_wilkie_sky");
            w.begin("config");
            // Mitsuba sunsky uses 'sun_direction' and 'turbidity'
            Vec3 sun = get_point(emitter, "sun_direction", {0.5f, 0.28f, 0.82f});
            float turb = get_float(emitter, "turbidity", 3.0f);
            w.kv("sun_direction", vec3(sun));
            w.kv("turbidity", ff(turb));
            w.end();
        }
        else {
            if (opts_.verbose)
                err_ << "Warning: unsupported emitter type '" << type << "', using black\n";
            w.kv("type", "color");
            w.begin("config");
            w.kv("value", "col3(0, 0, 0)");
            w.end();
        }

        w.end();
    }

public:
    Converter(const ConvertOptions& o, std::ostream& e,
              std::vector<ResourceEntry>* resources)
        : opts_(o), err_(e), resources_(resources) {}

    bool convert(pugi::xml_document& doc, std::ostream& out) {
        auto scene = doc.child("scene");
        if (!scene) {
            err_ << "Error: no <scene> root element found\n";
            return false;
        }

        // Collect <default> entries
        for (auto c : scene.children("default"))
            defaults_[c.attribute("name").value()] = c.attribute("value").value();

        // Collect all elements with an id attribute
        std::function<void(pugi::xml_node)> collect_ids = [&](pugi::xml_node n) {
            if (auto id = n.attribute("id")) id_map_[id.value()] = n;
            for (auto c : n.children()) collect_ids(c);
        };
        collect_ids(scene);

        // Inner buffers — depth 1 so content is indented inside the section block
        std::ostringstream cam_buf, mat_buf, geo_buf, obj_buf;
        NCFWriter cam_w(cam_buf, 1), mat_w(mat_buf, 1), geo_w(geo_buf, 1), obj_w(obj_buf, 1);

        // --- sensors → cameras ---
        for (auto n : scene.children("sensor"))
            emit_sensor(cam_w, n);

        // --- top-level named BSDFs ---
        for (auto n : scene.children("bsdf")) {
            std::string id = n.attribute("id").value();
            std::string name = id.empty() ? ("mat_" + std::to_string(mat_count_++)) : sanitize(id);
            emit_material(mat_w, n, name, {});
            emitted_mats_.insert(name);
        }

        // --- shapes → geometry + objects ---
        std::vector<ShapeOut> objects;
        for (auto n : scene.children("shape"))
            objects.push_back(emit_shape(geo_w, mat_w, n));

        for (auto& [geo, mat] : objects) {
            obj_w.begin(geo);
            obj_w.kv("geometry", geo);
            obj_w.kv("material", mat);
            obj_w.end();
        }

        // --- environment emitter ---
        pugi::xml_node env_emitter;
        for (auto n : scene.children("emitter")) {
            std::string etype = n.attribute("type").value();
            if (etype == "envmap" || etype == "constant" || etype == "sunsky" || etype == "sky") {
                env_emitter = n;
                break;
            }
        }

        // --- write output ---
        NCFWriter w(out);
        w.comment("Generated by mitsuba2xtracer");
        w.blank();

        std::string title = path_stem(opts_.input_path);
        w.kv("title",   title);
        w.kv("version", "1.0");
        w.blank();

        if (env_emitter) {
            emit_environment(w, env_emitter);
            w.blank();
        }

        auto write_section = [&](const std::string& name, const std::ostringstream& buf) {
            std::string s = buf.str();
            if (s.empty()) return;
            out << name << " = {\n" << s << "}\n\n";
        };

        write_section("camera",   cam_buf);
        write_section("material", mat_buf);
        write_section("geometry", geo_buf);
        write_section("object",   obj_buf);

        return true;
    }
};

// ─── public API ───────────────────────────────────────────────────────────────

bool convert_mitsuba(const ConvertOptions& opts, std::ostream& out, std::ostream& err,
                     std::vector<ResourceEntry>* resources) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(opts.input_path.c_str());
    if (!result) {
        err << "Error: failed to parse '" << opts.input_path
            << "': " << result.description() << "\n";
        return false;
    }
    Converter c(opts, err, resources);
    return c.convert(doc, out);
}
