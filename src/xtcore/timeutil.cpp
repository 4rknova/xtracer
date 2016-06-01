#include "log.hpp"
#include "timeutil.hpp"

void convert_mlseconds(double mlsecs, unsigned int &days,
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

void print_time_breakdown(double mlsecs)
{
		unsigned int days = 0;
		unsigned int hours = 0;
		unsigned int mins = 0;
		float secs = 0;

		convert_mlseconds(mlsecs, days, hours, mins, secs);

		Log::handle().log_message("Total time:");

		if (days  > 0){
			Log::handle().set_append();
			Log::handle().log_message(" %i days,", days);
		}
		if (hours  > 0) {
			Log::handle().set_append();
			Log::handle().log_message(" %i hours,", hours);
		}
		if (mins  > 0) {
			Log::handle().set_append();
			Log::handle().log_message(" %i mins,", mins);
		}
		Log::handle().set_append();
		Log::handle().log_message(" %f seconds.", secs);
}
