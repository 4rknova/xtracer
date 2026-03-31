#include <cstdio>
#include <deque>
#include <fstream>
#include <list>
#include "util.h"
#include "ncf.h"

namespace ncf {

namespace {

static bool is_inline_group_literal(const std::string &value)
{
    if (value.size() < 2) return false;
    return value.front() == '{' && value.back() == '}';
}

static bool split_inline_block_items(const std::string &body, std::deque<std::string> &items)
{
    size_t start = 0;
    int paren_depth = 0;
    int brace_depth = 0;
    bool in_quotes = false;

    for (size_t i = 0; i < body.size(); ++i) {
        const char c = body[i];

        if (c == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (in_quotes) continue;

        if (c == '(') {
            ++paren_depth;
            continue;
        }
        if (c == ')') {
            --paren_depth;
            if (paren_depth < 0) return false;
            continue;
        }
        if (c == '{') {
            ++brace_depth;
            continue;
        }
        if (c == '}') {
            --brace_depth;
            if (brace_depth < 0) return false;
            continue;
        }

        if ((c == ',') && (paren_depth == 0) && (brace_depth == 0)) {
            std::string entry = body.substr(start, i - start);
            util::trim(entry);
            if (!entry.empty()) items.push_back(entry);
            start = i + 1;
        }
    }

    if (in_quotes || paren_depth != 0 || brace_depth != 0) return false;

    std::string tail = body.substr(start);
    util::trim(tail);
    if (!tail.empty()) items.push_back(tail);

    return true;
}

} // namespace

// ERROR MESSAGES
const char* g_error_messages[] = {
      "ok"
    , "not an ascii file"
    , "io error"
    , "syntax error"
    , "unbalanced groups (missing bracket)"
    , "group of the same name as the property already exists"
    , "property of the same name as the group already exists"
    , "error in external file"
};

int err(error_t *error, ERROR_CODE code, size_t line)
{
    if (error) {
        error->code    = code;
        error->message = g_error_messages[code];
        error->line    = line;
    }
    return code;
}

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

int NCF::parse(error_t *error)
{
	// Stack of groups for parsing.
	std::list<NCF*> group_stack;

	group_stack.push_front(this);

	// Process file.
	std::fstream in(m_p_filepath.c_str(), std::ios::in);

	if (!in.good()) return err(error, ERROR_CODE_IO, 0);

	int linep = 0;
	int sectionp = 0;

    std::deque<std::pair<int, std::string> > pending_lines;

	std::string raw_input;
	while (getline(in, raw_input))
	{
		++linep;
        pending_lines.push_back(std::make_pair(linep, raw_input));

        while (!pending_lines.empty())
        {
            const std::pair<int, std::string> current = pending_lines.front();
            pending_lines.pop_front();

            std::string line, comment;
            util::split(current.second, line, comment, '#'); // Strip comments.
            util::trim(line); // Trim spaces and unused characters.

            if (!line.length())
            {
                // Empty line - do nothing.
            }
            else if((line.length() > 1) && (line.find('%') != std::string::npos))
            {
                // get the identifier and its value.
                std::string com;
                std::string value;
                util::split(line, com, value, ' ');
                // Trim the com and its value from unused characters and spaces.
                util::trim(com);
                util::trim(value);

                if (!com.compare("%include"))
                {
                    std::string base, file;
                    // Extract base path and file name of the active script.
                    util::path_comp(m_p_filepath, base, file);

                    // Append the inclusion value to the base.
                    base.append(value);

                    group_stack.front()->m_p_filepath = base;

                    if (group_stack.front()->parse()) {
                        return err(error, ERROR_CODE_EXTERNAL_FILE, current.first);
                    }
                }
                else
                {
                    return err(error, ERROR_CODE_SYNTAX, current.first);
                }
            }
            else if (line.length() > 2 && (line.find('=') != std::string::npos))
            {
                // Get the identifier and its value.
                std::string name, value;
                util::split(line, name, value, '=');

                // Trim the name and its value from unused characters and spaces.
                util::trim(name);
                util::trim(value);

                // If this is an inline group declaration, rewrite to multiline form and re-parse.
                if (is_inline_group_literal(value))
                {
                    pending_lines.push_front(std::make_pair(current.first, "}"));

                    std::string body = value.substr(1, value.length() - 2);
                    util::trim(body);
                    if (!body.empty())
                    {
                        std::deque<std::string> items;
                        if (!split_inline_block_items(body, items)) {
                            return err(error, ERROR_CODE_SYNTAX, current.first);
                        }
                        while (!items.empty()) {
                            pending_lines.push_front(std::make_pair(current.first, items.back()));
                            items.pop_back();
                        }
                    }
                    pending_lines.push_front(std::make_pair(current.first, name + " = {"));
                    continue;
                }

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
                    if (group_stack.front()->query_property(name.c_str())) return err(error, ERROR_CODE_AMBIGUOUS_GROUP, current.first);
                    group_stack.push_front(group_stack.front()->m_p_groups[name]);
                }
                // If this is a regular assignment.
                else
                {
                    std::list<NCF*>::reverse_iterator end = group_stack.rend();
                    for (std::list<NCF*>::reverse_iterator it = group_stack.rbegin(); it != end; ++it) {
                        (*it)->expand_symbol(m_p_symbols, value);
                    }

                    if (group_stack.front()->query_group(name.c_str())) return err(error, ERROR_CODE_AMBIGUOUS_PROPERTY, current.first);
                    group_stack.front()->m_p_symbols[name] = value;
                }
            }
            // If this is a section end.
            else if( (line.length() == 1) && (line.find('}') != std::string::npos) )
            {
                --sectionp;

                // Check for unbalanced sections.
                if (sectionp < 0) return err(error, ERROR_CODE_UNBALANCED_GROUPS, current.first);

                group_stack.pop_front();
            }
            else
            {
                return err(error, ERROR_CODE_SYNTAX, current.first);
            }
        }
	}

	in.close();

	// Check for unbalanced sections.
	if(sectionp) return err(error, ERROR_CODE_UNBALANCED_GROUPS, linep);

	return err(error, ERROR_CODE_OK, 0);
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
		util::extract(source,search,'<','>');

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
				util::replace_first_of(s, search, it->second);
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

const char* NCF::get_property_by_index(size_t index) const
{
	if (count_properties() == 0) return NULL;

	std::map<std::string, std::string>::iterator it = m_p_symbols.begin();
	std::map<std::string, std::string>::iterator et = m_p_symbols.end();
	for (size_t i = 0; (it != et) && (index != i++); ++it);
	return it->second.c_str();
}

const char* NCF::get_property_name_by_index(size_t index) const
{
	if (count_properties() == 0) return NULL;

	std::map<std::string, std::string>::iterator it = m_p_symbols.begin();
	std::map<std::string, std::string>::iterator et = m_p_symbols.end();
	for (size_t i = 0; (it != et) && (index != i++); ++it);
	return it->first.c_str();
}

void NCF::set_property(const char *name, const char *value)
{
	m_p_symbols[name] = value;
}

bool NCF::remove_property(const char *name)
{
	if (!name) return false;
	std::map<std::string, std::string>::iterator it = m_p_symbols.find(name);
	if (it == m_p_symbols.end()) return false;
	m_p_symbols.erase(it);
	return true;
}

bool NCF::remove_group(const char *name)
{
	if (!name) return false;
	std::map<std::string, NCF*>::iterator it = m_p_groups.find(name);
	if (it == m_p_groups.end()) return false;
	delete it->second;
	m_p_groups.erase(it);
	return true;
}

NCF* NCF::get_group_by_name(const char *name) const
{
	std::map<std::string, NCF* >::iterator it = m_p_groups.find(name);

	// If the requested key doesn't exist then we create it.
	if (it == m_p_groups.end())
	{
		m_p_groups[name] = new NCF();
		m_p_groups[name]->m_p_name = name ? name : "";
		m_p_groups[name]->m_p_level = m_p_level + 1;

#ifdef CONFIG_DEBUG
		printf("NCF parser: [!node: %p]\n", (void *)m_p_groups[name]);
#endif /* CONFIG_DEBUG */
	}

	return m_p_groups[name];
}

NCF* NCF::get_group_by_index(size_t index) const
{
	if (count_groups() == 0) return NULL;

	std::map<std::string, NCF*>::iterator it = m_p_groups.begin();
	std::map<std::string, NCF*>::iterator et = m_p_groups.end();
	for (size_t i = 0; (it != et) && (index != i++); ++it);
	return it->second;
}

size_t NCF::count_entries() const
{
    return m_p_groups.size() + m_p_symbols.size();
}

size_t NCF::count_groups() const
{
	return m_p_groups.size();
}

size_t NCF::count_properties() const
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
	for (size_t i = 0; i < m_p_level; i++) ident.append("\t");

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

} /* namespace ncf */
