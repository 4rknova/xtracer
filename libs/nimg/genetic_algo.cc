#if defined (NIMG_TEST)

#include <ctime>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <nimg/pixmap.h>
#include <nimg/ppm.hpp>
#include <nimg/diff.h>
#include <nimg/rasteriser.h>
#include <nimg/genetic.h>

int test(int argc, char **argv)
{
    if (argc != 4) return 1;
    NImg::Pixmap src, dst;
	if (NImg::IO::Import::ppm_raw(argv[1], src)) return 2;
    dst.init(src.width(), src.height());
    srand(time(NULL));
    NImg::Genetic::init_seed(time(NULL));

    float curr   = NImg::Operator::diff_euclid(src, dst);
    float target = atof(argv[2]);
    float step   = atof(argv[3]);
    float iter   = curr;
    int gen      = 0;

    for (; target < iter; iter -= step ) {
        gen += NImg::Genetic::tri(src, dst, iter);
        curr = NImg::Operator::diff_euclid(src, dst);

        printf("\rGen %4d -> %0.3f", gen, curr); fflush(stdout);
    }
/*
    std::vector<NImg::Painter::triangle_t> triangles;
    gen += NImg::Genetic::tri(src, dst, triangles, target, (size_t)step);
    curr = NImg::Operator::diff_euclid(src, dst);
    printf("\rGen %4d -> %0.3f %f", gen, curr, target); fflush(stdout);
*/
    NImg::IO::Export::ppm_raw(std::string(argv[1]).append("_res.ppm").c_str(), dst);
    return 0;
}

#endif
