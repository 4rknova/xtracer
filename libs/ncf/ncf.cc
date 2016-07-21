#include <cstdio>
#include <fstream>
#include <list>
#include "util.h"
#include "ncf.h"

NCF::NCF()
	: m_p_level(0)
{}

NCF::NCF(const NCF &)
	: m_p_level(0)
{
	// Not implemented.
	// This constructor is private and is never used internally.
	// No need to check for shallow copying ( no copying will ever be done ).
}

NCF &NCF::operator =(const NCF &)
{
	// Not implemented.
	// This constructor is private and is never used internally.
	// No need to check for shallow copying ( no copying will ever be done ).
	return *this;
}

NCF::~NCF()
{
	release();
}

void NCF::set_source(const char *file)
{
	m_p_filepath = file;
}

const char* NCF::get_source() const
{
	return m_p_filepath.c_str();
}

void NCF::purge()
{
	release();
}

void NCF::release()
{
	std::map<std::string, NCF*>::iterator it = m_p_groups.begin();
	std::map<std::string, NCF*>::iterator et = m_p_groups.end();

	for (; it != et; ++it)
	{
		delete it->second;

#ifdef CONFIG_DEBUG
		printf("NCF parser: [-node: %p]\n", (void *)it->second);
#endif /* CONFIG_DEBUG */
	}

	m_p_groups.clear();
	m_p_symbols.clear();
}

int NCF::parse()
{
	// Sanity check
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
	std::list<NCF*> group_stack;

	group_stack.push_front(this);

	// Process file.
	std::fstream in(m_p_filepath.c_str(), std::ios::in);

	if (!in.good()) return 2;

	int linep = 0;
	int sectionp = 0;

	std::string input;
	while (getline(in, input))
	{
		linep++;

		std::string line, comment;

		Util::String::split(input, line, comment, '#'); // Strip comments.
		Util::String::trim(line); // Trim spaces and unused characters.

		if (!line.length())
		{
			// Empty line - do nothing.
		}
		else if (line.length() > 2 && (line.find('=') != std::string::npos))
		{
			// Get the identifier and its value.
			std::string name, value;
			Util::String::split(line, name, value, '=');

			// Trim the name and its value from unused characters and spaces.
			Util::String::trim(name);
			Util::String::trim(value);

			// If this is a section start.
			if (value == "{")
			{
				sectionp++;

				std::map<std::string,NCF*>::iterator it = group_stack.front()->m_p_groups.find(name);

				// Check whether the group name already exists and create a new node, if needed.
				if (it == group_stack.front()->m_p_groups.end())
				{
					NCF *new_group = new NCF();
					new_group->m_p_level = group_stack.front()->m_p_level + 1;
					group_stack.front()->m_p_groups[name] = new_group;
					new_group->m_p_name = name;

#ifdef CONFIG_DEBUG
					printf("NCF parser: [+node: %p]\n", (void *)new_group);
#endif /* CONFIG_DEBUG */
				}

				// Make it the active node.
				group_stack.push_front(group_stack.front()->m_p_groups[name]);
			}
			// If this is a regular assignment.
			else
			{
				std::list<NCF*>::reverse_iterator end = group_stack.rend();
				for (std::list<NCF*>::reverse_iterator it = group_stack.rbegin(); it != end; ++it) {
					(*it)->expand_symbol(m_p_symbols, value);
				}

				group_stack.front()->m_p_symbols[name] = value;
			}
		}
		// If this is a section end.
		else if( (line.length() == 1) && (line.find('}') != std::string::npos) )
		{
			--sectionp;

			// Check for unbalanced sections.
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
			Util::String::split(line, com, value, ' ');
			// Trim the com and its value from unused characters and spaces.
			Util::String::trim(com);
			Util::String::trim(value);

			if (!com.compare("%include"))
			{
				std::string base, file;
				// Extract base path and file name of the active script.
				Util::String::path_comp(m_p_filepath, base, file);

				// Append the inclusion value to the base.
				base.append(value);

#ifdef CONFIG_DEBUG
				printf("NCF inline %s\n", base.c_str());
#endif /* CONFIG_DEBUG */

				group_stack.front()->m_p_filepath = base;
				group_stack.front()->parse();
			}
			else
			{
#ifdef CONFIG_DEBUG
				printf("Syntax error: Invalid modifier [ %s ] at line:%i\n", com.c_str(), linep);
#endif /* CONFIG_DEBUG */
			}
		}
		else
		{
			fprintf(stderr, "Syntax error: Invalid entry at line: %i\n", linep);
			return 3;
		}
	}

	in.close();

	// Check for unbalanced sections.
	if(sectionp)
	{
#ifdef CONFIG_DEBUG
		fprint("Syntax error: Final unbalanced sections [ %i ].\n", linep);
#endif /* CONFIG_DEBUG */
		return 4;
	}

	return 0;
}

void NCF::expand_symbol(std::map<std::string, std::string> &symbols, std::string &s)
{
	if (   s.empty()
		|| s.length() < 3
		|| s.find_first_of('<') == s.npos
		|| s.find_first_of('<') == s.find_last_of('>'))
		return; // Nothing to expand, so return.

	std::string source = s;

	do {
		std::string search;

		// Extract reference.
		Util::String::extract(source,search,'<','>');

		// If there is no valid pattern, return.
		if (search.length() < 3) return;

		// Purify string.
		std::string psearch = search.substr(1,search.length() - 2);

		// Try to match.
		std::map<std::string, std::string>::iterator end = symbols.end();
		for (std::map<std::string, std::string>::iterator it = symbols.begin(); it != end; ++it) {
#ifdef CONFIG_DEBUG
			printf("NCF parser: %s == %s ?\n", search.c_str(), it->first.c_str());
#endif /* CONFIG_DEBUG */

			size_t pos = psearch.compare(it->first);
			if (!pos)
			{
				Util::String::replace_first_of(s, search, it->second);
#ifdef CONFIG_DEBUG
				printf("NCF parser: Matched! %s\n", s.c_str());
#endif /* CONFIG_DEBUG */
			}
		}
	} while (1);
}

const char* NCF::get_property_by_name(const char *name) const
{
	std::map<std::string, std::string>::iterator it = m_p_symbols.find(name);
	return  it == m_p_symbols.end()
			? NULL
			: it->second.c_str();
}

const char* NCF::get_property_by_index(unsigned int index) const
{
	if (count_properties() == 0) return NULL;

	std::map<std::string, std::string>::iterator it = m_p_symbols.begin();
	std::map<std::string, std::string>::iterator et = m_p_symbols.end();
	for (unsigned int i = 0; (it != et) && (index != i++); ++it);
	return it->second.c_str();
}

const char* NCF::get_property_name_by_index(unsigned int index) const
{
	if (count_properties() == 0) return NULL;

	std::map<std::string, std::string>::iterator it = m_p_symbols.begin();
	std::map<std::string, std::string>::iterator et = m_p_symbols.end();
	for (unsigned int i = 0; (it != et) && (index != i++); ++it);
	return it->first.c_str();
}

void NCF::set_property(const char *name, const char *value)
{
	m_p_symbols[name] = value;
}

NCF* NCF::get_group_by_name(const char *name) const
{
	std::map<std::string, NCF* >::iterator it = m_p_groups.find(name);

	// If the requested key doesn't exist then we create it.
	if (it == m_p_groups.end())
	{
		m_p_groups[name] = new NCF();
		m_p_groups[name]->m_p_level = m_p_level + 1;

#ifdef CONFIG_DEBUG
		printf("NCF parser: [!node: %p]\n", (void *)m_p_groups[name]);
#endif /* CONFIG_DEBUG */
	}

	return m_p_groups[name];
}

NCF* NCF::get_group_by_index(unsigned int index) const
{
	if (count_groups() == 0) return NULL;

	std::map<std::string, NCF*>::iterator it = m_p_groups.begin();
	std::map<std::string, NCF*>::iterator et = m_p_groups.end();
	for (unsigned int i = 0; (it != et) && (index != i++); ++it);
	return it->second;
}

unsigned int NCF::count_groups() const
{
	return m_p_groups.size();
}

unsigned int NCF::count_properties() const
{
	return m_p_symbols.size();
}

bool NCF::query_property(const char* name) const
{
	return m_p_symbols.find(name) != m_p_symbols.end();
}

bool NCF::query_group(const char* name) const
{
	return m_p_groups.find(name) != m_p_groups.end();
}

const char *NCF::get_name() const
{
	return m_p_name.c_str();
}

int NCF::dump(const char *file, int create) const
{
	std::ios_base::openmode fmode = std::ios_base::out;

	if (!create) fmode = fmode | std::ios_base::app;
	std::fstream out(file, fmode);

	if (!out.good()) {
#ifdef NCF_DEBUG
		printf("CF1 parser: %s -> I/O error\n", file)printf;
#endif
		return 1;
	}

	if (create) out << "# CF1 dump\n";

	std::string ident;
	for (unsigned int i = 0; i < m_p_level; i++) ident.append("\t");

	std::map<std::string, std::string>::iterator pit = m_p_symbols.begin();
	std::map<std::string, std::string>::iterator pet = m_p_symbols.end();

	for (; pit != pet; ++pit) {
		out << ident.c_str() << pit->first.c_str() << " = " << pit->second.c_str() << "\n";
		out.flush();
	}

	std::map<std::string, NCF*>::iterator git = m_p_groups.begin();
	std::map<std::string, NCF*>::iterator get = m_p_groups.end();

	for (; git != get; ++git) {
		out << ident.c_str() << git->first.c_str() << " = {" << "\n";
		out.flush();
		out.close();
		git->second->dump(file, 0);
		fmode = fmode | std::ios_base::app;
		out.open(file, fmode);

		if (!out.good()) {
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
