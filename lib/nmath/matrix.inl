#ifndef NMATH_MATRIX_INL_INCLUDED
#define NMATH_MATRIX_INL_INCLUDED

#ifndef NMATH_MATRIX_H_INCLUDED
    #error "matrix.h must be included before matrix.inl"
#endif /* NMATH_MATRIX_H_INCLUDED */

#include <cstring>

namespace nmath {

inline scalar_t *Matrix3x3f::operator [](int index)
{
    return data[index < 9 ? index : 8];
}

inline const scalar_t *Matrix3x3f::operator [](int index) const
{
    return data[index < 9 ? index : 8];
}

inline void Matrix3x3f::reset_identity()
{
    memcpy(data, identity.data, 9 * sizeof(scalar_t));
}

inline scalar_t *Matrix4x4f::operator [](int index) 
{
    return data[index < 16 ? index : 15];
}

inline const scalar_t *Matrix4x4f::operator [](int index) const
{
    return data[index < 16 ? index : 15];
}

inline void Matrix4x4f::reset_identity()
{
    memcpy(data, identity.data, 16 * sizeof(scalar_t));
}

} /* namespace nmath */

#endif /* NMATH_MATRIX_INL_INCLUDED */
