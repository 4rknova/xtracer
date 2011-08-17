/*

    This file is part of xtracer.

    console.cpp
    Console class

    Copyright (C) 2010, 2011
    Papadopoulos Nikolaos

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General
    Public License along with this library; if not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301 USA

*/

#include <iomanip>
#include <iostream>

#include "console.hpp"

Console::Console() 
{}

Console::~Console()
{
}


void Console::review()
{
	std::cout << "\r";
}

void Console::progress(float progress, int worker, int workers)
{
	static const unsigned int length = 25;
	std::cout << "Progress [ ";
	for (unsigned int i = 0; i < length; i++)
	{
		float p = progress * length / 100;
		if (i < p)
			std::cout << '=';
		else if (i - p < 1)
			std::cout << '>';
		else
			std::cout << ' ';
	}
	
	// get the string length of totalw
	int wlen = 0;
	int num = workers;
	while (num>0)
	{
		num /= 10;
		wlen++;
	}
	
	std::cout
		<< " "
		<< std::setw(6) << std::setprecision(2)
		<< progress
		<< "% ] [ T: " << workers << " C: " << std::setw(wlen) << worker + 1 << " ]"
		<< std::flush;
}
