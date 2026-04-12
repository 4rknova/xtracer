#include "xtcore.h"
#include "config.h"
#include "strpool.h"
#include "res/license.h"

namespace xtcore {

const char *get_version()
{
    return XTCORE_VERSION;
}

const char *get_license()
{
    return __LICENSE;
}

const char *get_empty_scene_template()
{
    static const char *k_template =
        "title = Untitled Scene\n"
        "description = Empty scene scaffold\n"
        "version = 1.0\n"
        "\n"
        "environment = {\n"
        "    type = gradient\n"
        "    config = {\n"
        "        a = col3(1,1,1)\n"
        "        b = col3(0.5,0.7,1.0)\n"
        "    }\n"
        "}\n"
        "\n"
        "camera = {\n"
        "    default = {\n"
        "        type = thin-lens\n"
        "        up = vec3(0,1,0)\n"
        "        position = vec3(0,1,-5)\n"
        "        target = vec3(0,1,0)\n"
        "        fov = 60\n"
        "        flength = 50\n"
        "        aperture = 0\n"
        "    }\n"
        "}\n";
    return k_template;
}

int init()
{
    xtcore::pool::str::init();
    return 0;
}

int deinit()
{
    xtcore::pool::str::release();
    return 0;
}

} /* namespace xtcore */
