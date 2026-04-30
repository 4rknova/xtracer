#ifndef XTRACER_FRONTEND_WEB_WS_HUB_H_INCLUDED
#define XTRACER_FRONTEND_WEB_WS_HUB_H_INCLUDED

#include <map>
#include <mutex>
#include <set>
#include <string>

#include <crow_all.h>

namespace xtracer {
namespace frontend {
namespace web {

// ---------------------------------------------------------------------------
// job_ws_hub_t
//
// Tracks WebSocket connections subscribed to a specific render job.
// Render threads call broadcast_text / broadcast_binary from arbitrary
// threads; a mutex serialises all sends.
// ---------------------------------------------------------------------------
class job_ws_hub_t
{
public:
    void subscribe(const std::string &job_id, crow::websocket::connection *conn)
    {
        std::lock_guard<std::mutex> lock(mut_);
        subs_[job_id].insert(conn);
        conn_to_job_[conn] = job_id;
    }

    // Remove conn from whichever job it was subscribed to.
    void unsubscribe(crow::websocket::connection *conn)
    {
        std::lock_guard<std::mutex> lock(mut_);
        auto it = conn_to_job_.find(conn);
        if (it == conn_to_job_.end()) return;
        const std::string &job_id = it->second;
        auto sit = subs_.find(job_id);
        if (sit != subs_.end()) {
            sit->second.erase(conn);
            if (sit->second.empty()) subs_.erase(sit);
        }
        conn_to_job_.erase(it);
    }

    void broadcast_text(const std::string &job_id, const std::string &msg)
    {
        std::lock_guard<std::mutex> lock(mut_);
        auto it = subs_.find(job_id);
        if (it == subs_.end()) return;
        for (crow::websocket::connection *conn : it->second) {
            try { conn->send_text(msg); } catch (...) {}
        }
    }

    void broadcast_binary(const std::string &job_id, const std::string &data)
    {
        std::lock_guard<std::mutex> lock(mut_);
        auto it = subs_.find(job_id);
        if (it == subs_.end()) return;
        for (crow::websocket::connection *conn : it->second) {
            try { conn->send_binary(data); } catch (...) {}
        }
    }

private:
    std::mutex mut_;
    std::map<std::string, std::set<crow::websocket::connection *>> subs_;
    std::map<crow::websocket::connection *, std::string> conn_to_job_;
};

// ---------------------------------------------------------------------------
// log_ws_hub_t
//
// Tracks WebSocket connections subscribed to the backend log stream.
// ---------------------------------------------------------------------------
class log_ws_hub_t
{
public:
    void subscribe(crow::websocket::connection *conn)
    {
        std::lock_guard<std::mutex> lock(mut_);
        subs_.insert(conn);
    }

    void unsubscribe(crow::websocket::connection *conn)
    {
        std::lock_guard<std::mutex> lock(mut_);
        subs_.erase(conn);
    }

    void broadcast_text(const std::string &msg)
    {
        std::lock_guard<std::mutex> lock(mut_);
        for (crow::websocket::connection *conn : subs_) {
            try { conn->send_text(msg); } catch (...) {}
        }
    }

private:
    std::mutex mut_;
    std::set<crow::websocket::connection *> subs_;
};

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_WS_HUB_H_INCLUDED */
