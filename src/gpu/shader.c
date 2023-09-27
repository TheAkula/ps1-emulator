#include "shader.h"

void shader_create(Shader **shader, const char* vertex_path, const char* fragment_path) {
	FILE *vf;
	FILE *ff;
	
	char* vc;
	char* fc;

	vf = fopen(vertex_path, "r");

	if (vf == NULL) {
		printf("Failed to open vertex shader file: %s\n", vertex_path);
		exit(1);
	}
	
	ff = fopen(fragment_path, "r");

	if (ff == NULL) {
		printf("Failed to open fragment shader file: %s\n", fragment_path);
		exit(1);
	}

	uint32 vsize, fsize;
	
	fseek(vf, 0L, SEEK_END);
	vsize = ftell(vf);
	fseek(vf, 0, SEEK_SET);

	fseek(ff, 0L, SEEK_END);
	fsize = ftell(ff);
	fseek(ff, 0, SEEK_SET);

	vc = malloc(vsize + 1);
	fc = malloc(fsize + 1);
	
	fread(vc, vsize, 1, vf);
	fread(fc, fsize, 1, ff);	
	
	fclose(vf);
	fclose(ff);
	
	// 2. compile shaders
	unsigned int vertex, fragment;
	int success;
	char info_log[512];	
	
	// vertex Shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, (const GLchar **)&vc, NULL);	
	glCompileShader(vertex);
	// print compile errors if any
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex, 512, NULL, info_log);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED %s\n", info_log);
	}
	
	// similiar for Fragment
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, (const GLchar **)&fc, NULL);
	glCompileShader(fragment);
	// print compile errors if any
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment, 512, NULL, info_log);
		printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED %s\n", info_log);
	}

	// shader Program
	int id;
	id = glCreateProgram();
	glAttachShader(id, vertex);
	glAttachShader(id, fragment);
	glLinkProgram(id);
	// print linking errors if any
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if (success == 0) {
		glGetProgramInfoLog(id, 512, NULL, info_log);
		printf("ERROR::SHADER::PROGRAM::LINKING_FAILED %s\n", info_log);
	}

	*shader = malloc(sizeof(Shader));

	(*shader)->id = id;
	
	free(vc);
	free(fc);
  
	// delete the shaders as they're linked into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);	
}

void shader_bind(Shader *shader) {
	glUseProgram(shader->id);
}

void shader_unbind() {
	glUseProgram(0);
}

void shader_seti(Shader *shader, const char *name, int value){ 
	glUniform1i(glGetUniformLocation(shader->id, name), value); 
}
void shader_setf(Shader *shader, const char *name, float value) { 
	glUniform1f(glGetUniformLocation(shader->id, name), value); 
}
