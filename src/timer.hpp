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

        void start();   			// starts the timer
		void resume();				// resumes the timer
        void stop();    			// stops the timer

        double get_time_in_sec();   // returns elapsed time in seconds
        double get_time_in_mlsec(); // returns elapsed time in milli-seconds
        double get_time_in_mcsec(); // returns elapsed time in micro-seconds

    private:
        double m_stime; 			// starting time in micro-seconds
        double m_etime; 			// ending time in micro-seconds
        bool m_stopped; 			// stop flag

	#if defined(_WIN32)
        LARGE_INTEGER m_freq; 		// ticks per second
        LARGE_INTEGER m_scount; 	// start count
        LARGE_INTEGER m_ecount; 	// end count

	#elif defined(__linux) || defined(__unix) || defined (__posix)
        timeval m_scount; 			// start count
        timeval m_ecount; 			// end count

    #endif /* _WIN32  */
};

#endif /* FLUX_TIMER_HPP_INCLUDED */
