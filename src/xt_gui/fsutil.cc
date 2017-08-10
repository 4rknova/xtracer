#include <algorithm>
#define TINYFILES_IMPL
#include <tinyfiles/tinyfiles.h>
#include "fsutil.h"

namespace util {
    namespace filesystem {

int ls(fsvec &fsv, const char *path)
{
    fsv.clear();

    tfDIR dir;
    int is_valid = tfDirOpen(&dir, path);

    if (!is_valid) return 0;

    while (dir.has_next) {
        tfFILE file;
        tfReadFile(&dir, &file);
        fs_entry_t entry;
        fsv.push_back(entry);
        fs_entry_t *p = &(fsv.back());
        p->name = file.name;
        p->path = file.path;
        p->type = (file.is_dir ? FS_ENTRY_TYPE_DIRECTORY
                               : FS_ENTRY_TYPE_FILE);
        tfDirNext(&dir);
    }
    tfDirClose(&dir);

    std::sort(fsv.begin(), fsv.end(),
        [](const fs_entry_t &a, const fs_entry_t &b) -> bool {
            return a.type > b.type;
        }
    );

    return 1;
}

    } /* namespace filesystem */
} /* namespace util */
