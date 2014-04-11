#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include "log.hpp"

LogEntry::LogEntry()
	: p_type(LOGENTRY_MESSAGE)
{}

// Singleton.
Log Log::m_log_manager;

Log::Log() :
	m_max_log_size(0),
	m_flag_echo(true),
	m_flag_append(false)
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
	std::vector<LogEntry *>::iterator it;

	for (it = m_log.begin(); it != m_log.end(); it++) {
		delete (*it);
	}

	m_log.clear();
}

int Log::dump(const char* fpath)
{
	std::ofstream file;
	file.open(fpath);

	if (!file.is_open())
		return 1;

	std::vector<LogEntry *>::iterator it;

	for (it = m_log.begin(); it != m_log.end(); it++) {
		switch ((*it)->p_type) {
			case LOGENTRY_MESSAGE:
				break;
			case LOGENTRY_WARNING:
				file << "Warning: ";
				break;
			case LOGENTRY_ERROR:
				file << "Error: ";
				break;
		}

		file << (*it)->p_msg << std::endl;
		file.flush();
	}

	file.close();

	return 0;
}

std::string Log::get_log_entry(const unsigned int idx) const{

	if (idx < m_log.size())
		return m_log[m_log.size()-1-idx]->p_msg;
	return std::string("");
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

void Log::pulog(LOGENTRY_TYPE type, const char *msg, va_list args)
{

	// Store the length of the passed in string.
	unsigned int msg_size = strlen(msg);

	// We do not want to process an empty string.
	if(msg_size == 0)
		return;

	// This is the delimeter that is used to specify the format flags
	// you can use what you want.
	static char delim = '%';

    std::stringstream str;

	// The va_arg internal casting is inherently not safe here.
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

	if(m_flag_append) {
		m_log.back()->p_type = type;
		m_log.back()->p_msg.append(str.str());
	}
	else {
		LogEntry *entry = new (std::nothrow) LogEntry();

		if (!entry)
			return;

		entry->p_type = type;
		entry->p_msg = str.str();
		m_log.push_back(entry);
	}

	std::vector<LogEntry *>::reverse_iterator it = m_log.rbegin();

	if(m_flag_echo) {
		if (!m_flag_append && m_log.size() != 1) {
			std::cout << std::endl;
		}
		else {
			std::cout << '\r';
			m_flag_append = false;
		}

		switch ((*it)->p_type) {
			case LOGENTRY_MESSAGE:
				break;
			case LOGENTRY_WARNING:
				std::cout << "Warning: ";
				break;
			case LOGENTRY_ERROR:
				std::cout << "Error: ";
				break;
		}

		std::cout << (*it)->p_msg;
	}

	if (m_max_log_size) {
		while (m_max_log_size < m_log.size()) {
			pop_front();
		}
	}
}

void Log::log(LOGENTRY_TYPE type, const char * msg, ...)
{
	va_list args;
	va_start(args, msg);
	pulog(type, msg, args);
	va_end(args);
}

void Log::log_message(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	pulog(LOGENTRY_MESSAGE, msg, args);
	va_end(args);
}

void Log::log_error(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	pulog(LOGENTRY_ERROR, msg, args);
	va_end(args);
}

void Log::log_warning(const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	pulog(LOGENTRY_WARNING, msg, args);
	va_end(args);
}

void Log::rewind(bool clear)
{
	set_append();

	if (m_flag_echo) {
		if (clear) {
			std::cout << std::setw(m_log.back()->p_msg.length()) << ' ' << std::flush;
		}

		std::cout << '\r' << std::flush;
	}

	m_log.back()->p_msg.clear();
}

void Log::max_size(unsigned int size)
{
	m_max_log_size = size;
}

void Log::set_append()
{
	m_flag_append = true;
}

void Log::pop_back()
{
	m_log.pop_back();
}

void Log::pop_front()
{
	std::vector<LogEntry*>::iterator it = m_log.begin();
	m_log.erase(it);
}

unsigned int Log::count_entries()
{
	return m_log.size();
}
