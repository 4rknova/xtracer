#include "scene_validator.h"

#include <cstdio>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>

#include <ncf/ncf.h>
#include <ncf/util.h>

namespace xtcore {
namespace io {
namespace scn {

namespace {

const char *k_node_camera = "camera";
const char *k_node_geometry = "geometry";
const char *k_node_material = "material";
const char *k_node_medium = "medium";
const char *k_node_object = "object";
const char *k_prop_default_camera = "default_camera";
const char *k_prop_source = "source";
const char *k_prop_geometry = "geometry";
const char *k_prop_material = "material";
const char *k_prop_type = "type";
const char *k_prop_op = "op";
const char *k_prop_medium = "medium";
const char *k_prop_sigma_a = "sigma_a";
const char *k_prop_sigma_s = "sigma_s";
const char *k_prop_g = "g";
const char *k_prop_emission = "emission";
const char *k_group_left = "left";
const char *k_group_right = "right";

bool has_named_group(ncf::NCF *node, const std::string &name)
{
    if (!node || name.empty()) return false;
    return node->query_group(name.c_str());
}

bool parse_col3_value(const char *text, float &r, float &g, float &b)
{
    if (!text) return false;
    return std::sscanf(text, "col3(%f,%f,%f)", &r, &g, &b) == 3;
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
        if (!op.empty()
            && op != "union"
            && op != "soft_union"
            && op != "smooth_union"
            && op != "intersection"
            && op != "difference") {
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
    ncf::NCF *mediums = optional_group(&root, k_node_medium);
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

    if (mediums) {
        const size_t medium_count = mediums->count_groups();
        for (size_t i = 0; i < medium_count; ++i) {
            ncf::NCF *entry = mediums->get_group_by_index(i);
            if (!entry) continue;
            const char *name_c = entry->get_name();
            const std::string name = (name_c && *name_c) ? name_c : "<unnamed>";

            const std::string type = normalize_token(entry->get_property_by_name(k_prop_type));
            const bool is_homogeneous = (type == "homogeneous");
            const bool is_heterogeneous_noise = (type == "heterogeneous_noise");
            if (!is_homogeneous && !is_heterogeneous_noise) {
                std::ostringstream ss;
                ss << "medium '" << name << "' type must be 'homogeneous' or 'heterogeneous_noise'";
                add_error(errors, ss.str());
            }

            const char *sigma_a = entry->get_property_by_name(k_prop_sigma_a);
            const char *sigma_s = entry->get_property_by_name(k_prop_sigma_s);
            const char *emission = entry->get_property_by_name(k_prop_emission);
            const char *g_raw = entry->get_property_by_name(k_prop_g);

            float r = 0.0f, g = 0.0f, b = 0.0f;
            if (sigma_a && *sigma_a) {
                if (!parse_col3_value(sigma_a, r, g, b) || r < 0.0f || g < 0.0f || b < 0.0f) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid sigma_a";
                    add_error(errors, ss.str());
                }
            }
            if (sigma_s && *sigma_s) {
                if (!parse_col3_value(sigma_s, r, g, b) || r < 0.0f || g < 0.0f || b < 0.0f) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid sigma_s";
                    add_error(errors, ss.str());
                }
            }
            if (emission && *emission) {
                if (!parse_col3_value(emission, r, g, b) || r < 0.0f || g < 0.0f || b < 0.0f) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid emission";
                    add_error(errors, ss.str());
                }
            }
            if (g_raw && *g_raw) {
                const double gv = ncf::util::to_double(g_raw);
                if (gv < -1.0 || gv > 1.0) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid g (must be in [-1,1])";
                    add_error(errors, ss.str());
                }
            }

            if (is_heterogeneous_noise) {
                const char *density_raw = entry->get_property_by_name("density");
                const char *noise_scale_raw = entry->get_property_by_name("noise_scale");
                const char *noise_min_raw = entry->get_property_by_name("noise_min");
                const char *noise_max_raw = entry->get_property_by_name("noise_max");
                const char *octaves_raw = entry->get_property_by_name("octaves");
                const char *lacunarity_raw = entry->get_property_by_name("lacunarity");
                const char *gain_raw = entry->get_property_by_name("gain");

                if (entry->query_property("density") && density_raw && *density_raw && ncf::util::to_double(density_raw) < 0.0) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid density";
                    add_error(errors, ss.str());
                }
                if (entry->query_property("noise_scale") && noise_scale_raw && *noise_scale_raw && ncf::util::to_double(noise_scale_raw) <= 0.0) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid noise_scale";
                    add_error(errors, ss.str());
                }
                if (entry->query_property("noise_min") && noise_min_raw && *noise_min_raw && ncf::util::to_double(noise_min_raw) < 0.0) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid noise_min";
                    add_error(errors, ss.str());
                }
                if (entry->query_property("noise_max") && noise_max_raw && *noise_max_raw && ncf::util::to_double(noise_max_raw) < 0.0) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid noise_max";
                    add_error(errors, ss.str());
                }
                if (entry->query_property("noise_min") && entry->query_property("noise_max")
                    && noise_min_raw && *noise_min_raw && noise_max_raw && *noise_max_raw
                    && ncf::util::to_double(noise_min_raw) > ncf::util::to_double(noise_max_raw)) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' requires noise_min <= noise_max";
                    add_error(errors, ss.str());
                }
                if (entry->query_property("octaves") && octaves_raw && *octaves_raw && ncf::util::to_int(octaves_raw) <= 0) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid octaves";
                    add_error(errors, ss.str());
                }
                if (entry->query_property("lacunarity") && lacunarity_raw && *lacunarity_raw && ncf::util::to_double(lacunarity_raw) < 1.0) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid lacunarity";
                    add_error(errors, ss.str());
                }
                if (entry->query_property("gain") && gain_raw && *gain_raw) {
                    const double gain = ncf::util::to_double(gain_raw);
                    if (gain <= 0.0 || gain >= 1.0) {
                        std::ostringstream ss;
                        ss << "medium '" << name << "' has invalid gain (must be in (0,1))";
                        add_error(errors, ss.str());
                    }
                }
                if (entry->query_property("lacunarity") && (!lacunarity_raw || !*lacunarity_raw)) {
                    std::ostringstream ss;
                    ss << "medium '" << name << "' has invalid lacunarity";
                    add_error(errors, ss.str());
                }
            }
        }
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

            if (entry->query_group(k_prop_medium)) {
                std::ostringstream ss;
                ss << "object '" << name << "' uses inline medium block; use top-level 'medium' with 'medium = <id>' reference";
                add_error(errors, ss.str());
            }

            if (entry->query_property(k_prop_medium)) {
                const char *medium_name = entry->get_property_by_name(k_prop_medium);
                if (!medium_name || !*medium_name) {
                    std::ostringstream ss;
                    ss << "object '" << name << "' has empty medium reference";
                    add_error(errors, ss.str());
                } else if (!mediums || !has_named_group(mediums, medium_name)) {
                    std::ostringstream ss;
                    ss << "object '" << name << "' references missing medium '" << medium_name << "'";
                    add_error(errors, ss.str());
                }
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
