#include <chrono>
#include <cstdint>
#include <cstdio>

#include <nmath/matrix.h>
#include <nmath/prng.h>
#include <nmath/vector.h>

namespace {

using clock_t = std::chrono::high_resolution_clock;

template <typename Fn>
double run_bench(const char *name, const uint64_t iters, Fn fn, volatile nmath::scalar_t &sink)
{
    const auto t0 = clock_t::now();
    fn(sink);
    const auto t1 = clock_t::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double mops = (double)iters / (ms * 1000.0);
    std::printf("%-22s : %10.3f ms  (%8.2f Mops/s)\n", name, ms, mops);
    return ms;
}

} // namespace

int main()
{
    const uint64_t iters = 4000000ULL;
    volatile nmath::scalar_t sink = 0;

    double total_ms = 0.0;

    total_ms += run_bench("prng_c", iters, [&](volatile nmath::scalar_t &s) {
        for (uint64_t i = 0; i < iters; ++i) s += nmath::prng_c((nmath::scalar_t)-1.0, (nmath::scalar_t)1.0);
    }, sink);

    total_ms += run_bench("vec3 normalize", iters, [&](volatile nmath::scalar_t &s) {
        nmath::Vector3f v((nmath::scalar_t)1.0, (nmath::scalar_t)2.0, (nmath::scalar_t)3.0);
        for (uint64_t i = 0; i < iters; ++i) {
            v.x += (nmath::scalar_t)0.000001;
            const nmath::Vector3f n = v.normalized();
            s += n.x + n.y + n.z;
        }
    }, sink);

    total_ms += run_bench("vec3 dot+cross", iters, [&](volatile nmath::scalar_t &s) {
        nmath::Vector3f a((nmath::scalar_t)0.2, (nmath::scalar_t)0.5, (nmath::scalar_t)0.7);
        nmath::Vector3f b((nmath::scalar_t)-0.3, (nmath::scalar_t)0.9, (nmath::scalar_t)0.1);
        for (uint64_t i = 0; i < iters; ++i) {
            const nmath::scalar_t d = nmath::dot(a, b);
            const nmath::Vector3f c = nmath::cross(a, b);
            s += d + c.x + c.y + c.z;
            a.x += (nmath::scalar_t)0.0000001;
            b.y -= (nmath::scalar_t)0.0000001;
        }
    }, sink);

    total_ms += run_bench("mat4 mul", iters, [&](volatile nmath::scalar_t &s) {
        nmath::Matrix4x4f a;
        nmath::Matrix4x4f b;
        a.rotate(nmath::Vector3f((nmath::scalar_t)0.2, (nmath::scalar_t)0.4, (nmath::scalar_t)0.1));
        b.rotate(nmath::Vector3f((nmath::scalar_t)-0.1, (nmath::scalar_t)0.5, (nmath::scalar_t)0.3));
        for (uint64_t i = 0; i < iters; ++i) {
            a = a * b;
            s += a[0][0];
        }
    }, sink);

    std::printf("total                  : %10.3f ms\n", total_ms);
    std::printf("sink                   : %.6f\n", (double)sink);
    return 0;
}
