#include "workspace_manager.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace xtracer {
namespace frontend {
namespace web {

namespace {

const long long k_client_ttl_ms = 5LL * 60LL * 1000LL;
const long long k_workspace_idle_ttl_ms = 60LL * 60LL * 1000LL;
const size_t k_max_workspaces = 32;
const size_t k_max_scene_drafts_per_workspace = 16;
const size_t k_max_scene_draft_bytes = 1024 * 1024;
const size_t k_max_settings_json_bytes = 64 * 1024;

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
}

std::string workspace_manager_t::create_locked(const std::string &name,
                                               const std::string &owner_client_id)
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
    ws.scene_draft_order.clear();
    ws.quality_samples = 1;
    ws.quality_aa = 1;
    ws.quality_sample_distribution = "grid";
    ws.quality_rdepth = 15;
    ws.settings_json = "{}";
    ws.owner_client_id = owner_client_id;
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
        c.workspace_id = workspaces_.empty() ? "" : workspaces_.begin()->first;
        auto wit = workspaces_.find(c.workspace_id);
        if (wit != workspaces_.end() && wit->second.owner_client_id.empty()) {
            wit->second.owner_client_id = client_id;
            wit->second.updated_ms = now_ms();
        }
        clients_[client_id] = c;
        return;
    }
    it->second.last_seen_ms = now_ms();
    if (!exists_locked(it->second.workspace_id)) {
        it->second.workspace_id = workspaces_.empty() ? "" : workspaces_.begin()->first;
    }
    auto wit = workspaces_.find(it->second.workspace_id);
    if (wit != workspaces_.end() && wit->second.owner_client_id.empty()) {
        wit->second.owner_client_id = client_id;
        wit->second.updated_ms = now_ms();
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

void workspace_manager_t::clear_stale_workspace_owners_locked()
{
    for (auto it = workspaces_.begin(); it != workspaces_.end(); ++it) {
        if (!it->second.owner_client_id.empty()
            && clients_.find(it->second.owner_client_id) == clients_.end()) {
            it->second.owner_client_id.clear();
        }
    }
}

void workspace_manager_t::prune_workspaces_locked(long long now)
{
    clear_stale_workspace_owners_locked();
    if (workspaces_.empty()) return;

    std::map<std::string, size_t> client_counts;
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        if (!it->second.workspace_id.empty()) client_counts[it->second.workspace_id] += 1;
    }

    std::vector<std::pair<long long, std::string> > ttl_candidates;
    std::vector<std::pair<long long, std::string> > overflow_candidates;
    for (auto it = workspaces_.begin(); it != workspaces_.end(); ++it) {
        const workspace_t &ws = it->second;
        if (client_counts[ws.id] > 0) continue;
        if (!ws.active_job_id.empty()) continue;
        overflow_candidates.push_back(std::make_pair(ws.updated_ms, ws.id));
        if (now - ws.updated_ms > k_workspace_idle_ttl_ms) {
            ttl_candidates.push_back(std::make_pair(ws.updated_ms, ws.id));
        }
    }

    const auto cmp = [](const std::pair<long long, std::string> &a,
                        const std::pair<long long, std::string> &b) {
        if (a.first == b.first) return a.second < b.second;
        return a.first < b.first;
    };
    std::sort(ttl_candidates.begin(), ttl_candidates.end(), cmp);
    std::sort(overflow_candidates.begin(), overflow_candidates.end(), cmp);

    for (size_t i = 0; i < ttl_candidates.size(); ++i) {
        workspaces_.erase(ttl_candidates[i].second);
    }

    for (size_t i = 0; i < overflow_candidates.size() && workspaces_.size() > k_max_workspaces; ++i) {
        workspaces_.erase(overflow_candidates[i].second);
    }
}

std::string workspace_manager_t::ensure_client(const std::string &client_id)
{
    std::lock_guard<std::mutex> lock(mut_);
    const long long now = now_ms();
    prune_clients_locked(now);
    prune_workspaces_locked(now);
    touch_client_locked(client_id);
    if (client_id.empty()) return workspaces_.empty() ? "" : workspaces_.begin()->first;
    return clients_[client_id].workspace_id;
}

std::string workspace_manager_t::create(const std::string &name, const std::string &owner_client_id)
{
    std::lock_guard<std::mutex> lock(mut_);
    const long long now = now_ms();
    prune_clients_locked(now);
    prune_workspaces_locked(now);
    if (workspaces_.size() >= k_max_workspaces) return "";
    return create_locked(name, owner_client_id);
}

workspace_manager_t::remove_result_t workspace_manager_t::remove(const std::string &workspace_id,
                                                                 std::string &replacement_workspace_id_out)
{
    std::lock_guard<std::mutex> lock(mut_);
    const long long now = now_ms();
    prune_clients_locked(now);
    prune_workspaces_locked(now);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return REMOVE_NOT_FOUND;

    workspaces_.erase(it);
    replacement_workspace_id_out = workspaces_.empty() ? "" : workspaces_.begin()->first;

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
    const long long now = now_ms();
    prune_clients_locked(now);
    prune_workspaces_locked(now);
    if (!exists_locked(workspace_id)) return false;
    touch_client_locked(client_id);
    clients_[client_id].workspace_id = workspace_id;
    auto wit = workspaces_.find(workspace_id);
    if (wit != workspaces_.end() && wit->second.owner_client_id.empty()) {
        wit->second.owner_client_id = client_id;
        wit->second.updated_ms = now_ms();
    }
    return true;
}

bool workspace_manager_t::get_active(const std::string &client_id, std::string &workspace_id_out)
{
    std::lock_guard<std::mutex> lock(mut_);
    const long long now = now_ms();
    prune_clients_locked(now);
    prune_workspaces_locked(now);
    if (client_id.empty()) {
        workspace_id_out = workspaces_.empty() ? "" : workspaces_.begin()->first;
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
    const long long now = now_ms();
    prune_clients_locked(now);
    prune_workspaces_locked(now);
    if (!client_id.empty()) touch_client_locked(client_id);

    if (!client_id.empty()) active_workspace_out = clients_[client_id].workspace_id;
    else active_workspace_out = workspaces_.empty() ? "" : workspaces_.begin()->first;

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
        snap.is_owned_by_client = !client_id.empty() && (ws.owner_client_id == client_id);
        snap.updated_ms = ws.updated_ms;
        out.push_back(snap);
    }

    std::sort(out.begin(), out.end(), [](const workspace_snapshot_t &a, const workspace_snapshot_t &b) {
        if (a.updated_ms == b.updated_ms) return a.id < b.id;
        return a.updated_ms > b.updated_ms;
    });

    return true;
}

workspace_manager_t::store_result_t workspace_manager_t::set_scene_draft(const std::string &workspace_id,
                                                                         const std::string &scene_name,
                                                                         const std::string &source)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return STORE_NOT_FOUND;
    if (scene_name.empty()) return STORE_NOT_FOUND;
    if (source.size() > k_max_scene_draft_bytes) return STORE_TOO_LARGE;

    workspace_t &ws = it->second;
    ws.scene_drafts[scene_name] = source;
    for (std::deque<std::string>::iterator dit = ws.scene_draft_order.begin();
         dit != ws.scene_draft_order.end(); ++dit) {
        if (*dit == scene_name) {
            ws.scene_draft_order.erase(dit);
            break;
        }
    }
    ws.scene_draft_order.push_back(scene_name);
    while (ws.scene_draft_order.size() > k_max_scene_drafts_per_workspace) {
        const std::string evict_scene = ws.scene_draft_order.front();
        ws.scene_draft_order.pop_front();
        ws.scene_drafts.erase(evict_scene);
    }
    ws.active_scene = scene_name;
    ws.updated_ms = now_ms();
    return STORE_OK;
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

workspace_manager_t::store_result_t workspace_manager_t::set_settings_json(const std::string &workspace_id,
                                                                           const std::string &settings_json)
{
    std::lock_guard<std::mutex> lock(mut_);
    auto it = workspaces_.find(workspace_id);
    if (it == workspaces_.end()) return STORE_NOT_FOUND;
    if (settings_json.size() > k_max_settings_json_bytes) return STORE_TOO_LARGE;
    it->second.settings_json = settings_json.empty() ? "{}" : settings_json;
    it->second.updated_ms = now_ms();
    return STORE_OK;
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
