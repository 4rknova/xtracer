#define STRPOOL_IMPLEMENTATION
#include "log.h"
#include "strpool.h"
#define STRPOOL_U32 HASH_UINT32
#define STRPOOL_U64 HASH_UINT64
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
#include <strpool/strpool.h>
#pragma GCC diagnostic pop
#include <mutex>
#include <string>

#define LOG_DEBUG_PREFIX "xtcore::pool::str::"

namespace xtcore {
    namespace pool {
        namespace str {

strpool_t pool; // Global string pool
std::mutex pool_mut;

void init()
{
    std::lock_guard<std::mutex> lock(pool_mut);
    strpool_config_t conf = strpool_default_config;
    strpool_init(&pool, &conf);
}

void release()
{
    std::lock_guard<std::mutex> lock(pool_mut);
    strpool_term(&pool);
}

HASH_ID add(const char *str)
{
    std::lock_guard<std::mutex> lock(pool_mut);
    size_t len = strlen(str);
    HASH_ID id = strpool_inject(&pool, str, (int)len);
    strpool_incref(&pool, id);
    return id;
}

void del(HASH_ID id)
{
    std::lock_guard<std::mutex> lock(pool_mut);
    int count = strpool_decref(&pool, id);
    if (count == 0) {
        strpool_discard(&pool, id);
    }
}

char const* get(HASH_ID id)
{
    std::lock_guard<std::mutex> lock(pool_mut);
    const char *raw = strpool_cstr(&pool, id);
    if (!raw) return nullptr;
    static thread_local std::string tls_value;
    tls_value = raw;
    return tls_value.c_str();
}

        } /* namespace str */
    } /* namespace pool */
} /* namespace xtcore */
