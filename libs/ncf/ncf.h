#ifndef LIBNCF_NCF_H_INCLUDED
#define LIBNCF_NCF_H_INCLUDED

#include <cstdlib>
#include <string>
#include <map>
#include "declspec.h"

#pragma warning (push)
// Disable "<type> needs to have dll-interface to be used by clients"
// This warning refers to STL member variables which are private and
// therefore can be safely ignored.
#pragma warning (disable : 4251)

namespace ncf {

enum ERROR_CODE
{
      ERROR_CODE_OK
    , ERROR_CODE_NOT_ASCII_FILE
    , ERROR_CODE_IO
    , ERROR_CODE_SYNTAX
    , ERROR_CODE_UNBALANCED_GROUPS
    , ERROR_CODE_AMBIGUOUS_GROUP
    , ERROR_CODE_AMBIGUOUS_PROPERTY
    , ERROR_CODE_EXTERNAL_FILE
};

struct error_t
{
    ERROR_CODE  code;
    const char *message;
    size_t      line;
};

class DECLSPEC NCF
{
	public:
		NCF();
		~NCF();

		void set_source(const char *file);
		const char* get_source() const;

		void purge();

		int parse(error_t *error = 0);

		/* RETURN CODES:
		** 0 : Everything went well.
		** 1 : File I/O error.
		*/
		int dump(const char *file, int create = 1) const;

		bool query_property(const char* name) const;
		bool query_group(const char *name) const;

		size_t count_entries() const;
		size_t count_properties() const;
		size_t count_groups() const;

		void        set_property(const char *name, const char *value);
		const char* get_property_by_name(const char *name) const;
		const char* get_property_by_index(size_t index) const;
		const char* get_property_name_by_index(size_t index) const;
		NCF*		get_group_by_name(const char *name) const;
		NCF*		get_group_by_index(size_t index) const;

		const char *get_name() const;

    private:
		NCF(const NCF&);
		NCF &operator =(const NCF &);

		void release();

		void expand_symbol(std::map<std::string, std::string> &symbols, std::string &s);

		std::string m_p_name;									// Node name.
		mutable std::map<std::string, NCF*> m_p_groups;			// Sub-node map.
		mutable std::map<std::string, std::string> m_p_symbols; // Symbol map.
		std::string m_p_filepath;								// Source path.
		size_t m_p_level;   									// Node level.
};

} /* namespace ncf */

#pragma warning (pop)

#endif /* LIBNCF_NCF_H_INCLUDED */
