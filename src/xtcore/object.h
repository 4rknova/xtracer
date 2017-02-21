#ifndef XTRACER_OBJECT_H_INCLUDED
#define XTRACER_OBJECT_H_INCLUDED

#include <string>

namespace xtracer {
    namespace assets {

class Object
{
	public:
		std::string geometry;
		std::string material;
		std::string texture;
};

    } /* namespace assets */
} /* namespace xtracer */

#endif /* XTRACER_OBJECT_H_INCLUDED */
