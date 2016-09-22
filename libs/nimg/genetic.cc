#include <ctime>
#include <cfloat>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include "diff.h"
#include "genetic.h"

namespace NImg {
    namespace Genetic {

int init_seed(unsigned int seed)
{
    srand(seed);
    return seed;
}

int tri(const Pixmap &src, Pixmap &dst, const float threshold)
{
    if (   src.width() != dst.width()
        || src.height() != dst.height()
    ) return 1;

    int mutations = 0;

    float diff =  NImg::Operator::diff_euclid(src, dst);

    for (; diff >= threshold; ++mutations) {
        NImg::Pixmap temp = dst;

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

        diff = tdiff;
        dst = temp;
    }

    return mutations;
}

int tri(const Pixmap &src, Pixmap &dst
    , std::vector<Painter::triangle_t> triangles
    , const float threshold
    , const size_t budget)
{
    if (   src.width() != dst.width()
        || src.height() != dst.height()
    ) return 1;

    int mutations = 0;

    float diff =  NImg::Operator::diff_euclid(src, dst);

    for (unsigned int i = 0; i < budget; ++i) {
            Painter::triangle_t tri;

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

            triangles.push_back(tri);
    }

printf("\n%f",threshold); fflush(stdout);
    for (; diff >= threshold; ++mutations) {
        float tdiff = diff;
        int idx = rand() % budget;
        Painter::triangle_t t = triangles[idx];

        while (tdiff >= diff) {
            NImg::ColorRGBAf c;
            float l = (rand()%255) / 255.f;
            float a = (rand()%255) / 255.f;

            c.r(l);
            c.g(l);
            c.b(l);
            c.a(a);

            int w = src.width();
            int h = src.height();
            t.a.v.x = rand()%w;
            t.b.v.x = rand()%w;
            t.c.v.x = rand()%w;
            t.a.v.y = rand()%h;
            t.b.v.y = rand()%h;
            t.c.v.y = rand()%h;

            t.a.c = c;
            t.b.c = c;
            t.c.c = c;


            Pixmap temp;
            for (unsigned int i = 0; i < budget; ++i) NImg::Painter::fill_triangle(temp, triangles[i]);
            tdiff = NImg::Operator::diff_euclid(src, temp);
        }
        triangles[idx] = t;
        diff = tdiff;
    }

    for (unsigned int i = 0; i < budget; ++i) NImg::Painter::fill_triangle(dst, triangles[i]);
    return mutations;
}

    } /* namespace Genetic */
} /* namespace NImg */
