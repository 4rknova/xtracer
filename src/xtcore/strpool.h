#ifndef XT_STRPOOL_H_INCLUDED
#define XT_STRPOOL_H_INCLUDED

#define HASH_ID_INVALID (0)

#include <stdint.h>
#define HASH_UINT32 uint32_t
#define HASH_UINT64 uint64_t

#define HASH_ID HASH_UINT64

namespace xtcore {
    namespace pool {
        namespace str {

void init();
void release();

HASH_ID     add(const char *str); // Add new string
void        del(HASH_ID id);  // Remove existing string
char const* get(HASH_ID id);  // Get string from hash

        } /* namespace str */
    } /* namespace pool */
} /* namespace xtcore */

#endif /* XT_STRPOOL_H_INCLUDED */
