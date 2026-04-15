#ifndef XTRACER_FRONTEND_WEB_BACKEND_LOG_H_INCLUDED
#define XTRACER_FRONTEND_WEB_BACKEND_LOG_H_INCLUDED

#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <atomic>
#include <condition_variable>

namespace xtracer {
namespace frontend {
namespace web {

struct backend_log_entry_t
{
    unsigned long long id;
    std::string timestamp;
    std::string level;
    std::string message;
};

class backend_log_t
{
    public:
    static backend_log_t &handle();

    void add(const std::string &level, const std::string &message);
    std::vector<backend_log_entry_t> since(unsigned long long last_id) const;
    std::vector<backend_log_entry_t> wait_since(unsigned long long last_id, unsigned long timeout_ms) const;
    void set_push_callback(std::function<void(const backend_log_entry_t &)> cb);

    private:
    backend_log_t();

    mutable std::mutex mut;
    mutable std::condition_variable cv;
    std::deque<backend_log_entry_t> entries;
    std::atomic<unsigned long long> next_id;
    size_t max_entries;
    std::function<void(const backend_log_entry_t &)> push_callback_;
};

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_BACKEND_LOG_H_INCLUDED */
