#include "scene_validator.h"

#include <cstdio>
#include <sstream>

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

    root.purge();
    return (errors && !errors->empty()) ? 1 : 0;
}

} /* namespace scn */
} /* namespace io */
} /* namespace xtcore */
