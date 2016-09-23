#ifndef NIMG_COLOR_H_INCLUDED
#define NIMG_COLOR_H_INCLUDED

namespace nimg {

// Forward declarations
class ColorRGBf;
class ColorRGBAf;

// COLOR RGB
class ColorRGBf
{
	public:
		explicit ColorRGBf(const float r = 0.0f, const float g = 0.0f, const float b = 0.0f);
		ColorRGBf(const ColorRGBf &rhs);
		ColorRGBf(const ColorRGBAf &rhs);

		// Operators
		inline ColorRGBf &operator +=(const ColorRGBf &rhs);
		inline ColorRGBf &operator -=(const ColorRGBf &rhs);
		inline ColorRGBf &operator *=(const ColorRGBf &rhs);
		inline ColorRGBf &operator *=(const float s);

		inline void r(const float r);
		inline void g(const float g);
		inline void b(const float b);

		inline float r() const;
		inline float g() const;
		inline float b() const;

	private:
		float m_r, m_g, m_b;

};

// Auxiliary operators
inline ColorRGBf operator +(const ColorRGBf &lhs, const ColorRGBf &rhs);
inline ColorRGBf operator -(const ColorRGBf &lhs, const ColorRGBf &rhs);
inline ColorRGBf operator *(const ColorRGBf &lhs, const ColorRGBf &rhs);
inline ColorRGBf operator *(const ColorRGBf &lhs, const float s);
inline ColorRGBf operator *(const float s, const ColorRGBf &lhs);

/*
    COLOR RGBA
*/
class ColorRGBAf
{
	public:
		explicit ColorRGBAf(const float r = 0.0f, const float g = 0.0f, const float b = 0.0f, const float a = 1.0f);
		ColorRGBAf(const ColorRGBf &rhs);
		ColorRGBAf(const ColorRGBAf &rhs);

		// Operators
		inline ColorRGBAf &operator +=(const ColorRGBAf &rhs);
		inline ColorRGBAf &operator -=(const ColorRGBAf &rhs);
		inline ColorRGBAf &operator *=(const ColorRGBAf &rhs);
		inline ColorRGBAf &operator *=(const float s);

		void r(const float r);
		void g(const float g);
		void b(const float b);
		void a(const float a);

		float r() const;
		float g() const;
		float b() const;
		float a() const;

	private:
		float m_r, m_g, m_b, m_a;

};

// Auxiliary operators
inline ColorRGBAf operator +(const ColorRGBAf &lhs, const ColorRGBAf &rhs);
inline ColorRGBAf operator -(const ColorRGBAf &lhs, const ColorRGBAf &rhs);
inline ColorRGBAf operator *(const ColorRGBAf &lhs, const ColorRGBAf &rhs);
inline ColorRGBAf operator *(const ColorRGBAf &lhs, const float s);
inline ColorRGBAf operator *(const float s, const ColorRGBAf &lhs);

} /* namespace nimg */

#include "color.inl"

#endif /* NIMG_COLOR_H_INCLUDED */
