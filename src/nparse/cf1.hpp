/*

	This file is part of the Nemesis parse library.

	cf1.hpp
	CF1 format parser

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

#ifndef LIBNPARSE_CF1_HPP_INCLUDED
#define LIBNPARSE_CF1_HPP_INCLUDED

#include <map>
#include <string>
#include <list>

/*
	## Config file structure ##

	Parsing rules
	-------------

	a. Each line is treated as separate rule entry.
	b. Trailing and leading whitespaces in entries are stripped at parsing stage.
	c. Parsed config entries are stored in a symbol map.
	d. Config files can be structured (to arbitrary depth).

	Entry types
	-----------

	include		:   %include <path_to_file>

	Comments	:	Strings beginning with '#' are comments and will be ignored.

	Assignments	:	Entries with name-value assignments follow the form:
						<name> = <value>

					Bottom-most assignment within a group, breveil.

	Sub sections:	Sub sections follow the form:
						<name> = {
							<content>
						}

					Again, content will be multilined, following the one command per line rule.
					'{' must be in the same line as the group name, following the pattern of assignments.
					'}' must be alone in an empty line.

					example of invalid formats
					--------------------------
					a.	<name> = { <content>
						}
					b.	<name> = {
						<content> }
					c.	<name> = { <content> }

	References	:	Values may reuse already defined symbols as variables which get expanded during the parsing process.
					The patterns for expansion are searched from the current sub group upwards, and follow the notation:
						<<Symbolic name>>

					Top-most references in group hierarchy breveil.

	

	Future expansions
	-----------------

	- Support for multilined content
	- Basic arithmetic (+,-,/,*,%)

*/

/*
#define NCF1_DEBUG
*/


class NCF1
{
	public:
		NCF1(const char *file, const char **envp = 0);
		~NCF1();

		/* Get source file path */
		const char* get_source();							/* Get the source file path */

		int parse();										/* Parse the source file */
		void purge();										/* Purge the subtree */
		int dump(const char *file, int create = 1);			/* Dump to file */
        
		const char* get(const char *name);					/* Get property value by name */
		const char* get(unsigned int index);				/* Get property value by index */
        void set(const char *name, const char *value);		/* Set property value */
		unsigned int count_properties();					/* Count the properties */
		void list_properties(std::string &l, char delim = ',');
															/* Prepare a list of the properties */
		unsigned int query_property(const char* name);		/* Return 1 if the property exists and 0 if not */

		NCF1* group(const char *name);				/* Get a pointer to a labeled sub group. */
		NCF1* group(unsigned int index);				/* Get a pointer to an indexed sub group */
		unsigned int count_groups();						/* Count the subgroups */
		void list_groups(std::string &l, char delim = ',');	/* Prepare a list of the groups */
		unsigned int query_group(const char *name);			/* Return 1 if the group exists and 0 if not */
		
		std::string node();									/* Return the node's name */

    private:
		/* Private constructor for sub groups. */
		NCF1();
		void destroy();
		/* Expand symbols. */
		void expand_symbol(std::map<std::string, std::string> &symbols, std::string &s);

		/* Group name */
		std::string m_p_name;

		/* Sub-groups map */
		std::map<std::string, NCF1* > m_p_groups;

		/* Group symbol map */
		std::map<std::string, std::string> m_p_symbols;
		/* Static environment symbols map. */
		static std::map<std::string, std::string> m_p_env_symbols;

		/* cfg filepath */
		std::string m_p_filepath;

		/* node level */
		unsigned int m_p_level;
};


#endif /* LIBNPARSER_CF1_HPP_INCLUDED */
