#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>

#include <xtcore/scene_validator.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "scene_validator_test: %s\n", msg);
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

void collect_scene_files(const std::string &root, std::vector<std::string> &out)
{
    DIR *dir = opendir(root.c_str());
    if (!dir) return;

    struct dirent *ent = 0;
    while ((ent = readdir(dir)) != 0) {
        const char *name = ent->d_name;
        if (!name) continue;
        if (std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0) continue;

        const std::string full = join_path(root, name);
        if (is_dir(full)) {
            collect_scene_files(full, out);
            continue;
        }

        if (is_file(full) && full.size() >= 4 && full.substr(full.size() - 4) == ".scn") {
            out.push_back(full);
        }
    }

    closedir(dir);
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

} // namespace

int main()
{
    std::string repo_root;
    if (!find_repo_root(repo_root)) return fail("failed to locate repository root");

    const std::string scene_root = join_path(repo_root, "scene");
    std::vector<std::string> scenes;
    collect_scene_files(scene_root, scenes);

    if (scenes.empty()) return fail("no .scn files found under scene/");

    std::sort(scenes.begin(), scenes.end());

    bool all_ok = true;
    size_t invalid_count = 0;

    for (size_t i = 0; i < scenes.size(); ++i) {
        const std::string &scene = scenes[i];
        std::vector<std::string> errors;
        const int rc = xtcore::io::scn::validate(scene.c_str(), &errors);
        if (rc != 0) {
            all_ok = false;
            ++invalid_count;
            std::fprintf(stderr, "scene_validator_test: invalid scene %s\n", scene.c_str());
            for (size_t j = 0; j < errors.size(); ++j) {
                std::fprintf(stderr, "  - %s\n", errors[j].c_str());
            }
        }
    }

    if (!all_ok) {
        std::fprintf(stderr, "scene_validator_test: %lu/%lu scenes failed validation\n",
                     static_cast<unsigned long>(invalid_count),
                     static_cast<unsigned long>(scenes.size()));
        return 1;
    }

    std::printf("scene_validator_test: ok (%lu scenes)\n",
                static_cast<unsigned long>(scenes.size()));
    return 0;
}
