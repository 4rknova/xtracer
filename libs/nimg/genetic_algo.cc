#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <nimg/pixmap.h>
#include <nimg/ppm.hpp>
#include <nimg/diff.h>
#include <nimg/rasteriser.h>

int main(int argc, char **argv)
{
    if (argc != 3) return 1;

    srand(time(NULL));

    int approx = atoi(argv[2]);

    NImg::Pixmap src, dst;

	if (NImg::IO::Import::ppm_raw(argv[1], src)) return 2;

    dst.init(src.width(), src.height());

    float diff = NImg::Operator::diff_euclid(src, dst);

    for (int i = 0 ; diff > approx; ++i) {
        NImg::Pixmap temp = dst;

        printf("\rMutation %3d %5.3f", i, diff);
        fflush(stdout);

        float tdiff = diff;
        while (tdiff >= diff) {
            temp = dst;
            NImg::Painter::triangle_t tri;

            NImg::ColorRGBAf c;
            float l = (rand()%255) / 255.f;
            float a = (rand()%255) / 255.f;

            c.r(l);
            c.g(l);
            c.b(l);
            c.a(a);

            int w = src.width();
            int h = src.height();
            tri.a.v.x = rand()%w;
            tri.b.v.x = rand()%w;
            tri.c.v.x = rand()%w;
            tri.a.v.y = rand()%h;
            tri.b.v.y = rand()%h;
            tri.c.v.y = rand()%h;

            tri.a.c = c;
            tri.b.c = c;
            tri.c.c = c;

            NImg::Painter::fill_triangle(temp, tri);
            tdiff = NImg::Operator::diff_euclid(src, temp);
        }


        std::stringstream t;
        t << i;
//	    NImg::IO::Export::ppm_raw((t.str().append(".ppm")).c_str(), dst);
        diff = tdiff;
        dst = temp;
        NImg::IO::Export::ppm_raw(std::string(argv[1]).append("_intermediate.ppm").c_str(), dst);
    }

	NImg::IO::Export::ppm_raw(std::string(argv[1]).append("_final.ppm").c_str(), dst);

    return 0;
}
