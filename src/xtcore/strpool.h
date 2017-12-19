#ifndef XT_STRPOOL_H_INCLUDED
#define XT_STRPOOL_H_INCLUDED

#include <stdint.h>
#define HASH_UINT32 uint32_t
#define HASH_UINT64 uint64_t

namespace xtcore {
    namespace pool {
        namespace str {

void init();
void release();

HASH_UINT64 add(const char *str); // Add new string
void        del(HASH_UINT64 id);  // Remove existing string
char const* get(HASH_UINT64 id);  // Get string from hash

        } /* namespace str */
    } /* namespace pool */
} /* namespace xtcore */

#endif /* XT_STRPOOL_H_INCLUDED */
