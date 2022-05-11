#ifndef NPLATFORM_FS_H_INCLUDED
#define NPLATFORM_FS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int np_scan_directory(const char* path, void(*f)(const char *));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NPLATFORM_FS_H_INCLUDED */

