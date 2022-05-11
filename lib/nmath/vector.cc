#include "vector.h"

#ifdef __cplusplus

namespace nmath {

Vector2f::Vector2f(scalar_t aX, scalar_t aY): x(aX), y(aY){}
Vector2f::Vector2f(const Vector2f& v): x(v.x), y(v.y){}
Vector2f::Vector2f(const Vector3f& v): x(v.x), y(v.y){}
Vector2f::Vector2f(const Vector4f& v): x(v.x), y(v.y){}
Vector3f::Vector3f(scalar_t aX, scalar_t aY, scalar_t aZ): x(aX), y(aY), z(aZ){}
Vector3f::Vector3f(const Vector3f& v): x(v.x), y(v.y), z(v.z){}
Vector3f::Vector3f(const Vector2f& v): x(v.x), y(v.y), z(0.0f){}
Vector3f::Vector3f(const Vector4f& v): x(v.x), y(v.y), z(v.z){}
Vector4f::Vector4f(scalar_t aX, scalar_t aY, scalar_t aZ, scalar_t aW): x(aX), y(aY), z(aZ), w(aW){}
Vector4f::Vector4f(const Vector4f& v): x(v.x), y(v.y), z(v.z), w(v.w){}
Vector4f::Vector4f(const Vector2f& v): x(v.x), y(v.y), z(0.0f), w(0.0f){}
Vector4f::Vector4f(const Vector3f& v): x(v.x), y(v.y), z(v.z), w(0.0f){}

#endif  /* __cplusplus */

} /* namespace nmath */
