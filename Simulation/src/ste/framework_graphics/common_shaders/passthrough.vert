
#type vert
#version 440

layout(location = 1) in vec3 vert;

void main() {
	gl_Position = vec4(vert, 1);
}
