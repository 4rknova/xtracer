#include <cmath>
#include <cstdio>

#include <nmath/matrix.h>
#include <nmath/vector.h>

namespace {

nmath::scalar_t ref_dot(const nmath::Vector3f &a, const nmath::Vector3f &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

nmath::scalar_t ref_len_sq(const nmath::Vector3f &v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

nmath::Vector3f ref_add(const nmath::Vector3f &a, const nmath::Vector3f &b)
{
    return nmath::Vector3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

nmath::Vector3f ref_sub(const nmath::Vector3f &a, const nmath::Vector3f &b)
{
    return nmath::Vector3f(a.x - b.x, a.y - b.y, a.z - b.z);
}

nmath::Vector3f ref_mul(const nmath::Vector3f &a, const nmath::Vector3f &b)
{
    return nmath::Vector3f(a.x * b.x, a.y * b.y, a.z * b.z);
}

nmath::Vector3f ref_div(const nmath::Vector3f &a, const nmath::Vector3f &b)
{
    return nmath::Vector3f(a.x / b.x, a.y / b.y, a.z / b.z);
}

nmath::Matrix4x4f ref_mat4_add(const nmath::Matrix4x4f &a, const nmath::Matrix4x4f &b)
{
    nmath::Matrix4x4f r;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r[i][j] = a[i][j] + b[i][j];
        }
    }
    return r;
}

nmath::Matrix4x4f ref_mat4_sub(const nmath::Matrix4x4f &a, const nmath::Matrix4x4f &b)
{
    nmath::Matrix4x4f r;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r[i][j] = a[i][j] - b[i][j];
        }
    }
    return r;
}

nmath::Matrix4x4f ref_mat4_mul(const nmath::Matrix4x4f &a, const nmath::Matrix4x4f &b)
{
    nmath::Matrix4x4f r;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r[i][j] = a[i][0] * b[0][j] +
                      a[i][1] * b[1][j] +
                      a[i][2] * b[2][j] +
                      a[i][3] * b[3][j];
        }
    }
    return r;
}

nmath::Matrix4x4f ref_mat4_mul_scalar(const nmath::Matrix4x4f &a, nmath::scalar_t s)
{
    nmath::Matrix4x4f r;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r[i][j] = a[i][j] * s;
        }
    }
    return r;
}

nmath::Vector4f ref_mat4_mul_vec4(const nmath::Matrix4x4f &m, const nmath::Vector4f &v)
{
    return nmath::Vector4f(
        m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
        m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
        m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
        m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
    );
}

bool almost_equal(nmath::scalar_t a, nmath::scalar_t b, nmath::scalar_t eps)
{
    return std::fabs(a - b) <= eps;
}

bool vec_almost_equal(const nmath::Vector3f &a, const nmath::Vector3f &b, nmath::scalar_t eps)
{
    return almost_equal(a.x, b.x, eps)
        && almost_equal(a.y, b.y, eps)
        && almost_equal(a.z, b.z, eps);
}

bool vec4_almost_equal(const nmath::Vector4f &a, const nmath::Vector4f &b, nmath::scalar_t eps)
{
    return almost_equal(a.x, b.x, eps)
        && almost_equal(a.y, b.y, eps)
        && almost_equal(a.z, b.z, eps)
        && almost_equal(a.w, b.w, eps);
}

bool mat4_almost_equal(const nmath::Matrix4x4f &a, const nmath::Matrix4x4f &b, nmath::scalar_t eps)
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!almost_equal(a[i][j], b[i][j], eps)) return false;
        }
    }
    return true;
}

int fail(const char *msg)
{
    std::fprintf(stderr, "nmath_simd_test: %s\n", msg);
    return 1;
}

nmath::scalar_t series(int i, nmath::scalar_t f0, nmath::scalar_t f1)
{
    return nmath::scalar_t(std::sin((nmath::scalar_t)i * f0) * 3.0 + std::cos((nmath::scalar_t)i * f1) * 2.0);
}

} // namespace

int main()
{
    const nmath::scalar_t eps = (nmath::scalar_t)1e-11;

    for (int i = 1; i <= 12000; ++i) {
        const nmath::Vector3f a(
            series(i, (nmath::scalar_t)0.11, (nmath::scalar_t)0.07),
            series(i, (nmath::scalar_t)0.13, (nmath::scalar_t)0.05),
            series(i, (nmath::scalar_t)0.17, (nmath::scalar_t)0.03)
        );
        const nmath::Vector3f b(
            series(i, (nmath::scalar_t)0.19, (nmath::scalar_t)0.09) + (nmath::scalar_t)0.37,
            series(i, (nmath::scalar_t)0.23, (nmath::scalar_t)0.15) - (nmath::scalar_t)0.29,
            series(i, (nmath::scalar_t)0.29, (nmath::scalar_t)0.21) + (nmath::scalar_t)0.41
        );

        const nmath::Vector3f add = a + b;
        const nmath::Vector3f sub = a - b;
        const nmath::Vector3f mul = a * b;
        const nmath::Vector3f div = a / b;

        if (!vec_almost_equal(add, ref_add(a, b), eps)) return fail("operator+ mismatch");
        if (!vec_almost_equal(sub, ref_sub(a, b), eps)) return fail("operator- mismatch");
        if (!vec_almost_equal(mul, ref_mul(a, b), eps)) return fail("operator* mismatch");
        if (!vec_almost_equal(div, ref_div(a, b), (nmath::scalar_t)2e-11)) return fail("operator/ mismatch");

        nmath::Vector3f c = a;
        c += b;
        if (!vec_almost_equal(c, ref_add(a, b), eps)) return fail("operator+= mismatch");
        c = a;
        c -= b;
        if (!vec_almost_equal(c, ref_sub(a, b), eps)) return fail("operator-= mismatch");
        c = a;
        c *= b;
        if (!vec_almost_equal(c, ref_mul(a, b), eps)) return fail("operator*= mismatch");
        c = a;
        c /= b;
        if (!vec_almost_equal(c, ref_div(a, b), (nmath::scalar_t)2e-11)) return fail("operator/= mismatch");

        const nmath::scalar_t d = nmath::dot(a, b);
        if (!almost_equal(d, ref_dot(a, b), eps)) return fail("dot mismatch");

        const nmath::scalar_t lsq = a.length_squared();
        if (!almost_equal(lsq, ref_len_sq(a), eps)) return fail("length_squared mismatch");

        const nmath::scalar_t len = a.length();
        if (!almost_equal(len * len, ref_len_sq(a), (nmath::scalar_t)1e-9)) return fail("length mismatch");

        const nmath::scalar_t lsq_b = ref_len_sq(b);
        if (lsq_b > (nmath::scalar_t)1e-18) {
            const nmath::Vector3f bn = b.normalized();
            const nmath::scalar_t inv_len = (nmath::scalar_t)1.0 / std::sqrt(lsq_b);
            const nmath::Vector3f bref(b.x * inv_len, b.y * inv_len, b.z * inv_len);
            if (!vec_almost_equal(bn, bref, (nmath::scalar_t)2e-11)) return fail("normalized mismatch");

            nmath::Vector3f bi = b;
            bi.normalize();
            if (!vec_almost_equal(bi, bref, (nmath::scalar_t)2e-11)) return fail("normalize mismatch");
        }

        nmath::Matrix4x4f m1(
            a.x, a.y, a.z, (nmath::scalar_t)i * (nmath::scalar_t)0.001,
            b.x, b.y, b.z, (nmath::scalar_t)i * (nmath::scalar_t)-0.002,
            a.x - b.x, a.y + b.y, a.z - b.z, (nmath::scalar_t)1.0,
            (nmath::scalar_t)0.5, (nmath::scalar_t)-0.25, (nmath::scalar_t)0.75, (nmath::scalar_t)1.0
        );
        nmath::Matrix4x4f m2(
            b.z, a.y, (nmath::scalar_t)0.25, (nmath::scalar_t)-0.5,
            a.x, b.y, (nmath::scalar_t)0.75, (nmath::scalar_t)1.25,
            b.x, a.z, (nmath::scalar_t)-1.0, (nmath::scalar_t)0.33,
            (nmath::scalar_t)0.1, (nmath::scalar_t)0.2, (nmath::scalar_t)0.3, (nmath::scalar_t)1.0
        );

        if (!mat4_almost_equal(m1 + m2, ref_mat4_add(m1, m2), (nmath::scalar_t)2e-11)) return fail("mat4 operator+ mismatch");
        if (!mat4_almost_equal(m1 - m2, ref_mat4_sub(m1, m2), (nmath::scalar_t)2e-11)) return fail("mat4 operator- mismatch");
        if (!mat4_almost_equal(m1 * m2, ref_mat4_mul(m1, m2), (nmath::scalar_t)6e-11)) return fail("mat4 operator* mismatch");

        nmath::Matrix4x4f mc = m1;
        mc *= m2;
        if (!mat4_almost_equal(mc, ref_mat4_mul(m1, m2), (nmath::scalar_t)6e-11)) return fail("mat4 operator*= mismatch");

        const nmath::scalar_t s = (nmath::scalar_t)0.37;
        if (!mat4_almost_equal(m1 * s, ref_mat4_mul_scalar(m1, s), (nmath::scalar_t)2e-11)) return fail("mat4 scalar right-mul mismatch");
        if (!mat4_almost_equal(s * m1, ref_mat4_mul_scalar(m1, s), (nmath::scalar_t)2e-11)) return fail("mat4 scalar left-mul mismatch");

        nmath::Matrix4x4f ms = m1;
        ms *= s;
        if (!mat4_almost_equal(ms, ref_mat4_mul_scalar(m1, s), (nmath::scalar_t)2e-11)) return fail("mat4 scalar compound mismatch");

        const nmath::Vector4f v4(a.x, a.y, a.z, (nmath::scalar_t)1.0);
        if (!vec4_almost_equal(m1 * v4, ref_mat4_mul_vec4(m1, v4), (nmath::scalar_t)4e-11)) return fail("mat4 vec4 mul mismatch");
    }

    std::printf("nmath_simd_test: ok\n");
    return 0;
}
