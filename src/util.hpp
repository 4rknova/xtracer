/*

	This file is part of libncf.

	util.hpp
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

#ifndef NCF_UTIL_HPP_INCLUDED
#define NCF_UTIL_HPP_INCLUDED


#include <string>

namespace NCF {
	namespace Util {

// Trims '"' at start and end of string. (eg.  "Hello" -> Hello )
void unquote(std::string &s);

// Strips the string of all occurences of the given character.
void strip(std::string &s, char c);

// Trims ' ', '\t' at the start of the given string.
void trim_left(std::string &s);

// Trims ' ', '\n', '\t', '\r' at the end of the given string.
void trim_right(std::string &s);

// Trims ' ', '\n', '\t', '\r' at the start and the end of the given string.
void trim(std::string &s);

// Splits the given array into two string at first occurance of given char.
void split(std::string in, std::string &left, std::string &right, char c);

// Extracts the first balanced substring included between two pX chars.
void extract(std::string &s, std::string &t, const char p1, const char p2);

// Extracts the base name and path from a string.
#ifdef _WIN32
	#define NCF_PATHDELIM '\\'
#else
	#define NCF_PATHDELIM '/'
#endif /* _WIN32 */

void path_comp(const std::string &s, std::string &base, std::string &file, const char delim = NCF_PATHDELIM);

// Replace a matched substring p with r.
void replace_first_of(std::string &s, const std::string &p, const std::string &r);

// Conversions.
void to_lower_case(std::string &s);			// Convert to lower case.
void to_upper_case(std::string &s);			// Convert to upper case.

bool to_bool(const std::string &s);			// Convert to boolean type.
int to_int(const std::string &s);			// Convert to integer type.
double to_double(const std::string &s);		// Convert to double type.

void to_string(std::string &out, bool val);	// Convert to string.
void to_string(std::string &out, int val);		// Convert to string.
void to_string(std::string &out, double val);	// Convert to string.

	} /* namespace Util */
} /* namespace NCF */

#endif /* NCF_UTIL_HPP_INCLUDED */
