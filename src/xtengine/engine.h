#include <xtcore/status.h>
#include <xtcore/plugin.h>
#include <xtcore/renderer.h>

typedef std::vector<Plugin<Renderer> *> Plugin_Renderers;

class Engine
{
	static Engine &handle();

	Status add_renderer(Plugin<Renderer> &plugin);

	private:
	Engine();
	~Engine();
    Engine(const Engine&);
    Engine& operator=(const Engine&);

	Plugin_Renderers renderers;
};
