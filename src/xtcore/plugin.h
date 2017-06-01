#ifndef XTCORE_PLUGIN_H_INCLUDED
#define XTCORE_PLUGIN_H_INCLUDED

#include <stdint.h>
#include <vector>
#include "status.h"

typedef enum {
	  PLUGIN_TYPE_UNDEFINED
	, PLUGIN_TYPE_OUTDRV    // Output driver
	, PLUGIN_TYPE_RENDERER  // Renderer
} PLUGIN_TYPE;

typedef struct {
	int32_t major;
    int32_t minor;
} Plugin_Version_t;


template <typename T>
class Plugin
{
	public:
	T *handle();

	Plugin();

	virtual ~Plugin();
	virtual Status set(const char *property
				     , const char *value);
	virtual void init()   = 0;
	virtual void deinit() = 0;
	virtual const char * get_name() = 0;

	private:
	T *m_impl;

};

#include "plugin.tml"

// Forward declarations
class Renderer;
class Exporter;

typedef Plugin<Renderer> PluginRenderer;
typedef Plugin<Exporter> PluginExporter;

#endif /* XTCORE_PLUGIN_H_INCLUDED */
