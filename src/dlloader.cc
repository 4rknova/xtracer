#include "dlloader.h"
#include "plugin.h"

#include <stdio.h>
#include <stdint.h>

#if defined (__linux__)
#include <dlfcn.h>
#include <dirent.h>
#elif defined (_WIN32)
#include <windows.h>
#else
#error Target platform is not supported
#endif


void scan_directory(const char *path, void(*f)(const char*))
{
#if defined(_MSC_VER)
#error not yet implemented!
#elif defined(__GNUC__) // GNU Compiler
	DIR           *d;
	struct dirent *dir;
	d = opendir(path);
	if (d) {
	    while ((dir = readdir(d)) != NULL) {
			f(dir->d_name);
	    }
		closedir(d);
	}
#endif
}

void* load_library(const char *dlpath)
{
#if defined(_MSC_VER)
	return (void*)LoadLibrary(dlpath);
#elif defined(__GNUC__) // GNU compiler
	return dlopen(dlpath, RTLD_NOW);
#endif
}

void* load_function(void *lib, const char *fname)
{
#if defined(_MSC_VER)
	    return (void*)GetProcAddress((HINSTANCE)lib, fname);
#elif defined(__GNUC__) // GNU compiler
	    return dlsym(lib, fname);
#endif
}

int free_library(void *lib)
{
#if defined(_MSC_VER)
		// FreeLibrary will return non-zero value on success.
	    return FreeLibrary((HINSTANCE)lib) != 0 ? 0 : 1;
#elif defined(__GNUC__)
		// dlclose will return 0 on success.
	    return dlclose(lib);
#endif
}
