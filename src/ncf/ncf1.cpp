/*

	This file is part of libncf.

	ncf1.cpp
	NCF parser compliant with specification version 1.

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

#include <fstream>
#include <iostream>
#include <cstdio>

#include "util.hpp"
#include "ncf1.hpp"

namespace NCF {

NCF1::NCF1()
	: m_p_level(0)
{}

NCF1::NCF1(const NCF1&)
	: m_p_level(0)
{
	// NOTE: Not implemented yet.
	// This constructor is private and is never used internally.
	// Therefore:
	// 2. I do not check about shallow copying ( no copying will ever be performed ).
}

NCF1 &NCF1::operator =(const NCF1 &)
{
	// NOTE: Not implemented yet.
	// This operator is private and is never used internally.
	// Therefore:
	// 1. I do not check about self assignement.
	// 2. I do not check about shallow copying ( no copying will ever be performed ).
	return *this;
}


NCF1::~NCF1()
{
	release();
}

void NCF1::source(const char *file)
{
	m_p_filepath = file;
}

const char* NCF1::source() const
{
	return m_p_filepath.c_str();
}

void NCF1::purge()
{
	release();
	m_p_symbols.clear();
}

int NCF1::parse()
{
	{
		int c;
		std::ifstream in(m_p_filepath.c_str());

		if (!in.good())
			return 2;

		while((c = in.get()) != EOF && c <= 127);
		in.close();

		if(c != EOF)
			return 1;
	}

	// Stack of groups for parsing.
    std::list< NCF1* > group_stack;

	group_stack.push_front(this);

    // Process file.
    std::fstream in(m_p_filepath.c_str(), std::ios::in);

	if (!in.good())
		return 2;

	int linep = 0;
	int sectionp = 0;

	std::string input;
	while (getline(in, input))
	{
		linep++;

	    std::string line, comment;
	    
	    NCF::Util::split(input, line, comment, '#'); // Strip comments.
		NCF::Util::trim(line); // Trim spaces and unused characters.

		if (!line.length())
		{ 
			// Empty line - do nothing. 
		}
		else if (line.length() > 2 && (line.find('=') != std::string::npos))
		{
			// Get the identifier and its value.
			std::string name;
			std::string value;
			NCF::Util::split(line, name, value, '=');

			// Trim the name and its value from unused characters and spaces.
			NCF::Util::trim(name);
			NCF::Util::trim(value);

			// If this is a section start.
			if (value == "{")
			{
				sectionp++;

				std::map<std::string, NCF1* >::iterator it = group_stack.front()->m_p_groups.find(name);

				// Check whether the group name already exists and create a new node, if needed.
				if (it == group_stack.front()->m_p_groups.end())
				{
					NCF1 *new_group = new NCF1();
					new_group->m_p_level = group_stack.front()->m_p_level + 1;
					group_stack.front()->m_p_groups[name] = new_group;
					new_group->m_p_name = name;

					#ifdef NCF_DEBUG
						printf("CF1 parser: [+node: %p]\n", (void *)new_group);
					#endif /* NCF_DEBUG */
				}

				// Make it the active node.
				group_stack.push_front(group_stack.front()->m_p_groups[name]);
			}
			// If this is a regular assignment.
			else
			{
				for (std::list< NCF1* >::reverse_iterator it = group_stack.rbegin(); it != group_stack.rend(); ++it)
					(*it)->expand_symbol(m_p_symbols, value);

				group_stack.front()->m_p_symbols[name] = value;
			}
		}
		// If this is a section end.
        else if( (line.length() == 1) && (line.find('}') != std::string::npos) )
        {
			sectionp--;

			// Check for unbalanced sections to prevent segfaults.
			if (sectionp < 0)
			{
				fprintf(stderr, "Syntax error: Unbalanced sections at line: %i.\n", linep);
				return 1;
			}

			group_stack.pop_front();
        }
		else if((line.length() > 1) && (line.find('%') != std::string::npos))
		{
			// get the identifier and its value.
			std::string com;
			std::string value;
			NCF::Util::split(line, com, value, ' ');
			// Trim the com and its value from unused characters and spaces.
			NCF::Util::trim(com);
			NCF::Util::trim(value);


			if (!com.compare("%include"))
			{
				std::string base, file;
				// Extract base path and file name of the active script.
				NCF::Util::path_comp(m_p_filepath, base, file);

				// Append the inclusion value to the base.
				base.append(value);

				#ifdef NCF_DEBUG
					printf("Including %s\n", base.c_str());
				#endif /* NCF_DEBUG */

				group_stack.front()->m_p_filepath = base;
				group_stack.front()->parse();				
			}
			else
			{
				fprintf(stderr, "Syntax error: Invalid modifier [ %s ] at line:%i\n", com.c_str(), linep);
			}
		}
		else
		{
			fprintf(stderr, "Syntax error: Invalid entry at line: %i\n", linep);
			return 1;
		}
	}

	// Check for unbalanced sections.
	if(sectionp)
	{
		fprintf(stderr,"Syntax error: Final unbalanced sections [ %i ].\n", linep);
	}

	in.close();

	return 0;
}

int NCF1::dump(const char *file, int create) const
{
	// Process file.
    std::ios_base::openmode fmode = std::ios_base::out;

    if (!create)
		fmode = fmode | std::ios_base::app;

    std::fstream out(file, fmode);

	if (!out.good())
	{
		// File I/O error.
		#ifdef NCF_DEBUG
			printf("CF1 parser: %s -> I/O error\n", file);
		#endif
		return 1;
	}

	if (create)
		out << "# CF1 dump\n";

	// Prepare identation string.
	std::string ident;
	for (unsigned int i = 0; i < m_p_level; i++)
		ident.append("\t");

	for (std::map<std::string, std::string>::iterator it = m_p_symbols.begin(); it != m_p_symbols.end(); ++it)
	{
		out << ident.c_str() << it->first.c_str() << " = " << it->second.c_str() << "\n";
		out.flush();
	}

	for (std::map<std::string, NCF1* >::reverse_iterator it = m_p_groups.rbegin(); it != m_p_groups.rend(); ++it)
	{
		out << ident.c_str() << it->first.c_str() << " = {" << "\n";
		out.flush();
		out.close();

		it->second->dump(file, 0);

		fmode = fmode | std::ios_base::app;
		out.open(file, fmode);

		if (!out.good())
		{
			// File I/O error.
			#ifdef NCF_DEBUG
				printf("CF1 parser: %s -> I/O error\n", file);
			#endif /* NCF_DEBUG */
			return 1;
		}

		out << ident.c_str() << "}" << "\n";
	}

	out.flush();
	out.close();
	return 0;
}

void NCF1::expand_symbol(std::map<std::string, std::string> &symbols, std::string &s)
{
	if (s.empty() || (s.length() < 3) || (s.find_first_of('<') == s.npos) || (s.find_first_of('<') == s.find_last_of('>')))
        return; // Nothing to expand, so return.

	std::string source = s;

	do
	{
		std::string search;

		// Extract reference.
		NCF::Util::extract(source,search,'<','>');

		// If there is no valid pattern, return.
		if (search.length() < 3)
			return;

		// Purify string.
		std::string psearch = search.substr(1,search.length() - 2);

		// Try to match.
		for (std::map<std::string, std::string>::iterator it = symbols.begin(); it != symbols.end(); ++it)
		{
			#ifdef NCF_DEBUG
				printf("CF1 parser: %s == %s ?\n", search.c_str(), it->first.c_str());
			#endif /* NCF_DEBUG */

			size_t pos = psearch.compare(it->first);
			if (!pos)
			{
				NCF::Util::replace_first_of(s, search, it->second);
				#ifdef NCF_DEBUG
					printf("CF1 parser: Matched! %s\n", s.c_str());
				#endif /* NCF_DEBUG */
			}
		}
	} while (1);
}

void NCF1::release()
{
	for (std::map<std::string, NCF1* >::reverse_iterator it = m_p_groups.rbegin(); it != m_p_groups.rend(); it++)
	{
		delete it->second;
		#ifdef NCF_DEBUG
			printf("CF1 parser: [-node: %p]\n", (void *)it->second);
		#endif /* NCF_DEBUG */
	}
	m_p_groups.clear();
}

const char* NCF1::get(const char *name) const
{
	std::map<std::string, std::string>::iterator it = m_p_symbols.find(name);
	return (it == m_p_symbols.end()) ? "" : it->second.c_str();
}

const char* NCF1::get(unsigned int index) const
{
	std::map<std::string, std::string >::iterator it;
	
	if (index > count_groups())
	{
		it = m_p_symbols.end();
		it--;
	}
	else
	{
		for (it = m_p_symbols.begin(); (it != m_p_symbols.end()) && --index ; it++);
	}
	return it->second.c_str();
}

const char* NCF1::name(unsigned int index) const
{
	unsigned int c = count_properties();
	
	if (!c)
		return NULL;
	
	std::map<std::string, std::string>::iterator it = m_p_symbols.begin();

	if (index >= c)
		index = c-1;

	while (index > 0) {
		index--;
		it++;
	}

	return it->first.c_str();
}


void NCF1::set(const char *name, const char *value)
{
	m_p_symbols[name] = value;
}

NCF1* NCF1::group(const char *name) const
{
    std::map<std::string, NCF1* >::iterator it = m_p_groups.find(name);
    // If the requested node key doesn't exist then we create it.
    if (it == m_p_groups.end())
    {
    	m_p_groups[name] = new NCF1();
    	m_p_groups[name]->m_p_level = m_p_level + 1;
		#ifdef NCF_DEBUG
			printf("CF1 parser: [!node: %p]\n", (void *)m_p_groups[name]);
		#endif /* NCF_DEBUG */
    }

    return m_p_groups[name];
}

NCF1* NCF1::group(unsigned int index) const
{
	std::map<std::string, NCF1* >::iterator it;

	if (index > count_groups())
	{
		it = m_p_groups.end();
		it--;
	}
	else
	{
		for (it = m_p_groups.begin(); (it != m_p_groups.end()) && --index ; it++);
	}

	return it->second;
}

unsigned int NCF1::count_groups() const
{
	return m_p_groups.size();
}

unsigned int NCF1::count_properties() const
{
	return m_p_symbols.size();
}

unsigned int NCF1::query_property(const char* name) const
{
	return m_p_symbols.find(name) != m_p_symbols.end() ? 1 : 0;
}

unsigned int NCF1::query_group(const char* name) const
{
	return m_p_groups.find(name) != m_p_groups.end() ? 1 : 0;
}

const char *NCF1::name() const
{
	return m_p_name.c_str();
}

} /* namespace NCF */
