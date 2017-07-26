#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include "log.h"

log_entry_t::log_entry_t()
	: type(LOGENTRY_DEBUG)
{}

// Singleton.
Log Log::m_log_manager;

Log::Log()
	: m_max_log_size(0)
	, m_flag_echo(true)
	, m_flag_rewind(false)
    , m_level(LOGENTRY_MESSAGE)
{}

Log::~Log()
{
	clear();

	if (m_flag_echo) {
		std::cout << std::endl;
	}
}

void Log::clear()
{
	std::vector<log_entry_t *>::iterator it;

	for (it = m_log.begin(); it != m_log.end(); ++it) {
		delete (*it);
	}

	m_log.clear();
}

int Log::dump(const char* fpath)
{
	std::ofstream file;
	file.open(fpath);

	if (!file.is_open()) return 1;

	std::vector<log_entry_t *>::iterator it;

	for (it = m_log.begin(); it != m_log.end(); ++it) {
   		switch ((*it)->type) {
            case LOGENTRY_DEBUG:
                file << "Debug: ";
                break;
    		case LOGENTRY_MESSAGE:
    		    break;
    		case LOGENTRY_WARNING:
    			file << "Warning: ";
    			break;
    		case LOGENTRY_ERROR:
    			file << "Error: ";
    			break;
    	}

        file << (*it)->message << std::endl;
        file.flush();
	}

	file.close();
	return 0;
}

log_entry_t Log::get_entry(size_t idx) const
{
	if (idx < m_log.size()) return *(m_log[m_log.size()-1-idx]);
	return log_entry_t();
}

size_t Log::get_size() const
{
    return m_log.size();
}

Log &Log::handle()
{
	return m_log_manager;
}

void Log::echo(bool state)
{
	m_flag_echo = state;
}

bool Log::echo() const
{
	return m_flag_echo;
}

void Log::rewind()
{
    m_flag_rewind = true;
}

void Log::pulog(LOGENTRY_TYPE type, const char *msg, va_list args)
{
    if (type < m_level) return;

	// Store the length of the passed in string.
	unsigned int msg_size = strlen(msg);

	// We do not want to process an empty string.
	if(msg_size == 0)
		return;

	// This is the delimeter that is used to specify the format flags
	// you can use what you want.
	static char delim = '%';

    std::stringstream str;

	for(unsigned int x = 0; x < msg_size; x++) {
		if(msg[x] == delim)	{
			// If there is a next character, and it is not a delim,
			if(x + 1 < msg_size && msg[x + 1] != delim) {
				// String
				if (msg[x + 1] == 's') {
					// We have a c-string, so get it and save it
					const char* temp = va_arg(args, const char*);
					str << temp;
					x++;
				}

                // Character
				else if (msg[x + 1] == 'c') {
					char temp = va_arg(args, int);
					str << temp;
					x++;
				}

				// Integer
				else if (msg[x + 1] == 'i') {
					int temp = va_arg(args, int);
					str << temp;
					x++;
				}

				// Float
				else if (msg[x + 1] == 'f') {
					double temp = va_arg(args, double);
					str << std::setprecision(3) << temp;
					x++;
				}
				// size_t
                else if (msg[x + 1] == 'l') {
					double temp = va_arg(args, size_t);
					str << temp;
					x++;
				}
			}
			else if(x + 1 < msg_size) {
				// Save the delim that "delimdelim" was used for
				str << delim;

				// We know what comes after the delim, so do not process it
				x++;
			}
		}
		// ("delimdelim" means add a single delim)
		else {
			str << msg[x];
		}
	}

	log_entry_t *entry = new (std::nothrow) log_entry_t();

	if (!entry)	return;

	entry->type = type;
	entry->message = str.str();

    if (m_flag_rewind) m_log.pop_back();

	m_log.push_back(entry);

	std::vector<log_entry_t *>::reverse_iterator it = m_log.rbegin();

	if (m_flag_echo) {
   		switch ((*it)->type) {
		    case LOGENTRY_MESSAGE:                           break;
   			case LOGENTRY_DEBUG  : std::cout << "Debug   :"; break;
       		case LOGENTRY_WARNING: std::cout << "Warning :"; break;
   			case LOGENTRY_ERROR  : std::cout << "Error   :"; break;
   		}

		std::cout << (*it)->message;

        if (m_flag_rewind) {
            std::cout << "\r" << std::flush;
            m_flag_rewind = false;
        }
        else {
            std::cout << std::endl;
        }
	}

	if (m_max_log_size) {
		while (m_max_log_size < m_log.size()) {
			pop_front();
		}
	}
}

void Log::post(LOGENTRY_TYPE type, const char * msg, ...)
{
	va_list args;
	va_start(args, msg);
	pulog(type, msg, args);
	va_end(args);
}

void Log::post_debug(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	pulog(LOGENTRY_DEBUG, msg, args);
	va_end(args);
}

void Log::post_message(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	pulog(LOGENTRY_MESSAGE, msg, args);
	va_end(args);
}

void Log::post_error(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	pulog(LOGENTRY_ERROR, msg, args);
	va_end(args);
}

void Log::post_warning(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	pulog(LOGENTRY_WARNING, msg, args);
	va_end(args);
}

void Log::max_size(unsigned int size)
{
	m_max_log_size = size;
}

void Log::pop_back()
{
	m_log.pop_back();
}

void Log::pop_front()
{
	std::vector<log_entry_t*>::iterator it = m_log.begin();
	m_log.erase(it);
}

unsigned int Log::count_entries()
{
	return m_log.size();
}
