#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include "extras.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace nmesh {
namespace generator {

namespace {

typedef nmath::Vector2f Vec2;
typedef nmath::Vector3f Vec3;
typedef nmath::scalar_t Scalar;

struct svg_transform_t
{
    float a, b, c, d, e, f;
};

struct svg_contour_t
{
    std::vector<Vec2> points;
};

struct svg_shape_t
{
    std::vector<svg_contour_t> contours;
    bool evenodd;
};

struct svg_bounds_t
{
    Scalar min_x;
    Scalar min_y;
    Scalar max_x;
    Scalar max_y;
    bool valid;

    svg_bounds_t()
        : min_x(0.0f), min_y(0.0f), max_x(0.0f), max_y(0.0f), valid(false)
    {}

    void include(const Vec2 &p)
    {
        if (!valid) {
            min_x = max_x = p.x;
            min_y = max_y = p.y;
            valid = true;
            return;
        }
        min_x = std::min(min_x, p.x);
        min_y = std::min(min_y, p.y);
        max_x = std::max(max_x, p.x);
        max_y = std::max(max_y, p.y);
    }

    Scalar width() const { return max_x - min_x; }
    Scalar height() const { return max_y - min_y; }
};

static svg_transform_t svg_identity()
{
    svg_transform_t out = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
    return out;
}

static Vec2 svg_apply(const svg_transform_t &m, const Vec2 &p)
{
    return Vec2(
        m.a * p.x + m.c * p.y + m.e,
        m.b * p.x + m.d * p.y + m.f
    );
}

static svg_transform_t svg_multiply(const svg_transform_t &lhs, const svg_transform_t &rhs)
{
    svg_transform_t out;
    out.a = lhs.a * rhs.a + lhs.c * rhs.b;
    out.b = lhs.b * rhs.a + lhs.d * rhs.b;
    out.c = lhs.a * rhs.c + lhs.c * rhs.d;
    out.d = lhs.b * rhs.c + lhs.d * rhs.d;
    out.e = lhs.a * rhs.e + lhs.c * rhs.f + lhs.e;
    out.f = lhs.b * rhs.e + lhs.d * rhs.f + lhs.f;
    return out;
}

static std::string trim_copy(const std::string &s)
{
    size_t begin = 0;
    while (begin < s.size() && std::isspace((unsigned char)s[begin])) ++begin;
    size_t end = s.size();
    while (end > begin && std::isspace((unsigned char)s[end - 1])) --end;
    return s.substr(begin, end - begin);
}

static std::string lower_copy(const std::string &s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
    return out;
}

static bool read_text_file(const char *path, std::string &out)
{
    if (!path || !*path) return false;
    std::ifstream in(path, std::ios::binary);
    if (!in.good()) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return in.good() || in.eof();
}

static bool parse_scalar(const std::string &raw, float &out)
{
    const std::string s = trim_copy(raw);
    if (s.empty()) return false;
    char *end = 0;
    const double value = std::strtod(s.c_str(), &end);
    if (end == s.c_str()) return false;
    out = (float)value;
    return true;
}

static bool parse_scalar_attr(const pugi::xml_node &node, const char *name, float &out)
{
    const pugi::xml_attribute attr = node.attribute(name);
    if (!attr) return false;
    return parse_scalar(attr.value(), out);
}

static std::string style_lookup(const pugi::xml_node &node, const char *name)
{
    const pugi::xml_attribute direct = node.attribute(name);
    if (direct) return trim_copy(direct.value());

    const pugi::xml_attribute style = node.attribute("style");
    if (!style) return std::string();

    std::stringstream ss(style.value());
    std::string item;
    const std::string want = lower_copy(name);
    while (std::getline(ss, item, ';')) {
        const size_t pos = item.find(':');
        if (pos == std::string::npos) continue;
        const std::string key = lower_copy(trim_copy(item.substr(0, pos)));
        if (key != want) continue;
        return trim_copy(item.substr(pos + 1));
    }
    return std::string();
}

static bool attr_is_none(const pugi::xml_node &node, const char *name)
{
    const std::string value = lower_copy(style_lookup(node, name));
    return value == "none";
}

static float attr_float_or(const pugi::xml_node &node, const char *name, float def)
{
    const std::string value = style_lookup(node, name);
    float out = def;
    if (!value.empty()) parse_scalar(value, out);
    return out;
}

static bool node_hidden(const pugi::xml_node &node, bool inherited_hidden)
{
    if (inherited_hidden) return true;
    const std::string display = lower_copy(style_lookup(node, "display"));
    if (display == "none") return true;
    const std::string visibility = lower_copy(style_lookup(node, "visibility"));
    if (visibility == "hidden") return true;
    if (attr_float_or(node, "opacity", 1.0f) <= 0.0f) return true;
    return false;
}

static bool node_has_fill(const pugi::xml_node &node)
{
    if (attr_is_none(node, "fill")) return false;
    if (attr_float_or(node, "fill-opacity", 1.0f) <= 0.0f) return false;
    if (attr_float_or(node, "opacity", 1.0f) <= 0.0f) return false;
    return true;
}

static bool approx_equal(const Vec2 &a, const Vec2 &b, float eps = 1e-5f)
{
    return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps;
}

static void cleanup_contour(std::vector<Vec2> &pts)
{
    if (pts.empty()) return;
    std::vector<Vec2> out;
    out.reserve(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) {
        if (!out.empty() && approx_equal(out.back(), pts[i])) continue;
        out.push_back(pts[i]);
    }
    if (out.size() >= 2 && approx_equal(out.front(), out.back())) out.pop_back();
    pts.swap(out);
}

static void append_closed_contour(std::vector<svg_contour_t> &out, std::vector<Vec2> pts)
{
    cleanup_contour(pts);
    if (pts.size() < 3) return;
    svg_contour_t contour;
    contour.points.swap(pts);
    out.push_back(contour);
}

static std::vector<float> parse_number_list(const std::string &text)
{
    std::vector<float> out;
    const char *s = text.c_str();
    while (*s) {
        while (*s && (std::isspace((unsigned char)*s) || *s == ',')) ++s;
        if (!*s) break;
        char *end = 0;
        const double value = std::strtod(s, &end);
        if (end == s) {
            ++s;
            continue;
        }
        out.push_back((float)value);
        s = end;
    }
    return out;
}

static svg_transform_t parse_transform_attr(const std::string &text)
{
    svg_transform_t total = svg_identity();
    size_t pos = 0;
    while (pos < text.size()) {
        while (pos < text.size() && std::isspace((unsigned char)text[pos])) ++pos;
        if (pos >= text.size()) break;
        const size_t begin = pos;
        while (pos < text.size() && std::isalpha((unsigned char)text[pos])) ++pos;
        if (begin == pos) break;
        const std::string name = lower_copy(text.substr(begin, pos - begin));
        while (pos < text.size() && std::isspace((unsigned char)text[pos])) ++pos;
        if (pos >= text.size() || text[pos] != '(') break;
        ++pos;
        int depth = 1;
        const size_t args_begin = pos;
        while (pos < text.size() && depth > 0) {
            if (text[pos] == '(') ++depth;
            else if (text[pos] == ')') --depth;
            ++pos;
        }
        if (depth != 0) break;
        const std::string args = text.substr(args_begin, pos - args_begin - 1);
        const std::vector<float> values = parse_number_list(args);
        svg_transform_t op = svg_identity();

        if (name == "matrix" && values.size() >= 6) {
            op.a = values[0];
            op.b = values[1];
            op.c = values[2];
            op.d = values[3];
            op.e = values[4];
            op.f = values[5];
        } else if (name == "translate" && !values.empty()) {
            op.e = values[0];
            op.f = values.size() >= 2 ? values[1] : 0.0f;
        } else if (name == "scale" && !values.empty()) {
            op.a = values[0];
            op.d = values.size() >= 2 ? values[1] : values[0];
        } else if (name == "rotate" && !values.empty()) {
            const float ang = values[0] * (float)(nmath::PI_DOUBLE / 180.0);
            const float c = std::cos(ang);
            const float s = std::sin(ang);
            svg_transform_t rot = {c, s, -s, c, 0.0f, 0.0f};
            if (values.size() >= 3) {
                svg_transform_t t0 = svg_identity();
                svg_transform_t t1 = svg_identity();
                t0.e = values[1];
                t0.f = values[2];
                t1.e = -values[1];
                t1.f = -values[2];
                op = svg_multiply(t0, svg_multiply(rot, t1));
            } else {
                op = rot;
            }
        } else if (name == "skewx" && !values.empty()) {
            op.c = std::tan(values[0] * (float)(nmath::PI_DOUBLE / 180.0));
        } else if (name == "skewy" && !values.empty()) {
            op.b = std::tan(values[0] * (float)(nmath::PI_DOUBLE / 180.0));
        }

        total = svg_multiply(total, op);
    }
    return total;
}

static Vec2 cubic_bezier(const Vec2 &p0, const Vec2 &p1, const Vec2 &p2, const Vec2 &p3, float t)
{
    const float it = 1.0f - t;
    return it * it * it * p0
         + 3.0f * it * it * t * p1
         + 3.0f * it * t * t * p2
         + t * t * t * p3;
}

static Vec2 quadratic_bezier(const Vec2 &p0, const Vec2 &p1, const Vec2 &p2, float t)
{
    const float it = 1.0f - t;
    return it * it * p0 + 2.0f * it * t * p1 + t * t * p2;
}

static int segment_count_from_length(float estimate, int min_segments, int max_segments)
{
    if (estimate <= 0.0f) return min_segments;
    int segments = (int)std::ceil(estimate * 0.18f);
    if (segments < min_segments) segments = min_segments;
    if (segments > max_segments) segments = max_segments;
    return segments;
}

static void append_cubic_curve(std::vector<Vec2> &pts, const Vec2 &p0, const Vec2 &p1, const Vec2 &p2, const Vec2 &p3)
{
    const float estimate = (p1 - p0).length() + (p2 - p1).length() + (p3 - p2).length();
    const int segments = segment_count_from_length(estimate, 8, 64);
    for (int i = 1; i <= segments; ++i) {
        const float t = (float)i / (float)segments;
        pts.push_back(cubic_bezier(p0, p1, p2, p3, t));
    }
}

static void append_quadratic_curve(std::vector<Vec2> &pts, const Vec2 &p0, const Vec2 &p1, const Vec2 &p2)
{
    const float estimate = (p1 - p0).length() + (p2 - p1).length();
    const int segments = segment_count_from_length(estimate, 6, 48);
    for (int i = 1; i <= segments; ++i) {
        const float t = (float)i / (float)segments;
        pts.push_back(quadratic_bezier(p0, p1, p2, t));
    }
}

static float vec_angle(const Vec2 &u, const Vec2 &v)
{
    const float det = u.x * v.y - u.y * v.x;
    const float dot = u.x * v.x + u.y * v.y;
    return std::atan2(det, dot);
}

static void append_arc_curve(std::vector<Vec2> &pts,
                             const Vec2 &from,
                             float rx,
                             float ry,
                             float x_axis_rotation_deg,
                             bool large_arc_flag,
                             bool sweep_flag,
                             const Vec2 &to)
{
    if (rx <= 0.0f || ry <= 0.0f || approx_equal(from, to)) {
        pts.push_back(to);
        return;
    }

    const float phi = x_axis_rotation_deg * (float)(nmath::PI_DOUBLE / 180.0);
    const float cos_phi = std::cos(phi);
    const float sin_phi = std::sin(phi);

    const float dx2 = (from.x - to.x) * 0.5f;
    const float dy2 = (from.y - to.y) * 0.5f;
    const float x1p = cos_phi * dx2 + sin_phi * dy2;
    const float y1p = -sin_phi * dx2 + cos_phi * dy2;

    rx = std::fabs(rx);
    ry = std::fabs(ry);

    const float rx_sq = rx * rx;
    const float ry_sq = ry * ry;
    const float x1p_sq = x1p * x1p;
    const float y1p_sq = y1p * y1p;

    const float lambda = x1p_sq / rx_sq + y1p_sq / ry_sq;
    if (lambda > 1.0f) {
        const float scale = std::sqrt(lambda);
        rx *= scale;
        ry *= scale;
    }

    const float rx2 = rx * rx;
    const float ry2 = ry * ry;
    const float num = rx2 * ry2 - rx2 * y1p_sq - ry2 * x1p_sq;
    const float den = rx2 * y1p_sq + ry2 * x1p_sq;
    const float factor = (den <= 0.0f) ? 0.0f : std::sqrt(std::max(0.0f, num / den));
    const float sign = (large_arc_flag == sweep_flag) ? -1.0f : 1.0f;
    const float cxp = sign * factor * ((rx * y1p) / ry);
    const float cyp = sign * factor * (-(ry * x1p) / rx);

    const float cx = cos_phi * cxp - sin_phi * cyp + (from.x + to.x) * 0.5f;
    const float cy = sin_phi * cxp + cos_phi * cyp + (from.y + to.y) * 0.5f;

    const Vec2 v0((x1p - cxp) / rx, (y1p - cyp) / ry);
    const Vec2 v1((-x1p - cxp) / rx, (-y1p - cyp) / ry);

    float theta0 = vec_angle(Vec2(1.0f, 0.0f), v0);
    float delta = vec_angle(v0, v1);
    if (!sweep_flag && delta > 0.0f) delta -= (float)(nmath::PI_DOUBLE * 2.0);
    if (sweep_flag && delta < 0.0f) delta += (float)(nmath::PI_DOUBLE * 2.0);

    const float estimate = std::max(rx, ry) * std::fabs(delta);
    const int segments = segment_count_from_length(estimate, 8, 96);

    for (int i = 1; i <= segments; ++i) {
        const float t = (float)i / (float)segments;
        const float theta = theta0 + delta * t;
        const float ct = std::cos(theta);
        const float st = std::sin(theta);
        const float x = cos_phi * rx * ct - sin_phi * ry * st + cx;
        const float y = sin_phi * rx * ct + cos_phi * ry * st + cy;
        pts.push_back(Vec2(x, y));
    }
}

class path_reader_t
{
public:
    explicit path_reader_t(const char *s) : s_(s ? s : "") {}

    void skip_delims()
    {
        while (*s_ && (std::isspace((unsigned char)*s_) || *s_ == ',')) ++s_;
    }

    bool eof()
    {
        skip_delims();
        return *s_ == 0;
    }

    bool next_is_command() const
    {
        return std::isalpha((unsigned char)*s_) != 0;
    }

    char read_command()
    {
        skip_delims();
        char c = *s_;
        if (c) ++s_;
        return c;
    }

    bool has_number()
    {
        skip_delims();
        if (*s_ == '+' || *s_ == '-' || *s_ == '.' || std::isdigit((unsigned char)*s_)) return true;
        return false;
    }

    bool read_number(float &out)
    {
        skip_delims();
        if (!*s_) return false;
        char *end = 0;
        const double value = std::strtod(s_, &end);
        if (end == s_) return false;
        out = (float)value;
        s_ = end;
        return true;
    }

private:
    const char *s_;
};

static bool parse_svg_path(const std::string &d, std::vector<svg_contour_t> &out)
{
    path_reader_t reader(d.c_str());
    std::vector<Vec2> current;
    Vec2 cursor(0.0f, 0.0f);
    Vec2 start(0.0f, 0.0f);
    Vec2 last_cubic_ctrl(0.0f, 0.0f);
    Vec2 last_quad_ctrl(0.0f, 0.0f);
    char cmd = 0;
    char prev_cmd = 0;

    while (!reader.eof()) {
        if (reader.next_is_command()) cmd = reader.read_command();
        if (!cmd) return false;

        const bool relative = std::islower((unsigned char)cmd) != 0;
        const char upper = (char)std::toupper((unsigned char)cmd);

        if (upper == 'M') {
            float x = 0.0f, y = 0.0f;
            if (!reader.read_number(x) || !reader.read_number(y)) return false;
            if (!current.empty()) append_closed_contour(out, current);
            current.clear();
            cursor = relative ? cursor + Vec2(x, y) : Vec2(x, y);
            start = cursor;
            current.push_back(cursor);
            prev_cmd = cmd;
            cmd = relative ? 'l' : 'L';
            continue;
        }

        if (upper == 'Z') {
            append_closed_contour(out, current);
            current.clear();
            cursor = start;
            last_cubic_ctrl = cursor;
            last_quad_ctrl = cursor;
            prev_cmd = cmd;
            continue;
        }

        if (upper == 'L') {
            float x = 0.0f, y = 0.0f;
            while (reader.read_number(x) && reader.read_number(y)) {
                cursor = relative ? cursor + Vec2(x, y) : Vec2(x, y);
                current.push_back(cursor);
            }
        } else if (upper == 'H') {
            float x = 0.0f;
            while (reader.read_number(x)) {
                cursor.x = relative ? cursor.x + x : x;
                current.push_back(cursor);
            }
        } else if (upper == 'V') {
            float y = 0.0f;
            while (reader.read_number(y)) {
                cursor.y = relative ? cursor.y + y : y;
                current.push_back(cursor);
            }
        } else if (upper == 'C') {
            float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, x = 0.0f, y = 0.0f;
            while (reader.read_number(x1) && reader.read_number(y1)
                && reader.read_number(x2) && reader.read_number(y2)
                && reader.read_number(x) && reader.read_number(y)) {
                Vec2 p1 = relative ? cursor + Vec2(x1, y1) : Vec2(x1, y1);
                Vec2 p2 = relative ? cursor + Vec2(x2, y2) : Vec2(x2, y2);
                Vec2 p3 = relative ? cursor + Vec2(x, y) : Vec2(x, y);
                append_cubic_curve(current, cursor, p1, p2, p3);
                cursor = p3;
                last_cubic_ctrl = p2;
            }
        } else if (upper == 'S') {
            float x2 = 0.0f, y2 = 0.0f, x = 0.0f, y = 0.0f;
            while (reader.read_number(x2) && reader.read_number(y2)
                && reader.read_number(x) && reader.read_number(y)) {
                const bool smooth_prev = prev_cmd == 'C' || prev_cmd == 'c' || prev_cmd == 'S' || prev_cmd == 's';
                Vec2 p1 = smooth_prev ? (cursor * 2.0f - last_cubic_ctrl) : cursor;
                Vec2 p2 = relative ? cursor + Vec2(x2, y2) : Vec2(x2, y2);
                Vec2 p3 = relative ? cursor + Vec2(x, y) : Vec2(x, y);
                append_cubic_curve(current, cursor, p1, p2, p3);
                cursor = p3;
                last_cubic_ctrl = p2;
            }
        } else if (upper == 'Q') {
            float x1 = 0.0f, y1 = 0.0f, x = 0.0f, y = 0.0f;
            while (reader.read_number(x1) && reader.read_number(y1)
                && reader.read_number(x) && reader.read_number(y)) {
                Vec2 p1 = relative ? cursor + Vec2(x1, y1) : Vec2(x1, y1);
                Vec2 p2 = relative ? cursor + Vec2(x, y) : Vec2(x, y);
                append_quadratic_curve(current, cursor, p1, p2);
                cursor = p2;
                last_quad_ctrl = p1;
            }
        } else if (upper == 'T') {
            float x = 0.0f, y = 0.0f;
            while (reader.read_number(x) && reader.read_number(y)) {
                const bool smooth_prev = prev_cmd == 'Q' || prev_cmd == 'q' || prev_cmd == 'T' || prev_cmd == 't';
                Vec2 p1 = smooth_prev ? (cursor * 2.0f - last_quad_ctrl) : cursor;
                Vec2 p2 = relative ? cursor + Vec2(x, y) : Vec2(x, y);
                append_quadratic_curve(current, cursor, p1, p2);
                cursor = p2;
                last_quad_ctrl = p1;
            }
        } else if (upper == 'A') {
            float rx = 0.0f, ry = 0.0f, rot = 0.0f;
            float laf = 0.0f, sf = 0.0f, x = 0.0f, y = 0.0f;
            while (reader.read_number(rx) && reader.read_number(ry)
                && reader.read_number(rot) && reader.read_number(laf)
                && reader.read_number(sf) && reader.read_number(x)
                && reader.read_number(y)) {
                Vec2 p = relative ? cursor + Vec2(x, y) : Vec2(x, y);
                append_arc_curve(current, cursor, rx, ry, rot, laf >= 0.5f, sf >= 0.5f, p);
                cursor = p;
            }
        } else {
            return false;
        }

        prev_cmd = cmd;
    }

    if (!current.empty()) append_closed_contour(out, current);
    return !out.empty();
}

static svg_contour_t make_rect_contour(float x, float y, float w, float h)
{
    svg_contour_t contour;
    contour.points.push_back(Vec2(x, y));
    contour.points.push_back(Vec2(x + w, y));
    contour.points.push_back(Vec2(x + w, y + h));
    contour.points.push_back(Vec2(x, y + h));
    return contour;
}

static svg_contour_t make_ellipse_contour(float cx, float cy, float rx, float ry, size_t segments)
{
    svg_contour_t contour;
    segments = std::max((size_t)16, segments);
    contour.points.reserve(segments);
    for (size_t i = 0; i < segments; ++i) {
        const float t = (float)i / (float)segments;
        const float a = (float)(nmath::PI_DOUBLE * 2.0) * t;
        contour.points.push_back(Vec2(cx + std::cos(a) * rx, cy + std::sin(a) * ry));
    }
    return contour;
}

static void transform_contours(std::vector<svg_contour_t> &contours,
                               const svg_transform_t &xf,
                               svg_bounds_t &bounds)
{
    for (size_t i = 0; i < contours.size(); ++i) {
        for (size_t j = 0; j < contours[i].points.size(); ++j) {
            contours[i].points[j] = svg_apply(xf, contours[i].points[j]);
            bounds.include(contours[i].points[j]);
        }
    }
}

static void collect_shape(svg_shape_t &shape,
                          std::vector<svg_shape_t> &shapes,
                          const svg_transform_t &xf,
                          svg_bounds_t &bounds)
{
    if (shape.contours.empty()) return;
    transform_contours(shape.contours, xf, bounds);
    std::vector<svg_contour_t> clean;
    for (size_t i = 0; i < shape.contours.size(); ++i) {
        std::vector<Vec2> pts = shape.contours[i].points;
        cleanup_contour(pts);
        if (pts.size() < 3) continue;
        svg_contour_t contour;
        contour.points.swap(pts);
        clean.push_back(contour);
    }
    if (clean.empty()) return;
    shape.contours.swap(clean);
    shapes.push_back(shape);
}

static void traverse_svg_nodes(const pugi::xml_node &node,
                               const svg_transform_t &inherited_xf,
                               bool inherited_hidden,
                               std::vector<svg_shape_t> &shapes,
                               svg_bounds_t &bounds)
{
    if (node.type() != pugi::node_element) return;

    const bool hidden = node_hidden(node, inherited_hidden);

    svg_transform_t xf = inherited_xf;
    const pugi::xml_attribute transform_attr = node.attribute("transform");
    if (transform_attr) {
        const svg_transform_t local = parse_transform_attr(transform_attr.value());
        xf = svg_multiply(inherited_xf, local);
    }

    const std::string name = lower_copy(node.name());
    if (!hidden && node_has_fill(node)) {
        svg_shape_t shape;
        shape.evenodd = lower_copy(style_lookup(node, "fill-rule")) == "evenodd";

        if (name == "path") {
            const pugi::xml_attribute d = node.attribute("d");
            if (d) parse_svg_path(d.value(), shape.contours);
        } else if (name == "rect") {
            float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
            parse_scalar_attr(node, "x", x);
            parse_scalar_attr(node, "y", y);
            parse_scalar_attr(node, "width", w);
            parse_scalar_attr(node, "height", h);
            if (w > 0.0f && h > 0.0f) shape.contours.push_back(make_rect_contour(x, y, w, h));
        } else if (name == "circle") {
            float cx = 0.0f, cy = 0.0f, r = 0.0f;
            parse_scalar_attr(node, "cx", cx);
            parse_scalar_attr(node, "cy", cy);
            parse_scalar_attr(node, "r", r);
            if (r > 0.0f) shape.contours.push_back(make_ellipse_contour(cx, cy, r, r, 64));
        } else if (name == "ellipse") {
            float cx = 0.0f, cy = 0.0f, rx = 0.0f, ry = 0.0f;
            parse_scalar_attr(node, "cx", cx);
            parse_scalar_attr(node, "cy", cy);
            parse_scalar_attr(node, "rx", rx);
            parse_scalar_attr(node, "ry", ry);
            if (rx > 0.0f && ry > 0.0f) shape.contours.push_back(make_ellipse_contour(cx, cy, rx, ry, 64));
        } else if (name == "polygon" || name == "polyline") {
            const pugi::xml_attribute points_attr = node.attribute("points");
            if (points_attr) {
                const std::vector<float> nums = parse_number_list(points_attr.value());
                std::vector<Vec2> pts;
                for (size_t i = 0; i + 1 < nums.size(); i += 2) {
                    pts.push_back(Vec2(nums[i], nums[i + 1]));
                }
                append_closed_contour(shape.contours, pts);
            }
        }

        collect_shape(shape, shapes, xf, bounds);
    }

    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
        traverse_svg_nodes(child, xf, hidden, shapes, bounds);
    }
}

static bool parse_svg_document(const char *file,
                               std::vector<svg_shape_t> &shapes,
                               svg_bounds_t &viewport,
                               svg_bounds_t &content_bounds)
{
    std::string xml;
    if (!read_text_file(file, xml)) return false;

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(xml.c_str());
    if (!result) return false;

    pugi::xml_node root = doc.child("svg");
    if (!root) {
        root = doc.document_element();
        if (!root || lower_copy(root.name()) != "svg") return false;
    }

    const pugi::xml_attribute view_box_attr = root.attribute("viewBox");
    if (view_box_attr) {
        const std::vector<float> nums = parse_number_list(view_box_attr.value());
        if (nums.size() >= 4) {
            viewport.min_x = nums[0];
            viewport.min_y = nums[1];
            viewport.max_x = nums[0] + nums[2];
            viewport.max_y = nums[1] + nums[3];
            viewport.valid = nums[2] > 0.0f && nums[3] > 0.0f;
        }
    }

    if (!viewport.valid) {
        float w = 0.0f, h = 0.0f;
        if (parse_scalar_attr(root, "width", w) && parse_scalar_attr(root, "height", h) && w > 0.0f && h > 0.0f) {
            viewport.min_x = 0.0f;
            viewport.min_y = 0.0f;
            viewport.max_x = w;
            viewport.max_y = h;
            viewport.valid = true;
        }
    }

    traverse_svg_nodes(root, svg_identity(), false, shapes, content_bounds);

    if (!viewport.valid) viewport = content_bounds;
    return viewport.valid && !shapes.empty();
}

static float contour_is_left(const Vec2 &a, const Vec2 &b, const Vec2 &p)
{
    return (b.x - a.x) * (p.y - a.y) - (p.x - a.x) * (b.y - a.y);
}

static int contour_winding_number(const svg_contour_t &contour, const Vec2 &p)
{
    const std::vector<Vec2> &pts = contour.points;
    if (pts.size() < 3) return 0;
    int winding = 0;
    for (size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++) {
        const Vec2 &a = pts[j];
        const Vec2 &b = pts[i];
        if (a.y <= p.y) {
            if (b.y > p.y && contour_is_left(a, b, p) > 0.0f) ++winding;
        } else {
            if (b.y <= p.y && contour_is_left(a, b, p) < 0.0f) --winding;
        }
    }
    return winding;
}

static bool contour_evenodd_contains(const svg_contour_t &contour, const Vec2 &p)
{
    const std::vector<Vec2> &pts = contour.points;
    if (pts.size() < 3) return false;
    bool inside = false;
    for (size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++) {
        const Vec2 &a = pts[j];
        const Vec2 &b = pts[i];
        const bool intersects = ((a.y > p.y) != (b.y > p.y))
            && (p.x < (b.x - a.x) * (p.y - a.y) / ((b.y - a.y) == 0.0f ? 1e-8f : (b.y - a.y)) + a.x);
        if (intersects) inside = !inside;
    }
    return inside;
}

static bool shape_contains(const svg_shape_t &shape, const Vec2 &p)
{
    if (shape.evenodd) {
        bool inside = false;
        for (size_t i = 0; i < shape.contours.size(); ++i) {
            if (contour_evenodd_contains(shape.contours[i], p)) inside = !inside;
        }
        return inside;
    }

    int winding = 0;
    for (size_t i = 0; i < shape.contours.size(); ++i) {
        winding += contour_winding_number(shape.contours[i], p);
    }
    return winding != 0;
}

static int append_vertex(object_t *obj, const Vec3 &p, const Vec3 &n, float u, float v)
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

static void append_triangle(shape_t &shape, int a, int b, int c)
{
    index_t ia;
    ia.v = a; ia.n = a; ia.uv = a;
    index_t ib;
    ib.v = b; ib.n = b; ib.uv = b;
    index_t ic;
    ic.v = c; ic.n = c; ic.uv = c;
    shape.mesh.indices.push_back(ia);
    shape.mesh.indices.push_back(ib);
    shape.mesh.indices.push_back(ic);
}

static void append_quad(object_t *obj,
                        shape_t &shape,
                        const Vec3 &p0,
                        const Vec3 &p1,
                        const Vec3 &p2,
                        const Vec3 &p3,
                        const Vec3 &n,
                        float u0,
                        float v0,
                        float u1,
                        float v1)
{
    const int i0 = append_vertex(obj, p0, n, u0, v1);
    const int i1 = append_vertex(obj, p1, n, u1, v1);
    const int i2 = append_vertex(obj, p2, n, u1, v0);
    const int i3 = append_vertex(obj, p3, n, u0, v0);
    append_triangle(shape, i0, i1, i2);
    append_triangle(shape, i0, i2, i3);
}

} // namespace

bool svg(object_t *obj, const char *file, size_t resolution, float height)
{
    if (!obj || !file || !*file) return false;

    std::vector<svg_shape_t> shapes;
    svg_bounds_t viewport;
    svg_bounds_t content_bounds;
    if (!parse_svg_document(file, shapes, viewport, content_bounds)) return false;

    const Scalar view_w = std::max((Scalar)1e-5, viewport.width());
    const Scalar view_h = std::max((Scalar)1e-5, viewport.height());
    const Scalar view_scale = std::max(view_w, view_h);

    resolution = std::max((size_t)8, std::min((size_t)256, resolution));
    const size_t cells_x = std::max((size_t)1, (size_t)std::lround((double)resolution * (double)view_w / (double)view_scale));
    const size_t cells_y = std::max((size_t)1, (size_t)std::lround((double)resolution * (double)view_h / (double)view_scale));
    const Scalar cell_w = view_w / (Scalar)cells_x;
    const Scalar cell_h = view_h / (Scalar)cells_y;
    const Scalar half_depth = std::max((Scalar)0.0005, (Scalar)std::fabs(height) * (Scalar)0.5);

    std::vector<unsigned char> filled(cells_x * cells_y, 0);
    size_t filled_count = 0;
    for (size_t y = 0; y < cells_y; ++y) {
        for (size_t x = 0; x < cells_x; ++x) {
            const Vec2 sample(
                viewport.min_x + ((float)x + 0.5f) * cell_w,
                viewport.min_y + ((float)y + 0.5f) * cell_h
            );
            bool inside = false;
            for (size_t i = 0; i < shapes.size(); ++i) {
                if (shape_contains(shapes[i], sample)) {
                    inside = true;
                    break;
                }
            }
            if (inside) {
                filled[y * cells_x + x] = 1;
                ++filled_count;
            }
        }
    }

    if (filled_count == 0) return false;

    shape_t shape;
    shape.name = "svg";
    obj->shapes.push_back(shape);
    shape_t &mesh = obj->shapes.back();

    const Scalar cx = (viewport.min_x + viewport.max_x) * (Scalar)0.5;
    const Scalar cy = (viewport.min_y + viewport.max_y) * (Scalar)0.5;
    const Scalar scale = view_scale;

    auto is_filled = [&](int x, int y) -> bool {
        if (x < 0 || y < 0) return false;
        if ((size_t)x >= cells_x || (size_t)y >= cells_y) return false;
        return filled[(size_t)y * cells_x + (size_t)x] != 0;
    };

    for (size_t y = 0; y < cells_y; ++y) {
        for (size_t x = 0; x < cells_x; ++x) {
            if (!filled[y * cells_x + x]) continue;

            const Scalar sx0 = viewport.min_x + (Scalar)x * cell_w;
            const Scalar sx1 = sx0 + cell_w;
            const Scalar sy0 = viewport.min_y + (Scalar)y * cell_h;
            const Scalar sy1 = sy0 + cell_h;

            const Scalar wx0 = (sx0 - cx) / scale;
            const Scalar wx1 = (sx1 - cx) / scale;
            const Scalar wy0 = (cy - sy0) / scale;
            const Scalar wy1 = (cy - sy1) / scale;

            const float u0 = (float)((sx0 - viewport.min_x) / view_w);
            const float u1 = (float)((sx1 - viewport.min_x) / view_w);
            const float v0 = (float)((sy0 - viewport.min_y) / view_h);
            const float v1 = (float)((sy1 - viewport.min_y) / view_h);

            append_quad(
                obj,
                mesh,
                Vec3(wx0, wy1, half_depth),
                Vec3(wx1, wy1, half_depth),
                Vec3(wx1, wy0, half_depth),
                Vec3(wx0, wy0, half_depth),
                Vec3(0.0f, 0.0f, 1.0f),
                u0, v0, u1, v1
            );

            append_quad(
                obj,
                mesh,
                Vec3(wx1, wy1, -half_depth),
                Vec3(wx0, wy1, -half_depth),
                Vec3(wx0, wy0, -half_depth),
                Vec3(wx1, wy0, -half_depth),
                Vec3(0.0f, 0.0f, -1.0f),
                u0, v0, u1, v1
            );

            if (!is_filled((int)x - 1, (int)y)) {
                append_quad(
                    obj,
                    mesh,
                    Vec3(wx0, wy1, -half_depth),
                    Vec3(wx0, wy1, half_depth),
                    Vec3(wx0, wy0, half_depth),
                    Vec3(wx0, wy0, -half_depth),
                    Vec3(-1.0f, 0.0f, 0.0f),
                    0.0f, 0.0f, 1.0f, 1.0f
                );
            }
            if (!is_filled((int)x + 1, (int)y)) {
                append_quad(
                    obj,
                    mesh,
                    Vec3(wx1, wy1, half_depth),
                    Vec3(wx1, wy1, -half_depth),
                    Vec3(wx1, wy0, -half_depth),
                    Vec3(wx1, wy0, half_depth),
                    Vec3(1.0f, 0.0f, 0.0f),
                    0.0f, 0.0f, 1.0f, 1.0f
                );
            }
            if (!is_filled((int)x, (int)y - 1)) {
                append_quad(
                    obj,
                    mesh,
                    Vec3(wx0, wy0, half_depth),
                    Vec3(wx1, wy0, half_depth),
                    Vec3(wx1, wy0, -half_depth),
                    Vec3(wx0, wy0, -half_depth),
                    Vec3(0.0f, 1.0f, 0.0f),
                    0.0f, 0.0f, 1.0f, 1.0f
                );
            }
            if (!is_filled((int)x, (int)y + 1)) {
                append_quad(
                    obj,
                    mesh,
                    Vec3(wx0, wy1, -half_depth),
                    Vec3(wx1, wy1, -half_depth),
                    Vec3(wx1, wy1, half_depth),
                    Vec3(wx0, wy1, half_depth),
                    Vec3(0.0f, -1.0f, 0.0f),
                    0.0f, 0.0f, 1.0f, 1.0f
                );
            }
        }
    }

    return !mesh.mesh.indices.empty();
}

} // namespace generator
} // namespace nmesh
