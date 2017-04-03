#ifndef FLUX_TIMER_HPP_INCLUDED
#define FLUX_TIMER_HPP_INCLUDED

#if defined(_WIN32)
    #define WIN64_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
    #include <Windows.h>
#elif defined(__linux) || defined(__unix) || defined (__posix)
    #include <sys/time.h>
#else
	#error "Unspecified or unsupported platform."
#endif /* _WIN32 */

class Timer
{
    public:
		Timer();
		~Timer();

		void start();
		void resume();
		void stop();

		double get_time_in_sec();
		double get_time_in_mlsec();
		double get_time_in_mcsec();

    private:
		double m_stime;
		double m_etime;
		bool m_stopped;

	#if defined(_WIN32)
		LARGE_INTEGER m_freq;
		LARGE_INTEGER m_scount;
		LARGE_INTEGER m_ecount;

	#elif defined(__linux) || defined(__unix) || defined (__posix)
		timeval m_scount;
		timeval m_ecount;

    #endif /* _WIN32  */
};

#endif /* FLUX_TIMER_HPP_INCLUDED */
