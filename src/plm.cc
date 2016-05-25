#include <stdio.h>

#include "plm.h"
#include "dlloader.h"


namespace PLM {

void list(const char *file)
{
	printf ("Checking %s..\n", file);
	void *lib = load_library(file);

	if (lib == NULL) {
		printf("Not a plugin!\n");
		return;
	}

	printf("Checking plugin\n");

	free_library(lib);
}

void load()
{
	scan_directory("plugins", list);
}

} /* namespace PLM */
