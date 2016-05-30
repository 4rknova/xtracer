/*

	This file is part of libncf.

	util.cpp
	Parsing utilities

	Copyright (C) 2008, 2010 - 2012
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

#include <sstream>
#include <algorithm>
#include "util.hpp"

namespace NCF {
	namespace Util {

void unquote(std::string &s)
{
    trim(s);
	if ( (s.length() > 2) && (s[0] == '"') && (s[s.length()-1] == '"'))
	{
		s = s.substr(1, std::string::npos);
		s = s.substr(0, s.length()-1);
    }
}

void strip(std::string &s, char c)
{
    size_t pos = s.find(c);

    while(pos != std::string::npos)
    {
        s.erase(pos+1);
        pos = s.find(c);
    }
}

void trim(std::string &s)
 {
    trim_left(s);
    trim_right(s);

	if ((s.length() == 1) && ((s[0] == ' ') || (s[0] =='\t') || (s[0] == '\n') || (s[0] == '\r')))
	{
		s.clear();
	}
}


void trim_left(std::string &s)
{
	while ( (s.length() > 1) && ((s[0] == ' ') || (s[0] =='\t') ||  (s[0] == '\n') || (s[0] == '\r')))
	{
		s = s.substr(1, std::string::npos);
	}
}

void trim_right(std::string &s)
{
	while ( (s.length() > 1) && ( (s[s.length()-1] == ' ') || (s[s.length()-1] == '\t') || (s[s.length()-1] == '\n') || (s[s.length()-1] == '\r') ) )
	{
		s = s.substr(0, s.length()-1);
	}
}


void split(std::string in, std::string &left, std::string &right, char c)
{
	size_t pos = in.find_first_of(c);

	if(pos == std::string::npos)
	{
		left = in;
		right = "";
	}
	else if (pos < 1)
	{
		left = "";
		right = in.substr(pos+1, std::string::npos);
	}
	else
	{
		left = in.substr(0, pos);
		right = in.substr(pos+1, std::string::npos);
	}
}

void extract(std::string &s, std::string &t, const char p1, const char p2)
{
	// Same delimiters, two different ones are needed.
	if (p1 == p2)
		return;

	unsigned int i=0, count_p1=0, count_p2=0;

	// Count the total occurencies of each pX.
	while (i < s.length())
	{
		if (s[i]==p1)
			count_p1++;
		else if (s[i]==p2)
			count_p2++;

		i++;

		// Not balanced.
		if (count_p2 > count_p1)
			return;
	}

	// Not balanced or 0.
	if ((count_p2 != count_p1) || (count_p1 == 0))
		return;

	// Extract index positions.
	unsigned int index=0, index_1=0, index_2=0;
	while ((index++ < s.length()) && (s[index]!=p2));
	index_2=index;
	while ((index-- >= 0) && (s[index]!=p1));
	index_1=index;

	// Extract tokens.
	t = s.substr(index_1, index_2 + 1 - index_1);
	s = s.substr(0, index_1).append(s.substr(index_2 + 1, s.length() - index_2));
}

void path_comp(const std::string &s, std::string &base, std::string &file, const char delim)
{
	size_t pos = s.find_last_of(delim)+1;
	file = s.substr(pos, std::string::npos);
	base = s.substr(0, pos);
}

void replace_first_of(std::string &s, const std::string &p, const std::string &r)
{
	size_t result = 1;

	// Locate the pattern.
	unsigned int index = 0;
	for(index=0; (index <= s.length() - p.length()) && result; index++)
		result = p.compare(s.substr(index, p.length()));

	// The pattern was not matched.
	if(result)
		return;

	std::string head = s.substr(0, index-1);
	std::string tail = s.substr(index - 1 + p.length());

	s = head.append(r);
	s.append(tail);
}

void to_lower_case(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

void to_upper_case(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
}

bool to_bool(const std::string &s)
{
	std::string val = s;
	to_lower_case(val);
	return ( (val == "yes") || (val == "true") ) ? true : false;
}

int to_int(const std::string &s)
{
	std::string val = s;
	return atoi(val.c_str());
}

double to_double(const std::string &s)
{
	std::string val = s;
	return atof(val.c_str());
}

void to_string(std::string &out, bool val)
{
	if (val) {
		out = "true";
	} else {
		out = "false";
	}
}

void to_string(std::string &out, int val)
{
	std::stringstream ss;
	ss << val;
	out = ss.str();
}

void to_string(std::string &out, double val)
{
	std::stringstream ss;
	ss << val;
	out = ss.str();
}

	} /* namespace Util */
} /* namespace NCF */
