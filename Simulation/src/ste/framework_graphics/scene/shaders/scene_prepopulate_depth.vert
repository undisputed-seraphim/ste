
#type vert
#version 450
#extension GL_ARB_shader_draw_parameters : enable

#include "mesh_descriptor.glsl"

layout(location = 0) in vec3 vert;
layout(location = 3) in vec2 tex_coords;

layout(std430, binding = 1) restrict readonly buffer mesh_data {
	mesh_descriptor mesh_descriptor_buffer[];
};
layout(std430, binding = 12) restrict readonly buffer id_to_drawid_data {
	uint id_to_drawid[];
};

out vs_out {
	vec2 uv;
	flat int matIdx;
} vout;

uniform mat4 projection_view_matrix;

void main() {
	uint draw_id = id_to_drawid[gl_DrawIDARB];
	mesh_descriptor md = mesh_descriptor_buffer[draw_id];

	mat4 view_model = projection_view_matrix * md.model;

	gl_Position = view_model * vec4(vert, 1);
	vout.uv = tex_coords;
	vout.matIdx = md.matIdx;
}
