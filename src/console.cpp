/*

    This file is part of xtracer.

    console.cpp
    Console class

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this program; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#include <iomanip>
#include <iostream>

// antialiasing loop
#ifdef _MSC_VER
	#include <fcntl.h>
	#include <io.h>
	#define WIN32_LEAN_AND_MEAN
	#define WIN64_LEAN_AND_MEAN
	#include <windows.h>
#endif /* _MSC_VER */

#include "console.hpp"

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

void Console::progress(float progress, int worker, int workers)
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
