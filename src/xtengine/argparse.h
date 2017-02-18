#ifndef XTRACER_ARGPARSE_H_INCLUDED
#define XTRACER_ARGPARSE_H_INCLUDED

#include <string>
#include <list>
#include <xtcore/context.h>

using xtracer::render::params_t;

int setup(int argc, char** argv
        , std::string            &renderer
        , std::string            &outdir
        , std::string            &scene
        , std::list<std::string> &modifiers
        , std::string            &camera
        , params_t               &params);

#endif /* XTRACER_ARGPARSE_H_INCLUDED */
