#ifndef XTRACER_DLLOADER_H_INCLUDED
#define XTRACER_DLLOADER_H_INCLUDED

void  scan_directory (const char *path, void(*f)(const char*));

void* load_library   (const char *dlpath);
void* load_function  (void *lib, const char *fname);
int   free_library   (void *lib);

#endif /* XTRACER_DLLOADER_H_INCLUDED */
