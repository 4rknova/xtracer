#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <vector>
#include "log.h"

namespace xtcore {

log_entry_t::log_entry_t()
	: type(LOGENTRY_DEBUG)
{}

// Singleton.
Log Log::m_log_manager;

Log::Log()
	: m_max_log_size(0)
	, m_flag_echo(true)
	, m_flag_rewind(false)
    , m_level(LOGENTRY_DEBUG)
    , m_callback(nullptr)
    , m_callback_user(nullptr)
    , m_mut()
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
    std::lock_guard<std::mutex> lock(m_mut);
	std::vector<log_entry_t *>::iterator it;

	for (it = m_log.begin(); it != m_log.end(); ++it) {
		delete (*it);
	}

	m_log.clear();
}

int Log::dump(const char* fpath)
{
    std::lock_guard<std::mutex> lock(m_mut);
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
    std::lock_guard<std::mutex> lock(m_mut);
	if (idx < m_log.size()) return *(m_log[m_log.size()-1-idx]);
	return log_entry_t();
}

size_t Log::get_size() const
{
    std::lock_guard<std::mutex> lock(m_mut);
    return m_log.size();
}

void Log::callback(callback_t fn, void *user)
{
    std::lock_guard<std::mutex> lock(m_mut);
    m_callback = fn;
    m_callback_user = user;
}

Log &Log::handle()
{
	return m_log_manager;
}

void Log::echo(bool state)
{
    std::lock_guard<std::mutex> lock(m_mut);
	m_flag_echo = state;
}

bool Log::echo() const
{
    std::lock_guard<std::mutex> lock(m_mut);
	return m_flag_echo;
}

void Log::rewind()
{
    std::lock_guard<std::mutex> lock(m_mut);
    m_flag_rewind = true;
}

void Log::pulog(LOGENTRY_TYPE type, const char *msg, va_list args)
{
    std::lock_guard<std::mutex> lock(m_mut);
    if (type < m_level) return;

    if (!msg || !*msg) return;

    va_list args_probe;
    va_copy(args_probe, args);
    const int needed = std::vsnprintf(nullptr, 0, msg, args_probe);
    va_end(args_probe);
    if (needed < 0) return;

    std::vector<char> buffer((size_t)needed + 1u, '\0');
    va_list args_format;
    va_copy(args_format, args);
    std::vsnprintf(buffer.data(), buffer.size(), msg, args_format);
    va_end(args_format);

	log_entry_t *entry = new (std::nothrow) log_entry_t();

	if (!entry)	return;

	entry->type = type;
	entry->message = std::string(buffer.data());

    if (m_flag_rewind) m_log.pop_back();

	m_log.push_back(entry);

    if (m_callback && !m_flag_rewind) {
        m_callback(type, entry->message, m_callback_user);
    }

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
            std::vector<log_entry_t*>::iterator it = m_log.begin();
            if (it == m_log.end()) break;
            delete (*it);
			m_log.erase(it);
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
    std::lock_guard<std::mutex> lock(m_mut);
	m_max_log_size = size;
}

void Log::pop_back()
{
    std::lock_guard<std::mutex> lock(m_mut);
	m_log.pop_back();
}

void Log::pop_front()
{
    std::lock_guard<std::mutex> lock(m_mut);
	std::vector<log_entry_t*>::iterator it = m_log.begin();
	m_log.erase(it);
}

unsigned int Log::count_entries()
{
    std::lock_guard<std::mutex> lock(m_mut);
	return m_log.size();
}

} /* namespace xtcore */
