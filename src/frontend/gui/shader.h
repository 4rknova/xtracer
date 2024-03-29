#ifndef XTGUI_SHADER_H_INCLUDED
#define XTGUI_SHADER_H_INCLUDED

#include <vector>
#include <nmath/vector.h>

class Shader {
    private:
	bool compiled;

    public:
	unsigned int shader;

	Shader(unsigned int type);
	~Shader();

	void set_source(const char *src);

	bool compile();
	bool is_compiled() const;

	bool load(const char *fname);
};

class Program {
    private:
	std::vector<Shader*> shaders;
	bool linked;

    public:
	unsigned int prog;

	Program();
	~Program();

	void add_shader(Shader *shader);
	bool link();

	bool load(const char *vsfname, const char *psfname);

	int get_uniform_location(const char *name);
	int get_attribute_location(const char *name);

	void set_uniform(const char *name, int val);
	void set_uniform(const char *name, float val);
	void set_uniform(const char *name, const nmath::Vector2f &v);
	void set_uniform(const char *name, const nmath::Vector3f &v);
	void set_uniform(const char *name, const nmath::Vector4f &v);
	void set_uniform(const char *name, const nmath::Matrix4x4f &mat);

	void set_uniform(int loc, int val);
	void set_uniform(int loc, float val);
	void set_uniform(int loc, const nmath::Vector2f &v);
	void set_uniform(int loc, const nmath::Vector3f &v);
	void set_uniform(int loc, const nmath::Vector4f &v);
	void set_uniform(int loc, const nmath::Matrix4x4f &mat);
};

bool bind_program(const Program *prog);

#endif	/* XTGUI_SHADER_H_INCLUDED */
