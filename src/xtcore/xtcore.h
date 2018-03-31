#ifndef XT_XTCORE_H_INCLUDED
#define XT_XTCORE_H_INCLUDED

namespace xtcore {

const char *get_version();
const char *get_license();

int init();   // Initialize subsystems
int deinit(); // Release subsystems

} /* namespace xtcore */

#endif /* XT_XTCORE_H_INCLUDED */
