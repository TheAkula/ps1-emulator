#version 330 core

in vec3 color;
in vec2 texCoord;

out vec4 fragColor;

uniform int texture_depth;

uniform sampler2D tex4;
uniform sampler2D tex8;
uniform sampler2D tex16;

uniform int clut4[16];
uniform int clut8[256];

vec4 split_colors(int data) {
    vec4 color;
    color.r = (data << 3) & 0xf8;
    color.g = (data >> 2) & 0xf8;
    color.b = (data >> 7) & 0xf8;
    color.a = 255.0f;

    return color;
}

vec4 sample_texel() {
    if (texture_depth == 0) {
        vec4 index = texture2D(tex4, texCoord);
        int texel = clut4[int(index.r * 255)];

        return split_colors(texel) / vec4(255.0f);
    } else if (texture_depth == 1) {
        vec4 index = texture2D(tex8, texCoord);
        int texel = clut8[int(index.r * 255)];

        return split_colors(texel) / vec4(255.0f);  
    } else {
        int texel = int(texture2D(tex16, texCoord).r * 255);
        return split_colors(texel) / vec4(255.0f);
    }
}

void main() {	
  fragColor = sample_texel();
}
