#if defined(__linux) || defined(__unix) || defined (__posix)
	#include <string.h> // For NULL
#endif

#include "timer.hpp"

Timer::Timer()
{
#if defined(_WIN32)
    QueryPerformanceFrequency(&m_freq);
    m_scount.QuadPart = 0;
    m_ecount.QuadPart = 0;
#elif defined(__linux) || defined(__unix) || defined (__posix)
    m_scount.tv_sec = m_scount.tv_usec = 0;
    m_ecount.tv_sec = m_ecount.tv_usec = 0;
#endif /* _WIN32 */

    m_stopped = true;
    m_stime = 0;
    m_etime = 0;
}

Timer::~Timer()
{}

void Timer::start()
{
    m_stopped = false;

#if defined(_WIN32)
    QueryPerformanceCounter(&m_scount);
#elif defined(__linux) || defined(__unix) || defined (__posix)
    gettimeofday(&m_scount, NULL);
#endif /* _WIN32 */
}

void Timer::resume()
{
	m_stopped = false;
}

void Timer::stop()
{
    m_stopped = true;

#if defined(_WIN32)
    QueryPerformanceCounter(&m_ecount);
#elif defined(__linux) || defined(__unix) || defined (__posix)
    gettimeofday(&m_ecount, NULL);
#endif /* _WIN32 */
}

double Timer::get_time_in_mcsec()
{
#if defined(_WIN32)
    if(!m_stopped)
        QueryPerformanceCounter(&m_ecount);

    m_stime = m_scount.QuadPart * (1000000.0 / m_freq.QuadPart);
    m_etime = m_ecount.QuadPart * (1000000.0 / m_freq.QuadPart);
#elif defined(__linux) || defined(__unix) || defined (__posix)
    if(!m_stopped)
        gettimeofday(&m_ecount, NULL);

    m_stime = (m_scount.tv_sec * 1000000.0) + m_scount.tv_usec;
    m_etime = (m_ecount.tv_sec * 1000000.0) + m_ecount.tv_usec;
#endif /* _WIN32 */

    return m_etime - m_stime;
}

double Timer::get_time_in_mlsec()
{
    return this->get_time_in_mcsec() * 0.001;
}

double Timer::get_time_in_sec()
{
    return this->get_time_in_mcsec() * 0.000001;
}
