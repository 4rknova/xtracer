#ifndef FLUX_TIMEUTIL_HPP_INCLUDED
#define FLUX_TIMEUTIL_HPP_INCLUDED

void convert_mlseconds(double mlsecs, unsigned int &days,
					   unsigned int &hours, unsigned int &mins,
					   float &secs);

void print_time_breakdown(double mlsecs);

#endif /* FLUX_TIMEUTIL_HPP_INCLUDED */
