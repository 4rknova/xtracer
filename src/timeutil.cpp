#include "timeutil.hpp"

void convert_mlseconds(const double mlsecs, unsigned int &days,
					   unsigned int &hours, unsigned int &mins,
					   float &secs)
{
	unsigned int total_msec = (unsigned int)(mlsecs);
	unsigned int total_secs = (unsigned int)(mlsecs / 1000.f);

	days  = (unsigned int)(total_secs) / 86400;
	hours = (unsigned int)(total_secs / 3600) - (days * 24);
	mins  = (unsigned int)(total_secs / 60) - (days * 1440) - (hours * 60);
	secs  = (float)(total_secs % 60) + (((float)total_msec / 1000.f) - (float)total_secs);
}
