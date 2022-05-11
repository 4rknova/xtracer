#if defined (NIMG_TEST)

#include <ctime>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <nimg/pixmap.h>
#include <nimg/img.h>
#include <nimg/diff.h>
#include <nimg/rasteriser.h>
#include <nimg/genetic.h>

int test(int argc, char **argv)
{
    if (argc != 4) return 1;
    nimg::Pixmap src, dst;
    int load_err = nimg::io::load::image(argv[1], src);

	if (load_err) { printf("\nerr:%i\n",load_err); return 2; }
    dst.init(src.width(), src.height());
    srand(time(NULL));
    nimg::genetic_algorithms::init_seed(time(NULL));

    float curr   = nimg::eval::diff_euclid(src, dst);
    float target = atof(argv[2]);
    float step   = atof(argv[3]);
    float iter   = curr;
    int gen      = 0;

    for (; target < iter; iter -= step ) {
        gen += nimg::genetic_algorithms::tri(src, dst, iter);
        curr = nimg::eval::diff_euclid(src, dst);

        printf("\rGen %4d -> %0.3f", gen, curr); fflush(stdout);
    nimg::io::save::png(std::string(argv[1]).append("_res.png").c_str(), dst);
    }
/*
    std::vector<nimg::Painter::triangle_t> triangles;
    gen += nimg::Genetic::tri(src, dst, triangles, target, (size_t)step);
    curr = nimg::eval::diff_euclid(src, dst);
    printf("\rGen %4d -> %0.3f %f", gen, curr, target); fflush(stdout);
*/
    nimg::io::save::png(std::string(argv[1]).append("_res.png").c_str(), dst);
    return 0;
}

#endif
