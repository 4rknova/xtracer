#ifndef LIBNCF_UTIL_H_INCLUDED
#define LIBNCF_UTIL_H_INCLUDED

#include <string>
#include "declspec.h"

#ifdef _WIN32
#define CONFIG_DELIM_PATH '\\'
#else
#define CONFIG_DELIM_PATH '/'
#endif /* _WIN32 */

namespace ncf {
	namespace util {

DECLSPEC void   to_lower_case(std::string &s);           // Convert to lower case.
DECLSPEC void   to_upper_case(std::string &s);           // Convert to upper case.
DECLSPEC bool   to_bool(const std::string &s);           // Convert to boolean type.
DECLSPEC int	to_int(const std::string &s);            // Convert to integer type.
DECLSPEC float  to_float(const std::string &s);          // Convert to float type.
DECLSPEC double to_double(const std::string &s);         // Convert to double type.
DECLSPEC void   to_string(std::string &out, bool val);   // Convert to string.
DECLSPEC void   to_string(std::string &out, int val);    // Convert to string.
DECLSPEC void   to_string(std::string &out, float val);  // Convert to string.
DECLSPEC void   to_string(std::string &out, double val); // Convert to string.

DECLSPEC void strip(std::string &s, char c); // Removes all occurrences of the given character.
DECLSPEC void unquote(std::string &s);       // Trims '"' at start and end.
DECLSPEC void trim_left(std::string &s);     // Trims ' ', '\t' at the start of the given string.
DECLSPEC void trim_right(std::string &s);    // Trims ' ', '\n', '\t', '\r' at the end.
DECLSPEC void trim(std::string &s);          // Trims ' ', '\n', '\t', '\r' at the start and end.

// Splits the given array into two string at first occurance of given char.
DECLSPEC void split(std::string in, std::string &left, std::string &right, char c);
// Extracts the first balanced substring included between two pX chars.
DECLSPEC void extract(std::string &s, std::string &t, const char p1, const char p2);
// Extracts the base name and path from a string.
DECLSPEC void path_comp(const std::string &s, std::string &base, std::string &file, const char delim = CONFIG_DELIM_PATH);
// Replace a matched substring p with r.
DECLSPEC void replace_first_of(std::string &s, const std::string &p, const std::string &r);

	} /* namespace util */
} /* namespace ncf */

#endif /* LIBNCF_UTIL_H_INCLUDED */
