#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <sys/stat.h>

#include "log.h"
#include "asset_fetcher.h"

namespace xtcore {
namespace asset_fetcher {

namespace {

#ifdef XTCORE_ASSET_FETCHER_CURL

uint64_t fnv1a(const std::string &s)
{
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

std::string cache_dir()
{
    const char *env = std::getenv("XTRACER_ASSET_CACHE");
    if (env && env[0]) return std::string(env);

    const char *home = std::getenv("HOME");
    if (home && home[0]) return std::string(home) + "/.cache/xtracer";

    return "/tmp/xtracer_cache";
}

bool mkdir_p(const std::string &dir)
{
    for (size_t i = 1; i <= dir.size(); ++i) {
        if (i == dir.size() || dir[i] == '/') {
            std::string sub = dir.substr(0, i);
            struct stat st;
            if (stat(sub.c_str(), &st) != 0) {
                if (::mkdir(sub.c_str(), 0755) != 0) return false;
            }
        }
    }
    return true;
}

std::string url_basename(const std::string &url)
{
    std::string s = url;
    auto q = s.find('?');
    if (q != std::string::npos) s = s.substr(0, q);
    auto pos = s.rfind('/');
    if (pos != std::string::npos && pos + 1 < s.size())
        return s.substr(pos + 1);
    return s;
}

// Shell-escape a string for use inside single quotes.
std::string shell_quote(const std::string &s)
{
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else           out += c;
    }
    out += "'";
    return out;
}


std::string download(const std::string &url)
{
    const std::string dir = cache_dir();
    if (!mkdir_p(dir)) {
        Log::handle().post_warning("asset_fetcher: could not create cache dir: %s", dir.c_str());
        return std::string();
    }

    std::ostringstream name;
    name << std::hex << fnv1a(url) << "_" << url_basename(url);
    const std::string dest     = dir + "/" + name.str();
    const std::string dest_tmp = dest + ".tmp";

    // Return cached file if it already exists.
    {
        struct stat st;
        if (stat(dest.c_str(), &st) == 0) {
            Log::handle().post_message("asset_fetcher: cache hit %s", dest.c_str());
            return dest;
        }
    }

    Log::handle().post_message("asset_fetcher: downloading %s", url.c_str());

    // curl -L  : follow redirects
    // curl -f  : fail (exit non-zero) on HTTP error responses
    // curl -s  : silent — no progress bar
    // curl -S  : but do show errors
    std::string cmd = "curl -LfsS -o " + shell_quote(dest_tmp) + " " + shell_quote(url);
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        Log::handle().post_warning("asset_fetcher: download failed (curl exit %d): %s", rc, url.c_str());
        std::remove(dest_tmp.c_str());
        return std::string();
    }

    if (std::rename(dest_tmp.c_str(), dest.c_str()) != 0) {
        Log::handle().post_warning("asset_fetcher: rename failed for %s", dest.c_str());
        std::remove(dest_tmp.c_str());
        return std::string();
    }

    Log::handle().post_message("asset_fetcher: cached as %s", dest.c_str());
    return dest;
}
#endif // XTCORE_ASSET_FETCHER_CURL

} // anonymous namespace

bool is_url(const std::string &path)
{
    return path.size() > 7
        && (path.substr(0, 7) == "http://" || path.substr(0, 8) == "https://");
}

std::string resolve(const std::string &path)
{
    if (!is_url(path)) return path;
#ifdef XTCORE_ASSET_FETCHER_CURL
    return download(path);  // empty string on failure
#else
    Log::handle().post_warning("asset_fetcher: remote URLs not supported (build with -DXTRACER_ENABLE_REMOTE_ASSETS=ON): %s", path.c_str());
    return std::string();
#endif
}

} // namespace asset_fetcher
} // namespace xtcore
