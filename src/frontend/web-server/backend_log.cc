#include "backend_log.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace xtracer {
namespace frontend {
namespace web {

namespace {

std::string now_iso8601()
{
    std::time_t t = std::time(nullptr);
    std::tm tm_now;
#if defined(_WIN32)
    gmtime_s(&tm_now, &t);
#else
    gmtime_r(&t, &tm_now);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm_now, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

} // namespace

backend_log_t::backend_log_t()
    : mut()
    , entries()
    , next_id(0)
    , max_entries(2000)
{}

backend_log_t &backend_log_t::handle()
{
    static backend_log_t log;
    return log;
}

void backend_log_t::add(const std::string &level, const std::string &message)
{
    backend_log_entry_t e;
    e.id = ++next_id;
    e.timestamp = now_iso8601();
    e.level = level;
    e.message = message;

    {
        std::lock_guard<std::mutex> lock(mut);
        entries.push_back(e);
        while (entries.size() > max_entries) {
            entries.pop_front();
        }
    }
    cv.notify_all();
}

std::vector<backend_log_entry_t> backend_log_t::since(unsigned long long last_id) const
{
    std::vector<backend_log_entry_t> out;
    std::lock_guard<std::mutex> lock(mut);
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].id > last_id) out.push_back(entries[i]);
    }
    return out;
}

std::vector<backend_log_entry_t> backend_log_t::wait_since(unsigned long long last_id, unsigned long timeout_ms) const
{
    std::unique_lock<std::mutex> lock(mut);

    auto has_new_entries = [this, last_id]() {
        if (entries.empty()) return false;
        return entries.back().id > last_id;
    };

    if (!has_new_entries()) {
        cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), has_new_entries);
    }

    std::vector<backend_log_entry_t> out;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].id > last_id) out.push_back(entries[i]);
    }
    return out;
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
