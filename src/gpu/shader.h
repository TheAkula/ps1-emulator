#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h> // include glad to get all the required OpenGL headers
  
#include "../platform.h"

typedef struct {
	uint32 id;        
} Shader;

void shader_create(Shader **shader, const char* vertex_path, const char* fragment_path);
void shader_bind(Shader *shader);
void shader_unbind();
void shader_seti(Shader *shader, const char *name, int value);   
void shader_setf(Shader *shader, const char *name, float value);    

#endif
