#include <cstdio>
#include <list>
#include <string>
#include <vector>

#include <xtcore/context.h>
#include <xtcore/strpool.h>
#include <xtcore/xtcore.h>

#include <frontend/cli/argparse.h>

namespace {

int fail(const char *msg)
{
    std::fprintf(stderr, "cli_setup_test: %s\n", msg);
    return 1;
}

int run_setup(const std::vector<std::string> &args,
              std::string &renderer,
              std::string &outdir,
              std::string &scene,
              std::list<std::string> &modifiers,
              HASH_UINT64 &camera,
              xtcore::render::params_t &params)
{
    std::vector<char *> argv;
    argv.reserve(args.size());
    for (size_t i = 0; i < args.size(); ++i) {
        argv.push_back(const_cast<char *>(args[i].c_str()));
    }
    return setup((int)argv.size(), argv.data(), renderer, outdir, scene, modifiers, camera, params);
}

} // namespace

int main()
{
    xtcore::init();

    {
        std::string renderer, outdir, scene;
        std::list<std::string> modifiers;
        HASH_UINT64 camera = HASH_ID_INVALID;
        xtcore::render::params_t params;

        const int rc = run_setup({"xtracer_cli", "scene/lab-camera-modes-showcase.scn"},
            renderer, outdir, scene, modifiers, camera, params);
        if (rc != 0) return fail("minimal scene parse failed");
        if (scene != "scene/lab-camera-modes-showcase.scn") return fail("scene path mismatch");
    }

    {
        std::string renderer, outdir, scene;
        std::list<std::string> modifiers;
        HASH_UINT64 camera = HASH_ID_INVALID;
        xtcore::render::params_t params;

        const int rc = run_setup({
            "xtracer_cli", "scene/lab-camera-modes-showcase.scn",
            "-renderer", "stencil",
            "-res", "64x32",
            "-samples", "4",
            "-aa", "2",
            "-rdepth", "5",
            "-tile_size", "8",
            "-threads", "2",
            "-outdir", "/tmp",
            "-cam", "erp"
        }, renderer, outdir, scene, modifiers, camera, params);

        if (rc != 0) return fail("full parse failed");
        if (renderer != "stencil") return fail("renderer parse mismatch");
        if (outdir != "/tmp") return fail("outdir parse mismatch");
        if (params.width != 64 || params.height != 32) return fail("resolution parse mismatch");
        if (params.samples != 4) return fail("samples parse mismatch");
        if (params.aa != 2) return fail("aa parse mismatch");
        if (params.rdepth != 5) return fail("rdepth parse mismatch");
        if (params.tile_size != 8) return fail("tile_size parse mismatch");
        if (params.threads != 2) return fail("threads parse mismatch");
        if (camera == HASH_ID_INVALID) return fail("camera parse mismatch");
    }

    {
        std::string renderer, outdir, scene;
        std::list<std::string> modifiers;
        HASH_UINT64 camera = HASH_ID_INVALID;
        xtcore::render::params_t params;

        const int rc = run_setup({"xtracer_cli", "scene/lab-camera-modes-showcase.scn", "-aa", "1"},
            renderer, outdir, scene, modifiers, camera, params);
        if (rc != 2) return fail("invalid aa should fail with rc=2");
    }

    {
        std::string renderer, outdir, scene;
        std::list<std::string> modifiers;
        HASH_UINT64 camera = HASH_ID_INVALID;
        xtcore::render::params_t params;

        const int rc = run_setup({"xtracer_cli", "scene/lab-camera-modes-showcase.scn", "-does-not-exist"},
            renderer, outdir, scene, modifiers, camera, params);
        if (rc != 2) return fail("unknown flag should fail with rc=2");
    }

    {
        std::string renderer, outdir, scene;
        std::list<std::string> modifiers;
        HASH_UINT64 camera = HASH_ID_INVALID;
        xtcore::render::params_t params;

        const int rc = run_setup({"xtracer_cli"},
            renderer, outdir, scene, modifiers, camera, params);
        if (rc != 1) return fail("missing scene should fail with rc=1");
    }

    xtcore::deinit();

    std::printf("cli_setup_test: ok\n");
    return 0;
}
