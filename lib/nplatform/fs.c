#include <string.h>

#if defined(__linux) || defined(__unix) || defined (__posix)
#include <dirent.h>
#elif defined (_WIN32)
#include <windows.h>
#else
#error Target platform is not supported
#endif

#include "fs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int np_scan_directory(const char *path, void(*f)(const char*))
{
#if defined(_MSC_VER)
	WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

	if((hFind = FindFirstFile(sPath, &fdFile)) != INVALID_HANDLE_VALUE) {
		do {
	        if (   strcmp(fdFile.cFileName, "." ) != 0
				&& strcmp(fdFile.cFileName, "..") != 0) {
				f(sPath);
        	}
    	}
	    while (FindNextFile(hFind, &fdFile));
	    FindClose(hFind);
		return 0;
	}

#elif defined(__GNUC__)
	DIR           *d;
	struct dirent *dir;
	d = opendir(path);
	if (d) {
	    while ((dir = readdir(d)) != 0) {
	        if (   strcmp(dir->d_name, "." ) != 0
				&& strcmp(dir->d_name, "..") != 0) {
				f(dir->d_name);
			}
	    }
		closedir(d);
		return 0;
	}
#endif

	return 1;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
