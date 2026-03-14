#ifndef XTRACER_FRONTEND_WEB_WORKSPACE_MANAGER_H_INCLUDED
#define XTRACER_FRONTEND_WEB_WORKSPACE_MANAGER_H_INCLUDED

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace xtracer {
namespace frontend {
namespace web {

struct workspace_snapshot_t
{
    std::string id;
    std::string name;
    std::string active_scene;
    std::string active_job_id;
    std::string last_job_id;
    size_t draft_count;
    size_t client_count;
    size_t quality_samples;
    size_t quality_aa;
    std::string quality_sample_distribution;
    size_t quality_rdepth;
    std::string settings_json;
    long long updated_ms;
};

class workspace_manager_t
{
    public:
    enum remove_result_t {
        REMOVE_OK = 0,
        REMOVE_NOT_FOUND,
        REMOVE_LAST_WORKSPACE
    };

    workspace_manager_t();

    std::string ensure_client(const std::string &client_id);
    std::string create(const std::string &name);
    remove_result_t remove(const std::string &workspace_id, std::string &replacement_workspace_id_out);
    bool set_active(const std::string &client_id, const std::string &workspace_id);
    bool get_active(const std::string &client_id, std::string &workspace_id_out);
    bool list(const std::string &client_id, std::vector<workspace_snapshot_t> &out, std::string &active_workspace_out);

    bool set_scene_draft(const std::string &workspace_id,
                         const std::string &scene_name,
                         const std::string &source);
    bool get_scene_draft(const std::string &workspace_id,
                         const std::string &scene_name,
                         std::string &source_out);
    bool set_quality_settings(const std::string &workspace_id,
                              size_t samples,
                              size_t aa,
                              const std::string &sample_distribution,
                              size_t rdepth);
    bool get_quality_settings(const std::string &workspace_id,
                              size_t &samples_out,
                              size_t &aa_out,
                              std::string &sample_distribution_out,
                              size_t &rdepth_out);
    bool set_settings_json(const std::string &workspace_id, const std::string &settings_json);
    bool get_settings_json(const std::string &workspace_id, std::string &settings_json_out);

    void set_active_scene(const std::string &workspace_id, const std::string &scene_name);
    void mark_job_started(const std::string &workspace_id, const std::string &job_id);
    void mark_job_finished(const std::string &workspace_id, const std::string &job_id);

    private:
    struct workspace_t {
        std::string id;
        std::string name;
        std::string active_scene;
        std::string active_job_id;
        std::string last_job_id;
        std::map<std::string, std::string> scene_drafts;
        size_t quality_samples;
        size_t quality_aa;
        std::string quality_sample_distribution;
        size_t quality_rdepth;
        std::string settings_json;
        long long updated_ms;
    };

    struct client_t {
        std::string workspace_id;
        long long last_seen_ms;
    };

    std::string create_locked(const std::string &name);
    bool exists_locked(const std::string &workspace_id) const;
    void touch_client_locked(const std::string &client_id);
    void prune_clients_locked(long long now_ms);
    long long now_ms() const;

    mutable std::mutex mut_;
    std::map<std::string, workspace_t> workspaces_;
    std::map<std::string, client_t> clients_;
    unsigned long long next_id_;
};

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_WORKSPACE_MANAGER_H_INCLUDED */
