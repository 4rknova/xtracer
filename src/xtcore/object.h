#ifndef XTCORE_OBJECT_H_INCLUDED
#define XTCORE_OBJECT_H_INCLUDED

#include <string>

namespace xtcore {
    namespace assets {

class Object
{
	public:
	std::string geometry;
	std::string material;

    Object();
};

    } /* namespace assets */
} /* namespace xtcore */

#endif /* XTCORE_OBJECT_H_INCLUDED */
