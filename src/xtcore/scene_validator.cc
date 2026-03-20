#include "scene_validator.h"

#include <cstdio>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>

#include <ncf/ncf.h>

namespace xtcore {
namespace io {
namespace scn {

namespace {

const char *k_node_camera = "camera";
const char *k_node_geometry = "geometry";
const char *k_node_material = "material";
const char *k_node_object = "object";
const char *k_prop_default_camera = "default_camera";
const char *k_prop_source = "source";
const char *k_prop_geometry = "geometry";
const char *k_prop_material = "material";
const char *k_prop_type = "type";
const char *k_prop_op = "op";
const char *k_group_left = "left";
const char *k_group_right = "right";

bool has_named_group(ncf::NCF *node, const std::string &name)
{
    if (!node || name.empty()) return false;
    return node->query_group(name.c_str());
}

ncf::NCF *optional_group(ncf::NCF *node, const char *name)
{
    if (!node || !name || !*name) return 0;
    if (!node->query_group(name)) return 0;
    return node->get_group_by_name(name);
}

void add_error(std::vector<std::string> *errors, const std::string &msg)
{
    if (!errors) return;
    errors->push_back(msg);
}

std::string normalize_token(const char *text)
{
    if (!text) return std::string();
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
    out.erase(std::remove_if(out.begin(), out.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }), out.end());
    return out;
}

bool is_supported_csg_leaf_type(const std::string &type)
{
    static const std::set<std::string> k_types = {
        "sphere",
        "point",
        "plane",
        "menger_sponge",
        "sierpinski_tetrahedron",
        "mandelbulb",
        "julia"
    };
    return k_types.find(type) != k_types.end();
}

void validate_csg_node(ncf::NCF *node, const std::string &path, size_t depth, std::vector<std::string> *errors)
{
    if (!node) {
        add_error(errors, path + " is missing");
        return;
    }
    if (depth > 32) {
        add_error(errors, path + " exceeds max recursion depth (32)");
        return;
    }

    const bool has_left = node->query_group(k_group_left);
    const bool has_right = node->query_group(k_group_right);
    const bool has_op = node->query_property(k_prop_op);
    const bool branch = has_left || has_right || has_op;

    if (branch) {
        if (!has_op) add_error(errors, path + " is missing 'op'");
        if (!has_left) add_error(errors, path + " is missing 'left'");
        if (!has_right) add_error(errors, path + " is missing 'right'");

        const std::string op = normalize_token(node->get_property_by_name(k_prop_op));
        if (!op.empty() && op != "union" && op != "intersection" && op != "difference") {
            add_error(errors, path + " has invalid op '" + op + "'");
        }

        if (has_left) validate_csg_node(node->get_group_by_name(k_group_left), path + ".left", depth + 1, errors);
        if (has_right) validate_csg_node(node->get_group_by_name(k_group_right), path + ".right", depth + 1, errors);
        return;
    }

    const std::string leaf_type = normalize_token(node->get_property_by_name(k_prop_type));
    if (leaf_type.empty()) {
        add_error(errors, path + " leaf is missing 'type'");
        return;
    }
    if (!is_supported_csg_leaf_type(leaf_type)) {
        add_error(errors, path + " leaf type '" + leaf_type + "' is not supported in CSG");
    }
}

} // namespace

int validate(const char *filename, std::vector<std::string> *errors)
{
    if (errors) errors->clear();
    if (!filename || !*filename) {
        add_error(errors, "missing filename");
        return 1;
    }

    ncf::NCF root;
    root.set_source(filename);

    ncf::error_t parse_error;
    if (root.parse(&parse_error)) {
        std::ostringstream ss;
        ss << "parse error at line " << static_cast<unsigned long>(parse_error.line)
           << ": " << (parse_error.message ? parse_error.message : "unknown error");
        add_error(errors, ss.str());
        return 1;
    }

    ncf::NCF *camera = optional_group(&root, k_node_camera);
    ncf::NCF *geometry = optional_group(&root, k_node_geometry);
    ncf::NCF *material = optional_group(&root, k_node_material);
    ncf::NCF *object = optional_group(&root, k_node_object);

    if (!camera) add_error(errors, "missing top-level 'camera' group");
    if (!geometry) add_error(errors, "missing top-level 'geometry' group");
    if (!material) add_error(errors, "missing top-level 'material' group");
    if (!object) add_error(errors, "missing top-level 'object' group");

    if (camera && camera->count_groups() == 0) add_error(errors, "'camera' group has no entries");

    const char *default_camera = root.get_property_by_name(k_prop_default_camera);
    if (camera && default_camera && *default_camera && !camera->query_group(default_camera)) {
        std::ostringstream ss;
        ss << "default_camera '" << default_camera << "' does not exist in 'camera'";
        add_error(errors, ss.str());
    }

    if (object) {
        const size_t object_count = object->count_groups();
        for (size_t i = 0; i < object_count; ++i) {
            ncf::NCF *entry = object->get_group_by_index(i);
            if (!entry) continue;

            const char *name_c = entry->get_name();
            const std::string name = (name_c && *name_c) ? name_c : "<unnamed>";

            const bool is_external = entry->query_property(k_prop_source);
            if (is_external) {
                const char *src = entry->get_property_by_name(k_prop_source);
                if (!src || !*src) {
                    std::ostringstream ss;
                    ss << "object '" << name << "' has empty source";
                    add_error(errors, ss.str());
                }
                continue;
            }

            if (!entry->query_property(k_prop_geometry)) {
                std::ostringstream ss;
                ss << "object '" << name << "' is missing geometry reference";
                add_error(errors, ss.str());
            }
            if (!entry->query_property(k_prop_material)) {
                std::ostringstream ss;
                ss << "object '" << name << "' is missing material reference";
                add_error(errors, ss.str());
            }

            const char *geo_name = entry->get_property_by_name(k_prop_geometry);
            if (geometry && geo_name && *geo_name && !has_named_group(geometry, geo_name)) {
                std::ostringstream ss;
                ss << "object '" << name << "' references missing geometry '" << geo_name << "'";
                add_error(errors, ss.str());
            }

            const char *mat_name = entry->get_property_by_name(k_prop_material);
            if (material && mat_name && *mat_name && !has_named_group(material, mat_name)) {
                std::ostringstream ss;
                ss << "object '" << name << "' references missing material '" << mat_name << "'";
                add_error(errors, ss.str());
            }
        }
    }

    if (geometry) {
        const size_t geometry_count = geometry->count_groups();
        for (size_t i = 0; i < geometry_count; ++i) {
            ncf::NCF *entry = geometry->get_group_by_index(i);
            if (!entry) continue;

            const std::string name = (entry->get_name() && *entry->get_name()) ? entry->get_name() : std::string("<unnamed>");
            const std::string type = normalize_token(entry->get_property_by_name(k_prop_type));
            if (type != "csg") continue;

            validate_csg_node(entry, "geometry." + name, 0, errors);
        }
    }

    root.purge();
    return (errors && !errors->empty()) ? 1 : 0;
}

} /* namespace scn */
} /* namespace io */
} /* namespace xtcore */
