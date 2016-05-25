#include <iomanip>
#include <iostream>

#ifdef _MSC_VER
	#include <fcntl.h>
	#include <io.h>
	#define WIN32_LEAN_AND_MEAN
	#define WIN64_LEAN_AND_MEAN
	#include <windows.h>
#endif /* _MSC_VER */

#include "console.h"

// Singleton
Console Console::m_console;

Console::Console()
{}

Console::~Console()
{}

Console &Console::handle()
{
	return m_console;
}

void Console::update(float progress, int worker, int workers)
{
	std::cout.setf(std::ios::fixed, std::ios::floatfield);
	std::cout.setf(std::ios::showpoint);

	static const unsigned int length = 25;
	std::cout << "\rProgress [ ";

	for (unsigned int i = 0; i < length; i++) {
		float p = progress * length / 100;
		if		(i < p) std::cout << '=';
		else if (i - p < 1) std::cout << '>';
		else	std::cout << ' ';
	}

	int totalw = workers;
	// get the string length of totalw
	int wlen = 0;
	int num = totalw;

	while (num>0) {
		num /= 10;
		wlen++;
	}

	std::cout	<< " " << std::setw(6) << std::setprecision(2) << progress
				<< "% ] [ T: " << totalw << " C: " << std::setw(wlen)
				<< worker + 1 << " ]" << std::flush;
}
