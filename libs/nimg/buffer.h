#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

namespace nimg {
    namespace util {

template <class T>
class Buffer
{
	public:
		Buffer();
		Buffer(const Buffer &rhs);
		Buffer &operator =(const Buffer &rhs);
		virtual ~Buffer();

		int init(const unsigned int count);
		void clear();

		unsigned int count() const;

		T& operator[] (const unsigned int idx);
		const T& operator[] (const unsigned int idx) const;

	private:
		void release();

		unsigned int m_count;
		// The following is a default value that will be used if
		// an index based access request is made on an uninitialized 
		// buffer.
		T m_single_object;
		T *m_data;
};

    } /* namespace util */
} /* namespace nimg */

#include "buffer.tml"

#endif /* BUFFER_H_INCLUDED */
