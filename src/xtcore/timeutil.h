#ifndef FLUX_TIMEUTIL_HPP_INCLUDED
#define FLUX_TIMEUTIL_HPP_INCLUDED

#include <string>
#include <chrono>

void convert_mlseconds(double mlsecs, unsigned int &days,
					   unsigned int &hours, unsigned int &mins,
					   float &secs);

void print_time_breakdown(std::string &str, double mlsecs);

class Timer
{
public:
    void start();
    void stop();
    double get_time_in_mlsec() const;

private:
    std::chrono::steady_clock::time_point m_start;
    std::chrono::steady_clock::time_point m_end;
};

#endif /* FLUX_TIMEUTIL_HPP_INCLUDED */
