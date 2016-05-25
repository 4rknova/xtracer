#ifndef XTRACER_CONSOLE_H_INCLUDED
#define XTRACER_CONSOLE_H_INCLUDED

class Console
{
	public:
		static Console &handle();

		void update(float progress, int worker, int workers);

	private:
		Console();
		~Console();

		static Console m_console;
};

#endif /* XTRACER_CONSOLE_H_INCLUDED */
