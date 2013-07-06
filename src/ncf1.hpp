/*

	This file is part of libncf

	ncf1.hpp
	NCF parser compliant with specification version 1.

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

#ifndef NCF_NCF1_HPP_INCLUDED
#define NCF_NCF1_HPP_INCLUDED

#include <map>
#include <string>
#include <list>



// For visual studio:
// Disable "<type> needs to have dll-interface to be used by clients"
// This warning refers to STL member variables which are private and
// therefore can be safely ignored.
#pragma warning (push)
#pragma warning (disable : 4251)

namespace NCF {

class NCF1
{
	public:
		NCF1();
		~NCF1();

		void source(const char *file);						// Set source file.
		const char* source() const;							// Get source file.
		
		void purge();										// Purge the subtree.

		// RETURN CODES:
		// 0. Everything went well.
		// 1. Not an ASCII file.
		// 2. File I/O error.
		int parse();										// Parse the script file.

		// RETURN CODES:
		// 0. Everything went well.
		// 1. File I/O error.
		int dump(const char *file, int create = 1) const;	// Dump to file.
        
		const char* get(const char *name) const;			// Get property value by name.
		const char* get(unsigned int index) const;			// Get property value by index.
		const char* name(unsigned int index) const;			// Get property name by index.
        void set(const char *name, const char *value);		// Set property value.

		NCF1* group(const char *name) const;				// Get a pointer to a labeled sub group.
		NCF1* group(unsigned int index) const;				// Get a pointer to an indexed sub group.
		
		unsigned int count_properties() const;				// Count the properties.
		unsigned int count_groups() const;					// Count the subgroups.
		unsigned int query_property(const char* name) const;// Return 1 if the property exists and 0 if not.
		unsigned int query_group(const char *name) const;	// Return 1 if the group exists and 0 if not.
		
		const char *name() const;							// Return the node's name.

    private:
	 	// Dissallow copy construction.
		// Note: This will create a shallow copy and therefore it is NEVER used internally.
	   	// The reason is that the group tree is recursive and dynamic.
	    NCF1(const NCF1&);
		NCF1 &operator =(const NCF1 &);

		void release();
		
		/* Expand symbols. */
		void expand_symbol(std::map<std::string, std::string> &symbols, std::string &s);

		std::string m_p_name;									// Node name.
		mutable std::map<std::string, NCF1* > m_p_groups;		// Sub-node map.
		mutable std::map<std::string, std::string> m_p_symbols; // Symbol map.
		std::string m_p_filepath;								// Source path.
		unsigned int m_p_level;									// Node level.
};

} /* namespace NCF */

#pragma warning (pop)

#endif /* NCF_NCF1_HPP_INCLUDED */
