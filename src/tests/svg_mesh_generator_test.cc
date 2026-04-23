#include <cstdio>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <string>

#include <xtcore/mesh.h>
#include <xtcore/parseutil.h>
#include <xtcore/scene.h>
#include <xtcore/xtcore.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "svg_mesh_generator_test: %s\n", msg);
    return 1;
}

bool is_dir(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode) != 0;
}

bool is_file(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISREG(st.st_mode) != 0;
}

std::string join_path(const std::string &a, const std::string &b)
{
    if (a.empty()) return b;
    if (a[a.size() - 1] == '/') return a + b;
    return a + "/" + b;
}

std::string parent_dir(const std::string &path)
{
    if (path.empty()) return std::string();
    const size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) return std::string();
    if (pos == 0) return "/";
    return path.substr(0, pos);
}

bool find_repo_root(std::string &repo_root)
{
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) return false;

    std::string cur = cwd;
    for (size_t i = 0; i < 10; ++i) {
        if (is_dir(join_path(cur, "scene")) && is_file(join_path(cur, "CMakeLists.txt"))) {
            repo_root = cur;
            return true;
        }
        const std::string parent = parent_dir(cur);
        if (parent.empty() || parent == cur) break;
        cur = parent;
    }
    return false;
}

bool write_text(const std::string &path, const std::string &content)
{
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.good()) return false;
    out << content;
    return out.good();
}

} // namespace

int main()
{
    xtcore::init();

    std::string repo_root;
    if (!find_repo_root(repo_root)) return fail("failed to locate repository root");

    const std::string svg_path = join_path(repo_root, "xtracer_svg_mesh_generator_test.svg");
    const std::string scene_path = join_path(repo_root, "xtracer_svg_mesh_generator_test.scn");

    const std::string svg_text =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 120 120\">\n"
        "  <g transform=\"translate(60 60)\">\n"
        "    <rect x=\"-8\" y=\"-42\" width=\"16\" height=\"84\" transform=\"rotate(45)\" fill=\"#111111\"/>\n"
        "    <rect x=\"-8\" y=\"-42\" width=\"16\" height=\"84\" transform=\"rotate(-45)\" fill=\"#111111\"/>\n"
        "    <circle cx=\"0\" cy=\"0\" r=\"12\" fill=\"#111111\"/>\n"
        "  </g>\n"
        "</svg>\n";

    const std::string scene_text =
        "title = SVG Mesh Generator Test\n"
        "description = smoke test for gen(svg)\n"
        "version = 1.0\n"
        "default_camera = cam\n"
        "environment = { type = color, config = { value = col3(0.05,0.05,0.05) } }\n"
        "camera = {\n"
        "  cam = {\n"
        "    type = thin-lens\n"
        "    fov = 45\n"
        "    position = vec3(0,0,-4)\n"
        "    target = vec3(0,0,0)\n"
        "    up = vec3(0,1,0)\n"
        "  }\n"
        "}\n"
        "geometry = {\n"
        "  badge = {\n"
        "    type = mesh\n"
        "    source = gen(svg)\n"
        "    svg_source = xtracer_svg_mesh_generator_test.svg\n"
        "    resolution = 96\n"
        "    height = 0.18\n"
        "  }\n"
        "}\n"
        "material = {\n"
        "  matte = {\n"
        "    type = lambert\n"
        "    properties = {\n"
        "      samplers = {\n"
        "        diffuse = { type = color, value = col3(0.8,0.8,0.8) }\n"
        "      }\n"
        "    }\n"
        "  }\n"
        "}\n"
        "object = {\n"
        "  badge = { geometry = badge, material = matte }\n"
        "}\n";

    if (!write_text(svg_path, svg_text)) return fail("failed to write temporary svg");
    if (!write_text(scene_path, scene_text)) return fail("failed to write temporary scene");

    xtcore::Scene scene;
    const int rc = xtcore::io::scn::load(&scene, scene_path.c_str(), 0, 0);
    unlink(scene_path.c_str());
    unlink(svg_path.c_str());
    if (rc != 0) return fail("failed to load svg mesh scene");
    if (scene.m_surface.empty()) return fail("scene has no generated surfaces");

    const xtcore::asset::ISurface *surface = scene.m_surface.begin()->second;
    const xtcore::surface::Mesh *mesh = dynamic_cast<const xtcore::surface::Mesh *>(surface);
    if (!mesh) return fail("generated surface is not a mesh");
    if (mesh->triangles().empty()) return fail("generated mesh has no triangles");

    const float depth = mesh->aabb.max.z - mesh->aabb.min.z;
    if (depth <= 0.05f) return fail("generated svg mesh has no meaningful extrusion depth");

    std::printf("svg_mesh_generator_test: ok (%lu triangles)\n",
                (unsigned long)mesh->triangles().size());
    return 0;
}
