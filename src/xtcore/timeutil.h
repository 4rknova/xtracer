#ifndef FLUX_TIMEUTIL_HPP_INCLUDED
#define FLUX_TIMEUTIL_HPP_INCLUDED

#include <string>

void convert_mlseconds(double mlsecs, unsigned int &days,
					   unsigned int &hours, unsigned int &mins,
					   float &secs);

void print_time_breakdown(std::string &str, double mlsecs);

#endif /* FLUX_TIMEUTIL_HPP_INCLUDED */
