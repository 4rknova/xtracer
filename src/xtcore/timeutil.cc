#include <sstream>
#include "timeutil.h"

void convert_mlseconds(double mlsecs
                     , size_t &days
					 , size_t &hours
                     , size_t &mins
					 , float  &secs)
{
	unsigned int total_msec = (unsigned int)(mlsecs);
	unsigned int total_secs = (unsigned int)(mlsecs / 1000.f);

	days  = (unsigned int)(total_secs) / 86400;
	hours = (unsigned int)(total_secs / 3600) - (days * 24);
	mins  = (unsigned int)(total_secs / 60) - (days * 1440) - (hours * 60);
	secs  = (float)(total_secs % 60) + (((float)total_msec / 1000.f) - (float)total_secs);
}

void print_time_breakdown(std::string &str, double mlsecs)
{
    size_t d = 0  // days
         , h = 0  // hours
	     , m = 0; // minutes
	float  s = 0; // seconds

	convert_mlseconds(mlsecs, d, h, m, s);

    std::stringstream ss;

	if (d > 0)  ss << d <<    "days ";
	if (h > 0)  ss << h <<   "hours ";
	if (m > 0)  ss << m << "minutes ";
	if (s > 0)  ss << s << "seconds ";

    str = ss.str();
}
