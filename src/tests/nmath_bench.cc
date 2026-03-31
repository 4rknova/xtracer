#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>

#if defined(NMATH_BENCH_FORCE_SCALAR)
#if defined(NMATH_ENABLE_SIMD)
#undef NMATH_ENABLE_SIMD
#endif
#if defined(NMATH_ENABLE_SIMD_AVX)
#undef NMATH_ENABLE_SIMD_AVX
#endif
#endif

#include <nmath/matrix.h>
#include <nmath/prng.h>
#include <nmath/vector.h>

namespace {

using clock_t = std::chrono::high_resolution_clock;

template <typename Fn>
double run_bench(const char *name, const char *metric_key, const uint64_t iters, Fn fn, volatile nmath::scalar_t &sink)
{
    const auto t0 = clock_t::now();
    fn(sink);
    const auto t1 = clock_t::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double mops = (double)iters / (ms * 1000.0);
    std::printf("%-22s : %10.3f ms  (%8.2f Mops/s)\n", name, ms, mops);
    if (metric_key && metric_key[0] != '\0') {
        std::printf("metric|%s|%.6f\n", metric_key, ms);
    }
    return ms;
}

} // namespace

int main(int argc, char **argv)
{
    uint64_t iters = 4000000ULL;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--iters") == 0 && (i + 1) < argc) {
            const unsigned long long parsed = std::strtoull(argv[i + 1], nullptr, 10);
            if (parsed > 0ULL) iters = static_cast<uint64_t>(parsed);
            i += 1;
        }
    }
    volatile nmath::scalar_t sink = 0;

    double total_ms = 0.0;

    std::printf("mode                   : %s\n",
#if defined(NMATH_ENABLE_SIMD)
#if defined(NMATH_ENABLE_SIMD_AVX) && defined(__AVX__)
        "simd+avx"
#else
        "simd"
#endif
#else
        "scalar"
#endif
    );

    total_ms += run_bench("prng_c", "prng_c", iters, [&](volatile nmath::scalar_t &s) {
        for (uint64_t i = 0; i < iters; ++i) s += nmath::prng_c((nmath::scalar_t)-1.0, (nmath::scalar_t)1.0);
    }, sink);

    total_ms += run_bench("vec3 normalize", "vec3_normalize", iters, [&](volatile nmath::scalar_t &s) {
        nmath::Vector3f v((nmath::scalar_t)1.0, (nmath::scalar_t)2.0, (nmath::scalar_t)3.0);
        for (uint64_t i = 0; i < iters; ++i) {
            v.x += (nmath::scalar_t)0.000001;
            const nmath::Vector3f n = v.normalized();
            s += n.x + n.y + n.z;
        }
    }, sink);

    total_ms += run_bench("vec3 dot+cross", "vec3_dot_cross", iters, [&](volatile nmath::scalar_t &s) {
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

    total_ms += run_bench("mat4 mul", "mat4_mul", iters, [&](volatile nmath::scalar_t &s) {
        nmath::Matrix4x4f a;
        nmath::Matrix4x4f b;
        a.rotate(nmath::Vector3f((nmath::scalar_t)0.2, (nmath::scalar_t)0.4, (nmath::scalar_t)0.1));
        b.rotate(nmath::Vector3f((nmath::scalar_t)-0.1, (nmath::scalar_t)0.5, (nmath::scalar_t)0.3));
        for (uint64_t i = 0; i < iters; ++i) {
            a = a * b;
            s += a[0][0];
        }
    }, sink);

    const std::size_t batch_count = 4096;
    const uint64_t batch_rounds = std::max<uint64_t>(1ULL, iters / static_cast<uint64_t>(batch_count));
    std::vector<nmath::scalar_t> ax(batch_count), ay(batch_count), az(batch_count);
    std::vector<nmath::scalar_t> bx(batch_count), by(batch_count), bz(batch_count);
    std::vector<nmath::scalar_t> nx(batch_count), ny(batch_count), nz(batch_count);
    std::vector<nmath::scalar_t> d(batch_count), cx(batch_count), cy(batch_count), cz(batch_count);
    for (std::size_t i = 0; i < batch_count; ++i) {
        const nmath::scalar_t t = static_cast<nmath::scalar_t>(i + 1) * (nmath::scalar_t)0.001;
        ax[i] = t;
        ay[i] = t * (nmath::scalar_t)0.5 + (nmath::scalar_t)0.1;
        az[i] = t * (nmath::scalar_t)0.25 + (nmath::scalar_t)0.2;
        bx[i] = (nmath::scalar_t)0.7 - t * (nmath::scalar_t)0.1;
        by[i] = (nmath::scalar_t)-0.2 + t * (nmath::scalar_t)0.05;
        bz[i] = (nmath::scalar_t)0.3 + t * (nmath::scalar_t)0.02;
    }

    total_ms += run_bench("vec3 batch normalize", "vec3_batch_normalize", batch_rounds * batch_count, [&](volatile nmath::scalar_t &s) {
        for (uint64_t r = 0; r < batch_rounds; ++r) {
            nmath::batch::vec3_normalize_soa(ax.data(), ay.data(), az.data(), nx.data(), ny.data(), nz.data(), batch_count);
            const std::size_t idx = static_cast<std::size_t>((r * 131ULL) % batch_count);
            s += nx[idx] + ny[idx] + nz[idx];
            ax[idx] += (nmath::scalar_t)0.0000001;
        }
    }, sink);

    total_ms += run_bench("vec3 batch dot+cross", "vec3_batch_dot_cross", batch_rounds * batch_count, [&](volatile nmath::scalar_t &s) {
        for (uint64_t r = 0; r < batch_rounds; ++r) {
            nmath::batch::vec3_dot_cross_soa(ax.data(), ay.data(), az.data(), bx.data(), by.data(), bz.data(), d.data(), cx.data(), cy.data(), cz.data(), batch_count);
            const std::size_t idx = static_cast<std::size_t>((r * 97ULL) % batch_count);
            s += d[idx] + cx[idx] + cy[idx] + cz[idx];
            by[idx] -= (nmath::scalar_t)0.0000001;
        }
    }, sink);

    std::printf("total                  : %10.3f ms\n", total_ms);
    std::printf("metric|total|%.6f\n", total_ms);
    std::printf("sink                   : %.6f\n", (double)sink);
    return 0;
}
