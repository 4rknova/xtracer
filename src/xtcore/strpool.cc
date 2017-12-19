#define STRPOOL_IMPLEMENTATION
#include "log.h"
#include "strpool.h"
#define STRPOOL_U32 HASH_UINT32
#define STRPOOL_U64 HASH_UINT64
#include <strpool/strpool.h>

#define LOG_DEBUG_PREFIX "xtcore::pool::str::"

namespace xtcore {
    namespace pool {
        namespace str {

strpool_t pool; // Global string pool

void init()
{
    strpool_config_t conf = strpool_default_config;
    strpool_init(&pool, &conf);
}

void release()
{
    strpool_term(&pool);
}

HASH_UINT64 add(const char *str)
{
    size_t len = strlen(str);
    HASH_UINT64 id = strpool_inject(&pool, str, (int)len);
    strpool_incref(&pool, id);
    Log::handle().post_debug(LOG_DEBUG_PREFIX "add [%i] %s", strpool_getref(&pool, id), str);
    return id;
}

void del(HASH_UINT64 id)
{
    int count = strpool_decref(&pool, id);
    if (count == 0) {
        Log::handle().post_debug(LOG_DEBUG_PREFIX "del %s", strpool_cstr(&pool,id));
        strpool_discard(&pool, id);
    }
}

char const* get(HASH_UINT64 id)
{
    return strpool_cstr(&pool, id);
}

        } /* namespace str */
    } /* namespace pool */
} /* namespace xtcore */
