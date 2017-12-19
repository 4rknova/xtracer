#ifndef XTRACER_ARGPARSE_H_INCLUDED
#define XTRACER_ARGPARSE_H_INCLUDED

#include <list>
#include <xtcore/xtcore.h>

using xtcore::render::params_t;

int setup(int argc, char** argv
        , std::string            &renderer
        , std::string            &outdir
        , std::string            &scene
        , std::list<std::string> &modifiers
        , HASH_UINT64            &camera
        , params_t               &params);

#endif /* XTRACER_ARGPARSE_H_INCLUDED */
