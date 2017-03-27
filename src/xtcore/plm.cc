#include <stdio.h>
#include <string>

#include <nplatform/dlloader.h>

#include "plm.h"


namespace PLM {

void list(const char *file)
{
/*
	std::string fpath = PLUGIN_PATH;
	fpath.append("/");
	fpath.append(file);

	printf ("Checking %s..\n", fpath.c_str());
	void *lib = load_library(fpath.c_str());

	if (lib == NULL) {
		printf("Not a plugin!\n");
		return;
	}

	printf("Checking plugin\n");

	free_library(lib);
*/
}

void load()
{
/*
	scan_directory(PLUGIN_PATH, list);
*/
}

} /* namespace PLM */
