/*

	This file is part of the Nemesis parse library.

	util.hpp
	Parse utilities

	Copyright (C) 2008, 2010
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

#ifndef LIBNPARSE_UTIL_HPP_INCLUDED
#define LIBNPARSE_UTIL_HPP_INCLUDED

#include <string>

/* Trims '"' at start and end of string. (eg.  "Hello" -> Hello ) */
void nstring_unquote(std::string &s);

/* Strips the string of all occurences of the given character. */
void nstring_strip(std::string &s, char c);

/* Trims ' ', '\t' at the start of the given string. */
void nstring_trim_left(std::string &s);

/* Trims ' ', '\n', '\t', '\r' at the end of the given string. */
void nstring_trim_right(std::string &s);

/* Trims ' ', '\n', '\t', '\r' at the start and the end of the given string. */
void nstring_trim(std::string &s);

/* Splits the given array into two string at first occurance of given char. */
void nstring_split(std::string in, std::string &left, std::string &right, char c);

/* Extracts the first balanced substring included between two pX chars. */
void nstring_extract(std::string &s, std::string &t, const char p1, const char p2);

/* Replace a matched substring p with r */
void nstring_replace_first_of(std::string &s, const std::string &p, const std::string &r);

/* Converts string to lower case. */
void nstring_to_lower_case(std::string &s);

/* Converts string to upper case. */
void nstring_to_upper_case(std::string &s);

/* Converts string to boolean */
bool nstring_to_bool(const std::string &s);

/* Converts string to integer */
int nstring_to_int(const std::string &s);

/* Converts string to integer */
double nstring_to_double(const std::string &s);

/* Extracts the base name and path from a string */
void nstring_path_comp(const std::string &s, std::string &base, std::string &file);

#endif /* LIBNPARSE_UTIL_HPP_INCLUDED */
