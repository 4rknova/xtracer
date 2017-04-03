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
