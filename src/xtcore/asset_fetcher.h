#ifndef XTCORE_ASSET_FETCHER_H_INCLUDED
#define XTCORE_ASSET_FETCHER_H_INCLUDED

#include <string>

namespace xtcore {
namespace asset_fetcher {

bool        is_url(const std::string &path);

// If path begins with http:// or https://, downloads the file to the local
// cache and returns the cached file path. Otherwise returns path unchanged.
// Returns an empty string on download failure.
std::string resolve(const std::string &path);

} // namespace asset_fetcher
} // namespace xtcore

#endif // XTCORE_ASSET_FETCHER_H_INCLUDED
