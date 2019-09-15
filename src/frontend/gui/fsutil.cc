#include <cstring>
#include <algorithm>

#define TINYFILES_IMPLEMENTATION
#include <tinyfiles/tinyfiles.h>
#include "fsutil.h"

namespace util {
    namespace filesystem {

void decompose_path(const char *path, std::string &base, std::string &file, const char delim)
{
    std::string s = path; 
	size_t pos = s.find_last_of(delim)+1;
	file = s.substr(pos, std::string::npos);
	base = s.substr(0, pos);
}

int ls(fsvec &fsv, const char *path)
{
    fsv.clear();
    
    tfDIR dir;
    int is_valid = tfDirOpen(&dir, path);

    std::string path_dir, path_file;
    if (!is_valid) {
        decompose_path(path, path_dir, path_file);
        is_valid = tfDirOpen(&dir, path_dir.c_str());
    }

    if (!is_valid) return 0;

    while (dir.has_next) {

        tfFILE file;
        tfReadFile(&dir, &file);

        std::string fsname = file.name;
        if (fsname.rfind(path_file, 0) != 0 && fsname.compare("..") != 0) {
            tfDirNext(&dir);
            continue;
        }

        fs_entry_t entry;
        fsv.push_back(entry);
        fs_entry_t *p = &(fsv.back());
        p->name = file.name;
        p->path = file.path;

        FS_ENTRY_TYPE type = FS_ENTRY_TYPE_UNKNOWN;

        if (file.is_dir) {
                 if (p->name.compare("." ) == 0) type = FS_ENTRY_TYPE_SPECIAL_DIR_CURRENT;
            else if (p->name.compare("..") == 0) type = FS_ENTRY_TYPE_SPECIAL_DIR_PARENT;
            else                                 type = FS_ENTRY_TYPE_DIRECTORY;
            p->path += PATH_DELIMITER;
        printf("%s #  %s \n", p->path.c_str(), p->name.c_str());
        }
        else type = FS_ENTRY_TYPE_FILE;

        p->type = type;

        tfDirNext(&dir);
    }
    tfDirClose(&dir);

    std::sort(fsv.begin(), fsv.end(),
        [](const fs_entry_t &a, const fs_entry_t &b) -> bool {
            if (a.type != b.type) return a.type < b.type;
            return a.name < b.name;
        }
    );

    return 1;
}

bool file_exists(const char *filepath)
{
    bool valid_path = (tfFileExists(filepath) == 1);
    if (!valid_path) return false;

    tfDIR d;
    bool valid_dir = tfDirOpen(&d, filepath);
    if (valid_dir) tfDirClose(&d);
    return valid_path && !valid_dir;
}

bool has_extension(fs_entry_t *entry, const char *extension)
{
    if (!entry || !extension) return false;

    std::string src = entry->name;
    size_t      idx = src.find_last_of('.');

    if (  (idx == std::string::npos)
       || (src.length() - (++idx)) != strlen(extension)
    ) return false;

    std::string ext = src.substr(idx);
    std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

    return ext.compare(extension) == 0;
}

    } /* namespace filesystem */
} /* namespace util */
