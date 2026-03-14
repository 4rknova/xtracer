#ifndef XTRACER_FRONTEND_WEB_ROUTES_H_INCLUDED
#define XTRACER_FRONTEND_WEB_ROUTES_H_INCLUDED

#include <string>

namespace httplib {
class Server;
}

namespace xtracer {
namespace frontend {
namespace web {

class job_manager_t;
class workspace_manager_t;

void setup_routes(httplib::Server &server,
                  job_manager_t &jobs,
                  workspace_manager_t &workspaces,
                  const std::string &scene_dir,
                  const std::string &web_root);

} /* namespace web */
} /* namespace frontend */
} /* namespace xtracer */

#endif /* XTRACER_FRONTEND_WEB_ROUTES_H_INCLUDED */
