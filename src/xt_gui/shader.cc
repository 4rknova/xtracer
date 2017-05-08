#include <stdio.h>
#include <assert.h>
#include <map>
#ifdef _MSC_VER
#include <memory.h>
#else
#include <alloca.h>
#endif
#include "opengl.h"
#include "shader.h"

Shader::Shader(unsigned int type)
{
	shader = glCreateShader(type);
	compiled = false;

	assert(glGetError() == GL_NO_ERROR);
}

Shader::~Shader()
{
	glDeleteShader(shader);
}

void Shader::set_source(const char *src)
{
	glShaderSource(shader, 1, &src, 0);
	compiled = false;

	assert(glGetError() == GL_NO_ERROR);
}

bool Shader::compile()
{
	if (compiled) return true;

	glCompileShader(shader);

	int len;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

	if(len) {
		char *buf = (char*)alloca(len + 1);
		glGetShaderInfoLog(shader, len, &len, buf);
		fprintf(stderr, "compiler: %s\n", buf);
	}

	int status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if(!status) {
		fprintf(stderr, "shader compilation failed\n");
		return false;
	}

	compiled = true;
	return true;
}

bool Shader::is_compiled() const
{
	return compiled;
}

bool Shader::load(const char *fname)
{
	FILE *fp;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to open shader file: %s\n", fname);
		return false;
	}

	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	rewind(fp);

	char *buf = new char[sz + 1];
	if((long)fread(buf, 1, sz, fp) < sz) {
		fprintf(stderr, "failed to read shader contents\n");
		fclose(fp);
		return false;
	}
	fclose(fp);

	set_source(buf);
	delete [] buf;

	return compile();
}

static std::map<std::string, Shader*> shadercache;

Shader *get_shader(const char *fname, unsigned int type)
{
	auto it = shadercache.find(std::string(fname));
	if(it != shadercache.end()) {
		return it->second;
	}

	Shader *shader = new Shader(type);
	if(!shader->load(fname)) {
		delete shader;
		return 0;
	}
	shadercache[fname] = shader;
	return shader;
}

Program::Program()
{
	prog = glCreateProgram();
	linked = false;
	assert(glGetError() == GL_NO_ERROR);
}

Program::~Program()
{
	glDeleteProgram(prog);
}

void Program::add_shader(Shader *shader)
{
	if(shader->compile()) {
		glAttachShader(prog, shader->shader);
		assert(glGetError() == GL_NO_ERROR);

		shaders.push_back(shader);
		linked = false;
	}
}

bool Program::link()
{
	if (linked) return true;
    glLinkProgram(prog);

	int len;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if(len) {
		char *buf = (char*)alloca(len + 1);
		glGetProgramInfoLog(prog, len, &len, buf);
		assert(glGetError() == GL_NO_ERROR);
		fprintf(stderr, "linker: %s\n", buf);
	}

	int status;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if(!status) {
		fprintf(stderr, "program linking failed\n");
		return false;
	}

	linked = true;
	return true;
}

bool Program::load(const char *vsfname, const char *psfname)
{
	Shader *shader;

	if(vsfname) {
		if(!(shader = get_shader(vsfname, GL_VERTEX_SHADER))) {
			return false;
		}
		add_shader(shader);
	}

	if(psfname) {
		if(!(shader = get_shader(psfname, GL_FRAGMENT_SHADER))) {
			return false;
		}
		add_shader(shader);
	}
	return link();
}

int Program::get_uniform_location(const char *name)
{
	bind_program(this);
	return glGetUniformLocation(prog, name);
}

int Program::get_attribute_location(const char *name)
{
	bind_program(this);
	return glGetAttribLocation(prog, name);
}

void Program::set_uniform(const char *name, int val)
{
	set_uniform(get_uniform_location(name), val);
}

void Program::set_uniform(const char *name, float val)
{
	set_uniform(get_uniform_location(name), val);
}

void Program::set_uniform(const char *name, const NMath::Vector2f &v)
{
	set_uniform(get_uniform_location(name), v);
}

void Program::set_uniform(const char *name, const NMath::Vector3f &v)
{
	set_uniform(get_uniform_location(name), v);
}

void Program::set_uniform(const char *name, const NMath::Vector4f &v)
{
	set_uniform(get_uniform_location(name), v);
}

void Program::set_uniform(const char *name, const NMath::Matrix4x4f &mat)
{
	set_uniform(get_uniform_location(name), mat);
}


void Program::set_uniform(int loc, int val)
{
	if(loc >= 0) {
		bind_program(this);
		glUniform1i(loc, val);
	}
}

void Program::set_uniform(int loc, float val)
{
	if(loc >= 0) {
		bind_program(this);
		glUniform1f(loc, val);
	}
}

void Program::set_uniform(int loc, const NMath::Vector2f &v)
{
	if(loc >= 0) {
		bind_program(this);
		glUniform2f(loc, v.x, v.y);
	}
}

void Program::set_uniform(int loc, const NMath::Vector3f &v)
{
	if(loc >= 0) {
		bind_program(this);
		glUniform3f(loc, v.x, v.y, v.z);
	}
}

void Program::set_uniform(int loc, const NMath::Vector4f &v)
{
	if(loc >= 0) {
		bind_program(this);
		glUniform4f(loc, v.x, v.y, v.z, v.w);
	}
}

void Program::set_uniform(int loc, const NMath::Matrix4x4f &mat)
{
	if(loc >= 0) {
		bind_program(this);
		// loading transpose matrix (3rd arg true)
		glUniformMatrix4dv(loc, 1, GL_TRUE, (double*)mat.data);
	}
}

bool bind_program(const Program *prog)
{
	if(!prog) {
		glUseProgram(0);
		return true;
	}

	if(!((Program*)prog)->link()) {
		return false;
	}
	glUseProgram(prog->prog);
	if(glGetError()) {
		return false;
	}
	return true;
}
