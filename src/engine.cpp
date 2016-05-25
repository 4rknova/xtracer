#include "engine.h"

Engine::Engine()
{}

Engine::~Engine()
{}

Engine::Engine(const Engine&)
{}

Engine &Engine::operator=(const Engine&)
{
	return *this;
}

Engine &Engine::handle()
{
	static Engine inst;
	return inst;
}

Status Engine::add_renderer(Plugin<Renderer> &plugin)
{
	return STATUS_OK;
}
