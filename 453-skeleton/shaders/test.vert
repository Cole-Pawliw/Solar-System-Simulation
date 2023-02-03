#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoords;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

uniform mat3 norm_R;

out vec4 frag_pos;
out vec3 n;
out vec2 tex;

void main() {
	vec4 position = M * vec4(pos, 1.0);
	n = norm_R * normal;
	frag_pos = position;
	gl_Position = P * V * position;
	tex = texCoords;
}
