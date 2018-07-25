#ifndef XTCORE_BROADCAST_H_INCLUDED
#define XTCORE_BROADCAST_H_INCLUDED

namespace xtcore {
    namespace network {

int broadcast();
int listen(bool *done);
int wget(const char *url, const char *path);

    } /* namespace network */
} /* namespace xtcore */

#endif /* XTCORE_BROADCAST_H_INCLUDED */
