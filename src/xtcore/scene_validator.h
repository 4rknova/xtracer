#ifndef XTCORE_SCENE_VALIDATOR_H_INCLUDED
#define XTCORE_SCENE_VALIDATOR_H_INCLUDED

#include <string>
#include <vector>

namespace xtcore {
namespace io {
namespace scn {

int validate(const char *filename, std::vector<std::string> *errors);

} /* namespace scn */
} /* namespace io */
} /* namespace xtcore */

#endif /* XTCORE_SCENE_VALIDATOR_H_INCLUDED */
