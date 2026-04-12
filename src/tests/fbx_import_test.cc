#include <cstdio>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <string>

#include <xtcore/import_asset.h>
#include <xtcore/parseutil.h>
#include <xtcore/scene.h>
#include <xtcore/xtcore.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "fbx_import_test: %s\n", msg);
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

    const std::string fixture = join_path(repo_root, "scene/assets/fbx/blender_272_cube_7400_binary.fbx");
    if (!is_file(fixture)) return fail("missing FBX fixture");

    {
        xtcore::imported_asset_t asset;
        std::string err;
        if (!xtcore::import_asset_file(fixture.c_str(), asset, err)) {
            std::fprintf(stderr, "fbx_import_test: import_asset_file failed: %s\n", err.c_str());
            return 1;
        }
        if (asset.shapes.empty()) return fail("imported FBX contains no shapes");
        if (asset.attributes.v.empty()) return fail("imported FBX contains no vertices");
        if (asset.attributes.n.empty()) return fail("imported FBX contains no normals");
    }

    {
        const std::string scene_path = join_path(repo_root, "xtracer_fbx_import_test.scn");
        const std::string scene_text = std::string(
            "title = FBX Import Test\n"
            "description = object.source smoke test for FBX assets\n"
            "version = 1.0\n"
            "default_camera = cam\n"
            "environment = { type = color, config = { value = col3(0.05,0.05,0.05) } }\n"
            "camera = {\n"
            "  cam = {\n"
            "    type = thin-lens\n"
            "    fov = 45\n"
            "    position = vec3(0,0,-5)\n"
            "    target = vec3(0,0,0)\n"
            "    up = vec3(0,1,0)\n"
            "  }\n"
            "}\n"
            "object = {\n"
            "  imported = {\n"
            "    source = ") + fixture +
            "\n"
            "    prefix = imported_\n"
            "  }\n"
            "}\n";

        if (!write_text(scene_path, scene_text)) return fail("failed to write temporary scene");

        xtcore::Scene scene;
        if (xtcore::io::scn::load(&scene, scene_path.c_str()) != 0) {
            return fail("failed to load temporary FBX object scene");
        }
        if (scene.m_objects.empty()) return fail("loaded FBX object scene has no objects");
        if (scene.m_surface.empty()) return fail("loaded FBX object scene has no surfaces");
        if (scene.m_materials.empty()) return fail("loaded FBX object scene has no materials");
        unlink(scene_path.c_str());
    }

    xtcore::deinit();
    std::printf("fbx_import_test: ok\n");
    return 0;
}
