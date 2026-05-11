#ifndef XTCORE_FRACTAL_H_INCLUDED
#define XTCORE_FRACTAL_H_INCLUDED

#include <array>
#include <nmath/precision.h>
#include <nmath/vector.h>

#include "ray.h"
#include "surface.h"

using nmath::Vector3f;
using nmath::scalar_t;

namespace xtcore {
namespace surface {

class MengerSponge : public xtcore::asset::ISurface
{
public:
    MengerSponge();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    Vector3f origin;
    Vector3f orientation;
    scalar_t radius;
    size_t iterations;
};

class SierpinskiTetrahedron : public xtcore::asset::ISurface
{
public:
    SierpinskiTetrahedron();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    Vector3f origin;
    scalar_t radius;
    size_t iterations;
};

class Mandelbulb : public xtcore::asset::ISurface
{
public:
    Mandelbulb();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    Vector3f origin;
    scalar_t radius;
    size_t iterations;
    scalar_t power;
    scalar_t bailout;
    int orbit_trap_channel; // 0=smooth_iter  1=sphere  2=plane_y
};

class JuliaFractal : public xtcore::asset::ISurface
{
public:
    JuliaFractal();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    Vector3f origin;
    Vector3f julia_c;
    scalar_t radius;
    size_t iterations;
    scalar_t power;
    scalar_t bailout;
    int orbit_trap_channel; // 0=smooth_iter  1=sphere  2=plane_y
};


class MandelBox : public xtcore::asset::ISurface
{
public:
    MandelBox();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    Vector3f origin;
    scalar_t radius;
    size_t   iterations;
    scalar_t fold_size;
    scalar_t min_r;
    scalar_t scale;
    scalar_t bailout;
    int orbit_trap_channel; // 0=escape_iter  1=sphere  2=box
};

class QuaternionJulia : public xtcore::asset::ISurface
{
public:
    QuaternionJulia();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    Vector3f origin;
    Vector3f quat_c;   /* xyz components of quaternion constant */
    scalar_t quat_cw;  /* w component of quaternion constant */
    scalar_t radius;
    size_t   iterations;
    scalar_t bailout;
    int orbit_trap_channel; // 0=escape_iter  1=sphere  2=plane_y
};

class BurningShip3D : public xtcore::asset::ISurface
{
public:
    BurningShip3D();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    Vector3f origin;
    scalar_t radius;
    size_t   iterations;
    scalar_t power;
    scalar_t bailout;
    int orbit_trap_channel; // 0=smooth_iter  1=sphere  2=plane_y
};

class CantorDust3D : public xtcore::asset::ISurface
{
public:
    CantorDust3D();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    Vector3f origin;
    Vector3f orientation;
    scalar_t radius;
    size_t   iterations;
};

class IcosahedralIFS : public xtcore::asset::ISurface
{
public:
    IcosahedralIFS();

    bool intersection(const Ray &ray, hit_record_t *i_hit_record) const;
    nmath::scalar_t distance(nmath::Vector3f p) const;
    void calc_aabb();

    Vector3f point_sample() const;
    Ray ray_sample() const;
    Vector3f emitter_position() const;

    Vector3f origin;
    scalar_t radius;
    size_t   iterations;
    scalar_t scale;
};

} /* namespace surface */
} /* namespace xtcore */

#endif /* XTCORE_FRACTAL_H_INCLUDED */
