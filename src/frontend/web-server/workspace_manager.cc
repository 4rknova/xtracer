#include "workspace_manager.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace xtracer {
namespace frontend {
namespace web {

namespace {

const long long k_client_ttl_ms = 5LL * 60LL * 1000LL;

std::string trimmed_or_default_name(const std::string &name, const std::string &fallback)
{
    std::string out;
    out.reserve(name.size());
    for (size_t i = 0; i < name.size(); ++i) {
        const char c = name[i];
        if (c == '\r' || c == '\n' || c == '\t') continue;
        out.push_back(c);
    }
    while (!out.empty() && out[0] == ' ') out.erase(out.begin());
    while (!out.empty() && out[out.size() - 1] == ' ') out.erase(out.end() - 1);
    if (out.empty()) return fallback;
    if (out.size() > 64) out.resize(64);
    return out;
}

} // namespace

workspace_manager_t::workspace_manager_t()
    : mut_()
    , workspaces_()
    , clients_()
    , next_id_(0)
{
    create_locked("Workspace 1");
}

std::string workspace_manager_t::create_locked(const std::string &name)
{
    const unsigned long long id = ++next_id_;
    std::ostringstream ss;
    ss << "ws_" << id;

    workspace_t ws;
    ws.id = ss.str();
    ws.name = trimmed_or_default_name(name, "Workspace " + std::to_string(id));
    ws.active_scene.clear();
    ws.active_job_id.clear();
    ws.last_job_id.clear();
    ws.scene_drafts.clear();
    ws.quality_samples = 1;
    ws.quality_aa = 1;
    ws.quality_sample_distribution = "grid";
    ws.quality_rdepth = 10;
    ws.settings_json = "{}";
    ws.updated_ms = now_ms();

    workspaces_[ws.id] = ws;
    return ws.id;
}

bool workspace_manager_t::exists_locked(const std::string &workspace_id) const
{
    return workspaces_.find(workspace_id) != workspaces_.end();
}

long long workspace_manager_t::now_ms() const
{
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return (long long)ms.count();
}

void workspace_manager_t::touch_client_locked(const std::string &client_id)
{
    if (client_id.empty()) return;
    auto it = clients_.find(client_id);
    if (it == clients_.end()) {
        client_t c;
        c.last_seen_ms = now_ms();
        c.workspace_id = workspaces_.empty() ? create_locked("Workspace 1") : workspaces_.begin()->first;
        clients_[client_id] = c;
        return;
    }
    it->second.last_seen_ms = now_ms();
    if (!exists_locked(it->second.workspace_id)) {
        it->second.workspace_id = workspaces_.empty() ? create_locked("Workspace 1") : workspaces_.begin()->first;
    }
}

void workspace_manager_t::prune_clients_locked(long long now)
{
    for (auto it = clients_.begin(); it != clients_.end();) {
        if (now - it->second.last_seen_ms > k_client_ttl_ms) {
            it = clients_.erase(it);
            continue;
        }
        ++it;
    }
}

std::string workspace_manager_t::ensure_client(const std::string &client_id)
{
    std::lock_guard<std::mutex> lock(mut_);
    if (workspaces_.empty()) create_locked("Workspace 1");
    touch_client_locked(client_id);
    if (client_id.empty()) return workspaces_.begin()->first;
    return clients_[client_id].workspace_id;
}

std::string workspace_manager_t::create(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mut_);
    return create_locked(name);
}

workspace_manager_t::remove_result_t workspace_manager_t::remove(const std::string &workspace_id,
                                                                 std::string &replacement_workspace_id_out)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return REMOVE_NOT_FOUND;
    if (workspaces_.size() <= 1) return REMOVE_LAST_WORKSPACE;

    workspaces_.erase(it);
    replacement_workspace_id_out = workspaces_.begin()->first;

    for (auto cit = clients_.begin(); cit != clients_.end(); ++cit) {
        if (cit->second.workspace_id == workspace_id) {
            cit->second.workspace_id = replacement_workspace_id_out;
        }
    }

    return REMOVE_OK;
}

bool workspace_manager_t::set_active(const std::string &client_id, const std::string &workspace_id)
{
    std::lock_guard<std::mutex> lock(mut_);
    if (workspace_id.empty() || !exists_locked(workspace_id)) return false;
    if (client_id.empty()) return false;
    touch_client_locked(client_id);
    clients_[client_id].workspace_id = workspace_id;
    return true;
}

bool workspace_manager_t::get_active(const std::string &client_id, std::string &workspace_id_out)
{
    std::lock_guard<std::mutex> lock(mut_);
    if (workspaces_.empty()) create_locked("Workspace 1");
    if (client_id.empty()) {
        workspace_id_out = workspaces_.begin()->first;
        return true;
    }
    touch_client_locked(client_id);
    workspace_id_out = clients_[client_id].workspace_id;
    return true;
}

bool workspace_manager_t::list(const std::string &client_id,
                               std::vector<workspace_snapshot_t> &out,
                               std::string &active_workspace_out)
{
    std::lock_guard<std::mutex> lock(mut_);
    if (workspaces_.empty()) create_locked("Workspace 1");

    if (!client_id.empty()) touch_client_locked(client_id);
    prune_clients_locked(now_ms());

    if (!client_id.empty()) active_workspace_out = clients_[client_id].workspace_id;
    else active_workspace_out = workspaces_.begin()->first;

    std::map<std::string, size_t> client_counts;
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        if (!it->second.workspace_id.empty()) client_counts[it->second.workspace_id] += 1;
    }

    out.clear();
    for (auto it = workspaces_.begin(); it != workspaces_.end(); ++it) {
        const workspace_t &ws = it->second;
        workspace_snapshot_t snap;
        snap.id = ws.id;
        snap.name = ws.name;
        snap.active_scene = ws.active_scene;
        snap.active_job_id = ws.active_job_id;
        snap.last_job_id = ws.last_job_id;
        snap.draft_count = ws.scene_drafts.size();
        snap.client_count = client_counts[ws.id];
        snap.quality_samples = ws.quality_samples;
        snap.quality_aa = ws.quality_aa;
        snap.quality_sample_distribution = ws.quality_sample_distribution;
        snap.quality_rdepth = ws.quality_rdepth;
        snap.settings_json = ws.settings_json;
        snap.updated_ms = ws.updated_ms;
        out.push_back(snap);
    }

    std::sort(out.begin(), out.end(), [](const workspace_snapshot_t &a, const workspace_snapshot_t &b) {
        if (a.updated_ms == b.updated_ms) return a.id < b.id;
        return a.updated_ms > b.updated_ms;
    });

    return true;
}

bool workspace_manager_t::set_scene_draft(const std::string &workspace_id,
                                          const std::string &scene_name,
                                          const std::string &source)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return false;
    if (scene_name.empty()) return false;
    it->second.scene_drafts[scene_name] = source;
    it->second.active_scene = scene_name;
    it->second.updated_ms = now_ms();
    return true;
}

bool workspace_manager_t::get_scene_draft(const std::string &workspace_id,
                                          const std::string &scene_name,
                                          std::string &source_out)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return false;
    auto dit = it->second.scene_drafts.find(scene_name);
    if (dit == it->second.scene_drafts.end()) return false;
    source_out = dit->second;
    return true;
}

bool workspace_manager_t::set_quality_settings(const std::string &workspace_id,
                                               size_t samples,
                                               size_t aa,
                                               const std::string &sample_distribution,
                                               size_t rdepth)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return false;
    it->second.quality_samples = samples;
    it->second.quality_aa = aa;
    it->second.quality_sample_distribution = sample_distribution;
    it->second.quality_rdepth = rdepth;
    it->second.updated_ms = now_ms();
    return true;
}

bool workspace_manager_t::get_quality_settings(const std::string &workspace_id,
                                               size_t &samples_out,
                                               size_t &aa_out,
                                               std::string &sample_distribution_out,
                                               size_t &rdepth_out)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return false;
    samples_out = it->second.quality_samples;
    aa_out = it->second.quality_aa;
    sample_distribution_out = it->second.quality_sample_distribution;
    rdepth_out = it->second.quality_rdepth;
    return true;
}

bool workspace_manager_t::set_settings_json(const std::string &workspace_id, const std::string &settings_json)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return false;
    it->second.settings_json = settings_json.empty() ? "{}" : settings_json;
    it->second.updated_ms = now_ms();
    return true;
}

bool workspace_manager_t::get_settings_json(const std::string &workspace_id, std::string &settings_json_out)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return false;
    settings_json_out = it->second.settings_json;
    return true;
}

void workspace_manager_t::set_active_scene(const std::string &workspace_id, const std::string &scene_name)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return;
    it->second.active_scene = scene_name;
    it->second.updated_ms = now_ms();
}

void workspace_manager_t::mark_job_started(const std::string &workspace_id, const std::string &job_id)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return;
    it->second.active_job_id = job_id;
    it->second.last_job_id = job_id;
    it->second.updated_ms = now_ms();
}

void workspace_manager_t::mark_job_finished(const std::string &workspace_id, const std::string &job_id)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return;
    if (it->second.active_job_id == job_id) it->second.active_job_id.clear();
    if (!job_id.empty()) it->second.last_job_id = job_id;
    it->second.updated_ms = now_ms();
}

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */
