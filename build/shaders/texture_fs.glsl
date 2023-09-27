#version 330 core

in vec3 color;
in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D tex;

void main() {	
	fragColor = vec4(mix(color, vec3(texture(tex, texCoord)), 0.5), 1.0);
}
