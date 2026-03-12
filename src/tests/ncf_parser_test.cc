#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include <ncf/ncf.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "ncf_parser_test: %s\n", msg);
    return 1;
}

bool write_text(const std::string &path, const std::string &data)
{
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.good()) return false;
    out << data;
    return out.good();
}

bool equals(const char *lhs, const char *rhs)
{
    if (!lhs || !rhs) return false;
    return std::string(lhs) == std::string(rhs);
}

bool has_group(const ncf::NCF *node, const char *name)
{
    return node && node->query_group(name);
}

} // namespace

int main()
{
    const std::string path = "/tmp/xtracer_ncf_parser_test.scn";
    const std::string content =
        "# UTF-8 comments should not invalidate parsing: \xE2\x80\x94 \xC2\xB1 \xCE\xB1\n"
        "object = {\n"
        "  obj_floor = { geometry = geo_floor, material = mat_white }\n"
        "}\n"
        "material = {\n"
        "  mat_white = {\n"
        "    properties = {\n"
        "      samplers = {\n"
        "        diffuse = { type = color, value = col3(0.7, 0.7, 0.7) }\n"
        "      }\n"
        "      scalars = { }\n"
        "    }\n"
        "  }\n"
        "}\n";

    if (!write_text(path, content)) return fail("failed to write temporary scene");

    ncf::NCF root;
    root.set_source(path.c_str());
    ncf::error_t error;
    if (root.parse(&error)) {
        std::fprintf(stderr, "ncf_parser_test: parse failed at line %lu (%s)\n",
                     static_cast<unsigned long>(error.line), error.message);
        return 1;
    }

    if (!has_group(&root, "object")) return fail("missing object group");
    ncf::NCF *object = root.get_group_by_name("object");
    if (!has_group(object, "obj_floor")) return fail("missing inline object entry");

    ncf::NCF *obj_floor = object->get_group_by_name("obj_floor");
    if (!equals(obj_floor->get_property_by_name("geometry"), "geo_floor")) return fail("geometry property mismatch");
    if (!equals(obj_floor->get_property_by_name("material"), "mat_white")) return fail("material property mismatch");

    if (!has_group(&root, "material")) return fail("missing material group");
    ncf::NCF *material = root.get_group_by_name("material");
    if (!has_group(material, "mat_white")) return fail("missing mat_white group");

    ncf::NCF *mat_white = material->get_group_by_name("mat_white");
    if (!has_group(mat_white, "properties")) return fail("missing properties group");
    ncf::NCF *properties = mat_white->get_group_by_name("properties");

    if (!has_group(properties, "samplers")) return fail("missing samplers group");
    ncf::NCF *samplers = properties->get_group_by_name("samplers");
    if (!has_group(samplers, "diffuse")) return fail("missing diffuse inline sampler");
    ncf::NCF *diffuse = samplers->get_group_by_name("diffuse");
    if (!equals(diffuse->get_property_by_name("type"), "color")) return fail("sampler type mismatch");
    if (!equals(diffuse->get_property_by_name("value"), "col3(0.7, 0.7, 0.7)")) return fail("sampler value mismatch");

    if (!has_group(properties, "scalars")) return fail("missing empty inline scalars group");
    if (properties->get_group_by_name("scalars")->count_entries() != 0) return fail("empty inline group should have no entries");

    std::remove(path.c_str());
    std::printf("ncf_parser_test: ok\n");
    return 0;
}
