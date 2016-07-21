#include "dlloader.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#if defined(__linux) || defined(__unix) || defined (__posix)
#include <dlfcn.h>
#include <dirent.h>
#elif defined (_WIN32)
#include <windows.h>
#else
#error Target platform is not supported
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int scan_directory(const char *path, void(*f)(const char*))
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
	    while ((dir = readdir(d)) != NULL) {
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

void* load_library(const char *dlpath)
{
#if defined(_MSC_VER)
	return (void*)LoadLibrary(dlpath);
#elif defined(__GNUC__)
	return dlopen(dlpath, RTLD_NOW);
#endif
}

void* load_function(void *lib, const char *fname)
{
#if defined(_MSC_VER)
	    return (void*)GetProcAddress((HINSTANCE)lib, fname);
#elif defined(__GNUC__)
	    return dlsym(lib, fname);
#endif
}

int free_library(void *lib)
{
#if defined(_MSC_VER)
		/* FreeLibrary will return non-zero value on success. */
	    return FreeLibrary((HINSTANCE)lib) != 0 ? 0 : 1;
#elif defined(__GNUC__)
		/* dlclose will return 0 on success. */
	    return dlclose(lib);
#endif
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
